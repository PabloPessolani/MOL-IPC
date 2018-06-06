#include <asm/ptrace.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "stub_syscall.h"
#include "./kernel/minix/config.h"
#include "./kernel/minix/ipc.h"
#include "./kernel/minix/kipc.h"

   
void  main ( int argc, char *argv[] )
{
	int dcid, pid, endpoint, p_nr, ret;
	message *m_ptr;

    	if ( argc != 3) {
 	        printf( "Usage: %s <dcid> <p_nr> \n", argv[0] );
 	        exit(1);
	    }

	dcid = atoi(argv[1]);
	if ( dcid < 0 || dcid >= NR_DCS) {
 	        printf( "Invalid dcid [0-%d]\n", NR_DCS-1 );
 	        exit(1);
	    }

		
	/*---------------- Allocate memory for message  ---------------*/
	posix_memalign( (void **) &m_ptr, getpagesize(), sizeof(message) );
	if (m_ptr== NULL) {
   		fprintf(stderr, "message posix_memalign\n");
   		exit(1);
	}
		
	p_nr = atoi(argv[2]);
	pid = getpid();

   	printf("Binding process %d to DC%d with p_nr=%d\n",pid,dcid,p_nr);
    endpoint =	mnx_bind(dcid, p_nr);
	if( endpoint < 0 ) {
		printf("BIND ERROR %d\n",endpoint);
		exit(endpoint);
	}
	printf("Process endpoint=%d m_addr=%p\n",endpoint, m_ptr);
/*
	printf("Dump procs of DC%d\n",dcid); 
   	ret = mnx_proc_dump(dcid);
      	if( ret != OK){
		printf("PROCDUMP ERROR %d\n",ret); 
		exit(1);
	}
*/

    ret = mnx_receive( ANY , (long) m_ptr);


//    ret = mnx_rcvrqst((long) &m);
    if( ret != OK){
		printf("RECEIVE ERROR %d\n",ret); 
		exit(1);
	}

	printf(MSG1_FORMAT,MSG1_FIELDS(m_ptr));

	ret = mnx_unbind(dcid, m_ptr->m_source);

#ifdef HABILITAR
	m_ptr->m1_i1+=1;
	m_ptr->m1_i2+=1;
	m_ptr->m1_i3+=1;
//	ret = mnx_send(m.m_source, (long) &m);
	ret = mnx_reply(m_ptr->m_source, (long) m_ptr);

    if( ret != OK){
		printf("SEND ERROR %d\n",ret); 
		exit(1);
	}
   	printf("SEND msg: m_source=%d, m_type=%d, m1_i1=%d, m1_i2=%d, m1_i3=%d\n",
		m_ptr->m_source,
		m_ptr->m_type,
		m_ptr->m1_i1,
		m_ptr->m1_i2,
		m_ptr->m1_i3);
#endif
 	
		
		
 }



