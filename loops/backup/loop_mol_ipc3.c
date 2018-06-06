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


#include "./kernel/minix/moldebug.h"
#define	printk	printf
#ifdef MOLDBG
#undef MOLDBG
#endif

#define 	VMID	0
#define 	SRC_NR	1
#define 	DST_NR	2
#define	LOOPS	500000


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
	int dcid, src_pid, dst_pid, src_ep, dst_ep, src_nr, dst_nr, ret, i,ntransf;
	message m;
	double t_start, t_stop, t_total;
	
		printf("PROGRAM: %s\n",argv[0]);

		
	dcid 	= VMID;
	src_nr	= SRC_NR;
	dst_nr 	= DST_NR;
	
	dst_pid = getpid();
    dst_ep =	mnx_bind(dcid, dst_nr);
	if( dst_ep < 0 ) 
		printf("BIND ERROR dst_ep=%d\n",dst_ep);

   	printf("BIND DESTINATION dcid=%d dst_pid=%d dst_nr=%d dst_ep=%d\n",
		dcid,
		dst_pid,
		dst_nr,
		dst_ep);

	if( (src_pid = fork()) != 0 )	{		/* PARENT = DESTINATION */
		printf("Dump procs of VM%d\n",dcid); 
   		ret = mnx_proc_dump(dcid);
      		if( ret != OK){
			printf("PROCDUMP ERROR %d\n",ret); 
			exit(1);
		}
	
		do {
			src_ep = mnx_getep(src_pid);
			sleep(1);
			} while(src_ep < 0);
			
		printf("RECEIVER pause before RECEIVE: src_ep=%d \n", src_ep);
		sleep(5); /* PAUSE before RECEIVE*/
		ntransf = 0;

		do {
			src_ep =mnx_getep(src_pid);
		}while(src_ep < 0);
		
    	ret = mnx_receive(ANY, (long) &m);
   		ret = mnx_send(src_ep, (long) &m);

		t_start = dwalltime();
		for( i = 0; i < LOOPS; i++) {
		    	ret = mnx_receive(ANY, (long) &m);
	    		ret = mnx_notify(src_ep);
		}
     	t_stop  = dwalltime();


	    	printf("UNBIND DESTINATION dcid=%d dst_pid=%d dst_nr=%d dst_ep=%d\n",
			dcid,
			dst_pid,
			dst_nr,
			dst_ep);
	    	mnx_unbind(dcid,dst_ep);
		t_total = (t_stop-t_start);
 		printf("t_start=%.2f t_stop=%.2f t_total=%.2f\n",t_start, t_stop, t_total);
 		printf("Loops = %d\n", LOOPS);
 		printf("Time for a pair of SEND/RECEIVE= %f[ms]\n", 1000*t_total/2/(double)LOOPS);
 		printf("Throuhput = %f [SEND-RECEIVE/s]\n", (double)LOOPS*2/t_total);

	}else{						/* SON = SOURCE		*/
		src_pid = getpid();
    	src_ep =mnx_bind(dcid, src_nr);
		if( src_ep < 0 ) 
			printf("BIND ERROR src_ep=%d\n",src_ep);

   		printf("BIND SOURCE dcid=%d src_pid=%d src_nr=%d src_ep=%d\n",
			dcid,
			src_pid,
			src_nr,
			src_ep);

		printf("SENDER pause before NOTIFY\n");
   		ret = mnx_sendrec(dst_ep,(long) &m);
		for( i = 0; i < LOOPS; i++){
	    		ret = mnx_notify(dst_ep);
		    	ret = mnx_receive(ANY, (long) &m);
		}

    		printf("UNBIND SOURCE dcid=%d src_pid=%d src_nr=%d src_ep=%d\n",
			dcid,
			src_pid,
			src_nr,
			src_ep);
	    	mnx_unbind(dcid, src_ep);
	}
 printf("\n");
 

 
 exit(0);
}



