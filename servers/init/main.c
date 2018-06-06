#define _TABLE
#include "init.h"

int mol_child_exit(int lnx_pid, int status);

VM_usr_t  vmu, *vm_ptr;
drvs_usr_t drvs, *drvs_ptr;
int local_nodeid;

#define NOENDPOINT -1
#define INIT_PROC_EP	(INIT_PROC_NR+local_nodeid)
#define INIT_PID_NODE	(INIT_PID+local_nodeid)

#define	MAX_CHILDREN	150

proc_usr_t pm;

/*===========================================================================*
 *				do_fork				     *
 *===========================================================================*/
pid_t do_fork(void)
{
	int lpid_2nd, child_lpid, rcode, child_pid, child_ep,parent_pid;
	message m;
	int n1,n2;

SVRDEBUG("PARENT\n");
		
    if ((child_lpid = fork()) == 0) {     /* first child */	
		rcode = mnx_wait4bind();
		SVRDEBUG("CHILD: mnx_wait4bind  rcode=%d\n", rcode);
		if( rcode < 0) exit(EXIT_FAILURE);
		return(OK);
	}
	
	SVRDEBUG("PARENT: child_lpid=%d\n", child_lpid);
	if( pm.p_nodeid == local_nodeid) {	/* LOCAL  INIT */
		child_pid = mol_fork(child_lpid, NOENDPOINT);
		SVRDEBUG("local mol_fork: child_lpid=%d child_pid=%d\n", child_lpid, child_pid);
	}else{				  	/* REMOTE INIT */
		/* BIND CHILD to local SYSTASK */
		child_ep = sys_fork(child_lpid);
		if(child_ep < 0)
			ERROR_RETURN(child_ep);
		child_pid = mol_fork(child_lpid, child_ep);
		SVRDEBUG("local mol_fork: child_lpid=%d child_pid=%d child_ep=%d\n", child_lpid, child_pid, child_ep);
	}
		
	/* Wakeup the CHILD */
	if( child_pid < 0){                                          
		/* mnx_errno= child_pid; */
		ERROR_RETURN(child_pid);
	}

	return(child_pid);   
}

/*===========================================================================*
 *				get_drvs_params				     *
 *===========================================================================*/
void get_drvs_params(void)
{
	local_nodeid = mnx_getdrvsinfo(&drvs);
SVRDEBUG("local_nodeid=%d\n",local_nodeid);
	if( local_nodeid < DRVS_NO_INIT) ERROR_EXIT(local_nodeid);
	drvs_ptr = &drvs;
SVRDEBUG(DRVS_USR_FORMAT, DRVS_USR_FIELDS(drvs_ptr));

}

/*===========================================================================*
 *				get_vm_params				     *
 *===========================================================================*/
void get_vm_params(int vmid)
{
	int rcode;

	if ( vmid < 0 || vmid >= drvs.d_nr_vms) {
 	        printf( "Invalid vmid [0-%d]\n", drvs.d_nr_vms );
 	        ERROR_EXIT(EMOLBADVMID);
	}
SVRDEBUG("vmid=%d\n", vmid);
	rcode = mnx_getvminfo(vmid, &vmu);
	if( rcode ) ERROR_EXIT(rcode);
	vm_ptr = &vmu;
SVRDEBUG(VM_USR_FORMAT, VM_USR_FIELDS(vm_ptr));
}


/*===========================================================================*
 *				main					     *
 *===========================================================================*/
