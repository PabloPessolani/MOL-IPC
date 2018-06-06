#define _TABLE
#include "init.h"

int mol_child_exit(int lnx_pid, int status);

DC_usr_t  dcu, *dc_ptr;
dvs_usr_t dvs, *dvs_ptr;
int local_nodeid;

#define NOENDPOINT -1
//#define INIT_PROC_EP	(INIT_PROC_NR+local_nodeid)
//#define INIT_PID_NODE	(INIT_PID+local_nodeid)
#define INIT_PROC_EP	(INIT_PROC_NR)
#define INIT_PID_NODE	(INIT_PID)

#define	MAX_CHILDREN	150 
#define FORK_WAIT_MS 1000
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
		do { 
			rcode = mnx_wait4bind_T(FORK_WAIT_MS);
			SVRDEBUG("CHILD: mnx_wait4bind_T  rcode=%d\n", rcode);
			if (rcode == EMOLTIMEDOUT) {
				SVRDEBUG("CHILD: mnx_wait4bind_T TIMEOUT\n");
				continue ;
			}else if( rcode < 0) 
				exit(EXIT_FAILURE);
		} while	(rcode < OK); 
		return(OK);
	}
	
	SVRDEBUG("PARENT: child_lpid=%d\n", child_lpid);
/* SOLO PARA PRUEBA */
sleep(2);

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
	int vmid, n1,n2, ret, head, tail;	
	int init_lpid, init_pid, child_ep, init_ep, init_ep2;
	int child_pid, child_sts, child_lpid, child_lsts; 
	double a, b;
 
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
	init_ep = mnx_replbind(dc_ptr->dc_dcid, init_lpid, INIT_PROC_EP);
	if(init_ep < 0) ERROR_EXIT(init_ep);
	SVRDEBUG("init_ep=%d\n", init_ep);

	/* Bind to LOCAL SYSTASK */
	SVRDEBUG("Register INIT into LOCAL SYSTASK init_lpid=%d\n",init_lpid);
	init_ep = sys_bindproc(INIT_PROC_EP, init_lpid, REPLICA_BIND);
	if(init_ep < 0) ERROR_EXIT(init_ep);

	/* Bind to PM  */
	init_pid = mol_bindproc(INIT_PROC_EP, INIT_PID_NODE, init_lpid);
	SVRDEBUG("INIT MINIX PID=%d\n",init_pid);
	if( init_pid < 0) ERROR_EXIT(init_pid);
	
	ret = mnx_getprocinfo(dcu.dc_dcid, PM_PROC_NR, &pm);
	if(ret) ERROR_RETURN(ret);
	
S	VRDEBUG("PARENT: forking\n");
	head = 0;
	tail = 0;
	do {
		if( (child_pid = do_fork()) == 0) {
		/*-----------------------------------------------------------------------------*/		
		/*				CHILD							*/
		/*-----------------------------------------------------------------------------*/		
			struct timeval tv;
			time_t tt,tr;
			int tzp, newtime, ep, parent_ep, child_ep, pid;
			struct tms buf;
			static char iname[]="init";

			srandom(child_pid);
			
			setsid();
			
			pid = mol_getpid();
			SVRDEBUG("INIT CHILD Linux PID:%d Minix PID:%d\n",getpid(), pid);
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
		/*-----------------------------------------------------------------------------*/		
		/*				PARENT							*/
		/*-----------------------------------------------------------------------------*/		
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
	
