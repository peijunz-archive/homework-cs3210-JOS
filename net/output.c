#include "ns.h"
#include <inc/lib.h>

extern union Nsipc nsipcbuf;

void
output(envid_t ns_envid)
{
	binaryname = "ns_output";

	// LAB 6: Your code here:
	// 	- read a packet from the network server
	//	- send the packet to the device driver
	struct jif_pkt* pkt = (void*)UTEMP;

	envid_t from_env;
	int perm, r;
	while (1){
		if ((r = ipc_recv(&from_env, UTEMP, &perm)) != NSREQ_OUTPUT){
			cprintf("Output helper: unknown request value %d from env %d\n", r, from_env);
		}
		// cprintf("Packet info: length %d, %20s\n", pkt->jp_len, pkt->jp_data);
		sys_net_send(pkt->jp_data, pkt->jp_len);
	}
}