int main ( int argc, char *argv[] )
{
	int vmid, n1,n2, ret, head, tail;	
	int init_lpid, init_pid, child_ep, init_ep, init_ep2;
	int child_pid, child_sts, child_lpid, child_lsts; 
	double a, b;
 
	if ( argc != 2) {
 	        printf( "Usage: %s <vmid> \n", argv[0] );
 	        exit(1);
    	}

	get_drvs_params();

	srand(local_nodeid*3);
	
	vmid = atoi(argv[1]);
	get_vm_params(vmid);

	init_lpid = getpid();	
	SVRDEBUG("INIT SERVER %d\n",init_lpid);

	/* Bind INIT to the kernel */
SVRDEBUG("Binding process lpid=%d to VM%d with p_nr=%d\n",init_lpid,vm_ptr->vm_vmid,INIT_PROC_EP);
	init_ep = mnx_bind(vm_ptr->vm_vmid, INIT_PROC_EP);
	if(init_ep < 0) ERROR_EXIT(init_ep);
SVRDEBUG("init_ep=%d\n", init_ep);

	/* Bind to LOCAL SYSTASK */
SVRDEBUG("Register INIT into LOCAL SYSTASK init_lpid=%d\n",init_lpid);
	init_ep = sys_bindproc(INIT_PROC_EP, init_lpid);
	if(init_ep < 0) ERROR_EXIT(init_ep);

	/* Bind to PM  */
	init_pid = mol_bindproc(INIT_PROC_EP, INIT_PID_NODE, init_lpid);
SVRDEBUG("INIT MINIX PID=%d\n",init_pid);
	if( init_pid < 0) ERROR_EXIT(init_pid);
	
	ret = mnx_getprocinfo(vmu.vm_vmid, PM_PROC_NR, &pm);
	if(ret) ERROR_RETURN(ret);
	
SVRDEBUG("PARENT: forking\n");
	head = 0;
	tail = 0;
	do {
		if( (child_pid = do_fork()) == 0) {
				
			struct timeval tv;
			time_t tt,tr;
			int tzp, newtime, ep, parent_ep, child_ep, pid;
			struct tms buf;
			static char iname[]="init";

			srandom(child_pid);
			
			setsid();
			
			SVRDEBUG("INIT CHILD Linux PID:%d Minix PID:%d\n",getpid(), mol_getpid());
			mol_gettimeofday(&tv, &tzp);
			SVRDEBUG("mol_gettimeofday: tv.tv_sec=%ld tv.tv_usec=%ld\n",tv.tv_sec,tv.tv_usec);

			tr = mol_time(&tt);
			SVRDEBUG("mol_time: tt=%ld tr=%ld\n",tt,tt);

			newtime= tr + 1000;
			mol_stime(&newtime);

			child_ep = mol_getprocnr();
			SVRDEBUG("mol_getprocnr: child_ep=%d\n",child_ep);

			parent_ep = mol_getpprocnr();
			SVRDEBUG("mol_getpprocnr: parent_ep=%d\n",parent_ep);

			pid = mol_getpid();
			ep  = mol_getnprocnr(pid);
			SVRDEBUG("mol_getnprocnr: ep=%d\n",ep);

			ret = mol_pm_findproc(iname, &ep);
			if(ret) SVRDEBUG("mol_pm_findproc error=%d\n", ret);
			SVRDEBUG("mol_pm_findproc: ep=%d\n",ep);

#ifdef ANULADO			
			ret = mol_alarm(10);
			SVRDEBUG("mol_alarm: ret=%d\n",ret);
						
			sleep((random()/RAND_MAX*60) + 1 );

			mol_gettimeofday(&tv, &tzp);
			SVRDEBUG("mol_gettimeofday: tv.tv_sec=%ld tv.tv_usec=%ld\n",tv.tv_sec,tv.tv_usec);

			tr = mol_time(&tt);
			SVRDEBUG("mol_time: tt=%ld tr=%ld\n",tt,tt);

			SVRDEBUG("INIT EXIT CHILD Linux PID:%d Minix PID:%d\n",getpid(), mol_getpid());
			
			newtime = mol_times(&buf);
			SVRDEBUG("newtime=%ld utime=%ld stime=%ld cutime=%ld cstime=%ld\n",newtime, 
				buf.tms_utime,
  				buf.tms_stime,
  				buf.tms_cutime,
  				buf.tms_cstime);
#else			
			a = (float)random() * 120.0;
			b = (float) RAND_MAX;
			sleep( (int) (a/b) + 1 );
			
#endif
		
			SVRDEBUG("CHILD EXITING PID:%d\n",getpid());
			mol_exit(-7);
			sys_exit(child_ep);
			exit(0);
		} else {
			head++;
			if( (head-tail) <  MAX_CHILDREN) {
				sleep(1);
			}else{
/*
			child_pid = mol_wait(&child_sts);
				if( child_pid < 0) 
					SVRDEBUG("ERROR mol_wait=%d\n",child_pid);
				SVRDEBUG("MINIX child %d exiting status=%d\n",child_pid, child_sts);
*/		
				do {
					child_lpid = wait(&child_lsts);
					if( child_lpid < 0) 
						SVRDEBUG("ERROR wait=%d\n",child_lpid);
					}while( child_lpid < 0);
				SVRDEBUG("LINUX child %d exiting status=%d\n",child_lpid, child_lsts);
				tail++;
			}
			SVRDEBUG("head=%d tail=%d dif=%d\n",head, tail, (head-tail));
		}
	}while(1);  /*child_pid > 0); */


//	ret= mol_wait(&status);
// SVRDEBUG("child exit status=%d ret=%d\n",status, ret);
	
SVRDEBUG("INIT SERVER WAIT CHILDREN\n");
	while((child_pid = wait(&child_sts)) >= 0) {
SVRDEBUG("child %d exiting status=%d\n",child_pid, child_sts);
//		ret = mol_child_exit(child_pid, child_sts);
//SVRDEBUG("ret=%d\n",ret);
	}
	

	
	sys_exit(INIT_PROC_EP); 
SVRDEBUG("INIT SERVER %d EXITING\n",init_lpid);
	exit(0);

}
	
