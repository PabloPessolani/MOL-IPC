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
	message m;
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
		t_start = dwalltime();
		for( i = 0; i < (children*loops); i++) {
//printf("SERVER endpoint=%d before receive\n", svr_ep);
		    	ret = mnx_receive(ANY, (long) &m);
if(ret) printf("SERVER ERROR:%d endpoint=%d receive\n",ret, svr_ep);
//printf("SERVER endpoint=%d before send\n", svr_ep);
	    		ret = mnx_send(m.m_source, (long) &m);
if(ret) printf("SERVER ERROR:%d endpoint=%d send\n",ret, svr_ep);
		}
    	t_stop  = dwalltime();
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
 		printf("t_start=%.2f t_stop=%.2f t_total=%.2f\n",t_start, t_stop, t_total);
 		printf("Loops = %d forks =%d loops*forks=%d\n", loops, children, (children*loops));
 		printf("Time for a pair of SEND/RECEIVE= %f[ms]\n", 1000*t_total/2/(double)(children*loops));
 		printf("Throuhput = %f [SEND-RECEIVE/s]\n", 2*(double)(children*loops)/t_total);
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

		for( i = 0; i < loops; i++){
//printf("endpoint=%d before send\n", clt_ep);
				ret = mnx_send(svr_ep, (long) &m);
if(ret) printf("ERROR:%d endpoint=%d send\n",ret, clt_ep);
//printf("endpoint=%d before receive\n", clt_ep);
	    		ret = mnx_receive(svr_ep, (long) &m);
if(ret) printf("ERROR:%d endpoint=%d receive\n",ret, clt_ep);
		}
		printf("Ending process:%d endpoint=%d\n",clt_pid,clt_ep);
	}
 exit(0);
}



