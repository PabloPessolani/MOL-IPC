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
#include <string.h>
#include <errno.h>
#include <ctype.h>

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
#define 	SVR_NR	1
#define 	CLT_NR	2
#define 	MAXTHREADS	100

int dcid, loops, dst_ep, src_nr, src_ep,  dst_nr, threads ;
int t_nr[MAXTHREADS];
message *m_th[MAXTHREADS]; /* message pointer array for threads */

void *thread_function(void *arg) {
	int i , ret;
	pid_t tid;
	int thread_nr, thread_ep;
	int *p;
	
	p = (int*) arg;

	printf("thread %d\n", *p);

  	thread_nr = CLT_NR + *p;
	thread_ep = mnx_tbind(dcid, thread_nr);
	tid = (pid_t) syscall (SYS_gettid);
	printf("THREAD arg=%d thread_nr=%d thread_ep=%d tid=%d\n", *p, thread_nr, thread_ep, tid);

 	m_th[*p]->m_type = *p;
	m_th[*p]->m1_i1 = i = 0;
	m_th[*p]->m1_i2 = tid;
	ret = mnx_sendrec(SVR_NR, (long) m_th[*p]);

 	printf("THREAD %d: Starting loop\n", *p);
	for( i = 0; i < loops ; i++){
 		m_th[*p]->m1_i1 = i;
		ret = mnx_sendrec(dst_ep, (long) m_th[*p]);
		if( ret != 0 ) printf("THREAD %d SENDREC ret=%d\n", *p, ret);
	}
	printf("THREAD %d: exiting\n", *p);

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
	message *m_ptr;
	double t_start, t_stop, t_total;
    pthread_t mythread[MAXTHREADS];
	pid_t tid[MAXTHREADS], mytid;

  	if (argc != 3) {
    		printf ("usage: %s <threads> <loops> \n", argv[0]);
    		exit(1);
  	}

	threads = atoi(argv[1]);
  	loops = atoi(argv[2]);
	if( loops <= 0 || threads <= 0 || threads > MAXTHREADS) {
    		printf ("loops > 0 and  0 < threads <= %d\n", MAXTHREADS);
    		exit(1);
  	}

	dcid 	= DCID;
	src_nr	= CLT_NR;
	dst_nr 	= SVR_NR;
	
	posix_memalign( (void **) &m_ptr, getpagesize(), sizeof(message) );
	if (m_ptr== NULL) {
   		perror("posix_memalign");
   		exit(1);
	}
	printf("m_ptr=%p\n",m_ptr);	
	
	dst_pid = getpid();
    dst_ep =	mnx_bind(dcid, dst_nr);
	if( dst_ep < 0 ) 
		printf("BIND ERROR dst_ep=%d\n",dst_ep);

   	printf("BIND SERVER dcid=%d dst_pid=%d dst_nr=%d dst_ep=%d\n",
		dcid, dst_pid, dst_nr, 	dst_ep);
	
	for( i = 0; i < threads ; i++){	/* creates threads childs*/
		t_nr[i] = i;
		/* ALLOC message space for threads */
		posix_memalign( (void **) &m_th[i], getpagesize(), sizeof(message) );
		if (m_th[i]== NULL) {
			perror("posix_memalign");
			exit(1);
		}
		printf("m_th[%d]=%p\n", i ,m_th[i]);	
		
		printf("pthread_create %d\n",i);
		if ( (ret = pthread_create(&mythread[i], NULL, thread_function,(void*) &t_nr[i]))) {
			perror("error creating thread.");
			exit(1);
		}
	}

	/* To synchronize the  with threads */
 	printf("SERVER: Waiting for synchronizing messages from threads\n");
	for( i = 0; i < threads ; i++){	
    	ret = mnx_receive(ANY, (long) m_ptr);
		printf("SERVER: " MSG1_FORMAT, MSG1_FIELDS(m_ptr));
	}

 	printf("SERVER: Sending synchronizing messages to threads\n");
	for( i = 0; i < threads ; i++){	
    	ret = mnx_send(CLT_NR+i, (long) m_ptr);
	}
	
 	printf("SERVER: Starting loop\n");
	t_start = dwalltime();
	for( i = 0; i < (loops*threads); i++) {
    	ret = mnx_rcvrqst((long) m_ptr);
   		ret = mnx_reply(m_ptr->m_source, (long) m_ptr);
	}
	t_stop  = dwalltime();

	printf("RECEIVE msg: " MSG1_FORMAT, MSG1_FIELDS(m_ptr));
	
	t_total = (t_stop-t_start);
 	printf("t_start=%.2f t_stop=%.2f t_total=%.2f\n",t_start, t_stop, t_total);
 	printf("Loops = %d\n", loops);
 	printf("Time for a pair of SENDREC/RCVRQST-REPLY= %f[ms]\n", 1000*t_total/2/(double)(loops*threads));
 	printf("Throuhput = %f [SENDREC/RCVRQST-REPLY/s]\n", (double)(loops*threads)*2/t_total);
/*
	printf("MAIN Waiting threads\n");
	for( i = 0; i < threads; i++){
		if ( pthread_join ( mythread[i], NULL ) ) {
			printf("error joining thread %d.\n", i);
			abort();
		}else{
			printf("Thread %d has joint\n", i);
		}
	}
*/	
}



