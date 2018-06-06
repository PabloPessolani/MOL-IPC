#include <asm/ptrace.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/times.h>
#include <time.h>
#include <pthread.h>

#include "./kernel/minix/config.h"
#include "./kernel/minix/ipc.h"
#include "./kernel/minix/kipc.h"

#include "./kernel/minix/proxy.h"

#include "stub_syscall.h"
#include "mol_thread.h"

#include "./kernel/minix/moldebug.h"
#define	printk	printf
#ifdef MOLDBGXXXXX
#undef MOLDBG
#endif

#define 	VMID	0
#define 	DST_NR	1
#define 	SRC_NR	10
#define 	MAXTHREADS   5
#define		LOOPS	100

static int src_ep, dst_ep, r;
pthread_mutex_t mol_mutex = PTHREAD_MUTEX_INITIALIZER; 

double dwalltime()
{
	double sec;
	struct timeval tv;

	gettimeofday(&tv,NULL);
	sec = tv.tv_sec + tv.tv_usec/1000000.0;
	return sec;
}


static void *threadFunc(void *arg)
{
	int i;
	char car, *ptr;
	message msg;

	ptr = (char *) arg;
	car = *ptr;
	sleep(1);

  	msg.m_type= 0xFF;
	msg.m1_i1 = car;
	msg.m1_i2 = 0x00;
	msg.m1_i3 = 0x03;
	printf("SEND msg: m_type=%d, m1_i1=%d, m1_i2=%d, m1_i3=%d\n",
		msg.m_type,
		msg.m1_i1,
		msg.m1_i2,
		msg.m1_i3);

	for( i = 0; i < LOOPS; i++){
		msg.m1_i2 = i;
//		r = lock_send(dst_ep, &msg);
//		if( r != 0 )
		   	printf("SEND src_ep=%d\n",src_ep);
	}

	r = (int)car;
	return((void*)&r);
}

void  main ( int argc, char *argv[] )
{
	int vmid, src_pid, dst_pid,  src_nr, dst_nr, ret, i, f, s, j;
	message m;
	double t_start, t_stop, t_total;
    	pthread_t t[MAXTHREADS];

	vmid 	= VMID;
	src_nr	= SRC_NR;
	dst_nr 	= DST_NR;
	
	dst_pid = getpid();
    	dst_ep = lock_bind(vmid, dst_nr);

	if( dst_ep < 0 ) 
		printf("BIND ERROR dst_ep=%d\n",dst_ep);

   	printf("BIND DESTINATION vmid=%d dst_pid=%d dst_nr=%d dst_ep=%d\n",
		vmid,
		dst_pid,
		dst_nr,
		dst_ep);

	if( (src_pid = fork()) != 0 )	{		/* PARENT = DESTINATION */
		printf("Dump procs of VM%d\n",vmid); 
   		ret = lock_proc_dump(vmid);
      		if( ret != OK){
			printf("PROCDUMP ERROR %d\n",ret); 
			exit(1);
		}

		printf("RECEIVER pause before SENDREC\n");
		sleep(5); 
			
		t_start = dwalltime();
		for( i = 0; i < (MAXTHREADS*LOOPS); i++) {
			printf("SERVER receive %d\n",i);
	    	ret = lock_receive(ANY,  &m);
		}
     		t_stop  = dwalltime();

		ret = lock_proc_dump(vmid);
      		if( ret != OK){
			printf("PROCDUMP ERROR %d\n",ret); 
			exit(1);
		}	
   		printf("RECEIVE msg: m_source=%d, m_type=%d, m1_i1=%d, m1_i2=%d, m1_i3=%d\n",
			m.m_source,
			m.m_type,
			m.m1_i1,
			m.m1_i2,
			m.m1_i3);

	    	printf("UNBIND DESTINATION vmid=%d dst_pid=%d dst_nr=%d dst_ep=%d\n",
			vmid,
			dst_pid,
			dst_nr,
			dst_ep);
	    	lock_unbind(vmid, dst_ep);
		t_total = (t_stop-t_start);
 		printf("t_start=%.2f t_stop=%.2f t_total=%.2f\n",t_start, t_stop, t_total);
 		printf("Loops = %d\n", LOOPS);
 		printf("Time for a pair of SEND/RECEIVE= %f[ms]\n", 1000*t_total/(double)LOOPS);
 		printf("Throuhput = %f [SEND-RECEIVE/s]\n", (double)LOOPS/t_total);

	}else{						/* SON = SOURCE		*/

		src_pid = getpid();
	   	src_ep =  lock_bind(vmid, src_nr);
		if( src_ep < 0 ) 
			printf("BIND ERROR src_ep=%d\n",src_ep);
    	printf("BIND SOURCE vmid=%d src_pid=%d src_nr=%d src_ep=%d\n",
			vmid,
			src_pid,
			src_nr,
			src_ep);

		for (f = 1; f < MAXTHREADS; f++)	{				/* creates THREAD childs*/
			printf("pthread_create%d\n",f);
			s = pthread_create(&t[j], NULL, threadFunc, &f);
		}
		
		for( j = 0; j < MAXTHREADS; j++){
			s = pthread_join(t[j], NULL);
    			if (s != 0) {
	   			printf("pthread_join %d\n",j);
			    	lock_unbind(vmid, src_ep);
				exit(1);
			}
			else
	   			printf("pthread_join success %d\n",j);
		}

		printf("UNBIND SOURCE vmid=%d src_pid=%d src_nr=%d src_ep=%d\n",
			vmid,
			src_pid,
			src_nr,
			src_ep);
	    	lock_unbind(vmid, src_ep);
	}
 printf("\n");
 

 exit(0);
}



