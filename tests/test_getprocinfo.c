#include <asm/ptrace.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#define _GNU_SOURCE
#define  __USE_GNU
#include <sched.h>
#define cpumask_t cpu_set_t

#define  MOL_USERSPACE	1

#include "stub_syscall.h"
#include "./kernel/minix/config.h"
#include "./kernel/minix/ipc.h"
#include "./kernel/minix/kipc.h"
#include "./kernel/minix/proc_usr.h"
#include "./kernel/minix/proc_sts.h"
#include "./kernel/minix/dc_usr.h"
#include "./kernel/minix/molerrno.h"

#include "./kernel/minix/moldebug.h"
#define	printk	printf
#ifdef MOLDBGXXXXX
#undef MOLDBG
#endif

   
void  main ( int argc, char *argv[] )
{
	int dcid, p_nr, ret;
	struct proc_usr proc_usr, *proc_usr_ptr;

    	if ( argc != 3) {
 	        printf( "Usage: %s <dcid> <p_nr> \n", argv[0] );
 	        exit(1);
	    }

	dcid = atoi(argv[1]);
	p_nr = atoi(argv[2]);

	proc_usr_ptr  = &proc_usr;

	printf("getprocinfo dcid=%d p_nr=%d \n", dcid, p_nr);
	ret = mnx_getprocinfo(dcid, p_nr, proc_usr_ptr);

	if(ret < 0) {
 	        printf( "return error=%d \n", ret );
 	        exit(1);
	}

	printf(PROC_USR_FORMAT,PROC_USR_FIELDS(proc_usr_ptr));

 }



