#define  MOL_USERSPACE	1

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <setjmp.h>
#include <pthread.h>
#include <sched.h>
#include <assert.h>

#include <sys/types.h>
#include <sys/time.h>
#include <sys/times.h>
#include <sys/socket.h>
#include <sys/resource.h>
#include <sys/sysinfo.h> 
#include <sys/stat.h>
#include <fcntl.h>
#include <malloc.h>

#include <netinet/in.h>
#include <arpa/inet.h>
#include "stub_syscall.h"
#include "./kernel/minix/config.h"
#include "./kernel/minix/ipc.h"
#include "./kernel/minix/kipc.h"
#include "./kernel/minix/proc_usr.h"
#include "./kernel/minix/proc_sts.h"

#include "./kernel/minix/moldebug.h"
#define	printk	printf
#ifdef MOLDBGXXXXX
#undef MOLDBG
#endif

#define 	DCID	0
#define 	PARENT 	0
#define 	CHILD	1

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
	int dcid, clt_pid, svr_pid, clt_ep, par_ep, svr_ep, clt_nr, svr_nr, ret, i, f;
	message *m_ptr;
	double t_start, t_stop, t_total;
	int loops, peers, pid;
	proc_usr_t proc, *p_ptr;
	
	p_ptr = &proc;
	
	if (argc != 3) {
    	printf ("usage: %s <peers> <loops>\n", argv[0]);
    	exit(1);
  	}
	
	peers 	= atoi(argv[1]);
	if( peers%2) {
		printf ("peers must be even\n");
		exit(1);
	}
	
  	loops 		= atoi(argv[2]);
	dcid 	= DCID;
	
    for (f = 0; f < peers; f+=2)	{				
		printf("FORK %d\n",f);
		if( (pid = fork()) == 0) {
			/* Server binding */
			svr_pid = getpid();
			svr_nr	= CHILD + f;
			svr_ep =  mnx_bind(dcid, svr_nr);
			if( svr_ep < 0 ) 
				perror("SERVER BIND ERROR");
			printf("BIND SERVER %d dcid=%d svr_pid=%d svr_nr=%d svr_ep=%d\n",f,
				dcid,svr_pid,svr_nr,svr_ep);
			if( (pid = fork()) == 0 ) {		/* Clients */
				/* Client binding */
				clt_pid = getpid();
				f = f+1;
				clt_nr	= CHILD + f;
				clt_ep =  mnx_bind(dcid, clt_nr);
				if( clt_ep < 0 ) 
					perror("CLIENT BIND ERROR");
				printf("BIND CLIENT %d dcid=%d clt_pid=%d clt_nr=%d clt_ep=%d\n",f,
					dcid,clt_pid,clt_nr,clt_ep);
			}
			break;
		}
	}
	
	if(  f == peers  )	{		/* MAIN  */
		printf("MAIN \n");
		par_ep =  mnx_bind(dcid, PARENT);
		if( par_ep < 0 ) 
				printf("BIND ERROR par_ep=%d\n",par_ep);
			
		posix_memalign( (void **) &m_ptr, getpagesize(), sizeof(message) );
		if (m_ptr== NULL) {
    		perror("posix_memalign");
    		exit(1);
		}
		printf("MAIN m_ptr %p\n",m_ptr);		

		for (f = 0; f < peers; ){
			ret = mnx_getprocinfo(dcid, CHILD + f, p_ptr);
			printf("mnx_getprocinfo ret=%d\n",ret);
			if( ret == OK && p_ptr->p_rts_flags != SLOT_FREE){
				printf(PROC_USR_FORMAT, PROC_USR_FIELDS(p_ptr));
				f++;
			}else{
				sleep(1);
			}
		}
		
		
		/* To synchronize  the START */
		for (f = 0; f < peers; f++)
    		ret = mnx_send(CHILD+f, (long) m_ptr);
		t_start = dwalltime();
		
		/* To synchronize the END  */
		for (f = 0; f < peers; f++)
    		ret = mnx_receive(ANY, (long) m_ptr);		
		t_stop  = dwalltime();
		
		t_total = (t_stop-t_start);
 		printf("t_start=%.2f t_stop=%.2f t_total=%.2f\n",t_start, t_stop, t_total);
 		printf("Loops=%d peers=%d loops*peers=%d\n", loops, peers, (peers*loops));
 		printf("Time for a pair of SEND/RECEIVE= %f[ms]\n", 1000*t_total/(double)(peers*loops));
 		printf("Throuhput = %f [SEND-RECEIVE/s]\n", (double)(peers*loops)/t_total);

		for (f = 0; f < peers/2; f++)
			wait(&ret);

		printf("MAIN exiting \n");	
	}else{ /* peers */
		posix_memalign( (void **) &m_ptr, getpagesize(), sizeof(message) );
		if (m_ptr== NULL) {
    		perror("posix_memalign");
    		exit(1);
		}
		printf("%d m_ptr %p\n",f, m_ptr);
		
		/* To synchronize */
    	ret = mnx_receive(ANY, (long) m_ptr);
		if( f%2 == 0 ) {		/* Servers */
printf("SERVER endpoint=%d before loop\n", svr_ep);
			for( i = 0; i < loops; i++) {
		    	ret = mnx_receive(ANY, (long) m_ptr);
//				if(ret) printf("SERVER ERROR:%d endpoint=%d receive\n",ret, svr_ep);
printf("SERVER endpoint=%d before send\n", svr_ep);
	    		ret = mnx_send(m_ptr->m_source, (long) m_ptr);
//				if(ret) printf("SERVER ERROR:%d endpoint=%d send\n",ret, svr_ep);
			}
    		ret = mnx_send(PARENT, (long) m_ptr);
			wait(&ret);
			printf("Ending SERVER %d: pid=%d endpoint=%d\n", f, svr_pid,svr_ep);	
		}else{				/* Clients	*/
printf("CLIENT  endpoint=%d before loop\n", clt_ep);

			m_ptr->m_type= 0xFF;
			m_ptr->m1_i1 = clt_pid;
			m_ptr->m1_i2 = clt_ep;
			m_ptr->m1_i3 = 0x03;
			printf("SENDREC msg: m_type=%d, m1_i1=%d, m1_i2=%d, m1_i3=%d\n",
				m_ptr->m_type, m_ptr->m1_i1, m_ptr->m1_i2, m_ptr->m1_i3);
			for( i = 0; i < loops; i++){
printf("endpoint=%d before sendrec\n", clt_ep);
				ret = mnx_sendrec(svr_ep, (long) m_ptr);
//				if(ret) printf("ERROR:%d endpoint=%d sendrec\n",ret, clt_ep);
			}
    		ret = mnx_send(PARENT, (long) m_ptr);
			printf("Ending CLIENT %d: pid=%d endpoint=%d\n",f ,clt_pid,clt_ep);	
		}
	}
 exit(0);
}



