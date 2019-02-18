#include <kern/e1000.h>
#include <kern/pmap.h>
#include <inc/string.h>

// LAB 6: Your driver code here

struct tx_desc * sendq; // Transmit Queue
struct tx_desc * recvq; // Unused in part A

int init_queue(struct tx_desc **q_ptr){
    // Allocate contiguous memory. one page is perfect
    struct PageInfo *pp=page_alloc(ALLOC_ZERO);
    struct tx_desc * q = page2kva(pp); // Ensures 4K alignment of base
    *q_ptr = q;
    // Q->size = Q->tail - Q->head;
    for (int i=0; i < QCAP; i++){
        pp=page_alloc(ALLOC_ZERO);
        q[i].addr = page2pa(pp);
        q[i].length = PGSIZE;
        q[i].cmd |= E1000_TXD_CMD_RS;
        q[i].status |= E1000_TXD_STAT_DD;
    }
    return 0;
}

int pci_e1000_attach(struct pci_func *ef){
	pci_func_enable(ef);
	// lapicaddr is the physical address of the LAPIC's 4K MMIO
	// region.  Map it in to virtual memory so we can access it.
	e1000 = mmio_map_region(ef->reg_base[0], ef->reg_size[0]);
    cprintf("Status register is %x\n", REG(E1000_STATUS));
    init_queue(&sendq);
    init_queue(&recvq);

    // Allocate a region of memory for the transmit descriptor list. Software should insure this memory is
    // aligned on a paragraph (16-byte) boundary. Program the Transmit Descriptor Base Address
    // (TDBAL/TDBAH) register(s) with the address of the region. TDBAL is used for 32-bit addresses
    // and both TDBAL and TDBAH are used for 64-bit addresses.
    REG(E1000_TDBAL) = PADDR(sendq);
    // REG(TDBAH) = 0;

    // Set the Transmit Descriptor Length (TDLEN) register to the size (in bytes) of the descriptor ring.
    // This register must be 128-byte aligned.
    REG(E1000_TDLEN) = QCAP * sizeof(struct tx_desc);

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
    return 0;
}


// copying the packet data
int transmit_packet(void *buf, int size){
    int tail = REG(E1000_TDT);
    if (!(sendq[tail].status & E1000_TXD_STAT_DD)){
        return -E_BUF_FULL;
    }
    sendq[tail].cmd |= E1000_TXD_CMD_RS;
    sendq[tail].status &= ~E1000_TXD_STAT_DD;
    sendq[tail].cmd |= 0x1;
    if (size > PACKSIZE){
        panic("Not implemented error: The packet is too large.\n");
    }
    sendq[tail].length = size;
    memmove(KADDR(sendq[tail].addr), buf, size);
    tail = (tail + 1) % QCAP;
    REG(E1000_TDT) = tail;
    return 0;
}
