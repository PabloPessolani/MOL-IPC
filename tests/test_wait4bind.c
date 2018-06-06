#include <asm/ptrace.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "./kernel/minix/config.h"
#include "./kernel/minix/ipc.h"
#include "./kernel/minix/kipc.h"
#include "./kernel/minix/molerrno.h"
#include "stub_syscall.h"
   
void  main ( int argc, char *argv[] )
{
	int dcid, pid1, pid2, p_nr1, p_nr2, rcode, p_ep1, p_ep2;

    if ( argc != 4) {
		printf( "Usage: %s <dcid> <p_nr1> <p_nr2> \n", argv[0] );
 	    exit(1);
	}

	dcid = atoi(argv[1]);
	if ( dcid < 0 || dcid >= NR_DCS) {
 	        printf( "Invalid dcid [0-%d]\n", NR_DCS-1 );
 	        exit(1);
	}

	p_nr1 = atoi(argv[2]);
	pid1 = getpid();
	p_nr2 = atoi(argv[3]);
	
	if( pid2 = fork() == 0){/*CHILD */
		sleep(5);
		pid2 = getpid();
	    printf("CHILD Binding process %d to DC%d with p_nr=%d\n",pid2,dcid,p_nr2);
		p_ep2 = mnx_bind(dcid, p_nr2); 
		if( p_ep2 < 0){
			printf("CHILD: mnx_bind error=%d\n", p_ep2);
			exit(EXIT_FAILURE);
		} 
		sleep(5);
	}

	printf("PARENT: mnx_wait4bind_T SELF BEFORE \n");
	rcode = mnx_wait4bindep_T(p_nr1, 1000);
	printf("PARENT: mnx_wait4bindep_T SELF rcode=%d\n", rcode);
	
    printf("PARENT: Binding waiting process %d to DC%d with p_nr1=%d\n",pid1,dcid,p_nr1);
	p_ep1 = mnx_bind(dcid, p_nr1); 

	printf("PARENT: mnx_wait4bind_T SELF AFTER \n");
	rcode = mnx_wait4bindep_T(p_nr1, 1000);
	printf("PARENT: mnx_wait4bindep_T SELF rcode=%d\n", rcode);
	
	printf("PARENT: mnx_wait4bind_T %d\n", p_nr2);
	do { 
		rcode = mnx_wait4bindep_T(p_nr2, 1000);
		printf("PARENT: mnx_wait4bindep_T  rcode=%d\n", rcode);
		if (rcode == EMOLTIMEDOUT) {
			printf("PARENT: mnx_wait4bindep_T TIMEOUT\n");
			continue ;
		}else if( rcode < 0) 
			exit(EXIT_FAILURE);
	} while	(rcode < OK); 
	printf("PARENT: Child %d bound\n", p_nr2);
	
	printf("PARENT: mnx_wait4unbind_T\n");
	do { 
		rcode = mnx_wait4unbind_T(p_nr2, 1000);
		printf("PARENT: mnx_wait4unbind_T  rcode=%d\n", rcode);
		if (rcode == EMOLTIMEDOUT) {
			printf("PARENT: mnx_wait4unbind_T TIMEOUT\n");
			continue ;
		}else if( rcode < 0) 
			exit(EXIT_FAILURE);
	} while	(rcode < OK); 
	printf("PARENT: Child %d unbound\n", p_nr2);
	
 }



