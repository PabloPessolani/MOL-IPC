#define _TABLE
#include "is.h"



VM_usr_t  vmu, *vm_ptr;
drvs_usr_t drvs, *drvs_ptr;
int local_nodeid;
proc_usr_t *kproc, *kp;
mproc_t *mproc, *mp;
priv_usr_t *kpriv, *kv;
slot_t *slots, *sp;


/*===========================================================================*
 *				main					     *
 *===========================================================================*/
int main ( int argc, char *argv[] )
{
	int rcode, i;
	int vmid, n1,n2;	
	int is_lpid, is_pid, child_pid, child_ep, is_ep, is_ep2;


	if ( argc != 2) {
 	        printf( "Usage: %s <vmid> \n", argv[0] );
 	        exit(1);
    	}

	vmid = atoi(argv[1]);
	drvs_ptr = &drvs;
	vm_ptr = &vmu;


	is_lpid = getpid();	
	SVRDEBUG("IS SERVER %d\n",is_lpid);

	/* Bind IS to the kernel */
SVRDEBUG("Binding process lpid=%d to VM%d with p_nr=%d\n",is_lpid,vmid,IS_PROC_NR);
	is_ep = mnx_bind(vmid, IS_PROC_NR);
	if(is_ep < 0) ERROR_EXIT(is_ep);
SVRDEBUG("is_ep=%d\n", is_ep);

	/* Bind IS to the PM/SYSTASK */
	is_pid = mol_bindproc(IS_PROC_NR, PROC_NO_PID, is_lpid);
	SVRDEBUG("IS MINIX PID=%d\n",is_pid);
	if( is_pid < 0) ERROR_EXIT(is_pid);
	
     /* alloc dynamic memory for the KERNEL process table */
SVRDEBUG("Alloc dynamic memory for the Kernel process (NR_TASKS + NR_PROCS)=%d\n", (NR_TASKS + NR_PROCS));
     	kproc = malloc((NR_TASKS + NR_PROCS)*sizeof(proc_usr_t));
     	if(kproc == NULL) ERROR_EXIT(rcode);

     /* alloc dynamic memory for the KERNEL priviledge table */
SVRDEBUG("Alloc dynamic memory for the Kernel priviledge (NR_TASKS + NR_PROCS)=%d\n", (NR_TASKS + NR_PROCS));
     	kpriv = malloc((NR_TASKS + NR_PROCS)*sizeof(priv_usr_t));
     	if(kproc == NULL) ERROR_EXIT(rcode);

    /* alloc dynamic memory for the SYSTASK slots allocation  table */
SVRDEBUG("Alloc dynamic memory for the SYSTASK slots allocation  table  (NR_TASKS + NR_PROCS)=%d\n", (NR_TASKS + NR_PROCS));
     	slots = malloc((NR_TASKS + NR_PROCS)*sizeof(slot_t));
     	if(slots == NULL) ERROR_EXIT(rcode);
		
     /* alloc dynamic memory for the PM process table */
SVRDEBUG("Alloc dynamic memory for the PM process table NR_PROCS=%d\n", NR_PROCS);
     	mproc = malloc((NR_PROCS)*sizeof(mproc_t));
	if(mproc == NULL) ERROR_EXIT(rcode)

	while (1) {
SVRDEBUG("\n\nDRVS info\n");
	rcode = mol_getkinfo(&drvs);
	if( rcode) ERROR_EXIT(rcode);
	printf(DRVS_USR_FORMAT, DRVS_USR_FIELDS(drvs_ptr)); 

SVRDEBUG("\n\nVM info\n");
	rcode = mol_getmachine(&vmu);
	if( rcode) ERROR_EXIT(rcode);
	printf(VM_USR_FORMAT, VM_USR_FIELDS(vm_ptr)); 

SVRDEBUG("\n\nKernel Process Table\n");
	rcode = mol_getkproctab(kproc);
	if( rcode) ERROR_EXIT(rcode);
	for (i= 0; i< (drvs.d_nr_procs+drvs.d_nr_tasks); i++) {
	  	kp = &kproc[i];
		if (kp->p_rts_flags == SLOT_FREE) continue;
		printf(PROC_USR_FORMAT,PROC_USR_FIELDS(kp));
	}

SVRDEBUG("\n\nKernel Priviledge  Table\n");
	rcode = mol_getprivtab(kpriv);
	if( rcode) ERROR_EXIT(rcode);
	for (i= 0; i< (drvs.d_nr_procs+drvs.d_nr_tasks); i++) {
	  	kv = &kpriv[i];
	  	kp = &kproc[i];
		if (kp->p_rts_flags == SLOT_FREE) continue;
		printf("name=%s endp=%d "PRIV_USR_FORMAT,kp->p_name, kp->p_endpoint,PRIV_USR_FIELDS(kv));
	}

SVRDEBUG("\n\nSYSTASK Slots Allocation Table\n");
	rcode = mol_getslotstab(slots);
	if( rcode) ERROR_EXIT(rcode);
	for (i= vmu.vm_nr_sysprocs; i< (vmu.vm_nr_procs+vmu.vm_nr_tasks); i++) {
	  	sp = &slots[i];
		printf("[%i]" SLOTS_FORMAT,i,SLOTS_FIELDS(sp));
	}
	
SVRDEBUG("\n\nPM Process Table\n");
	rcode = mol_getpmproctab(mproc);
	if( rcode) ERROR_EXIT(rcode);
	for (i= 0; i< drvs.d_nr_procs; i++) {
	  	mp = &mproc[i];
	  	kp = &kproc[i+drvs.d_nr_tasks];
		if (mp->mp_pid == PROC_NO_PID) continue;
		printf("name=%s endp=%d "PM_PROC_FORMAT,kp->p_name, kp->p_endpoint, PM_PROC_FIELDS(mp));
	}

SVRDEBUG("\n\n program_invocation_short_name=[%s]\n", program_invocation_short_name);

	sleep(10); 
	}
	
	sys_exit(IS_PROC_NR); 

SVRDEBUG("IS SERVER %d EXITING\n",is_lpid);
	exit(0);
}
	
