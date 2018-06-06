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

   
void  main ( int argc, char *argv[] )
{
	int dcid, clt_pid, svr_pid, clt_ep, svr_ep, clt_nr, svr_nr, ret, i, maxbuf;
	message *m_ptr;
	char *buffer;


    	if ( argc != 3) {
 	        printf( "Usage: %s <clt_nr> <maxbuf> \n", argv[0] );
 	        exit(1);
	   }
		
	dcid = DCID;
	svr_nr = svr_ep = SVR_NR;
	svr_pid = getpid();

	clt_nr = atoi(argv[1]);

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
	
	for(i = 0; i < maxbuf-1; i++)
		buffer[i] = (((i+clt_nr)%25) + 'A');	
	buffer[maxbuf] = 0;
	
	if( maxbuf > 60) buffer[60] = 0;	
	printf("CLIENT buffer BEFORE = %s\n", buffer);		

	clt_pid = getpid();
	clt_ep = mnx_bind(dcid, clt_nr);
	if( clt_ep < 0 ) 
		printf("BIND ERROR clt_ep=%d\n",clt_ep);
		
   	printf("BIND CLIENT dcid=%d clt_pid=%d clt_nr=%d clt_ep=%d\n",
			dcid,
			clt_pid,
			clt_nr,
			clt_ep);
			
	printf("CLIENT pause before SENDREC\n");
	sleep(2); 

	m_ptr->m_type= 0x01;
	m_ptr->m1_i1 = maxbuf ;
	m_ptr->m1_i2 = 0x02;
	m_ptr->m1_i3 = 0x03;
	m_ptr->m1_p1 = buffer;
	
	printf(MSG1_FORMAT,  MSG1_FIELDS(m_ptr));
	if( (unsigned long)m_ptr->m1_p1%getpagesize() ){
		fprintf(stderr, "WARNING m_ptr->m1_p1 not page aligned %X\n", m_ptr->m1_p1);
	}
	
	ret = mnx_sendrec(svr_ep, (long) m_ptr);
	if( ret != 0 )
		printf("SENDREC ret=%d\n",ret);
	
   	printf("REPLY msg: m_source=%d, m_type=%d, m1_i1=%d, m1_i2=%d, m1_i3=%d\n",
		m_ptr->m_source,
		m_ptr->m_type,
		m_ptr->m1_i1,
		m_ptr->m1_i2,
		m_ptr->m1_i3);

	if( maxbuf > 60) buffer[60] = 0;	
	printf("CLIENT buffer AFTER = %s\n", buffer);	
 }



