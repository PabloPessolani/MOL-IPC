#include <asm/ptrace.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "stub_syscall.h"
#include "./kernel/minix/config.h"
#include "./kernel/minix/ipc.h"
#include "./kernel/minix/kipc.h"
#include "./kernel/minix/molerrno.h"

#define WAITTIME	3

   
void  main ( int argc, char *argv[] )
{
	int dcid, pid, src_nr, src_ep, dst_ep, ret;
	message m;

    	if ( argc != 3) {
 	        printf( "Usage: %s <dcid> <dst_ep> \n", argv[0] );
 	        exit(1);
	    }

	dcid = atoi(argv[1]);
	if ( dcid < 0 || dcid >= NR_DCS) {
 	        printf( "Invalid dcid [0-%d]\n", NR_DCS-1 );
 	        exit(1);
	    }

	src_nr = 2;
	dst_ep = atoi(argv[2]);
	pid = getpid();

	do {
	    	src_ep = mnx_bind(dcid, src_nr);
		if( (src_ep != EMOLBUSY) && (src_ep < 0))			
			exit(src_ep);
		src_nr++;
	}while( src_ep == EMOLBUSY );

    	printf("Binding process %d to DC%d with src_nr=%d src_ep=%d\n",pid,dcid,src_nr,src_ep);

  	m.m_type= 0xFF;
	m.m1_i1 = src_nr;
	m.m1_i2 = 0x02;
	m.m1_i3 = 0x03;
   	printf("SEND msg: m_type=%d, m1_i1=%d, m1_i2=%d, m1_i3=%d\n",
		m.m_type,
		m.m1_i1,
		m.m1_i2,
		m.m1_i3);
    	ret = mnx_sendrec(dst_ep, (long) &m);
	if( ret != 0 )
	    	printf("SENDREC ret=%d\n",ret);

	printf("CLIENT sleep before UNBIND\n");
	sleep(WAITTIME);
	

    	mnx_unbind(dcid, src_ep);

 }



