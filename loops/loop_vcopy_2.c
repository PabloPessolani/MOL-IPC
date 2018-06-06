#include <asm/ptrace.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/times.h>
#include <time.h>

#include "stub_syscall.h"
#include "./kernel/minix/config.h"
#include "./kernel/minix/ipc.h"
#include "./kernel/minix/kipc.h"

#include "./kernel/minix/proxy.h"
#include "./kernel/minix/molerrno.h"

char buffer[MAXBUFSIZE];

  
void  main ( int argc, char *argv[] )
{
	int dcid, pid, src_nr, src_ep, dst_ep, ret, ep, i, nodeid;
	message m;
	double t_start, t_stop, t_total;

    if ( argc != 4) {
 	        printf( "Usage: %s <dcid> <src_nr> <dst_ep>\n", argv[0] );
 	        exit(1);
	    }

	dcid = atoi(argv[1]);
	if ( dcid < 0 || dcid >= NR_DCS) {
 	        printf( "Invalid dcid [0-%d]\n", NR_DCS-1 );
 	        exit(1);
	    }

	src_nr = atoi(argv[2]);
	dst_ep = atoi(argv[3]);

	pid = getpid();

    src_ep = mnx_bind(dcid, src_nr);
	if( src_ep < 0 ) {
		printf("BIND ERROR %d\n",src_ep);
		exit(src_ep);
	}
    printf("Binding LOCAL process %d to DC%d with src_nr=%d\n",pid,dcid,src_nr);

	m.m_type= 0x01;
	m.m1_i1 = (int) &buffer;
	m.m1_i2 = 0x02;
	m.m1_i3 = 0x03;
   	ret = mnx_sendrec(dst_ep, (long) &m);
	if( ret != 0 )
    	printf("SENDREC ret=%d\n",ret);
	
	printf("buffer: [%s]\n", buffer);

    printf("Unbinding process %d from DC%d with src_nr=%d\n",pid,dcid,src_nr);
   	mnx_unbind(dcid,src_ep);

 }



