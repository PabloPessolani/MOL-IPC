#include <asm/ptrace.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/times.h>
#include <time.h>
#include <sys/types.h>
#include <sys/wait.h> 


#include "../../kernel/minix/config.h"
#include "../../kernel/minix/ipc.h"
#include "../../kernel/minix/kipc.h"
#include "../../kernel/minix/cmd.h"
#include "../../kernel/minix/proxy_sts.h"
#include "../../kernel/minix/proxy_usr.h"
#include "../../kernel/minix/molerrno.h"

#include "../../stub_syscall.h"

#include "../debug.h"


#define 	DCID	0
#define 	SVR_NR	0
#define 	MAXLOOPS	100000000


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
	int dcid, clt_pid, svr_pid, clt_ep, svr_ep, clt_nr, svr_nr, ret, i, maxbuf;
	message *m_ptr;
	long int loops = 1, total_bytes = 0 ;
	double t_start, t_stop, t_total,loopbysec, tput;
	char *buffer;
	
    	if ( argc != 3) {
 	        printf( "Usage: %s <loops> <maxbuf> \n", argv[0] );
 	        exit(1);
	   }
		
	dcid = DCID;
	svr_nr = SVR_NR;
	svr_pid = getpid();

	loops = atoi(argv[1]);
	if( loops  <= 0 || loops > MAXLOOPS) {
   		perror("loops");
    		exit(1);
	}

	maxbuf = atoi(argv[2]);
	if( maxbuf <= 0 || maxbuf > MAXCOPYLEN) {
   		perror("maxbuf");
    		exit(1);
	}
	
	posix_memalign( (void**) &m_ptr, getpagesize(), sizeof(message));
	if (m_ptr== NULL) {
    		perror("posix_memalign");
    		exit(1);
  	}
   		
	posix_memalign( (void**) &buffer, getpagesize(), MAXCOPYLEN);
	if (buffer== NULL) {
    		perror("posix_memalign");
    		exit(1);
  	}
	
   	svr_ep =	mnx_bind(dcid, svr_nr);
	if( svr_ep < 0 ) {
		printf("BIND ERROR svr_ep=%d\n",svr_ep);
		exit(1);
	}
   	printf("BIND SERVER dcid=%d svr_pid=%d svr_nr=%d svr_ep=%d\n",
		dcid,
		svr_pid,
		svr_nr,
		svr_ep);

	for(i = 0; i < maxbuf-1; i++)
		buffer[i] = ((i%25) + 'a');	
	buffer[maxbuf] = 0;
	
	if( maxbuf > 60) buffer[60] = 0;	
	printf("SERVER buffer before = %s\n", buffer);

	ret = mnx_receive(ANY, (long) m_ptr);
	printf(MSG1_FORMAT,  MSG1_FIELDS(m_ptr));
	if( (unsigned long)m_ptr->m1_p1%getpagesize() ){
		fprintf(stderr, "WARNING m_ptr->m1_p1 not page aligned %X\n", m_ptr->m1_p1);
		loops=0;
	}
	
	t_start = dwalltime();
	for( i = 0; i < loops; i++) {
		ret = mnx_vcopy(m_ptr->m_source, m_ptr->m1_p1, svr_ep, buffer, m_ptr->m1_i1);	
		if(ret < 0) {
			printf("VCOPY error=%d\n", ret);
			exit(1);
		}
	}
    t_stop  = dwalltime();
	ret = mnx_send(m_ptr->m_source, (long) m_ptr);

	if( maxbuf > 60) buffer[60] = 0;	
	printf("SERVER buffer after = %s\n", buffer);

	t_total = (t_stop-t_start);
	loopbysec = (double)(loops)/t_total;
	tput = loopbysec * (double)maxbuf;
		
 	printf("t_start=%.2f t_stop=%.2f t_total=%.2f\n",t_start, t_stop, t_total);
 	printf("transfer size=%d #transfers=%d loopbysec=%f\n", maxbuf , loops, loopbysec);
 	printf("Throuhput = %f [bytes/s]\n", tput);

 exit(0);
}



