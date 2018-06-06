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
#ifdef MOLDBGXXXXX
#undef MOLDBG
#endif

#define 	DCID	0
#define 	SVR_NR	1
#define 	CLT_NR	2

int child[200];

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
	int dcid, clt_pid, svr_pid, clt_ep, svr_ep, clt_nr, svr_nr, ret, i, f, j;
	message m;
	int loops, children, pid, maxbuf ;
	long long total_bytes = 0 ;
	double t_start, t_stop, t_total,loopbysec, tput;
	char *buffer;

	if (argc != 4) {
    		printf ("usage: %s <children> <loops> <bufsize> \n", argv[0]);
    		exit(1);
  	}
	
	children 	= atoi(argv[1]);
  	loops 		= atoi(argv[2]);
	maxbuf 		= atoi(argv[3]);

	posix_memalign( &buffer, getpagesize(), maxbuf );
	if (buffer== NULL) {
    		perror("posix_memalign");
    		exit(1);
  	}
	printf("buffer %p\n",buffer);
	
	dcid 	= DCID;
	clt_nr	= CLT_NR;
	svr_nr 	= SVR_NR;
	
	svr_pid = getpid();
    svr_ep = mnx_bind(dcid, svr_nr);

	if( svr_ep < 0 ) 
		printf("BIND ERROR svr_ep=%d\n",svr_ep);

   	printf("BIND SERVER dcid=%d svr_pid=%d svr_nr=%d svr_ep=%d\n",
		dcid,
		svr_pid,
		svr_nr,
		svr_ep);

    for (f = 0; f < children; f++)	{				/* creates children childs*/
		printf("FORK %d\n",f);
		if( (pid = fork()) == 0) break;
	}
	
	if(  f == children  )	{		/* SERVER  */
		printf("SERVER pause before SEND/RECEIVE\n");
		sleep(5); 
/*
		printf("Dump procs of DC%d\n",dcid); 
   		ret = mnx_proc_dump(dcid);
      		if( ret != OK){
			printf("PROCDUMP ERROR %d\n",ret); 
			exit(1);
		}
*/		
		for( i = 0; i < children; i++) {
	    	ret = mnx_receive(ANY, (long) &m);
			child[i] = m.m_source;
printf("SERVER: message received from %d\n", child[i]);
		}
		sleep(5); 

		t_start = dwalltime();
		for( j = 0; j < loops; j++) {
			for( i = 0; i < children; i++) {
				ret = mnx_vcopy( child[i], buffer, svr_ep, buffer, maxbuf);	
			}
		}
		t_stop  = dwalltime();
		
		for( i = 0; i < children; i++) {
	    	ret = mnx_send(child[i], (long) &m);
printf("SERVER: sending message to %d\n", child[i]);
		}
/*
		ret = mnx_proc_dump(dcid);
      	if( ret != OK){
			printf("PROCDUMP ERROR %d\n",ret); 
			exit(1);
		}	
*/
   		printf("RECEIVE msg: m_CLIENT=%d, m_type=%d, m1_i1=%d, m1_i2=%d, m1_i3=%d\n",
			m.m_source,
			m.m_type,
			m.m1_i1,
			m.m1_i2,
			m.m1_i3);

		t_total = (t_stop-t_start);
		loopbysec = (double)(loops*children)/t_total;
		tput = loopbysec * (double)maxbuf;
		
 		printf("t_start=%.2f t_stop=%.2f t_total=%.2f\n",t_start, t_stop, t_total);
 		printf("transfer size=%d #transfers=%d loopbysec=%f\n", maxbuf, loops, loopbysec);
 		printf("Throuhput = %f [bytes/s]\n", tput);
		
		for (f = 0; f < children; f++)
			wait(&ret);
		printf("SERVER exiting \n");	
	}else{						/* CHILDREN		*/

		clt_pid = getpid();
		clt_nr	= CLT_NR + f;
	   	clt_ep =  mnx_bind(dcid, clt_nr);
		if( clt_ep < 0 ) 
			printf("BIND ERROR clt_ep=%d\n",clt_ep);
    	printf("BIND CLIENT dcid=%d clt_pid=%d clt_nr=%d clt_ep=%d\n",
			dcid,
			clt_pid,
			clt_nr,
			clt_ep);

	  	m.m_type= 0xFF;
		m.m1_i1 = clt_pid;
		m.m1_i2 = clt_ep;
		m.m1_i3 = 0x03;
   		printf("SEND msg: m_type=%d, m1_i1=%d, m1_i2=%d, m1_i3=%d\n",
			m.m_type,
			m.m1_i1,
			m.m1_i2,
			m.m1_i3);

		ret = mnx_sendrec(svr_ep, (long) &m);
if(ret) printf("ERROR:%d endpoint=%d sendrec\n",ret, clt_ep);
		printf("Ending process:%d endpoint=%d\n",clt_pid,clt_ep);
	}
 exit(0);
}



