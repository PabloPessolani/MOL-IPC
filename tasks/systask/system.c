/****************************************************************/
/* 				SYSTASK				*/
/* SYSTASK is like the SYSTEM TASK of MINIX			*/
/****************************************************************/

#define _SYSTEM
#include "systask.h"

#define TRUE 1
#define ERESTARTSYS (-512)

/* Declaration of the call vector that defines the mapping of system calls 
 * to handler functions. The vector is initialized in sys_init() with map(), 
 * which makes sure the system call numbers are ok. No space is allocated, 
 * because the dummy is declared extern. If an illegal call is given, the 
 * array size will be negative and this won't compile. 
 */

int (*call_vec[NR_SYS_CALLS])(message *m_ptr);

#define map(call_nr, handler) call_vec[(call_nr-KERNEL_CALL)] = (handler)


/****************************************************************************************/
/*			SYSTEM TASK 						*/
/****************************************************************************************/
int  main ( int argc, char *argv[] )
{
	int dcid, rcode, nodeid;
	char *argVec[2];
	unsigned int call_nr;
	int result;
	message sp_msg, *sp_ptr; 		/* for use with SPREAD */
	proc_usr_t *proc_ptr;
	
	/*------------------------------------
	* initialize DVS info struct
 	*------------------------------------*/	
	nodeid = mnx_getdvsinfo(&dvs);
	if(nodeid < 0 )
		ERROR_EXIT(EMOLDVSINIT);
		
	dvs_ptr = &dvs;
	TASKDEBUG(DVS_USR_FORMAT, DVS_USR_FIELDS(dvs_ptr));
	local_nodeid = nodeid;
	TASKDEBUG("local_nodeid=%d\n", local_nodeid);

	/*------------------------------------
	 * Initialize the DC
	 *------------------------------------*/
	dc_ptr = &dcu;
	rcode = mnx_getdcinfo(dcu.dc_dcid, &dcu);	
	if(rcode) {
		TASKDEBUG("local_nodeid=%d\n", local_nodeid);
		do {		
			nodeid = dc_init(argc, argv);
			dcid   = dcu.dc_dcid;
			if( nodeid < 0 && nodeid != EMOLEXIST) ERROR_EXIT(nodeid);
			if( nodeid == EMOLEXIST) {
				rcode = mnx_dc_end(dcid);
				if(rcode) ERROR_EXIT(rcode);
			}
		} while( nodeid == EMOLEXIST);
	}else{
			dcid   = dcu.dc_dcid;		
	}
		
	/*------------------------------------
	 * Get NODE info
	 *------------------------------------*/
	node_ptr = &node;
	rcode = mnx_getnodeinfo(local_nodeid, &node);
	if(rcode) ERROR_EXIT(rcode);

	pagesize = sysconf(_SC_PAGESIZE); /*  getpagesize() */

/*-------------------------- FORK TO OTHER TASKS/SERVERS --------------------------------
	if( (pm_pid = fork()) == 0){		
		printf("Binding PM to DC %d with p_nr=%d pid=%d\n", dcid, PM_PROC_NR,  pm_pid);
    		sys_ep = mnx_bind(dcid, PM_PROC_NR);
		if( sys_ep < 0) ERROR_EXIT(sys_ep);
    			printf("MOL_PM has binded with p_endpoint=%d\n", sys_ep);
			
			rcode = execv(argVec[0], argVec);	
			if(rcode) ERROR_EXIT(rcode);
	}
-----------------------------------------------------------------------------------------*/
	
  	/* Initialize the system task. */
	rcode = init_systask(dcid);
	if(rcode) ERROR_EXIT(rcode);

	rcode = init_clock(dcid);
	if (rcode ) ERROR_EXIT(rcode);

#define TEST_ENDPOINT 2

	/*------------------------------------
	 * SYSTEM TASK LOOP
	 *------------------------------------*/
//	pthread_mutex_lock(&sys_mutex);

	while(TRUE) {

//		pthread_mutex_unlock(&sys_mutex);

//		if (sigsetjmp(senv, 1) != 0) { /* the SIGALARM has executed 	*/
//TASKDEBUG("sigsetjmp realtime=%ld\n", realtime);
//			rcode = mnx_notify(TEST_ENDPOINT);
//			continue;
//		}	

		/*------------------------------------
		* Receive Request 
	 	*------------------------------------*/
		TASKDEBUG("SYSTASK is waiting for requests\n");
//		rcode = mnx_rcvrqst(&m);
		rcode = mnx_receive(ANY,&m);

		TASKDEBUG("mnx_receive rcode=%d\n", rcode);
		if(rcode) {
			SYSERR(rcode);
//			continue;
			exit(1);
		}

		m_ptr = &m;
   		TASKDEBUG("RECEIVE msg:"MSG4_FORMAT,MSG4_FIELDS(m_ptr));
		
		/*------------------------------------
		* Process the Request 
	 	*------------------------------------*/

		call_nr = (unsigned) m_ptr->m_type - KERNEL_CALL;	
      	who_e = m_ptr->m_source;
		who_p = _ENDPOINT_P(who_e);
		TASKDEBUG("call_nr=%d who_e=%d\n", call_nr, who_e);
		if (call_nr >= NR_SYS_CALLS || call_nr < 0) {	/* check call number 	*/
			SYSERR(EMOLBADREQUEST);
			result = EMOLBADREQUEST;		/* illegal message type */
      	} else {	
			TASKDEBUG("Calling vector %d\n",call_nr);
//while(1) sleep(1);
//exit(1);	
        	result = (*call_vec[call_nr])(&m);	/* handle the system call*/
      	}
			
		/*------------------------------------
	 	* Send Reply 
 		*------------------------------------*/
		if (result != EMOLDONTREPLY) {
  	  		m_ptr->m_type = result;		/* report status of call */
			TASKDEBUG("REPLY msg:"MSG1_FORMAT,MSG1_FIELDS(m_ptr));
    		rcode = mnx_send(m.m_source, (long) &m);
//    			rcode = mnx_send(m.m_source, (long) &m);
			if(rcode) {
				switch(rcode){  /* Auto Unbind the failed process */
					case	EMOLSRCDIED:
					case	EMOLDSTDIED:
					case	EMOLNOPROXY:
					case	EMOLNOTCONN:
					case	EMOLDEADSRCDST:
#ifdef ALLOC_LOCAL_TABLE 			
						rcode = mnx_getprocinfo(dc_ptr->dc_dcid, m.m_source, proc_ptr);	
						if( rcode < 0) ERROR_RETURN(rcode );
#else /* ALLOC_LOCAL_TABLE */			
						proc_ptr = (proc_usr_t *) PROC_MAPPED(_ENDPOINT_P(m.m_source));
						rcode = OK;
#endif /* ALLOC_LOCAL_TABLE */			
						
						if( proc_ptr->p_rts_flags == SLOT_FREE ) break;					
						if( !(proc_ptr->p_rts_flags & (REMOTE) )){
							rcode = mcast_exit_proc(proc_ptr);
							if( rcode < 0) ERROR_RETURN(rcode );
						}
						rcode = mnx_unbind(dc_ptr->dc_dcid, m.m_source);
						if( rcode < 0) ERROR_RETURN(rcode );
#ifdef ALLOC_LOCAL_TABLE 			
						rcode = mnx_getprocinfo(dc_ptr->dc_dcid, m.m_source, proc_ptr);
						if( rcode < 0) ERROR_RETURN(rcode );
#endif /* ALLOC_LOCAL_TABLE */			
						break;
					default:
						break;
				}
				SYSERR(rcode);
				continue;
			}
		}
	}		

	if(rcode) ERROR_RETURN(rcode);
	return(OK);
 }


