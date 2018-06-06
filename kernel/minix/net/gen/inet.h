/*
server/ip/gen/inet.h
*/

#ifndef __SERVER__IP__GEN__INET_H__
#define __SERVER__IP__GEN__INET_H__

mnx_ipaddr_t inet_addr _ARGS(( const char *addr ));
mnx_ipaddr_t inet_network _ARGS(( const char *addr ));
char *inet_ntoa _ARGS(( mnx_ipaddr_t addr ));
int inet_aton _ARGS(( const char *cp, mnx_ipaddr_t *pin ));

#endif /* __SERVER__IP__GEN__INET_H__ */
