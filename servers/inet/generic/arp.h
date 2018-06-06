/*
arp.h

Copyright 1995 Philip Homburg
*/

#ifndef ARP_H
#define ARP_H

#define ARP_ETHERNET	1

#define ARP_REQUEST	1
#define ARP_REPLY	2

/* Prototypes */
typedef void (*arp_func_t) ( int fd, mnx_ipaddr_t ipaddr,
	mnx_ethaddr_t *ethaddr );

void arp_prep ( void );
void arp_init ( void );
void arp_set_ipaddr ( int eth_port, mnx_ipaddr_t ipaddr );
int arp_set_cb ( int eth_port, int ip_port, arp_func_t arp_func );
int arp_ip_eth ( int eth_port, mnx_ipaddr_t ipaddr, mnx_ethaddr_t *ethaddr );

int arp_ioctl ( int eth_port, int fd, ioreq_t req,
	get_userdata_t get_userdata, put_userdata_t put_userdata );

#endif /* ARP_H */

/*
 * $PchId: arp.h,v 1.7 2001/04/19 18:58:17 philip Exp $
 */