/*===========================================================================*
 *				init_systask				     *
 *===========================================================================*/
int init_systask(int dcid)
{
	priv_usr_t *priv_ptr;
	proc_usr_t *proc_ptr;
	int sp_nr, sp_ep, retries;
	int i, rcode, mtype, mnode, leftover, svc_type;
	message sp_msg, *sp_ptr;
	slot_t  *sp; 		
	dc_usr_t *v_ptr;
static char source_name[MAX_GROUP_NAME];
static char debug_path[MNX_PATH_MAX];

	last_rqst = time(NULL);

	/*------------------------------------
	 * Bind SYSTEM to the DC
	 *------------------------------------*/
	sys_pid = getpid();
	sys_nr = SYSTASK(local_nodeid);
   	TASKDEBUG( "Binding to DC %d with sys_nr=%d sys_pid=%d\n", dcid, sys_nr, sys_pid);
   	sys_ep = mnx_bind(dcid, sys_nr );
	if( sys_ep < EMOLERRCODE ) ERROR_RETURN(sys_ep);
    TASKDEBUG( "has binded with p_endpoint=%d \n", sys_ep);

			
  /* Initialize all alarm timers for all processes. */
//  for (sp=BEG_PRIV_ADDR; sp < END_PRIV_ADDR; sp++) {
//    tmr_inittimer(&(sp->s_alarm_timer));
//  }

  /* Initialize the call vector to a safe default handler. Some system calls 
   * may be disabled or nonexistant. Then explicitely map known calls to their
   * handler functions. This is done with a macro that gives a compile error
   * if an illegal call number is used. The ordering is not important here.
   */
  	TASKDEBUG( "Initialize the call vector to a safe default handler.\n");
  	for (i=0; i<NR_SYS_CALLS; i++) {
//		TASKDEBUG("Initilizing vector %d on address=%p\n",i, do_unused);
      		call_vec[i] = do_unused;
  	}

  /* Process management. */
    map(SYS_FORK, do_fork); 		/* a process forked a new process */
//  map(SYS_EXEC, do_exec);		/* update process after execute */
    map(SYS_EXIT, do_exit);		/* clean up after process exit */
    map(SYS_NICE, do_nice);		/* set scheduling priority */
    map(SYS_PRIVCTL, do_privctl);	/* system privileges control */
//  map(SYS_TRACE, do_trace);		/* request a trace operation */

  /* Signal handling. */
//  map(SYS_KILL, do_kill); 		/* cause a process to be signaled */
//  map(SYS_GETKSIG, do_getksig);	/* PM checks for pending signals */
//  map(SYS_ENDKSIG, do_endksig);	/* PM finished processing signal */
//  map(SYS_SIGSEND, do_sigsend);	/* start POSIX-style signal */
//  map(SYS_SIGRETURN, do_sigreturn);	/* return from POSIX-style signal */

  /* Device I/O. */
//  map(SYS_IRQCTL, do_irqctl);  	/* interrupt control operations */ 
//  map(SYS_DEVIO, do_devio);   	/* inb, inw, inl, outb, outw, outl */ 
//  map(SYS_SDEVIO, do_sdevio);		/* phys_insb, _insw, _outsb, _outsw */
//  map(SYS_VDEVIO, do_vdevio);  	/* vector with devio requests */ 
//  map(SYS_INT86, do_int86);  		/* real-mode BIOS calls */ 

  /* Memory management. */
//  map(SYS_NEWMAP, do_newmap);		/* set up a process memory map */
//  map(SYS_SEGCTL, do_segctl);		/* add segment and get selector */
    map(SYS_MEMSET, do_memset);		/* write char to memory area */
//  map(SYS_dc_SETBUF, do_dc_setbuf); 	/* PM passes buffer for page tables */
//  map(SYS_dc_MAP, do_dc_map); 	/* Map/unmap physical (device) memory */

  /* Copying. */
//  map(SYS_UMAP, do_umap);		/* map virtual to physical address */
    map(SYS_VIRCOPY, do_vircopy); 	/* use pure virtual addressing */
//  map(SYS_PHYSCOPY, do_physcopy); 	/* use physical addressing */
   map(SYS_VIRVCOPY, do_virvcopy);	/* vector with copy requests */
//  map(SYS_PHYSVCOPY, do_physvcopy);	/* vector with copy requests */

  /* Clock functionality. */
    map(SYS_TIMES, do_times);		/* get uptime and process times */
    map(SYS_SETALARM, do_setalarm);	/* schedule a synchronous alarm */

  /* System control. */
//  map(SYS_ABORT, do_abort);		/* abort MINIX */
    map(SYS_GETINFO, do_getinfo); 	/* request system information */ 
//  map(SYS_IOPENABLE, do_iopenable); 	/* Enable I/O */

   	map(SYS_KILLED, do_killed);
	map(SYS_BINDPROC, do_bindproc);	
	map(SYS_REXEC, do_rexec);	
	map(SYS_MIGRPROC, do_migrproc);	
	map(SYS_SETPNAME, do_setpname);	
	map(SYS_UNBIND, do_unbind);	

	dc_ptr = &dcu;

	/*------------------------------------
	* Alloc memory for process table 
 	*------------------------------------*/
#ifdef ALLOC_LOCAL_TABLE	
//	proc = (proc_usr_t *) malloc(sizeof(proc_usr_t) * (dcu.dc_nr_procs+dcu.dc_nr_tasks));
	posix_memalign( (void**) &proc, getpagesize(), sizeof(proc_usr_t) * (dcu.dc_nr_procs+dcu.dc_nr_tasks));
	if( proc == NULL) return (EMOLNOMEM);
  	TASKDEBUG( "Allocated space for nr_procs=%d + nr_tasks=%d processes\n",
		dcu.dc_nr_procs,dcu.dc_nr_tasks); 
#else /* ALLOC_LOCAL_TABLE */	
	/*------------------------------------
	* Map kernel process table 
 	*------------------------------------*/
	sprintf( debug_path, "/sys/kernel/debug/dvs/%s/procs",dc_ptr->dc_name);
  	TASKDEBUG( "Map kernel process table on %s\n",debug_path);
	debug_fd = open(debug_path, O_RDWR);
	if(debug_fd < 0) {
		fprintf(stderr, "%s\n", *strerror(errno));
		ERROR_EXIT(errno);
	}
  	TASKDEBUG( "open debug_fd=%d\n",debug_fd);

	kproc = mmap(NULL, ((dc_ptr->dc_nr_tasks + dc_ptr->dc_nr_procs) * dvs_ptr->d_size_proc) 
					, (PROT_READ | PROT_WRITE) , MAP_SHARED, debug_fd, 0);
	if (kproc == MAP_FAILED) {
		TASKDEBUG("mmap %d\n",EMOLFAULT);
		fprintf(stderr, "mmap %d\n",EMOLFAULT);
		ERROR_EXIT(EMOLFAULT);
	}	
	TASKDEBUG( "mmap debug_fd OK\n");
		
	for (i = -dc_ptr->dc_nr_tasks; i < (dc_ptr->dc_nr_procs); i++) {
		proc_ptr = (proc_usr_t *) PROC_MAPPED(i);
		TASKDEBUG("proc_ptr=%x " PROC_USR_FORMAT, proc_ptr, PROC_USR_FIELDS(proc_ptr));
	}
#endif /* ALLOC_LOCAL_TABLE */	

	/*------------------------------------
	* Alloc memory for slot descriptor table 
 	*------------------------------------*/
	posix_memalign( (void**) &slot, getpagesize(),  sizeof(slot_t)  * (dcu.dc_nr_procs+dcu.dc_nr_tasks));
	if(slot == NULL) return (EMOLNOMEM);
  	TASKDEBUG( "Allocated space for nr_procs=%d + nr_tasks=%d slots descriptors \n",
		dcu.dc_nr_procs,dcu.dc_nr_tasks); 

	/*------------------------------------
	* Alloc memory for temporal slot descriptor table used on merging
 	*------------------------------------*/
	posix_memalign( (void**) &slot_merge, getpagesize(),  sizeof(slot_t)  * (dcu.dc_nr_procs+dcu.dc_nr_tasks));
	if(slot_merge == NULL) return (EMOLNOMEM);
  	TASKDEBUG( "Allocated space for nr_procs=%d + nr_tasks=%d temporal slot descriptors \n",
		dcu.dc_nr_procs,dcu.dc_nr_tasks); 
		
	/*------------------------------------
	* Alloc memory for global status  
 	*------------------------------------*/
	sts_size = sizeof(message) + sizeof(dc_usr_t) +
				( sizeof(slot_t)  * (dcu.dc_nr_procs+dcu.dc_nr_tasks)); 
//	sts_ptr  = malloc(sts_size);
	posix_memalign( (void**) &sts_ptr, getpagesize(), sts_size);
	if(sts_ptr == NULL) return (EMOLNOMEM);
  	TASKDEBUG( "Allocated space for global status sts_size=%d\n",sts_size);
	msg_ptr = sts_ptr;
	memcpy( (void *) sts_ptr + sizeof(message) , &dcu, sizeof(dc_usr_t));
	v_ptr = (void *) sts_ptr + sizeof(message);
	TASKDEBUG( dc_USR_FORMAT, dc_USR_FIELDS(dc_ptr));
	TASKDEBUG( dc_USR_FORMAT, dc_USR_FIELDS(v_ptr));
	rst_ptr  = (char *) msg_ptr + sizeof(message) + sizeof(dc_usr_t);
	
	/*------------------------------------
	* Calculate slots limits 
 	*------------------------------------*/
	bm_active = 0;			/* initialize  active members'  bitmap */
	bm_donors = 0;			/* initialize  donors members'  bitmap */
	bm_init   = 0;			/* initialize  initialized  members'  bitmap */
	bm_waitsts	= 0;		/* bitmap of members waiting SYS_PUT_STATUS */
	bm_pending	= 0;		/* bitmap pending donating ZERO replies */
	
	/* total DC  slots for user space processes  */
	total_slots = (dc_ptr->dc_nr_tasks + dc_ptr->dc_nr_procs) - dc_ptr->dc_nr_sysprocs;

	/* the minimun number of non system slots that a node can have (FIXED) */
	/* min_owned_slots	= (total_slots/dc_ptr->dc_nr_nodes)+1;  si hacemos min_owned_slots * nr_nodes  > total_slots */
	min_owned_slots	= (total_slots/dc_ptr->dc_nr_nodes); /* heuristics  */

	/* Initialize the low treadhold */
	free_slots_low  = min_owned_slots/2; /* heuristics  */
	
	TASKDEBUG("total_slots=%d min_owned_slots=%d free_slots_low=%d\n"
		,total_slots, min_owned_slots, free_slots_low );

	/*------------------------------------
	 * Initialize the SLOTS thread
	 * group name = dc_ptr->dc_name 
	 * user name = node_ptr->n_nodeid
	 *------------------------------------*/
	if( dc_ptr->dc_nr_nodes > 1) {
		/*------------------------------------
		* Initialize process and slot tables 
		*------------------------------------*/
		TASKDEBUG( "Initialize process and slot table table \n");
		for (i = 0; i < (dc_ptr->dc_nr_tasks + dc_ptr->dc_nr_procs); i++) {
#ifdef ALLOC_LOCAL_TABLE 	
			proc_ptr = &proc[i];
			rcode  = mnx_getprocinfo(dcu.dc_dcid, (i-dc_ptr->dc_nr_tasks), proc_ptr);
#else  /* ALLOC_LOCAL_TABLE */	
			proc_ptr = (proc_usr_t *) PROC_MAPPED(i-dc_ptr->dc_nr_tasks);
			rcode = OK;
#endif /* ALLOC_LOCAL_TABLE */	
			
			TASKDEBUG(PROC_USR_FORMAT, PROC_USR_FIELDS(proc_ptr)); 
			slot[i].s_nr	   = i;
			slot[i].s_flags    = 0;
			if( proc_ptr->p_rts_flags != SLOT_FREE) { 
				slot[i].s_owner    = proc_ptr->p_nodeid;
				slot[i].s_endpoint = proc_ptr->p_endpoint;
			}else{
				slot[i].s_owner    = NO_PRIMARY_MBR;
				slot[i].s_endpoint = NONE;
			}
		}
		
		TASKDEBUG("Initializing SLOTS\n");
		rcode = init_slots();	
		if( rcode)ERROR_EXIT(rcode);
		TASKDEBUG("Starting SLOTS thread\n");
		rcode = pthread_create( &slots_thread, NULL, slots_read_thread, 0 );
		if( rcode)ERROR_EXIT(rcode);

		pthread_mutex_lock(&sys_mutex);
		pthread_cond_wait(&sys_barrier,&sys_mutex); /* unlock, wait, and lock again sys_mutex */	
		TASKDEBUG("SYSTASK has been signaled by the SLOTS thread  FSM_state=%X\n",  FSM_state);
		if( FSM_state == STS_LEAVE) {	/* An error occurs trying to join the spread group */
			pthread_mutex_unlock(&sys_mutex);
			ERROR_RETURN(EMOLCONNREFUSED);
		}
		retries = SLOTS_RETRIES;
		while( (FSM_state == STS_REQ_SLOTS) && (owned_slots == 0) && (retries>0)) {
			pthread_mutex_unlock(&sys_mutex);
			sleep(SLOTS_PAUSE);
			pthread_mutex_lock(&sys_mutex);
			rcode = mcast_rqst_slots(free_slots_low);
			last_rqst = time(NULL);
			pthread_cond_wait(&sys_barrier,&sys_mutex); /* unlock, wait, and lock again sys_mutex */
			retries--;		
		}
		if(retries <= 0) {
			pthread_mutex_unlock(&sys_mutex);
			ERROR_RETURN(EMOLNOSPC);
		}
			
	}else{
		/*------------------------------------
		* Initialize process and slot tables 
		*------------------------------------*/
		TASKDEBUG( "Initialize process table table \n");
		for (i = 0; i < (dc_ptr->dc_nr_tasks + dc_ptr->dc_nr_procs); i++) {
#ifdef ALLOC_LOCAL_TABLE 	
			proc_ptr = &proc[i];
			rcode  = mnx_getprocinfo(dcu.dc_dcid, (i-dc_ptr->dc_nr_tasks), proc_ptr);
#else  /* ALLOC_LOCAL_TABLE */	
			proc_ptr = (proc_usr_t *) PROC_MAPPED(i-dc_ptr->dc_nr_tasks);
			rcode = OK;
#endif /* ALLOC_LOCAL_TABLE */	
			TASKDEBUG(PROC_USR_FORMAT, PROC_USR_FIELDS(proc_ptr));	
			slot[i].s_nr	   = i;
			slot[i].s_flags    = 0;
			slot[i].s_owner    = local_nodeid;
		}
		TASKDEBUG("Local node is the only DC node to start\n");
		active_nodes = 1;
		init_nodes = 1;
		owned_slots = total_slots;
		bm_active = bm_init = bm_donors = 0;
		SET_BIT(bm_active, local_nodeid);
		SET_BIT(bm_init, local_nodeid);
		pthread_mutex_lock(&sys_mutex);
		FSM_state =  STS_RUNNING;			
	}
	
	/* sys_mutex is locked here */	
	free_slots  = owned_slots;
	next_child = (dc_ptr->dc_nr_sysprocs);
	/* the maximum number of non system slots that a node can have */
	max_owned_slots	= (total_slots - (min_owned_slots*(init_nodes-1)));
	TASKDEBUG("next_child=%d init_nodes=%d max_owned_slots=%d owned_slots=%d free_slots=%d\n"
		,next_child, init_nodes, max_owned_slots, owned_slots, free_slots);

	if( dc_ptr->dc_nr_nodes > 1) {
		/*	bind remote SYSTASKs */
		for(i=0; i < dvs_ptr->d_nr_nodes; i++){			/* all slot owners are initialized */
			if( i == local_nodeid) continue;
			if( TEST_BIT(bm_init, i)){
				rcode = bind_rmt_systask(i);
				if( rcode < EMOLERRCODE) 	ERROR_RETURN(rcode);
			}
		}		
	}
	pthread_mutex_unlock(&sys_mutex);
	
	/*------------------------------------
	* Inform other nodes about local processes
 	*------------------------------------*/
	rcode = mcast_binds2rmt();
	if(rcode) ERROR_RETURN(rcode);

	/*------------------------------------
	* Alloc memory for priv table 
 	*------------------------------------*/
//	priv = (priv_usr_t *) malloc(sizeof(priv_usr_t) * (dcu.dc_nr_procs+dcu.dc_nr_tasks));
	posix_memalign( (void**) &priv, getpagesize(), sizeof(priv_usr_t) * (dcu.dc_nr_procs+dcu.dc_nr_tasks));
	if( priv == NULL) return (EMOLNOMEM);
  	TASKDEBUG( "Allocated space for nr_procs=%d + nr_tasks=%d ptrivileges\n",
		dcu.dc_nr_procs,dcu.dc_nr_tasks); 

	/*------------------------------------
	* Initialize priv table 
 	*------------------------------------*/
	TASKDEBUG( "Initialize priv table\n");
	for (i = 0; i < (dc_ptr->dc_nr_tasks + dc_ptr->dc_nr_procs); i++) {
		priv_ptr = &priv[i];
		rcode  = mnx_getpriv(dcu.dc_dcid, (i-dc_ptr->dc_nr_tasks), priv_ptr);
		if( rcode < 0 && rcode != EMOLDSTDIED ) ERROR_RETURN(rcode);		
//TASKDEBUG(PRIV_USR_FORMAT, PRIV_USR_FIELDS(priv_ptr)); 
	}

	/*------------------------------------
	* Set SYSTASK/CLOCK privileges 
 	*------------------------------------*/
	priv_ptr = &priv[SYSTEM+dc_ptr->dc_nr_tasks];
	priv_ptr->s_level = SYSTEM_PRIV;
	priv_ptr->s_warn = NONE;
	rcode  = mnx_setpriv(dcu.dc_dcid, SYSTEM, priv_ptr);
	if( rcode < 0 && rcode != EMOLDSTDIED ) ERROR_RETURN(rcode);		

//	priv_ptr = &priv[CLOCK+dc_ptr->dc_nr_tasks];
//	priv_ptr->s_level = KERNEL_PRIV;
//	priv_ptr->s_warn = NONE;
//	rcode  = mnx_setpriv(dcu.dc_dcid, CLOCK, priv_ptr);
//	if( rcode < 0 && rcode != EMOLDSTDIED ) ERROR_RETURN(rcode);		

	rcode = setpriority(PRIO_PROCESS, getpid(), PRIO_SYSTASK); 
	if( rcode < 0) ERROR_RETURN(rcode);		

	return(OK);
}

