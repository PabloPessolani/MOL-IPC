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

#define 	VMID	0
#define 	SRC_NR	1
#define 	DST_NR	2


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
	int dcid, src_pid, dst_pid, src_ep, dst_ep, src_nr, dst_nr, ret, i, loops;
	message m;
	double t_start, t_stop, t_total;
	
  	if (argc != 2) {
    		printf ("usage: %s <loops>\n", argv[0]);
    		exit(1);
  	}

  	loops = atoi(argv[1]);

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

		printf("RECEIVER pause before RECEIVE\n");
		sleep(5); /* PAUSE before RECEIVE*/

		do {
			src_ep = mnx_getep(src_pid);
			} while(src_ep < 0);

		printf("Dump procs of VM%d\n",dcid); 
   		ret = mnx_proc_dump(dcid);
      		if( ret != OK){
			printf("PROCDUMP ERROR %d\n",ret); 
			exit(1);
		}

    		ret = mnx_receive(ANY, (long) &m);
   		ret = mnx_send(src_ep, (long) &m);
		
		t_start = dwalltime();
		for( i = 0; i < loops; i++) {
		    	ret = mnx_receive(ANY, (long) &m);
	    		ret = mnx_send(src_ep, (long) &m);
		}
     	t_stop  = dwalltime();

   		printf("RECEIVE msg: m_source=%d, m_type=%d, m1_i1=%d, m1_i2=%d, m1_i3=%d\n",
			m.m_source,
			m.m_type,
			m.m1_i1,
			m.m1_i2,
			m.m1_i3);

	    	printf("UNBIND DESTINATION dcid=%d dst_pid=%d dst_nr=%d dst_ep=%d\n",
			dcid,
			dst_pid,
			dst_nr,
			dst_ep);
	    	mnx_unbind(dcid,dst_ep);
		t_total = (t_stop-t_start);
 		printf("t_start=%.2f t_stop=%.2f t_total=%.2f\n",t_start, t_stop, t_total);
 		printf("Loops = %d\n", loops);
 		printf("Time for a pair of SEND/RECEIVE= %f[ms]\n", 1000*t_total/2/(double)loops);
 		printf("Throuhput = %f [SEND-RECEIVE/s]\n", (double)loops*2/t_total);
			wait(&ret);

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

	  	m.m_type= 0xFF;
		m.m1_i1 = 0x00;
		m.m1_i2 = 0x02;
		m.m1_i3 = 0x03;
   		printf("SEND msg: m_type=%d, m1_i1=%d, m1_i2=%d, m1_i3=%d\n",
			m.m_type,
			m.m1_i1,
			m.m1_i2,
			m.m1_i3);

		ret = mnx_sendrec(dst_ep, (long) &m);
		for( i = 0; i < loops; i++){
	    		ret = mnx_sendrec(dst_ep, (long) &m);
		}

		if( ret != 0 )
		    	printf("SEND ret=%d\n",ret);
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



