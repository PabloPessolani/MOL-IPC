#include <asm/ptrace.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#define _GNU_SOURCE
#define  __USE_GNU
#include <sched.h>
#define cpumask_t cpu_set_t

#include "stub_syscall.h"
#include "./kernel/minix/config.h"
#include "./kernel/minix/ipc.h"
#include "./kernel/minix/kipc.h"
#include "./kernel/minix/dc_usr.h"
#include "./kernel/minix/callnr.h"


dc_usr_t dcu = {
	.dc_dcid = 0,
	.dc_flags = 0,
    .dc_nr_procs = NR_PROCS,
	.dc_nr_tasks = NR_TASKS,
	.dc_nr_sysprocs = NR_SYS_PROCS,
	.dc_nr_nodes = NR_NODES,
	.dc_warn2proc = NONE,
	.dc_warnmsg = MOLUNUSED,
	.dc_name = "DC0"
};

void  main ( int argc, char *argv[] )
{
	int c, nodeid;
	dc_usr_t *dcu_ptr;
	
extern char *optarg;
extern int optind, optopt, opterr;

	while ((c = getopt(argc, argv, "d:p:t:s:n:N:m:P:")) != -1) {
		switch(c) {
			case 'd':
				dcu.dc_dcid = atoi(optarg);
				if( dcu.dc_dcid < 0 || dcu.dc_dcid >= NR_DCS) {
					fprintf (stderr, "Invalid dcid [0-%d]\n", NR_DCS-1 );
					exit(1);
				}
				break;
			case 'p':
				dcu.dc_nr_procs = atoi(optarg);
				if( dcu.dc_nr_procs <= 0 || dcu.dc_nr_procs  > NR_PROCS) {
					fprintf (stderr, "Invalid nr_procs [1-%d]\n", NR_PROCS);
					exit(1);
				}
				break;				
			case 't':
				dcu.dc_nr_tasks = atoi(optarg);
				if( dcu.dc_nr_tasks <= 0 || dcu.dc_nr_tasks  > NR_TASKS ) {
					fprintf (stderr, "Invalid nr_tasks [1-%d]\n", NR_TASKS);
					exit(1);
				}
				break;	
			case 's':
				dcu.dc_nr_sysprocs = atoi(optarg);
				if( dcu.dc_nr_sysprocs <= 0 || dcu.dc_nr_sysprocs > NR_SYS_PROCS ) {
					fprintf (stderr, "Invalid nr_sysprocs [1-%d]\n", NR_SYS_PROCS);
					exit(1);
				}
				break;
			case 'n':
				dcu.dc_nr_nodes = atoi(optarg);
				if( dcu.dc_nr_nodes <= 0 || dcu.dc_nr_nodes > NR_NODES ) {
					fprintf (stderr, "Invalid nr_nodes [1-%d]\n", NR_NODES);
					exit(1);
				}
				break;
			case 'N':
				if( strlen(optarg) > (MAXDCNAME-1) ) {
					fprintf (stderr, "DC name too long [%d]\n", (MAXDCNAME-1));
					exit(1);
				}
				strncpy(dcu.dc_name,optarg,(MAXDCNAME-1));
				break;
			case 'P':
				dcu.dc_warn2proc = atoi(optarg);
				break;
			case 'm':
				dcu.dc_warnmsg = atoi(optarg);
				break;
			default:
				fprintf (stderr,"usage: %s [-d dcid] [-p nr_procs] [-t nr_tasks] [-s nr_sysprocs][-n nr_nodes] [-P warn2proc] [-m warnmsg][-N DCname]\n", argv[0]);
				exit(1);
		}
	}
	
	if( argv[optind] != NULL){
		fprintf (stderr,"usage: %s [-d dcid] [-p nr_procs] [-t nr_tasks] [-s nr_sysprocs][-n nr_nodes] [-P warn2proc] [-m warnmsg][-N DCname]\n", argv[0]);
		exit(1);
	}
	
	if( dcu.dc_nr_tasks  > dcu.dc_nr_procs )  {
		fprintf (stderr, "Invalid nr_tasks: must be <= nr_procs(%d)\n", dcu.dc_nr_procs);
		exit(1);
	}

	if( dcu.dc_nr_sysprocs  > dcu.dc_nr_procs )  {
		fprintf (stderr, "Invalid nr_sys_tasks: must be <= nr_procs(%d)\n", dcu.dc_nr_procs);
		exit(1);
	}
	
	if( ( strcmp( dcu.dc_name, "DC0") == 0 ) &&
		dcu.dc_dcid != 0)
		sprintf(dcu.dc_name,"DC%d", dcu.dc_dcid);

	dcu_ptr =&dcu;
	printf(DC_USR_FORMAT, DC_USR_FIELDS(dcu_ptr));
	printf(DC_WARN_FORMAT, DC_WARN_FIELDS(dcu_ptr));
	nodeid = mnx_dc_init(&dcu);

	if( nodeid < 0) 
    		printf("ERROR %d: Initializing DC%d... \n", nodeid, dcu.dc_dcid);
	else
    		printf("DC%d has been initialized on node %d\n", dcu.dc_dcid, nodeid);
	exit(0);
 }



