#include <asm/ptrace.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/times.h>
#include <time.h>
#include <sys/types.h>
#include <sys/wait.h> 

#include "../../stub_syscall.h"
#include "../../kernel/minix/config.h"
#include "../../kernel/minix/ipc.h"
#include "../../kernel/minix/kipc.h"


 
double dwalltime()
{
	double sec;
	struct timeval tv;

	gettimeofday(&tv,NULL);
	sec = tv.tv_sec + tv.tv_usec/1000000.0;
	return sec;
}

#define	DCID	0
#define MAXRINGSIZE	30
#define MAXLOOPS	100000000

   
void  main ( int argc, char *argv[] )
{
	int dcid, self_pid, next_pid, self_ep, next_ep, self_nr, next_nr, rsize, ret, i, loops;
	message *m_ptr;
	double t_start, t_stop, t_total;

    if ( argc != 4) {
 	        printf( "Usage: %s <p_nr> <ring_size> <loops>\n", argv[0] );
 	        exit(1);
	    }

	dcid = DCID;
	
	self_nr =  atoi(argv[1]);
	rsize =  atoi(argv[2]);
	
	if( rsize < 0 || rsize >= MAXRINGSIZE) {
        printf( "error ring_size:%d\n", rsize);
        exit(1);
    }
	
	if( self_nr < 0 || self_nr >= rsize) {
        printf( "error p_nr:%d\n", self_nr);
        exit(1);
    }
	
	loops = atoi(argv[3]);
	if( loops < 0 || loops >= MAXLOOPS) {
        printf( "error loops:%d\n", loops);
        exit(1);
    }
	
	next_ep = (self_nr+1)%rsize;
	printf("next_ep=%d\n", next_ep);
	
	self_pid = getpid();
	self_ep = mnx_bind(dcid, self_nr);
	if( self_ep < 0 ) {
		printf("BIND ERROR self_ep=%d\n",self_ep);
        exit(1);
	}
   	printf("BIND dcid=%d self_pid=%d self_nr=%d self_ep=%d\n",
			dcid,
			self_pid,
			self_nr,
			self_ep);
			
	printf("CLIENT pause before SEND\n");
	sleep(2); 
	
	posix_memalign( (void**) &m_ptr, getpagesize(), sizeof(message));
	if (m_ptr== NULL) {
    		perror("posix_memalign");
    		exit(1);
  	}
   		
	m_ptr->m_type= 0x01;
	m_ptr->m1_i1 = 0;
	m_ptr->m1_i2 = 0;
	m_ptr->m1_i3 = 0x03;
   	printf("SEND msg: m_type=%d, m1_i1=%d, m1_i2=%d, m1_i3=%d\n",
			m_ptr->m_type,
			m_ptr->m1_i1,
			m_ptr->m1_i2,
			m_ptr->m1_i3);

	if( self_nr == 0) t_start = dwalltime();		
	for( i = 0; i < (loops); i++) {
		if( self_ep == 0) {  /* FIRST PROCESS IN RING */
			m_ptr->m1_i1 = i; /* LOOP COUNT */
			ret = mnx_send(next_ep, (long) m_ptr);
			if( ret != 0 )
				printf("SEND ret=%d\n",ret);
			ret = mnx_receive(ANY, (long) m_ptr);
			if( ret != 0 )
				printf("RECEIVE ret=%d\n",ret);	
			m_ptr->m1_i2++; /* HOP COUNT */ 
		} else {  /* OTHER PROCESS IN RING */
			ret = mnx_receive(ANY, (long) m_ptr);
			if( ret != 0 )
				printf("RECEIVE ret=%d\n",ret);	
			m_ptr->m1_i2++; 
			ret = mnx_send(next_ep, (long) m_ptr);
			if( ret != 0 )
				printf("SEND ret=%d\n",ret);

		}
	}
	if( self_nr == 0) t_stop  = dwalltime();
	
   	printf("RECEIVE msg: m_source=%d, m_type=%d, m1_i1=%d, m1_i2=%d, m1_i3=%d\n",
		m_ptr->m_source,
		m_ptr->m_type,
		m_ptr->m1_i1,
		m_ptr->m1_i2,
		m_ptr->m1_i3);
	if( self_nr == 0) {
		t_total = (t_stop-t_start);
		printf("t_start=%.2f t_stop=%.2f t_total=%.2f\n",t_start, t_stop, t_total);
		printf("Loops = %d ring_size=%d\n", loops, rsize);
		printf("Time for a pair of SEND/RECEIVE= %f[ms]\n", 1000*t_total/(double)(loops*rsize));
		printf("Throuhput = %f [SEND-RECEIVE/s]\n", (double)(loops*rsize)/t_total);	
	}
 }



