
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
#include "./kernel/minix/molerrno.h"


#include "./kernel/minix/moldebug.h"
#define	printk	printf
#ifdef MOLDBG
#undef MOLDBG
#endif

#define 	DCID	0
#define 	RQT_NR	1
#define 	SRC_NR	2
#define 	DST_NR	3


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
	int dcid, src_pid, dst_pid, src_ep, dst_ep, src_nr, dst_nr, ret, i, maxbuf;
	int rqt_pid, rqt_ep, rqt_nr;
	message m;
	long int loops, total_bytes=0;
	double t_start, t_stop, t_total,loopbysec, tput;
	char *buffer;

	  	if (argc != 3) {
    		printf ("usage: %s <loops> <bufsize> \n", argv[0]);
    		exit(1);
  	}

  	loops = atoi(argv[1]);
  	maxbuf = atoi(argv[2]);

	posix_memalign( (void**) &buffer, getpagesize(), maxbuf);
  	if (buffer== NULL) {
    		perror("malloc");
    		exit(1);
  	}

	dcid 	= DCID;
	src_nr	= SRC_NR;
	dst_nr 	= DST_NR;
	rqt_nr 	= RQT_NR;
	
	rqt_pid = getpid();
    	rqt_ep =	mnx_bind(dcid, rqt_nr);
	if( rqt_ep < 0 ) 
		printf("BIND ERROR rqt_ep=%d\n",rqt_ep);

   	printf("BIND REQUESTER dcid=%d rqt_pid=%d rqt_nr=%d rqt_ep=%d\n",
		dcid,
		rqt_pid,
		rqt_nr,
		rqt_ep);

	for(i = 0; i < maxbuf; i++)
		buffer[i] = ((i%10) + '0');	
		
	if( (src_pid = fork()) != 0 )	{		/* FIRST CHILD SOURCE */
		if( (dst_pid = fork()) != 0 )	{		/* PARENT  REQUESTER */
			printf("RECEIVER pause before RECEIVE\n");
			sleep(1); /* PAUSE before RECEIVE*/
			do {
				src_ep = mnx_getep(src_pid);
			} while(src_ep < 0);
			
			do {
				dst_ep = mnx_getep(dst_pid);
			} while(dst_ep < 0);

			printf("Dump procs of DC%d\n",dcid); 
   			ret = mnx_proc_dump(dcid);
      		if( ret != OK){
				printf("PROCDUMP ERROR %d\n",ret); 
				exit(1);
			}

		    	ret = mnx_receive(src_ep, (long) &m);
		    	ret = mnx_receive(dst_ep, (long) &m);
			t_start = dwalltime();
			for( i = 0; i < loops; i++) {
				ret = mnx_vcopy(dst_ep, buffer, src_ep, buffer, maxbuf);	
			}
			t_stop  = dwalltime();
			ret = mnx_send(src_ep, (long) &m);
			ret = mnx_send(dst_ep, (long) &m);

	    		printf("UNBIND REQUESTER dcid=%d rqt_pid=%d rqt_nr=%d rqt_ep=%d\n",
				dcid,
				rqt_pid,
				rqt_nr,
				rqt_ep);
	    		mnx_unbind(dcid,rqt_ep);

			t_total = (t_stop-t_start);
			total_bytes = loops*maxbuf;
			loopbysec = (double)(loops)/t_total;
			tput = loopbysec * (double)maxbuf;	
 			printf("t_start=%.2f t_stop=%.2f t_total=%.2f\n",t_start, t_stop, t_total);
 		printf("transfer size=%d #transfers=%d loopbysec=%f\n", maxbuf, loops, loopbysec);
 			printf("Throuhput = %f [bytes/s]\n", tput);
			wait(&ret);
		} else {					/* SECOND CHILD: DESTINATION */
			dst_pid = getpid();
			dst_ep =mnx_bind(dcid, dst_nr);
			if( dst_ep < 0 ) 
				printf("BIND ERROR dst_ep=%d\n",dst_ep);

			printf("BIND DESTINATION dcid=%d dst_pid=%d dst_nr=%d dst_ep=%d\n",
				dcid,
				dst_pid,
				dst_nr,
				dst_ep);

			m.m_type= 0x0001;
			m.m1_i1 = 0x00;
			m.m1_i2 = 0x02;
			m.m1_i3 = 0x03;
			printf("SEND msg: m_type=%d, m1_i1=%d, m1_i2=%d, m1_i3=%d\n",
				m.m_type,
				m.m1_i1,
				m.m1_i2,
				m.m1_i3);

			ret = mnx_sendrec(rqt_ep, (long) &m);

			printf("UNBIND DESTINATION dcid=%d dst_pid=%d dst_nr=%d dst_ep=%d\n",
				dcid,
				dst_pid,
				dst_nr,
				dst_ep);
				mnx_unbind(dcid, dst_ep);
		}
	}else{						/* FIRST CHILD: SOURCE		*/
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

    		ret = mnx_sendrec(rqt_ep, (long) &m);

    		printf("UNBIND SOURCE dcid=%d src_pid=%d src_nr=%d src_ep=%d\n",
			dcid,
			src_pid,
			src_nr,
			src_ep);
	    	mnx_unbind(dcid, src_ep);
		wait(&ret);

	}
 printf("\n");
  
 exit(0);
}



