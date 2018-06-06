#include <asm/ptrace.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "stub_syscall.h"
#include "./kernel/minix/config.h"
#include "./kernel/minix/ipc.h"
#include "./kernel/minix/kipc.h"

#define WAITTIME	3
#define LOOPS		10   
void  main ( int argc, char *argv[] )
{
	int dcid, pid, endpoint, p_nr, ret, i;
	message m;

    	if ( argc != 3) {
 	        printf( "Usage: %s <dcid> <p_nr> \n", argv[0] );
 	        exit(1);
	    }

	dcid = atoi(argv[1]);
	if ( dcid < 0 || dcid >= NR_DCS) {
 	        printf( "Invalid dcid [0-%d]\n", NR_DCS-1 );
 	        exit(1);
	    }

	p_nr = atoi(argv[2]);
	pid = getpid();

   	printf("Binding SERVER %d to DC%d with p_nr=%d\n",pid,dcid,p_nr);
    	endpoint =	mnx_bind(dcid, p_nr);
	if( endpoint < 0 ) {
		printf("BIND ERROR %d\n",endpoint);
		exit(endpoint);
	}
	printf("SERVER endpoint=%d\n",endpoint);


	for( i = 0; i < LOOPS; i++) {

	    	ret = mnx_receive(ANY, (long) &m);
     		if( ret != OK){
				mnx_unbind(dcid,endpoint);
			printf("RECEIVE ERROR %d\n",ret); 
			exit(1);
		}

   		printf("RECEIVE msg: m_source=%d, m_type=%d, m1_i1=%d, m1_i2=%d, m1_i3=%d\n",
			m.m_source,
			m.m_type,
			m.m1_i1,
			m.m1_i2,
			m.m1_i3);
		
		m.m_type |= 0x80;
	    	ret = mnx_send(m.m_source, (long) &m);
     		if( ret != OK){
				mnx_unbind(dcid,endpoint);
			printf("SEND ERROR %d\n",ret); 
			exit(1);
		}

	printf("SERVER sleep before RECEIVE\n");
	   sleep(WAITTIME);
	}

    	printf("Unbinding process %d from DC%d with endpoint=%d\n",pid,dcid,endpoint);
    	mnx_unbind(dcid,endpoint);

 }



