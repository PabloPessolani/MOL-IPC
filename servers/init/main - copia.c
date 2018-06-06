#define _TABLE
#include "init.h"

int mol_child_exit(int lnx_pid, int status);

DC_usr_t  dcu, *dc_ptr;
dvs_usr_t dvs, *dvs_ptr;
int local_nodeid;

#define NOENDPOINT -1
#define INIT_PROC_EP	(INIT_PROC_NR+local_nodeid)
#define INIT_PID_NODE	(INIT_PID+local_nodeid)



/*===========================================================================*
 *				do_fork				     *
 *===========================================================================*/
pid_t do_fork(void)
{
	int lpid_2nd, lpid_1st, rcode, child_pid, child_ep,parent_pid;
	message m;
	int n1,n2;
static	proc_usr_t pm;

SVRDEBUG("PARENT\n");
	child_pid = 3333;

	rcode = mnx_getprocinfo(dcu.dc_dcid, PM_PROC_NR, &pm);
	if(rcode) ERROR_RETURN(rcode);
		
    if ((lpid_1st = fork()) < 0) {
        ERROR_RETURN(lpid_1st);
    } else if (lpid_1st == 0) {     /* first child */	
		if ( (lpid_2nd = fork()) == 0) {  /* SECOND CHILD */
			SVRDEBUG("CHILD wait4bind\n");
			rcode = mnx_wait4bind();
			SVRDEBUG("CHILD rcode=%d\n", rcode);
			if( rcode < 0) exit(EXIT_FAILURE);
			return(0);            
		}else{
			SVRDEBUG("2ND PARENT:sleep before mol_fork lpid_2nd=%d pm.p_nodeid=%d\n", lpid_2nd, pm.p_nodeid);
			SVRDEBUG("2ND PARENT wait4bind\n");
			rcode = mnx_wait4bind();
			SVRDEBUG("2ND PARENT rcode=%d\n", rcode);
			if( rcode < 0) exit(EXIT_FAILURE);
			
			if( pm.p_nodeid == local_nodeid) {	/* LOCAL  INIT */
				child_pid = mol_fork(lpid_2nd, NOENDPOINT);
				SVRDEBUG("local mol_fork: lpid_2nd=%d child_pid=%d\n", lpid_2nd, child_pid);
			}else{				  	/* REMOTE INIT */
				/* BIND CHILD to local SYSTASK */
				child_ep = sys_fork(lpid_2nd);
				if(child_ep < 0)
					ERROR_RETURN(child_ep);
				child_pid = mol_fork(lpid_2nd, child_ep);
			}
			/* Wakeup the CHILD */
			if( child_pid < 0){                                          
				/* mnx_errno= child_pid; */
				ERROR_RETURN(child_pid);
			}
			SVRDEBUG("lpid_2nd=%d child_pid=%d\n",lpid_2nd, child_pid);
			mol_exit(-7);
			exit(0);						/* SECOND PARENT = FIRST CHILD */
		}
	}else { /* FIRST PARENT */
		parent_pid = mol_fork(lpid_1st, NOENDPOINT);
		SVRDEBUG("1ST PARENT:  mol_fork lpid_1st=%d parent_pid=%d\n", lpid_1st, parent_pid);
	    if ( (rcode  = waitpid(lpid_1st, NULL, 0)) != lpid_1st)  /* wait for first child */
			ERROR_RETURN(rcode);
	}
	return(child_pid);   
}

/*===========================================================================*
 *				get_dvs_params				     *
 *===========================================================================*/
void get_dvs_params(void)
{
	local_nodeid = mnx_getdvsinfo(&dvs);
SVRDEBUG("local_nodeid=%d\n",local_nodeid);
	if( local_nodeid < DVS_NO_INIT) ERROR_EXIT(local_nodeid);
	dvs_ptr = &dvs;
SVRDEBUG(DVS_USR_FORMAT, DVS_USR_FIELDS(dvs_ptr));

}

/*===========================================================================*
 *				get_dc_params				     *
 *===========================================================================*/
void get_dc_params(int vmid)
{
	int rcode;

	if ( vmid < 0 || vmid >= dvs.d_nr_dcs) {
 	        printf( "Invalid vmid [0-%d]\n", dvs.d_nr_dcs );
 	        ERROR_EXIT(EMOLBADDCID);
	}
SVRDEBUG("vmid=%d\n", vmid);
	rcode = mnx_getdcinfo(vmid, &dcu);
	if( rcode ) ERROR_EXIT(rcode);
	dc_ptr = &dcu;
SVRDEBUG(DC_USR_FORMAT, DC_USR_FIELDS(dc_ptr));
}


/*===========================================================================*
 *				main					     *
 *===========================================================================*/
int main ( int argc, char *argv[] )
{
	int vmid, n1,n2, ret, status;	
	int init_lpid, init_pid, child_pid, child_ep, child_sts, init_ep, init_ep2;

	if ( argc != 2) {
 	        printf( "Usage: %s <vmid> \n", argv[0] );
 	        exit(1);
    	}

	get_dvs_params();

	srand(local_nodeid*3);
	
	vmid = atoi(argv[1]);
	get_dc_params(vmid);

	init_lpid = getpid();	
	SVRDEBUG("INIT SERVER %d\n",init_lpid);

	/* Bind INIT to the kernel */
SVRDEBUG("Binding process lpid=%d to DC%d with p_nr=%d\n",init_lpid,dc_ptr->dc_dcid,INIT_PROC_EP);
	init_ep = mnx_bind(dc_ptr->dc_dcid, INIT_PROC_EP);
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
	

SVRDEBUG("PARENT: forking\n");
	do {
		if( (child_pid = do_fork()) == 0) {
				
			struct timeval tv;
			time_t tt,tr;
			int tzp, newtime, ep, pid;
			struct tms buf;
			static char iname[]="init";

			setsid();
			
			SVRDEBUG("INIT CHILD Linux PID:%d Minix PID:%d\n",getpid(), mol_getpid());
			mol_gettimeofday(&tv, &tzp);
			SVRDEBUG("mol_gettimeofday: tv.tv_sec=%ld tv.tv_usec=%ld\n",tv.tv_sec,tv.tv_usec);

			tr = mol_time(&tt);
			SVRDEBUG("mol_time: tt=%ld tr=%ld\n",tt,tt);

			newtime= tr + 1000;
			mol_stime(&newtime);

			ep = mol_getprocnr();
			SVRDEBUG("mol_getprocnr: ep=%d\n",ep);

			ep = mol_getpprocnr();
			SVRDEBUG("mol_getpprocnr: ep=%d\n",ep);

			pid = mol_getpid();
			ep  = mol_getnprocnr(pid);
			SVRDEBUG("mol_getnprocnr: ep=%d\n",ep);

			ret = mol_pm_findproc(iname, &ep);
			if(ret) SVRDEBUG("mol_pm_findproc error=%d\n", ret);
			SVRDEBUG("mol_pm_findproc: ep=%d\n",ep);

			/* ANULADO FOR TESTING
			ret = mol_alarm(10);
			SVRDEBUG("mol_alarm: ret=%d\n",ret);
			*/
			
			sleep(120);

			/* ANULADO FOR TESTING
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
			*/
			
			SVRDEBUG("CHILD EXITING PID:%d\n",getpid());
			mol_exit(-7);
			exit(0);
		} else {
			SVRDEBUG("PARENT child_pid=%d\n",child_pid);
			sleep(2-(local_nodeid*2));
		}
		if(child_pid < 0) sleep(10);
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
	
