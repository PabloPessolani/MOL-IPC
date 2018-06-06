
#include "systask.h"

VM_usr_t vmu = {
	.vm_vmid = 0,
	.vm_flags = 0,
    .vm_nr_procs = NR_PROCS,
	.vm_nr_tasks = NR_TASKS,
	.vm_nr_sysprocs = NR_SYS_PROCS,
	.vm_nr_nodes = NR_NODES, 
	.vm_nodes = 0, 
	.vm_name = "VM0"};
VM_usr_t *vm_ptr;

int VM_init( int argc, char *argv[] )
{
	int c, nodeid;
extern char *optarg;
extern int optind, optopt, opterr;

	while ((c = getopt(argc, argv, "v:p:t:s:n:N:")) != -1) {
		switch(c) {
			case 'v':
				vmu.vm_vmid = atoi(optarg);
				if( vmu.vm_vmid < 0 || vmu.vm_vmid  >= drvs_ptr->d_nr_vms) {
					fprintf (stderr, "Invalid vmid [0-%d]\n", drvs_ptr->d_nr_vms-1 );
					exit(1);
				}
				break;
			case 'p':
				vmu.vm_nr_procs = atoi(optarg);
				if( vmu.vm_nr_procs <= 0 || vmu.vm_nr_procs  > drvs_ptr->d_nr_procs) {
					fprintf (stderr, "Invalid nr_procs [1-%d]\n", drvs_ptr->d_nr_procs);
					exit(1);
				}
				break;				
			case 't':
				vmu.vm_nr_tasks = atoi(optarg);
				if( vmu.vm_nr_tasks <= 0 || vmu.vm_nr_tasks  > drvs_ptr->d_nr_tasks ) {
					fprintf (stderr, "Invalid nr_tasks [1-%d]\n", drvs_ptr->d_nr_tasks);
					exit(1);
				}
				break;	
			case 's':
				vmu.vm_nr_sysprocs = atoi(optarg);
				if( vmu.vm_nr_sysprocs <= 0 || vmu.vm_nr_sysprocs > drvs_ptr->d_nr_sysprocs ) {
					fprintf (stderr, "Invalid nr_sysprocs [1-%d]\n", drvs_ptr->d_nr_sysprocs);
					exit(1);
				}
				break;
			case 'n':
				vmu.vm_nr_nodes = atoi(optarg);
				if( vmu.vm_nr_nodes <= 0 || vmu.vm_nr_nodes > drvs_ptr->d_nr_nodes ) {
					fprintf (stderr, "Invalid nr_nodes [1-%d]\n",  drvs_ptr->d_nr_nodes);
					exit(1);
				}
				break;
			case 'N':
				if( strlen(optarg) > (MAXVMNAME-1) ) {
					fprintf (stderr, "VM name too long [%d]\n", (MAXVMNAME-1));
					exit(1);
				}
				strncpy(vmu.vm_name,optarg,(MAXVMNAME-1));
				break;
			default:
				fprintf (stderr,"usage: %s [-v vmid] [-p nr_procs] [-t nr_tasks] [-s nr_sysprocs] [-n nr_nodes] [-N VMname]\n", argv[0]);
				exit(1);
		}
	}
	
	if( argv[optind] != NULL){
		fprintf (stderr,"usage: %s [-v vmid] [-p nr_procs] [-t nr_tasks] [-s nr_sysprocs][-n nr_nodes] [-N VMname]\n", argv[0]);
		exit(1);
	}
	
	if( vmu.vm_nr_tasks  > vmu.vm_nr_procs )  {
		fprintf (stderr, "Invalid nr_tasks: must be <= nr_procs(%d)\n", vmu.vm_nr_procs);
		exit(1);
	}

	if( vmu.vm_nr_sysprocs  > vmu.vm_nr_procs )  {
		fprintf (stderr, "Invalid nr_sys_tasks: must be <= nr_procs(%d)\n", vmu.vm_nr_procs);
		exit(1);
	}

	
	TASKDEBUG("vmid 	= %d\n", vmu.vm_vmid);
	TASKDEBUG("nr_procs 	= %d\n", vmu.vm_nr_procs);
	TASKDEBUG("nr_tasks 	= %d\n", vmu.vm_nr_tasks);
	TASKDEBUG("nr_sysprocs	= %d\n", vmu.vm_nr_sysprocs);
	TASKDEBUG("nr_nodes	= %d\n", vmu.vm_nr_nodes);
	TASKDEBUG("nodes	= %d\n", vmu.vm_nodes);
	if( ( strcmp( vmu.vm_name, "VM0") == 0 ) &&
		vmu.vm_vmid != 0)
		sprintf(vmu.vm_name,"VM%d", vmu.vm_vmid);
	TASKDEBUG("vm_name	= %s\n", vmu.vm_name);
	
    nodeid = mnx_vm_init(&vmu);
    
	if( nodeid < 0) 
    		printf("ERROR %d: Initializing virtual machine %d... \n", nodeid, vmu.vm_vmid);
	else
    		TASKDEBUG("Virtual machine %d has been initialized on node %d\n", vmu.vm_vmid, nodeid);

	return(local_nodeid);
 }



