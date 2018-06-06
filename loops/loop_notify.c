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

double dwalltime()
{
	double sec;
	struct timeval tv;

	gettimeofday(&tv,NULL);
	sec = tv.tv_sec + tv.tv_usec/1000000.0;
	return sec;
}

   
void  main ( int argc, char *argv[] )
{
	int dcid, pid, src_nr, src_ep, dst_ep, ret, ep, i, nodeid, loops;
	message m;
	double t_start, t_stop, t_total;

    if ( argc != 5) {
 	        printf( "Usage: %s <dcid> <src_nr> <dst_ep> <loops>\n", argv[0] );
 	        exit(1);
	    }

	dcid = atoi(argv[1]);
	if ( dcid < 0 || dcid >= NR_DCS) {
 	        printf( "Invalid dcid [0-%d]\n", NR_DCS-1 );
 	        exit(1);
	    }

	src_nr = atoi(argv[2]);
	dst_ep = atoi(argv[3]);
	loops  = atoi(argv[4]);

	pid = getpid();

    src_ep = mnx_bind(dcid, src_nr);
	if( src_ep < 0 ) {
		printf("BIND ERROR %d\n",src_ep);
		exit(src_ep);
	}
    printf("Binding LOCAL process %d to DC%d with src_nr=%d\n",pid,dcid,src_nr);

	t_start = dwalltime();
	for (i = 0 ; i < loops; i++) {
    	ret = mnx_notify(dst_ep);
		if( ret != 0 )
	    	printf("NOTIFY ret=%d\n",ret);
	}
    	t_stop  = dwalltime();

	t_total = (t_stop-t_start);
 	printf("t_start=%.2f t_stop=%.2f t_total=%.2f\n",t_start, t_stop, t_total);
 	printf("Loops = %d\n", loops);
 	printf("Time for a pair of NOTIFY= %f[ms]\n", 1000*t_total/(double)loops);
 	printf("Throuhput = %f [NOTIFY/s]\n", (double)loops/t_total);

    printf("Unbinding process %d from DC%d with src_nr=%d\n",pid,dcid,src_nr);
   	mnx_unbind(dcid,src_ep);

 }



