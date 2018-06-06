#include <asm/ptrace.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/times.h>
#include <time.h>
#include <sys/types.h>
#include <sys/wait.h> 


#include "stub_syscall.h"
#include "./kernel/minix/config.h"
#include "./kernel/minix/ipc.h"
#include "./kernel/minix/kipc.h"


#include "./kernel/minix/moldebug.h"
#define	printk	printf
#ifdef MOLDBG
#undef MOLDBG
#endif

#define 	DCID	0
#define 	clt_NR	1
#define 	svr_NR	2


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
	int dcid, clt_pid, svr_pid, clt_ep, svr_ep, clt_nr, svr_nr, ret, i, loops;
	message m;
	double t_start, t_stop, t_total;
	
  	if (argc != 2) {
    		printf ("usage: %s <loops>\n", argv[0]);
    		exit(1);
  	}

  	loops = atoi(argv[1]);

	dcid 		= DCID;
	clt_nr	= clt_NR;
	svr_nr 	= svr_NR;
	
	t_start = dwalltime();
	for( i = 0; i < loops; i++) {
   		clt_ep =mnx_bind(dcid, clt_nr);
   		mnx_unbind(dcid, clt_nr);
	}
   	t_stop  = dwalltime();
			
	t_total = (t_stop-t_start);
	printf("t_start=%.2f t_stop=%.2f t_total=%.2f\n",t_start, t_stop, t_total);
	printf("Loops = %d\n", loops);
	printf("Time for a pair of BIND/UNBIND= %f[ms]\n", 1000*t_total/(double)loops);
	printf("Throuhput = %f [BIND-UNBIND/s]\n", (double)(loops)/t_total);

 exit(0);
}



