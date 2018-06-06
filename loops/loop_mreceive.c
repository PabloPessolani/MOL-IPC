#include <asm/ptrace.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>

#include "stub_syscall.h"
#include "./kernel/minix/config.h"
#include "./kernel/minix/ipc.h"
#include "./kernel/minix/kipc.h"

   
void  main ( int argc, char *argv[] )
{
	int dcid, pid, p_nr, dst_ep, ret, ep, i, f, loops, childPid;
	message m;

    if ( argc != 3) {
 	        printf( "Usage: %s <dcid> <loops>\n", argv[0] );
 	        exit(1);
	    }

	dcid = atoi(argv[1]);
	if ( dcid < 0 || dcid >= NR_DCS) {
 	        printf( "Invalid dcid [0-%d]\n", NR_DCS-1 );
 	        exit(1);
	    }

	loops  = atoi(argv[2]);

	#define NR_FORKS		8

	for (f = 0; f < NR_FORKS; f++)	{				/* creates NR_FORKS childs*/
		if( fork() == 0) break;	
	}
	p_nr = 1 + f;
	pid = getpid();
    printf("Binding LOCAL process %d to DC%d with p_nr=%d\n",pid,dcid,p_nr);

	
    dst_ep = mnx_bind(dcid, p_nr);
	if( dst_ep < 0 ) {
		printf("BIND ERROR %d\n",dst_ep);
		exit(dst_ep);
	}

	for (i = 0 ; i < loops; i++) {
    	ret = mnx_receive(ANY, (long) &m);
     	if( ret != OK)
			printf("RECEIVE ERROR %d\n",ret);
	}
	
    printf("Unbinding process %d from DC%d with src_nr=%d\n",pid,dcid,p_nr);
   	mnx_unbind(dcid,dst_ep);

if( f == NR_FORKS)
	while(1) { /* Parent waits for each child to exit */
		childPid = wait(NULL);
		if( childPid == -1) {
			if (errno == ECHILD) {
				printf("No more children - bye!\n");
				exit(EXIT_SUCCESS);
			} else { /* Some other (unexpected) error */
				printf("wait() error=%d\n",errno);
				exit(errno);
			}
		}
	}
 }



