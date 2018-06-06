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

#define 	VMID	0
#define 	DST_NR	1
#define 	SRC_NR	2
#define 	MAXTHREADS	100

int vmid, loops, dst_ep, src_nr, threads ;
int t_nr[MAXTHREADS];

void *thread_function(void *arg) {
	message m;
	int i , ret;
	pid_t tid;
	int thread_nr, thread_ep;
	int *p;
	
	p = (int*) arg;
	
  	thread_nr = SRC_NR + 1 + *p;
	thread_ep = mnx_bind(vmid, thread_nr);
	tid = (pid_t) syscall (SYS_gettid);
	printf("THREAD arg=%d thread_nr=%d thread_ep=%d tid=%d\n", *p, thread_nr, thread_ep, tid);

	ret = mnx_receive(ANY, (long) &m);

 	m.m_type = tid;
	m.m1_i1 = i = 0;
	m.m1_i2 = 0x06;
	m.m1_i3 = 0x07;

	for( i = 0; i < loops ; i++){
 		m.m1_i1 = i;
/*
		printf("THREAD SENDREC msg: m_type=%d, m1_i1=%d, m1_i2=%d, m1_i3=%d\n",
			m.m_type,
			m.m1_i1,
			m.m1_i2,
			m.m1_i3);
*/
		ret = mnx_sendrec(dst_ep, (long) &m);
		if( ret != 0 ) printf("THREAD SENDREC ret=%d\n",ret);
	}
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
	int  src_pid, dst_pid, src_ep,  dst_nr, ret, i;
	message m;
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

	vmid 	= VMID;
	src_nr	= SRC_NR;
	dst_nr 	= DST_NR;
	
	dst_pid = getpid();
      	dst_ep =	mnx_bind(vmid, dst_nr);
	if( dst_ep < 0 ) 
		printf("BIND ERROR dst_ep=%d\n",dst_ep);

   	printf("BIND DESTINATION vmid=%d dst_pid=%d dst_nr=%d dst_ep=%d\n",
		vmid,
		dst_pid,
		dst_nr,
		dst_ep);

	if( (src_pid = fork()) != 0 )	{		/* PARENT = DESTINATION */

		printf("RECEIVER pause before RECEIVE\n");
		sleep(2); /* PAUSE before RECEIVE*/

		do {
			src_ep = mnx_getep(src_pid);
			sleep(1);
			} while(src_ep < 0);
			
		loops *= (threads); 
   		ret = mnx_send(src_ep, (long) &m);
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
		wait(&ret);

	}else{						/* SON = SOURCE		*/

		src_pid = getpid();
    		src_ep =mnx_bind(vmid, src_nr);
		if( src_ep < 0 ) 
			printf("BIND ERROR src_ep=%d\n",src_ep);
   		printf("BIND SOURCE vmid=%d src_pid=%d src_nr=%d src_ep=%d\n",
			vmid,
			src_pid,
			src_nr,
			src_ep);

		for( i = 0; i < (threads-1) ; i++){
			t_nr[i] = i;
			if ( (ret = pthread_create(&mythread[i], NULL, thread_function,(void*) &t_nr[i]))) {
				printf("error creating thread.");
				abort();
			}
			printf("CHILD: Creating thread %d: ret=%d\n", i, ret);
		}

		
		mytid = (pid_t) syscall (SYS_gettid);
		m.m_type = mytid;
		m.m1_i1 = 0;
		m.m1_i2 = 0x06;
		m.m1_i3 = 0x07;

		sleep((threads>>1)+1);
    		ret = mnx_receive(ANY, (long) &m);
		for( i = 0; i < (threads-1) ; i++)
	   		ret = mnx_send((src_ep+1+i), (long) &m);

		for( i = 0; i < loops ; i++){
/*
		m.m1_i1 = i;
		printf("MAIN SENDREC  msg: m_type=%d, m1_i1=%d, m1_i2=%d, m1_i3=%d\n",
			m.m_type,
			m.m1_i1,
			m.m1_i2,
			m.m1_i3);
*/
	    		ret = mnx_sendrec(dst_ep, (long) &m);
			if( ret != 0 ) printf("MAIN SENDREC ret=%d\n",ret);
		}
			
		printf("MAIN Waiting threads\n");
		for( i = 0; i < (threads-1); i++){
			if ( pthread_join ( mythread[i], NULL ) ) {
				printf("error joining thread %d.\n", i);
				abort();
			}
		}
		
	}
 printf("\n");
 
 exit(0);
}



