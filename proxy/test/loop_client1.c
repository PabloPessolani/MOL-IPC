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


#define	DCID	0
#define CLT_NR	1
#define SVR_EP	0
   
void  main ( int argc, char *argv[] )
{
	int dcid, clt_pid, svr_pid, clt_ep, svr_ep, clt_nr, svr_nr, ret, i, loops;
	message *m_ptr;
	

    	if ( argc != 2) {
 	        printf( "Usage: %s <loops>\n", argv[0] );
 	        exit(1);
	    }

	dcid = DCID;
	if ( dcid < 0 || dcid >= NR_DCS) {
 	        printf( "Invalid dcid [0-%d]\n", NR_DCS-1 );
 	        exit(1);
	    }
	clt_nr = CLT_NR;
	svr_ep = svr_nr = SVR_EP;
	loops = atoi(argv[1]);

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
	
	posix_memalign( (void**) &m_ptr, getpagesize(), sizeof(message));
	if (m_ptr== NULL) {
    		perror("posix_memalign");
    		exit(1);
  	}
	
	m_ptr->m_type= 0x01;
	m_ptr->m1_i1 = 0x01;
	m_ptr->m1_i2 = 0x02;
	m_ptr->m1_i3 = 0x03;

	for( i = 0; i < (loops+1); i++) {
		m_ptr->m1_i1 = i;
		ret = mnx_sendrec(svr_ep, (long) m_ptr);
		printf(MSG1_FORMAT,MSG1_FIELDS(m_ptr));
		if( ret != 0 )
			printf("SENDREC ret=%d\n",ret);
	}
	
   	printf("REPLY msg: m_source=%d, m_type=%d, m1_i1=%d, m1_i2=%d, m1_i3=%d\n",
		m_ptr->m_source,
		m_ptr->m_type,
		m_ptr->m1_i1,
		m_ptr->m1_i2,
		m_ptr->m1_i3);

 }



