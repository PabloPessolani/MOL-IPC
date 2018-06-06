
#include "systask.h"

dc_usr_t dcu = {
	.dc_dcid = 0,
	.dc_flags = 0,
    .dc_nr_procs = NR_PROCS,
	.dc_nr_tasks = NR_TASKS,
	.dc_nr_sysprocs = NR_SYS_PROCS,
	.dc_nr_nodes = NR_NODES, 
	.dc_nodes = 0, 
	.dc_name = "DC0"};
dc_usr_t *dcu_ptr, *dcu_ptr2;
dc_usr_t dcu2;

int dc_init( int argc, char *argv[] )
{
	int c, nodeid ,rcode;
extern char *optarg;
extern int optind, optopt, opterr;

	while ((c = getopt(argc, argv, "v:p:t:s:n:N:")) != -1) {
		switch(c) {
			case 'v':
				dcu.dc_dcid = atoi(optarg);
				if( dcu.dc_dcid < 0 || dcu.dc_dcid  >= dvs_ptr->d_nr_dcs) {
					fprintf (stderr, "Invalid dcid [0-%d]\n", dvs_ptr->d_nr_dcs-1 );
					exit(1);
				}
				break;
			case 'p':
				dcu.dc_nr_procs = atoi(optarg);
				if( dcu.dc_nr_procs <= 0 || dcu.dc_nr_procs  > dvs_ptr->d_nr_procs) {
					fprintf (stderr, "Invalid nr_procs [1-%d]\n", dvs_ptr->d_nr_procs);
					exit(1);
				}
				break;				
			case 't':
				dcu.dc_nr_tasks = atoi(optarg);
				if( dcu.dc_nr_tasks <= 0 || dcu.dc_nr_tasks  > dvs_ptr->d_nr_tasks ) {
					fprintf (stderr, "Invalid nr_tasks [1-%d]\n", dvs_ptr->d_nr_tasks);
					exit(1);
				}
				break;	
			case 's':
				dcu.dc_nr_sysprocs = atoi(optarg);
				if( dcu.dc_nr_sysprocs <= 0 || dcu.dc_nr_sysprocs > dvs_ptr->d_nr_sysprocs ) {
					fprintf (stderr, "Invalid nr_sysprocs [1-%d]\n", dvs_ptr->d_nr_sysprocs);
					exit(1);
				}
				break;
			case 'n':
				dcu.dc_nr_nodes = atoi(optarg);
				if( dcu.dc_nr_nodes <= 0 || dcu.dc_nr_nodes > dvs_ptr->d_nr_nodes ) {
					fprintf (stderr, "Invalid nr_nodes [1-%d]\n",  dvs_ptr->d_nr_nodes);
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
			default:
				fprintf (stderr,"usage: %s [-v dcid] [-p nr_procs] [-t nr_tasks] [-s nr_sysprocs] [-n nr_nodes] [-N DCname]\n", argv[0]);
				exit(1);
		}
	}
	
	if( argv[optind] != NULL){
		fprintf (stderr,"usage: %s [-v dcid] [-p nr_procs] [-t nr_tasks] [-s nr_sysprocs][-n nr_nodes] [-N DCname]\n", argv[0]);
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
	
	dcu_ptr = &dcu;
	dcu_ptr2 = &dcu2;
	
	rcode = mnx_getdcinfo(dcu.dc_dcid, &dcu2);	
	if( rcode == OK)  *dcu_ptr = *dcu_ptr2;
	
	TASKDEBUG("dcid 	= %d\n", dcu.dc_dcid);
	TASKDEBUG("nr_procs 	= %d\n", dcu.dc_nr_procs);
	TASKDEBUG("nr_tasks 	= %d\n", dcu.dc_nr_tasks);
	TASKDEBUG("nr_sysprocs	= %d\n", dcu.dc_nr_sysprocs);
	TASKDEBUG("nr_nodes	= %d\n", dcu.dc_nr_nodes);
	TASKDEBUG("nodes	= %d\n", dcu.dc_nodes);
	if( ( strcmp( dcu.dc_name, "DC0") == 0 ) &&
		dcu.dc_dcid != 0)
		sprintf(dcu.dc_name,"DC%d", dcu.dc_dcid);
	TASKDEBUG("dc_name	= %s\n", dcu.dc_name);
	if( rcode ) {
		nodeid = mnx_dc_init(&dcu);
    	if( nodeid < 0) 
    		printf("ERROR %d: Initializing virtual machine %d... \n", nodeid, dcu.dc_dcid);
		else
    		TASKDEBUG("Virtual machine %d has been initialized on node %d\n", dcu.dc_dcid, nodeid);
	}else{
    	TASKDEBUG("Virtual machine %d was already initialized on node %d\n", dcu.dc_dcid, nodeid);	
	}
	
	return(local_nodeid);
 }