/*
* BIND REMOTE SYSTASK 
*/
int bind_rmt_systask(int node)
{	
	int i, rcode;
	proc_usr_t *proc_ptr;
	slot_t  *sp; 		

	/* BIND REMOTE SYSTASK */
	TASKDEBUG("node=%d SYSTASK(%d)=%d dc_ptr->dc_nr_tasks=%d bm_init=%x\n", 
			node, node, SYSTASK(node), dc_ptr->dc_nr_tasks, bm_init);	
	rcode = mnx_rmtbind(dc_ptr->dc_dcid,
				"systask",
				SYSTASK(node),
				node);			
	TASKDEBUG("Binding Remote SYSTASK p_nr=%d nodeid=%d rcode=%d\n",
				SYSTASK(node),
				node,rcode);
				
#ifdef ALLOC_LOCAL_TABLE 			
	proc_ptr = &proc[SYSTASK(node)+dc_ptr->dc_nr_tasks];
	rcode = mnx_getprocinfo(dc_ptr->dc_dcid, SYSTASK(node), proc_ptr);
	if( rcode < EMOLERRCODE ) 	ERROR_RETURN(rcode);
#else /* ALLOC_LOCAL_TABLE */			
	proc_ptr = (proc_usr_t *) PROC_MAPPED(SYSTASK(node));
	rcode = OK;
#endif /* ALLOC_LOCAL_TABLE */

	TASKDEBUG(PROC_USR_FORMAT,PROC_USR_FIELDS(proc_ptr));
	assert(proc_ptr->p_nodeid == node);
	sp = &slot[SYSTASK(node)+dc_ptr->dc_nr_tasks];
	sp->s_endpoint = SYSTASK(node);
	sp->s_flags = 0;
	sp->s_owner = node;
	TASKDEBUG(SLOTS_FORMAT, SLOTS_FIELDS(sp));
	return(OK);
}


