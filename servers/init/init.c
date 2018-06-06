#define _TABLE
#include "init.h"

int mol_child_exit(int lnx_pid, int status);

dc_usr_t  dcu, *dc_ptr;
dvs_usr_t dvs, *dvs_ptr;
int local_nodeid;

//#define INIT_PROC_EP	(INIT_PROC_NR+local_nodeid)
//#define INIT_PID_NODE	(INIT_PID+local_nodeid)
#define INIT_PROC_EP	(INIT_PROC_NR)
#define INIT_PID_NODE	(INIT_PID)

#define	MAX_CHILDREN	2 
#define FORK_WAIT_MS 1000
proc_usr_t pm;

/*===========================================================================*
 *				do_fork				     *
 *===========================================================================*/
pid_t do_fork(void)
{
	int lpid_2nd, child_lpid, rcode, child_pid, child_ep,parent_pid;
	int n1,n2;
	message *fork_ptr;
	static	message fork_msg __attribute__((aligned(0x1000)));

	SVRDEBUG("PARENT\n");
	fork_ptr = &fork_msg;
	
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
		
		/* Wait for PM to fork me  */
		rcode = mol_wait4fork();
		if( rcode < 0) 
			ERROR_EXIT(rcode);
		return(OK);
	}
	
	SVRDEBUG("PARENT: child_lpid=%d\n", child_lpid);
/* SOLO PARA PRUEBA */
sleep(2);

	child_pid = mol_fork(child_lpid, ANY);
	SVRDEBUG("mol_fork: child_lpid=%d child_pid=%d\n", child_lpid, child_pid);
		
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
	int n1,n2, rcode, head, tail;	
	int init_lpid, init_pid, child_ep, init_ep, init_ep2;
	int child_pid, child_sts, child_lpid, child_lsts; 
	double a, b;
 
	srand(local_nodeid*3);
	
	init_lpid = getpid();	
	SVRDEBUG("INIT SERVER %d\n",init_lpid);

	get_dvs_params();
	SVRDEBUG("INIT: mnx_wait4bind_T\n");
	do { 
		rcode = mnx_wait4bind_T(FORK_WAIT_MS);
		SVRDEBUG("INIT: mnx_wait4bind_T  rcode=%d\n", rcode);
		if(rcode > 0) break;
		if (rcode != EMOLTIMEDOUT) {
			ERROR_EXIT(rcode);
			continue ;
		}
		SVRDEBUG("INIT: mnx_wait4bind_T TIMEOUT\n");
	} while	(rcode == EMOLTIMEDOUT); 
	
	init_ep = rcode; 
	get_dc_params(PROC_NO_PID);
	
	SVRDEBUG("PARENT: forking\n");
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
			int rcode;
			message *m_ptr;

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

			rcode = mol_pm_findproc(iname, &ep);
			if(rcode) SVRDEBUG("mol_pm_findproc error=%d\n", rcode);
			SVRDEBUG("mol_pm_findproc: ep=%d\n",ep);

/* #define PROXY_AUTOBIND	 to test proxy remote binding */
#ifdef PROXY_AUTOBIND	
#define SINGLE_SERVER 	10 	/* endpoint of remote SINGLE SERVER */
			/*---------------- Allocate memory for message  ---------------*/
			posix_memalign( (void **) &m_ptr, getpagesize(), sizeof(message) );
			if (m_ptr== NULL) {
				fprintf(stderr, "CLIENT posix_memalign\n");
			}
			printf("CLIENT m_ptr=%p\n",m_ptr);
		
			printf(MSG1_FORMAT, MSG1_FIELDS(m_ptr));
			rcode = mnx_sendrec(SINGLE_SERVER , (long) m_ptr);
			if(rcode < 0) {
				fprintf(stderr,"CLIENT: mnx_sendrec rcode=%d\n", rcode);
			}
			printf(MSG1_FORMAT, MSG1_FIELDS(m_ptr));
			free(m_ptr);
#endif /* PROXY_AUTOBIND */
	
#ifdef ANULADO			
			rcode = mol_alarm(10);
			SVRDEBUG("mol_alarm: rcode=%d\n",rcode);
						
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


//	rcode= mol_wait(&status);
// SVRDEBUG("child exit status=%d rcode=%d\n",status, rcode);
	
	SVRDEBUG("INIT SERVER WAIT CHILDREN\n");
	while((child_pid = wait(&child_sts)) >= 0) {
		SVRDEBUG("child %d exiting status=%d\n",child_pid, child_sts);
//		rcode = mol_child_exit(child_pid, child_sts);
//SVRDEBUG("rcode=%d\n",rcode);
	}
	
	mol_exit(0);
	SVRDEBUG("INIT SERVER %d EXITING\n",init_lpid);
	exit(0);
}
	
