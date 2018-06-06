/*
server/ip/gen/eth_hdr.h
*/
 
#ifndef __SERVER__IP__GEN__ETH_HDR_H__
#define __SERVER__IP__GEN__ETH_HDR_H__

typedef struct eth_hdr
{
	mnx_ethaddr_t eh_dst;
	mnx_ethaddr_t eh_src;
	ether_type_t eh_proto;
} eth_hdr_t;

#define ETHHDR_FORMAT	"dst=%X:%X:%X:%X:%X:%X src=%X:%X:%X:%X:%X:%X proto=%X\n" 
#define ETHHDR_FIELDS(p) p->eh_dst.ea_addr[0],p->eh_dst.ea_addr[1], p->eh_dst.ea_addr[2], \
				p->eh_dst.ea_addr[3], p->eh_dst.ea_addr[4], p->eh_dst.ea_addr[5], \
				p->eh_src.ea_addr[0], p->eh_src.ea_addr[1], p->eh_src.ea_addr[2], \
				p->eh_src.ea_addr[3], p->eh_src.ea_addr[4], p->eh_src.ea_addr[5], p->eh_proto

#endif /* __SERVER__IP__GEN__ETH_HDR_H__ */
