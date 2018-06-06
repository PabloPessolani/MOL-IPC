/*
udp.h
 
Copyright 1995 Philip Homburg
*/

#ifndef UDP_H
#define UDP_H

#define UDP_DEF_OPT		NWUO_NOFLAGS
#define UDP_MAX_DATAGRAM	40000 /* 8192 */
#define UDP_READ_EXP_TIME	(10L * HZ)
#define UDP_TOS			0
#define UDP_IP_FLAGS		0

#define UDP0	0

struct acc;

void udp_prep( void );
void udp_init( void );
int udp_open( int port, int srfd,
	get_userdata_t get_userdata, put_userdata_t put_userdata, 
	put_pkt_t put_pkt, select_res_t select_res );
int udp_ioctl( int fd, ioreq_t req );
int udp_read( int fd, mnx_size_t count );
int udp_write( int fd, mnx_size_t count );
void udp_close( int fd );
int udp_cancel( int fd, int which_operation );

#endif /* UDP_H */


/*
 * $PchId: udp.h,v 1.9 2005/06/28 14:12:05 philip Exp $
 */
