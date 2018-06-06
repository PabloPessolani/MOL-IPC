#include <asm/ptrace.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "../kernel/minix/config.h"
#include "../kernel/minix/com.h"
#include "../kernel/minix/ipc.h"
#include "../kernel/minix/kipc.h"
#include "../kernel/minix/endpoint.h"
#include "../kernel/minix/callnr.h"
#include "../kernel/minix/molerrno.h"

void  main ( int argc, char *argv[] )
{
	
	int i, pid;
	
	for( i = 0 ; i < 10 ; i++){
		pid = mol_getpid();
		printf("%s pid=%d\n", argv[0], pid);
		sleep(2);
	}
	mol_exit(0);
	exit(0);
 }



