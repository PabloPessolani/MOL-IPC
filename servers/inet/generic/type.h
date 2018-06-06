/*
type.h

Copyright 1995 Philip Homburg
*/

#ifndef INET_TYPE_H
#define INET_TYPE_H

typedef struct acc *(*get_userdata_t) ( int fd, mnx_size_t offset,mnx_size_t count, int for_ioctl );
typedef int (*put_userdata_t) ( int fd, mnx_size_t offset,struct acc *data, int for_ioctl );
typedef void (*put_pkt_t) ( int fd, struct acc *data, mnx_size_t datalen );
typedef void (*select_res_t) ( int fd, unsigned ops );

#endif /* INET_TYPE_H */

/*
 * $PchId: type.h,v 1.6 2005/06/28 14:22:04 philip Exp $
 */
