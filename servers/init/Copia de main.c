#define _TABLE
#include "init.h"

int mol_child_exit(int lnx_pid, int status);

DC_usr_t  dcu, *dc_ptr;
dvs_usr_t dvs, *dvs_ptr;
int local_nodeid;

#define KEY1 	1111
#define KEY2	2222
#define NOENDPOINT -1
#define INIT_PROC_EP	(INIT_PROC_NR+local_nodeid)
#define INIT_PID_NODE	(INIT_PID+local_nodeid)

sem_t *mutex1, *mutex2;

sem_t *alloc_sem(int keynbr)
{
    key_t key = keynbr;
    int shmid;
    shmid = shmget(key, sizeof(sem_t), IPC_CREAT | 0666);
	if( shmid < 0) exit(EXIT_FAILURE);
    return ( (sem_t *) shmat(shmid, NULL, 0));
}


/*===========================================================================*
 *				do_fork				     *
 *===========================================================================*/
pid_t do_fork(void)
{
	int child_lpid, rcode, child_pid, child_ep;
	message m;
	int n1,n2;
static	proc_usr_t pm;

sem_getvalue(mutex1,&n1);
sem_getvalue(mutex2,&n2);
SVRDEBUG("PARENT mutex1=%d mutex2=%d\n",n1, n2);

	if ( (child_lpid = fork()) == 0) {  /* CHILD */
		mutex1 = alloc_sem(KEY1);
		mutex2 = alloc_sem(KEY2);
		/* Wait for parent */
sem_getvalue(mutex1,&n1);
sem_getvalue(mutex2,&n2);
SVRDEBUG("CHILD mutex1=%d mutex2=%d\n",n1, n2);
		sem_wait(mutex1); 		
		/* Wait PM reply */
		rcode = mnx_receive(PM_PROC_NR, &m);
		sem_post(mutex2);
		if( rcode) exit(EXIT_FAILURE);
        return(0);            
       	}                         
          
	rcode = mnx_getprocinfo(dcu.dc_dcid, PM_PROC_NR, &pm);
	if(rcode) ERROR_EXIT(rcode);

	if( pm.p_nodeid == local_nodeid) {	/* LOCAL  INIT */
		child_pid = mol_fork(child_lpid, NOENDPOINT);
	}else{				  	/* REMOTE INIT */
		/* BIND CHILD to local SYSTASK */
		child_ep = sys_fork(child_lpid);
		if(child_ep < 0)
			ERROR_RETURN(child_ep);
		child_pid = mol_fork(child_lpid, child_ep);
	}

SVRDEBUG("child_lpid=%d child_pid=%d\n",child_lpid, child_pid);

	/* Wakeup the CHILD */
	if( child_pid < 0){                                          
        	/* mnx_errno= child_pid; */
	        return(-1);
	}
	sem_post(mutex1);
	sem_wait(mutex2); 		
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
	int vmid, n1,n2, ret;	
	int init_lpid, init_pid, child_pid, child_ep, child_sts, init_ep, init_ep2;


	if ( argc != 2) {
 	        printf( "Usage: %s <vmid> \n", argv[0] );
 	        exit(1);
    	}

	get_dvs_params();

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
	
	mutex1 = alloc_sem(KEY1);
	mutex2 = alloc_sem(KEY2);

	sem_init(mutex1,1,0);
	sem_init(mutex2,1,0);

sem_getvalue(mutex1,&n1);
sem_getvalue(mutex2,&n2);
SVRDEBUG("PARENT mutex1=%d mutex2=%d\n",n1, n2);

SVRDEBUG("forking\n");
//	do {
		if( (child_pid = do_fork()) == 0) {

			SVRDEBUG("INIT CHILD Linux PID:%d Minix PID:%d\n",getpid(), mol_getpid());
		
			while(1) sleep(60);
			SVRDEBUG("INIT EXIT CHILD Linux PID:%d Minix PID:%d\n",getpid(), mol_getpid());
			exit(0);
		} else {
		SVRDEBUG("child_pid=%d\n",child_pid);
		}
//	}while(child_pid > 0);

	
SVRDEBUG("INIT SERVER WAIT CHILDREN\n");
	while((child_pid = wait(&child_sts)) >= 0) {
SVRDEBUG("child %d exiting status=%d\n",child_pid, child_sts);
		ret = mol_child_exit(child_pid, child_sts);
SVRDEBUG("ret=%d\n",ret);
	}
	
	sys_exit(INIT_PROC_EP); 
SVRDEBUG("INIT SERVER %d EXITING\n",init_lpid);
	exit(0);

}
	
