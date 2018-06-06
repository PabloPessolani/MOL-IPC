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

#define DCID	0	
#define	SVR_NR 	0
   
void  main ( int argc, char *argv[] )
{
	int dcid, pid, endpoint, p_nr, ret, i, loops;
	message *m_ptr;
	double t_start, t_stop, t_total;


    	if ( argc != 2) {
 	        printf( "Usage: %s  <loops> \n", argv[0] );
 	        exit(1);
	    }

	dcid = DCID;
	if ( dcid < 0 || dcid >= NR_DCS) {
 	        printf( "Invalid dcid [0-%d]\n", NR_DCS-1 );
 	        exit(1);
	    }
	p_nr = SVR_NR;
	loops = atoi(argv[1]);

	pid = getpid();

	posix_memalign( (void**) &m_ptr, getpagesize(), sizeof(message));
	if (m_ptr== NULL) {
    		perror("posix_memalign");
    		exit(1);
  	}
	
   	printf("Binding process %d to DC%d with p_nr=%d\n",pid,dcid,p_nr);
    	endpoint =	mnx_bind(dcid, p_nr);
	if( endpoint < 0 ) {
		printf("BIND ERROR %d\n",endpoint);
		exit(endpoint);
	}
	printf("Process endpoint=%d\n",endpoint);

	/*-------------------- START LOOP -----------------*/
	
	for( i = 0; i < loops+1; i++) {		
		if( i == 1) t_start = dwalltime();
		ret = mnx_rcvrqst( (long) m_ptr);
		if( ret != OK){
			printf("RCVRQST ERROR %d\n",ret); 
			exit(1);
		}
//		printf(MSG1_FORMAT,MSG1_FIELDS(m_ptr));
//		printf("RCVRQST OK from %d\n", m_ptr->m_source); 
		ret = mnx_reply(m_ptr->m_source, (long) m_ptr);
		if( ret != OK){
			printf("REPLY ERROR %d\n",ret); 
			exit(1);
		}
// printf("REPLY OK\n"); 

	}
     	t_stop  = dwalltime();
	/*-------------------- END LOOP -----------------*/

  	printf("RECEIVE msg: m_source=%d, m_type=%d, m1_i1=%d, m1_i2=%d, m1_i3=%d\n",
		m_ptr->m_source,
		m_ptr->m_type,
		m_ptr->m1_i1,
		m_ptr->m1_i2,
		m_ptr->m1_i3);
	
	t_total = (t_stop-t_start);
	printf("t_start=%.2f t_stop=%.2f t_total=%.2f\n",t_start, t_stop, t_total);
	printf("Loops = %d\n", loops);
	printf("Time for a pair of SEND/RECEIVE= %f[ms]\n", 1000*t_total/2/(double)loops);
	printf("Throuhput = %f [SEND-RECEIVE/s]\n", (double)(loops*2)/t_total);	
 }



