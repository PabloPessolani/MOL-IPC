#define _TABLE
#include "is.h"



dc_usr_t  dcu, *dc_ptr;
dvs_usr_t dvs, *dvs_ptr;
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
	dvs_ptr = &dvs;
	dc_ptr = &dcu;


	is_lpid = getpid();	
	SVRDEBUG("IS SERVER %d\n",is_lpid);

	/* Bind IS to the kernel */
SVRDEBUG("Binding process lpid=%d to DC%d with p_nr=%d\n",is_lpid,vmid,IS_PROC_NR);
	is_ep = mnx_bind(vmid, IS_PROC_NR);
	if(is_ep < 0) ERROR_EXIT(is_ep);
SVRDEBUG("is_ep=%d\n", is_ep);

	/* Register into SYSTASK (as an autofork) */
	SVRDEBUG("Register IS into SYSTASK is_lpid=%d\n",is_lpid);
	rcode = sys_bindproc(IS_PROC_NR, is_lpid, LCL_BIND);
	if(rcode != is_ep) ERROR_EXIT(rcode);

	/* Bind IS to the PM */
	is_pid = mol_bindproc(IS_PROC_NR, PROC_NO_PID, is_lpid);

	SVRDEBUG("IS MINIX PID=%d\n",is_pid);
	if( is_pid < 0) ERROR_EXIT(is_pid);
	
     /* alloc dynamic memory for the KERNEL process table */
SVRDEBUG("Alloc dynamic memory for the Kernel process (NR_TASKS + NR_PROCS)=%d\n", (NR_TASKS + NR_PROCS));
//     	kproc = malloc((NR_TASKS + NR_PROCS)*sizeof(proc_usr_t));
		posix_memalign( (void**) &kproc, getpagesize(), (NR_TASKS + NR_PROCS)*sizeof(proc_usr_t));
     	if(kproc == NULL) ERROR_EXIT(rcode);

     /* alloc dynamic memory for the KERNEL priviledge table */
SVRDEBUG("Alloc dynamic memory for the Kernel priviledge (NR_TASKS + NR_PROCS)=%d\n", (NR_TASKS + NR_PROCS));
//     	kpriv = malloc((NR_TASKS + NR_PROCS)*sizeof(priv_usr_t));
		posix_memalign( (void**) &kpriv, getpagesize(), (NR_TASKS + NR_PROCS)*sizeof(priv_usr_t));
     	if(kproc == NULL) ERROR_EXIT(rcode);

    /* alloc dynamic memory for the SYSTASK slots allocation  table */
SVRDEBUG("Alloc dynamic memory for the SYSTASK slots allocation  table  (NR_TASKS + NR_PROCS)=%d\n", (NR_TASKS + NR_PROCS));
//     	slots = malloc((NR_TASKS + NR_PROCS)*sizeof(slot_t));
		posix_memalign( (void**) &slots, getpagesize(), (NR_TASKS + NR_PROCS)*sizeof(slot_t));
     	if(slots == NULL) ERROR_EXIT(rcode);
		
     /* alloc dynamic memory for the PM process table */
SVRDEBUG("Alloc dynamic memory for the PM process table NR_PROCS=%d\n", NR_PROCS);
//     	mproc = malloc((NR_PROCS)*sizeof(mproc_t));
	posix_memalign( (void**) &mproc, getpagesize(), (NR_TASKS + NR_PROCS)*sizeof(mproc_t));
	if(mproc == NULL) ERROR_EXIT(rcode)

	while (1) {
SVRDEBUG("\n\nDVS info\n");
	rcode = mol_getkinfo(&dvs);
	if( rcode) ERROR_EXIT(rcode);
	printf(DVS_USR_FORMAT, DVS_USR_FIELDS(dvs_ptr)); 

SVRDEBUG("\n\nDC info\n");
	rcode = mol_getmachine(&dcu);
	if( rcode) ERROR_EXIT(rcode);
	printf(DC_USR_FORMAT, DC_USR_FIELDS(dc_ptr)); 

SVRDEBUG("\n\nKernel Process Table\n");
	rcode = mol_getkproctab(kproc);
	if( rcode) ERROR_EXIT(rcode);
	for (i= 0; i< (dvs.d_nr_procs+dvs.d_nr_tasks); i++) {
	  	kp = &kproc[i];
		if (kp->p_rts_flags == SLOT_FREE) continue;
		printf(PROC_USR_FORMAT,PROC_USR_FIELDS(kp));
	}

SVRDEBUG("\n\nKernel Priviledge  Table\n");
	rcode = mol_getprivtab(kpriv);
	if( rcode) ERROR_EXIT(rcode);
	for (i= 0; i< (dvs.d_nr_procs+dvs.d_nr_tasks); i++) {
	  	kv = &kpriv[i];
	  	kp = &kproc[i];
		if (kp->p_rts_flags == SLOT_FREE) continue;
		printf("name=%s endp=%d "PRIV_USR_FORMAT,kp->p_name, kp->p_endpoint,PRIV_USR_FIELDS(kv));
	}

SVRDEBUG("\n\nSYSTASK Slots Allocation Table\n");
	rcode = mol_getslotstab(slots);
	if( rcode) ERROR_EXIT(rcode);
	for (i= dcu.dc_nr_sysprocs; i< (dcu.dc_nr_procs+dcu.dc_nr_tasks); i++) {
	  	sp = &slots[i];
		printf("[%i]" SLOTS_FORMAT,i,SLOTS_FIELDS(sp));
	}
	
SVRDEBUG("\n\nPM Process Table\n");
	rcode = mol_getpmproctab(mproc);
	if( rcode) ERROR_EXIT(rcode);
	for (i= 0; i< dvs.d_nr_procs; i++) {
	  	mp = &mproc[i];
	  	kp = &kproc[i+dvs.d_nr_tasks];
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
	
