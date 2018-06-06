/*
icmp_lib.h

Created Sept 30, 1991 by Philip Homburg

Copyright 1995 Philip Homburg
*/

#ifndef ICMP_LIB_H
#define ICMP_LIB_H

/* Prototypes */

void icmp_snd_parmproblem( acc_t *pack );
void icmp_snd_time_exceeded( int port_nr, acc_t *pack, int code );
void icmp_snd_unreachable( int port_nr, acc_t *pack, int code );
void icmp_snd_redirect( int port_nr, acc_t *pack, int code,
							mnx_ipaddr_t gw );
void icmp_snd_mtu(int port_nr, acc_t *pack, U16_t mtu);

#endif /* ICMP_LIB_H */

/*
 * $PchId: icmp_lib.h,v 1.6 2002/06/08 21:32:44 philip Exp $
 */
