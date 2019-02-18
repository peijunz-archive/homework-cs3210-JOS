#include "ns.h"
#include <inc/lib.h>

extern union Nsipc nsipcbuf;


struct packet {
    int len;
    char data[0];
};


void
input(envid_t ns_envid)
{
	binaryname = "ns_input";
	// LAB 6: Your code here:
	// 	- read a packet from the device driver
	//	- send it to the network server
	// Hint: When you IPC a page to the network server, it will be
	// reading from it for a while, so don't immediately receive
	// another packet in to the same physical page.
	int r;
	struct packet *pkt=NULL;
	while (1){
		if (pkt == NULL){
			pkt = (void*)UTEMP;
			if ((r = sys_page_alloc(0, UTEMP, PTE_U|PTE_W|PTE_P)) < 0){
				panic("Input Failed to allocate page with error: %e\n", r);
			}
		}
		if ((r = sys_net_recv(pkt, PGSIZE))!=0){
			// cprintf("User hungry\n");
			sys_yield();
			continue;
		}
		ipc_send(ns_envid, NSREQ_INPUT, UTEMP, PTE_U|PTE_W|PTE_P);
		sys_page_unmap(0, UTEMP);
		pkt = NULL;
	}
}
