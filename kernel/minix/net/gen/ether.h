/*
server/ip/gen/ether.h
*/

#ifndef __SERVER__IP__GEN__ETHER_H__
#define __SERVER__IP__GEN__ETHER_H__

#define ETH_MIN_PACK_SIZE		  60
#define ETH_MAX_PACK_SIZE		1514
#define ETH_MAX_PACK_SIZE_TAGGED	1518
#define ETH_HDR_SIZE			  14
#define ETH_CRC_SIZE			   4

typedef struct ether_addr
{
	u8_t ea_addr[6];
} mnx_ethaddr_t;

#define MAC_FORMAT 	"%0X:%0X:%0X:%0X:%0X:%0X"
#define MAC_FIELDS(p) 	p->ea_addr[0],p->ea_addr[1],p->ea_addr[2],p->ea_addr[3],p->ea_addr[4],p->ea_addr[5]

typedef u16_t ether_type_t;
typedef U16_t Ether_type_t;

#define ETH_ARP_PROTO	 0x806
#define ETH_IP_PROTO	 0x800
#define ETH_VLAN_PROTO	0x8100

/* Tag Control Information field for VLAN and Priority tagging */
#define ETH_TCI_PRIO_MASK	0xe000
#define ETH_TCI_CFI		0x1000	/* Canonical Formal Indicator */
#define ETH_TCI_VLAN_MASK	0x0fff	/* 12-bit vlan number */

#endif /* __SERVER__IP__GEN__ETHER_H__ */

/*
 * $PchId: ether.h,v 1.6 2005/01/27 17:33:35 philip Exp $
 */
