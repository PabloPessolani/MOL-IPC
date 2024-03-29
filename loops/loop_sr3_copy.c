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


#include "./kernel/minix/moldebug.h"
#define	printk	printf
#ifdef MOLDBG
#undef MOLDBG
#endif

#define 	DCID	0
#define 	SRC_NR	1
#define 	DST_NR	2
#define		LOOPS		200000

#define		MAXBUF	4096
char	buffer[MAXBUF];


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
	int dcid, src_pid, dst_pid, src_ep, dst_ep, src_nr, dst_nr, ret, i, ntransf;
	message m;
	long int fbytes, total_bytes = 0 ;
	double t_start, t_stop, t_total;
	
		printf("PROGRAM: %s\n",argv[0]);

	dcid 	= DCID;
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

	for(i = 0; i < MAXBUF; i++)
		buffer[i] = ((i%10) + '0');	
		
	if( (src_pid = fork()) != 0 )	{		/* PARENT */
		printf("Dump procs of DC%d\n",dcid); 
   		ret = mnx_proc_dump(dcid);
      		if( ret != OK){
			printf("PROCDUMP ERROR %d\n",ret); 
			exit(1);
		}

		printf("RECEIVER pause before RECEIVE\n");
		sleep(5); /* PAUSE before RECEIVE*/
		do {
			src_ep = mnx_getep(src_pid);
			} while(src_ep < 0);
			
	
		total_bytes = 0;
		ntransf = 0;
	    	ret = mnx_receive(ANY, (long) &m);
		t_start = dwalltime();
		for( i = 0; i < LOOPS; i++) {
			ret = mnx_vcopy(dst_ep, buffer, src_ep, buffer, MAXBUF);	
			total_bytes+=MAXBUF;
			ntransf++;
		}
	    t_stop  = dwalltime();
  		ret = mnx_send(m.m_source, (long) &m);

	    	printf("UNBIND DESTINATION dcid=%d dst_pid=%d dst_nr=%d dst_ep=%d\n",
			dcid,
			dst_pid,
			dst_nr,
			dst_ep);
	    	mnx_unbind(dcid,dst_ep);

		t_total = (t_stop-t_start);
 		printf("t_start=%.2f t_stop=%.2f t_total=%.2f\n",t_start, t_stop, t_total);
 		printf("Total Bytes = %d transfer size=%d #transfers=%d\n", total_bytes, MAXBUF, ntransf);
 		printf("Throuhput = %f [bytes/s]\n", (double)total_bytes/t_total);

	}else{						/* SON = 		*/
		src_pid = getpid();
    		src_ep =mnx_bind(dcid, src_nr);
		if( src_ep < 0 ) 
			printf("BIND ERROR src_ep=%d\n",src_ep);

   		printf("BIND SOURCE dcid=%d src_pid=%d src_nr=%d src_ep=%d\n",
			dcid,
			src_pid,
			src_nr,
			src_ep);

	  	m.m_type= 0x0001;
		m.m1_i1 = 0x00;
		m.m1_i2 = 0x02;
		m.m1_i3 = 0x03;
   		printf("SEND msg: m_type=%d, m1_i1=%d, m1_i2=%d, m1_i3=%d\n",
			m.m_type,
			m.m1_i1,
			m.m1_i2,
			m.m1_i3);

    		ret = mnx_sendrec(dst_ep, (long) &m);

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



