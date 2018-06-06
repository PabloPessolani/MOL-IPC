#include <asm/ptrace.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/times.h>
#include <time.h>
#include <sys/types.h>
#include <sys/wait.h> 
#include <pthread.h>
#include <sys/time.h>
#include <pthread.h>

#include "stub_syscall.h"
#include "./kernel/minix/config.h"
#include "./kernel/minix/ipc.h"
#include "./kernel/minix/kipc.h"
#include <sys/syscall.h>

#include "./kernel/minix/moldebug.h"
#define	printk	printf
#ifdef MOLDBG
#undef MOLDBG
#endif

#define 	DCID	0
#define 	DST_NR	1
#define 	SRC_NR	2
#define 	MAXTHREADS	100

int dcid, loops, dst_ep, src_ep, src_nr, threads, dst_nr ;

#define MAXBUF 	4096
char	dst_buf[MAXBUF];
char	src_buf[MAXBUF];



void *thread_function(void *arg) {
	message m;
	int i , ret ;
	pid_t tid;

	tid = (pid_t) syscall (SYS_gettid);

	/* BIND PARENT THREAD */
  	dst_ep =	mnx_bind(dcid, dst_nr);
	if( dst_ep < 0 ) 
		printf("BIND PARENT ERROR dst_ep=%d\n",dst_ep);


    src_ep =mnx_tbind(dcid, src_nr);
	if( src_ep < 0 ) {
		printf("BIND SELF ERROR src_ep=%d\n",src_ep);
		return NULL;
	}
   	printf("BIND SOURCE dcid=%d tid=%d src_nr=%d src_ep=%d\n",
		dcid,
		tid,
		src_nr,
		src_ep);

 	m.m_type = tid;
	m.m1_i1 = i = 0;
	m.m1_i2 = 0x06;
	m.m1_i3 = 0x07;

	ret = mnx_send(dst_ep, (long) &m);
	for( i = 0; i < loops ; i++){
 		m.m1_i1 = i;
		ret = mnx_sendrec(dst_ep, (long) &m);
		if( ret != 0 ) printf("ERROR THREAD SENDREC ret=%d\n",ret);
	}

//	ret =  mnx_unbind(dcid, src_ep);
	printf("THREAD exiting\n");

	return NULL;
}

double dwalltime()
{
	double sec;
	struct timeval tv;

	gettimeofday(&tv,NULL);
	sec = tv.tv_sec + tv.tv_usec/1000000.0;
	return sec;
}

   
int  main ( int argc, char *argv[] )
{
	int  src_pid, dst_pid, ret, i;
	message m;
	double t_start, t_stop, t_total;
     pthread_t mythread;
	pid_t tid[MAXTHREADS], mytid;


  	if (argc != 2) {
    		printf ("usage: %s <loops> \n", argv[0]);
    		exit(1);
  	}

  	loops = atoi(argv[1]);

	dcid 	= DCID;
	src_nr	= SRC_NR;
	dst_nr 	= DST_NR;
	
	dst_pid = getpid();
//  	dst_ep =	mnx_bind(dcid, dst_nr);
//	if( dst_ep < 0 ) 
//		printf("BIND ERROR dst_ep=%d\n",dst_ep);

   	printf("BIND DESTINATION dcid=%d dst_pid=%d dst_nr=%d dst_ep=%d\n",
		dcid,
		dst_pid,
		dst_nr,
		dst_ep);


	if ( (ret = pthread_create(&mythread, NULL, thread_function, NULL)) ) {
			printf("error creating thread.");
			abort();
	}
	printf("MAIN: Creating thread tid=%ld\n", mythread);

	printf("RECEIVER pause before RECEIVE\n");
	sleep(2); /* PAUSE before RECEIVE*/

    	ret = mnx_receive(ANY, (long) &m);
	t_start = dwalltime();
	for( i = 0; i < loops; i++) {
	    	ret = mnx_receive(ANY, (long) &m);
    		ret = mnx_send(m.m_source, (long) &m);
	}
     	t_stop  = dwalltime();
   	printf("RECEIVE msg: m_source=%d, m_type=%d, m1_i1=%d, m1_i2=%d, m1_i3=%d\n",
			m.m_source,
			m.m_type,
			m.m1_i1,
			m.m1_i2,
			m.m1_i3);

	t_total = (t_stop-t_start);
 	printf("t_start=%.2f t_stop=%.2f t_total=%.2f\n",t_start, t_stop, t_total);
 	printf("Loops = %d\n", loops);
 	printf("Time for a pair of SEND/RECEIVE= %f[ms]\n", 1000*t_total/2/(double)loops);
 	printf("Throuhput = %f [SEND-RECEIVE/s]\n", (double)loops*2/t_total);

			
	printf("MAIN Waiting thread\n");
	if ( pthread_join ( mythread, NULL ) ) {
		printf("error joining thread\n");
		abort();
	}
 printf("\n");
 exit(0);
}



