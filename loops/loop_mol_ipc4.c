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
#include "./kernel/minix/moldebug.h"
#include "./kernel/minix/molerrno.h"
#define	printk	printf
#ifdef MOLDBG
#undef MOLDBG
#endif

#define 	DCID	0
#define 	SRC_NR	1
#define 	DST_NR	2
#define		FORK_WAIT_MS	500


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
	int dcid, src_pid, dst_pid, src_ep, dst_ep, src_nr, dst_nr, ret, i, loops;
	message *m_ptr;
	double t_start, t_stop, t_total;
	
  	if (argc != 2) {
    		printf ("usage: %s <loops>\n", argv[0]);
    		exit(1);
  	}

  	loops = atoi(argv[1]);

	dcid 	= DCID;
	src_nr	= SRC_NR;
	dst_nr 	= DST_NR;
	
	if( (src_pid = fork()) != 0 )	{		/* PARENT = DESTINATION */

		/* BIND PARENT(DESTINATION) */
		dst_pid = getpid();
		dst_ep =	mnx_bind(dcid, dst_nr);
		if( dst_ep < 0 ) 
			printf("BIND ERROR dst_ep=%d\n",dst_ep);
		printf("BIND DESTINATION dcid=%d dst_pid=%d dst_nr=%d dst_ep=%d\n",
			dcid, dst_pid, dst_nr, dst_ep);
			
		/* BIND CHILD(SOURCE) */
    	src_ep =mnx_lclbind(dcid, src_pid, src_nr);
		if( src_ep < 0 ) 
			printf("BIND ERROR src_ep=%d\n",src_ep);
   		printf("BIND SOURCE dcid=%d src_pid=%d src_nr=%d src_ep=%d\n",
			dcid, src_pid, src_nr, 	src_ep);
			
		printf("RECEIVER pause before RECEIVE\n");
		sleep(1); /* PAUSE before RECEIVE*/

		posix_memalign( (void **) &m_ptr, getpagesize(), sizeof(message) );
		if (m_ptr== NULL) {
    		perror("posix_memalign");
    		exit(1);
		}
		printf("m_ptr %p\n",m_ptr);
	
	   	ret = mnx_receive(ANY, (long) m_ptr); 
		for( i = 0; i < (loops+10); i++) {
			if( i == 10) t_start = dwalltime();
			ret = mnx_rcvrqst((long) m_ptr);
		    ret = mnx_reply(src_ep, (long) m_ptr);
		}
     	t_stop  = dwalltime();

   		printf("RECEIVE %d: " MSG1_FORMAT, dst_pid, MSG1_FIELDS(m_ptr));	
		t_total = (t_stop-t_start);
 		printf("t_start=%.2f t_stop=%.2f t_total=%.2f\n",t_start, t_stop, t_total);
 		printf("Loops = %d\n", loops);
 		printf("Time for a pair of SENDREC/RCVRQST-REPLY= %f[ms]\n", 1000*t_total/2/(double)loops);
 		printf("Throuhput = %f [SENDREC/RCVRQST-REPLY/s]\n", (double)(loops*2)/t_total);
		wait(&ret);
		
	}else{						/* CHILD = SOURCE		*/
	
		printf("CHILD: mnx_wait4bind_T\n");
		do { 
			ret = mnx_wait4bind_T(FORK_WAIT_MS);
			printf("CHILD: mnx_wait4bind_T  ret=%d\n", ret);
			if (ret == EMOLTIMEDOUT) {
				printf("CHILD: mnx_wait4bind_T TIMEOUT\n");
				continue ;
			}else if( ret == EMOLNOTREADY){
				break;	
			}else if( ret < 0) 
				exit(EXIT_FAILURE);
		} while	(ret < OK); 

		posix_memalign( (void **) &m_ptr, getpagesize(), sizeof(message) );
		if (m_ptr== NULL) {
    		perror("posix_memalign");
    		exit(1);
		}
		printf("m_ptr %p\n",m_ptr);

		dst_pid = getppid();
		dst_ep = mnx_getep(dst_pid);		
		src_pid = getpid();
	  	m_ptr->m_type= 0xFF;
		m_ptr->m1_i1 = 0x00;
		m_ptr->m1_i2 = 0x02;
		m_ptr->m1_i3 = 0x03;
   		printf("SENDREC %d: MSG1_FORMAT, ", src_pid, MSG1_FIELDS(m_ptr));

		m_ptr->m1_i1 = 0x1234;
   		ret = mnx_send(dst_ep, (long) m_ptr);
		for( i = 0; i < (loops+10); i++){
	    		ret = mnx_sendrec(dst_ep, (long) m_ptr);
		}

   		printf("RECEIVE %d: MSG1_FORMAT, ", src_pid, MSG1_FIELDS(m_ptr));
		if( ret != 0 )
	    	printf("SEND ret=%d\n",ret);
		}
 printf("exit \n");
 
 exit(0);
}

