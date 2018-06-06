/*
inet/qp.h

Handle queryparams requests

Created:	June 1995 by Philip Homburg <philip@f-mnx.phicoh.com>

Copyright 1995 Philip Homburg
*/ 

#ifndef INET__QP_H
#define INET__QP_H

void qp_init ( void );
int qp_query ( int proc, vir_bytes argp );

#endif /* INET__QP_H */

/*
 * $PchId: qp.h,v 1.4 2005/01/29 18:08:06 philip Exp $
 */
