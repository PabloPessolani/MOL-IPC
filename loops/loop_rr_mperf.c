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
	int dcid, clt_pid, svr_pid, clt_ep, svr_ep, clt_nr, svr_nr, ret, i, f;
	message *m_ptr;
	double t_start, t_stop, t_total;
	int loops, children, pid;
	
	
	if (argc != 3) {
    		printf ("usage: %s <children> <loops>\n", argv[0]);
    		exit(1);
  	}
	
	children 	= atoi(argv[1]);
  	loops 		= atoi(argv[2]);
		
	dcid 	= DCID;
	clt_nr	= CLT_NR;
	svr_nr 	= SVR_NR;
	
	svr_pid = getpid();
    svr_ep = mnx_bind(dcid, svr_nr);

	if( svr_ep < 0 ) 
		printf("BIND ERROR svr_ep=%d\n",svr_ep);

   	printf("BIND DESTINATION dcid=%d svr_pid=%d svr_nr=%d svr_ep=%d\n",
		dcid, svr_pid, svr_nr, svr_ep);

    for (f = 0; f < children; f++)	{				/* creates children childs*/
		printf("FORK %d\n",f);
		if( (pid = fork()) == 0 ) {		/* Clients */
			/* Client binding */
			svr_pid  = getppid();
			svr_ep = mnx_getep(svr_pid);
			clt_pid = getpid();
			clt_nr	= CLT_NR + f;
			clt_ep =  mnx_bind(dcid, clt_nr);
			if( clt_ep < 0 ) 
				perror("CLIENT BIND ERROR");
			printf("BIND CLIENT %d dcid=%d clt_pid=%d clt_nr=%d clt_ep=%d\n",f,
				dcid,clt_pid,clt_nr,clt_ep);
		}
	}
	
	if(  f == children  )	{		/* SERVER  */
		printf("SERVER pause before RECEIVE/SEND\n");
		posix_memalign( (void **) &m_ptr, getpagesize(), sizeof(message) );
		if (m_ptr== NULL) {
    		perror("posix_memalign");
    		exit(1);
		}
		printf("m_ptr %p\n",m_ptr);
		
		t_start = dwalltime();
		for( i = 0; i < (children*loops); i++) {
				ret = mnx_rcvrqst((long) m_ptr);
				ret = mnx_reply(m_ptr->m_source, (long) m_ptr);
		}
   		printf("RECEIVE msg:" MSG1_FORMAT, MSG1_FIELDS(m_ptr));

    	t_stop  = dwalltime();
		t_total = (t_stop-t_start);
 		printf("t_start=%.2f t_stop=%.2f t_total=%.2f\n",t_start, t_stop, t_total);
 		printf("Loops = %d forks =%d loops*forks=%d\n", loops, children, (children*loops));
 		printf("Time for a pair of RECVRQST-REPLY= %f[ms]\n", 1000*t_total/2/(double)(children*loops));
 		printf("Throuhput = %f [RECVRQST-REPLY/s]\n", 2*(double)(children*loops)/t_total);
		
		for (f = 0; f < children; f++)
			wait(&ret);
		printf("SERVER exiting \n");	
	}else{						/* CHILDREN		*/
		posix_memalign( (void **) &m_ptr, getpagesize(), sizeof(message) );
		if (m_ptr== NULL) {
    		perror("posix_memalign");
    		exit(1);
		}
		printf("m_ptr %p\n",m_ptr);
	  	m_ptr->m_type= 0xFF;
		m_ptr->m1_i1 = clt_pid;
		m_ptr->m1_i2 = clt_ep;
		m_ptr->m1_i3 = 0x03;
   		printf("SEND msg:" MSG1_FORMAT, MSG1_FIELDS(m_ptr));

		for( i = 0; i < loops; i++){
//printf("endpoint=%d before send\n", clt_ep);
				ret = mnx_sendrec(svr_ep, (long) m_ptr);
//if(ret) printf("ERROR:%d endpoint=%d send\n",ret, clt_ep);
		}
		printf("Ending process:%d endpoint=%d\n",clt_pid,clt_ep);
	}
 exit(0);
}



