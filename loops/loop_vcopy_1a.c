#include <asm/ptrace.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include "stub_syscall.h"
#include "./kernel/minix/config.h"
#include "./kernel/minix/ipc.h"
#include "./kernel/minix/kipc.h"

#define PAGE_SIZE 4096
#include "./kernel/minix/proxy.h"
#include "./kernel/minix/molerrno.h"

char buffer[MAXBUFSIZE];
   
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
	int dcid, pid, p_nr, lcl_ep, ret, ep, i, loops, len;
	message m;
	double t_start, t_stop, t_total;

    if ( argc != 4) {
 	        printf( "Usage: %s <dcid> <p_nr> <loops>\n", argv[0] );
 	        exit(1);
	    }

	dcid = atoi(argv[1]);
	if ( dcid < 0 || dcid >= NR_DCS) {
 	        printf( "Invalid dcid [0-%d]\n", NR_DCS-1 );
 	        exit(1);
	    }

	p_nr = atoi(argv[2]);
	loops  = atoi(argv[3]);

	pid = getpid();

    lcl_ep = mnx_bind(dcid, p_nr);
	if( lcl_ep < 0 ) {
		printf("BIND ERROR %d\n",lcl_ep);
		exit(lcl_ep);
	}
    printf("Binding LOCAL process %d to DC%d with p_nr=%d\n",pid,dcid,p_nr);

	ret = mnx_receive(ANY, (long) &m);
   	if( ret != OK)
	printf("RECEIVE ERROR %d\n",ret);
	
	t_start = dwalltime();
   	for (i = 0 ; i < loops; i++) {		
		ret = mnx_vcopy(m.m_source, m.m1_i1, lcl_ep, (int) buffer, m.m1_i2);
	
		if( ret != 0 )
		    	printf("VCOPY ret=%d\n",ret);
	}
 	t_stop  = dwalltime();

	printf("DATA [%s]\n", buffer);

	ret = mnx_send(m.m_source, (long) &m);
	if( ret != 0 )
   	printf("SEND ret=%d\n",ret);
	
	t_total = (t_stop-t_start);
 	printf("t_start=%.2f t_stop=%.2f t_total=%.2f\n",t_start, t_stop, t_total);
 	printf("Loops = %d\n", loops);
 	printf("Time for a pair of COPYIN= %f[ms]\n", 1000*t_total/(double)loops);
 	printf("Throuhput = %f [COPYIN/s]\n", (double)loops/t_total);

	
    printf("Unbinding process %d from DC%d with src_nr=%d\n",pid,dcid,p_nr);
   	mnx_unbind(dcid,lcl_ep);

 }



