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
	message m;
	long timeout;

    	if ( argc != 4) {
 	        printf( "Usage: %s <dcid> <p_nr> <timeout_sec> \n", argv[0] );
 	        exit(1);
	    }

	dcid = atoi(argv[1]);
	if ( dcid < 0 || dcid >= NR_DCS) {
 	        printf( "Invalid dcid [0-%d]\n", NR_DCS-1 );
 	        exit(1);
	    }

	p_nr = atoi(argv[2]);
	timeout = atoi(argv[3]);
	pid = getpid();

   	printf("Binding process %d to DC%d with p_nr=%d\n",pid,dcid,p_nr);
    endpoint =	mnx_bind(dcid, p_nr);
	if( endpoint < 0 ) {
		printf("BIND ERROR %d\n",endpoint);
		exit(endpoint);
	}
	printf("Process endpoint=%d\n",endpoint);
/*
	printf("Dump procs of DC%d\n",dcid); 
   	ret = mnx_proc_dump(dcid);
      	if( ret != OK){
		printf("PROCDUMP ERROR %d\n",ret); 
		exit(1);
	}
*/
    ret = mnx_receive_T( ANY , (long) &m, timeout*1000);
    if( ret != OK){
		printf("RECEIVE ERROR %d\n",ret); 
		exit(1);
	}
   	printf("RECEIVE msg: m_source=%d, m_type=%d, m1_i1=%d, m1_i2=%d, m1_i3=%d\n",
		m.m_source,
		m.m_type,
		m.m1_i1,
		m.m1_i2,
		m.m1_i3);

	m.m1_i1+=1;
	m.m1_i2+=1;
	m.m1_i3+=1;
#define HABILITAR
#ifdef HABILITAR
	ret = mnx_send(m.m_source, (long) &m);
    if( ret != OK){
		printf("SEND ERROR %d\n",ret); 
		exit(1);
	}
   	printf("SEND msg: m_source=%d, m_type=%d, m1_i1=%d, m1_i2=%d, m1_i3=%d\n",
		m.m_source,
		m.m_type,
		m.m1_i1,
		m.m1_i2,
		m.m1_i3);
#endif
 	
		
		
 }



