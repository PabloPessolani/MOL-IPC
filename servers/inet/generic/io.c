/*
io.c

Copyright 1995 Philip Homburg
*/


#include "../inet.h"

#include "io.h"

void writeIpAddr(mnx_ipaddr_t addr)
{
#define addrInBytes ((u8_t *)&addr)

	printf("%d.%d.%d.%d", addrInBytes[0], addrInBytes[1],
		addrInBytes[2], addrInBytes[3]);
#undef addrInBytes
}

void writeEtherAddr(mnx_ethaddr_t *addr)
{
#define addrInBytes ((u8_t *)addr->ea_addr)

	printf("%x:%x:%x:%x:%x:%x", addrInBytes[0], addrInBytes[1],
		addrInBytes[2], addrInBytes[3], addrInBytes[4], addrInBytes[5]);
#undef addrInBytes
}

/*
 * $PchId: io.c,v 1.6 1998/10/23 20:24:34 philip Exp $
 */
