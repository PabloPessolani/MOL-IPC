/*
eth.h

Copyright 1995 Philip Homburg
*/

#ifndef ETH_H
#define ETH_H

#define NWEO_DEFAULT    (NWEO_EN_LOC | NWEO_DI_BROAD | NWEO_DI_MULTI | \
	NWEO_DI_PROMISC | NWEO_REMANY | NWEO_RWDATALL)

#define eth_addrcmp(a,b) (memcmp((_VOIDSTAR)&a, (_VOIDSTAR)&b, \
	sizeof(a)))

/* Forward declatations */

struct acc;

/* prototypes */

void eth_prep( void );
void eth_init( void );
int eth_open( int port, int srfd,
	get_userdata_t get_userdata, put_userdata_t put_userdata,
	put_pkt_t put_pkt, select_res_t sel_res );
int eth_ioctl( int fd, ioreq_t req);
int eth_read( int port, mnx_size_t count );
int eth_write( int port, mnx_size_t count );
int eth_cancel( int fd, int which_operation );
int eth_select( int fd, unsigned operations );
void eth_close( int fd );
int eth_send( int port, struct acc *data, mnx_size_t data_len );

#endif /* ETH_H */

/*
 * $PchId: eth.h,v 1.8 2005/06/28 14:16:10 philip Exp $
 */
