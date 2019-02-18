#include <kern/e1000.h>
#include <kern/pmap.h>
#include <inc/string.h>

// LAB 6: Your driver code here

struct tx_desc * sendq; // Transmit Queue
struct rx_desc * recvq; // Receive Queue

void init_sendq(){
    // Allocate contiguous memory. one page is perfect
    struct PageInfo *pp=page_alloc(ALLOC_ZERO);
    sendq = page2kva(pp); // Ensures 4K alignment of base
    for (int i=0; i < TCAP; i++){
        // Wastes space!!! because largest packet is 1518 bytes
        pp=page_alloc(ALLOC_ZERO);
        sendq[i].addr = page2pa(pp);
        sendq[i].length = PGSIZE;
        sendq[i].cmd |= E1000_TXD_CMD_RS;
        sendq[i].status |= E1000_TXD_STAT_DD;
    }
}

void init_recvq(){
    // Allocate contiguous memory. one page is perfect
    struct PageInfo *pp=page_alloc(ALLOC_ZERO);
    recvq = page2kva(pp); // Ensures 4K alignment of base
    for (int i=0; i < RCAP; i++){
        // Wastes space!!! because largest packet is 1518 bytes
        pp=page_alloc(ALLOC_ZERO);
        recvq[i].addr = page2pa(pp);
        recvq[i].length = PGSIZE;
        // recvq[i].status |= E1000_TXD_STAT_DD; // should be 0
    }
}

void transmit_init(){
    init_sendq();
    // Allocate a region of memory for the transmit descriptor list. Software should insure this memory is
    // aligned on a paragraph (16-byte) boundary. Program the Transmit Descriptor Base Address
    // (TDBAL/TDBAH) register(s) with the address of the region. TDBAL is used for 32-bit addresses
    // and both TDBAL and TDBAH are used for 64-bit addresses.
    REG(E1000_TDBAL) = PADDR(sendq);
    // REG(TDBAH) = 0;

    // Set the Transmit Descriptor Length (TDLEN) register to the size (in bytes) of the descriptor ring.
    // This register must be 128-byte aligned.
    REG(E1000_TDLEN) = TCAP * sizeof(struct tx_desc);

    // The Transmit Descriptor Head and Tail (TDH/TDT) registers are initialized (by hardware) to 0b
    // after a power-on or a software initiated Ethernet controller reset. Software should write 0b to both
    // these registers to ensure this.
    REG(E1000_TDH) = 0;
    REG(E1000_TDT) = 0;

    // Initialize the Transmit Control Register (TCTL) for desired operation to include the following:
    // + Set the Enable (TCTL.EN) bit to 1b for normal operation.
    // + Set the Pad Short Packets (TCTL.PSP) bit to 1b.
    // + Configure the Collision Threshold (TCTL.CT) to the desired value. Ethernet standard is 10h. 
    //     This setting only has meaning in half duplex mode.
    // + Configure the Collision Distance (TCTL.COLD) to its expected value. For full duplex
    //     operation, this value should be set to 40h. For gigabit half duplex, this value should be set to
    //     200h. For 10/100 half duplex, this value should be set to 40h.
    REG(E1000_TCTL) = E1000_TCTL_EN | E1000_TCTL_PSP | ((0x10 << 4)&E1000_TCTL_CT) | ((0x40 << 12)&E1000_TCTL_COLD);
    // For the TCTL.COLD, you can assume full-duplex operation. For TIPG,
    // refer to the default values described in table 13-77 of section 13.4.34
    // for the IEEE 802.3 standard IPG (don't use the values in the table in section 14.5).
    REG(E1000_TIPG) = 0x10 | (0x4 << 10) | (0x6 << 20);
}

void receive_init(){
    init_recvq();
    REG(E1000_RA) = MAC_ADDR_LO;
    REG(E1000_RA+4) = MAC_ADDR_HI | E1000_RAH_AV;
    REG(E1000_MTA) = 0;
    REG(E1000_RDBAL) = PADDR(recvq);
    // REG(E1000_RDBAH) = 0;
    REG(E1000_RDLEN) = RCAP * sizeof(struct rx_desc);
    REG(E1000_RDH) = 1;
    REG(E1000_RDT) = 0;
    REG(E1000_RCTL) = E1000_RCTL_SZ_4096 | E1000_RCTL_BSEX | E1000_RCTL_BAM | E1000_RCTL_SECRC | E1000_RCTL_EN;
}

int pci_e1000_attach(struct pci_func *ef){
	pci_func_enable(ef);
	// Map reg_base to virtual memory so we can access E1000 registers in KVA
	e1000 = mmio_map_region(ef->reg_base[0], ef->reg_size[0]);
    transmit_init();
    receive_init();
    return 0;
}


// copying buf as the packet data
int transmit_packet(void *buf, int size){
    int tail = REG(E1000_TDT);
    if (!(sendq[tail].status & E1000_TXD_STAT_DD)){
        return -E_BUF_FULL;
    }
    sendq[tail].cmd |= E1000_TXD_CMD_RS;
    sendq[tail].status &= ~E1000_TXD_STAT_DD;
    sendq[tail].cmd |= 0x1; // EOP
    if (size > PACKSIZE){
        panic("Not implemented error: The packet is too large.\n");
    }
    sendq[tail].length = size;
    memmove(KADDR(sendq[tail].addr), buf, size);
    tail = (tail + 1) % TCAP;
    REG(E1000_TDT) = tail;
    return 0;
}


// copying the packet data to buf
int receive_packet(struct packet *pkt, int size){
    // Tail does not mean anything...
    // Tail + 1 holds the real data
    int tail = REG(E1000_RDT);
    if (!(recvq[(tail+1)%RCAP].status & E1000_RXD_STAT_DD)){
        // No available packet
        // cprintf("Hungry!\n");
        return -E_HUNGRY;
    }
    tail = (tail+1)%RCAP;
    // cprintf("Current head %d, tail %d -> %d\n", REG(E1000_RDH), REG(E1000_RDT), tail);
    recvq[tail].status = 0;
    // recvq[tail].status &= ~E1000_RXD_STAT_EOP;
    if (size < recvq[tail].length + sizeof(pkt)){
        panic("bufsize %d < received %d\n", size, recvq[tail].length);
    }
    memmove(pkt->data, KADDR(recvq[tail].addr), recvq[tail].length);
    pkt->len = recvq[tail].length;
    REG(E1000_RDT) = tail;
    return 0;
}