/*
* UNBIND REMOTE SYSTASK 
*/
int unbind_rmt_sysproc(int endpoint)
{	
	int i, rcode;
	proc_usr_t *proc_ptr;
	slot_t  *sp; 		

#ifdef ALLOC_LOCAL_TABLE 			
	proc_ptr = &proc[_ENDPOINT_P(endpoint)+dc_ptr->dc_nr_tasks];
	rcode = mnx_getprocinfo(dc_ptr->dc_dcid, endpoint, proc_ptr);
	if( rcode < 0) 	ERROR_RETURN(rcode);
#else /* ALLOC_LOCAL_TABLE */			
	proc_ptr = (proc_usr_t *) PROC_MAPPED(_ENDPOINT_P(endpoint));	
	rcode = OK;	
#endif /* ALLOC_LOCAL_TABLE */

	/* UNBIND REMOTE SYSPROCS  */
	TASKDEBUG(PROC_USR_FORMAT,PROC_USR_FIELDS(proc_ptr));
	rcode = mnx_unbind(dc_ptr->dc_dcid,endpoint);
	TASKDEBUG(PROC_USR_FORMAT,PROC_USR_FIELDS(proc_ptr));
	
	sp = &slot[_ENDPOINT_P(endpoint)+dc_ptr->dc_nr_tasks];
	sp->s_endpoint = NONE;
	sp->s_flags = 0;
	sp->s_owner = local_nodeid;
	TASKDEBUG(SLOTS_FORMAT, SLOTS_FIELDS(sp));
	return(OK);
}

