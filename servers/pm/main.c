/* This file contains the main program of the process manager and some related
 * procedures.  When MINIX starts up, the kernel runs for a little while,
 * initializing itself and its tasks, and then it runs PM and FS.  Both PM
 * and FS initialize themselves as far as they can. PM asks the kernel for
 * all free memory and starts serving requests.
 *
 * The entry points into this file are:
 *   main:	starts PM running
 *   setreply:	set the reply to be sent to process making an PM system call
 */

#define _TABLE
#include "pm.h"

void pm_init(int vmid);
void get_work(void);
		
/*===========================================================================*
 *				main					     *
 *===========================================================================*/
int main ( int argc, char *argv[] )
{
	int vmid, rcode, result, proc_nr, ds_lpid;
	mnxsigset_t sigset;
	mproc_t *rmp;	
	proc_usr_t *rkp;	
	struct timespec t; 
	long long tt, td;

	if ( argc != 2) {
 	        printf( "Usage: %s <vmid> \n", argv[0] );
 	        exit(1);
    	}

	vmid = atoi(argv[1]);
	if( vmid < 0 || vmid >= NR_VMS) ERROR_EXIT(EMOLRANGE);

	pm_init(vmid);
	
SVRDEBUG("This is PM's main loop-  get work and do it, forever and forever.\n");
  	while (TRUE) {
	
		get_work();		/* wait for an PM system call */

		/* Check for system notifications first. Special cases. */
		if (call_nr == SYN_ALARM) {
SVRDEBUG("SYN_ALARM\n");
			t = m_in.NOTIFY_TIMESTAMP;
			td = 1000000000/clockTicks;
			tt =  t.tv_nsec / td;
			tt += (t.tv_sec * clockTicks);
			pm_expire_timers((molclock_t) tt);
			result = SUSPEND;		/* don't reply */
		} else if (call_nr == SYS_SIG) {	/* signals pending */
			sigset = m_in.NOTIFY_ARG;
//			if (sigismember(&sigset, SIGKSIG))  {
//				(void) ksig_pending();
//			} 
			result = SUSPEND;		/* don't reply */
		}/* Else, if the system call number is valid, perform the call. */
		else if ((unsigned) call_nr >= NCALLS) {
			result = ENOSYS;
		} else {
			result = (*call_vec[call_nr])();
		}
SVRDEBUG("call_nr=%d result=%d\n", call_nr, result);

		/* Send the results back to the user to indicate completion. */
		if (result != SUSPEND) setreply(who_p, result);

		/* Send out all pending reply messages, including the answer to
	 	* the call just made above.  
	 	*/
		/*!!!!!!!!!!!!!!!!!!!!!!!! ESTO ES UNA BURRADA !!!!!!!!!!!!!!*/
		/* Hay que hacer lista enlazada de procesos */
SVRDEBUG("Send out all pending reply messages\n");
		for (proc_nr=0, rmp=mproc, rkp=(&kproc[0] + vm_ptr->vm_nr_tasks); 
			proc_nr < vm_ptr->vm_nr_procs; 
			proc_nr++, rmp++, rkp++) {
			/* In the meantime, the process may have been killed by a
			 * signal (e.g. if a lethal pending signal was unblocked)
		 	* without the PM realizing it. If the slot is no longer in
		 	* use or just a zombie, don't try to reply.
		 	*/
			if ((rmp->mp_flags & (REPLYPENDING | IN_USE | ZOMBIE)) == (REPLYPENDING | IN_USE)) {
SVRDEBUG("Replying to %d\n",rkp->p_endpoint);		   
				if ((rcode = mnx_send(rkp->p_endpoint, &rmp->mp_reply)) != OK) {
					SVRDEBUG("PM can't reply to %d (%s)\n",rkp->p_endpoint, rkp->p_name);
					switch(rcode){  /* Auto Unbind the failed process */
						case	EMOLSRCDIED:
						case	EMOLDSTDIED:
						case	EMOLNOPROXY:
						case	EMOLNOTCONN:
						case	EMOLDEADSRCDST:
							pm_exit(mp, rcode);
							break;
						default:
							break;
					}
					SYSERR(rcode);
				}
				rmp->mp_flags &= ~REPLYPENDING;
			}
		}	
	}

return(OK);
}

/*===========================================================================*
 *				get_work				     *
 *===========================================================================*/
void get_work(void)
{
	int ret;
	
	/* Wait for the next message and extract useful information from it. */
	while( TRUE) {
		SVRDEBUG("Wait for the next message and extract useful information from it.\n");

		if ( (ret=mnx_receive(ANY, &m_in)) != OK) ERROR_EXIT(ret);
		who_e = m_in.m_source;	/* who sent the message */
		call_nr = m_in.m_type;	/* system call number */
SVRDEBUG("Request received from who_e=%d, call_nr=%d\n", who_e, call_nr);

		if( (who_e == CLOCK) && (call_nr & NOTIFY_MESSAGE) ) return;

		if(( ret = pm_isokendpt(who_e, &who_p)) != OK) {
SVRDEBUG("BAD ENDPOINT endpoint=%d, call_nr=%d\n", who_e, call_nr);
//			m_in.m_type = EMOLENDPOINT;
//			ret = mnx_send(who_e, &m_in);
			continue;
		} 

		/* Process slot of caller. Misuse PM's own process slot if the kernel is
		* calling. This can happen in case of synchronous alarms (CLOCK) or or 
		* event like pending kernel signals (SYSTEM).
		*/
		kp = &kproc[(who_p < 0 ? PM_PROC_NR : who_p)+vm_ptr->vm_nr_tasks];
		mp = &mproc[who_p < 0 ? PM_PROC_NR : who_p];
		if(who_p >= 0 && kp->p_endpoint != who_e) {
			SVRDEBUG("PM endpoint number out of sync with kernel endpoint=%d.\n",kp->p_endpoint);
			continue;
			}
		return;
	}
 
}


/*===========================================================================*
 *				pm_init					     *
 *===========================================================================*/
void pm_init(int vmid)
{
  	int i, rcode, tab_len;
	char *sig_ptr;
	static char mess_sigs[] = { SIGTERM, SIGHUP, SIGABRT, SIGQUIT };

	/*testing*/
	int ds_lpid, ds_ep;
	
	pm_lpid = getpid();
	m_ptr = &m_in;

	/* Bind PM to the kernel */
SVRDEBUG("Binding process %d to VM%d with pm_nr=%d\n",pm_lpid,vmid,PM_PROC_NR);
	pm_ep = mnx_bind(vmid, PM_PROC_NR);
	if(pm_ep < 0) ERROR_EXIT(pm_ep);

	local_nodeid = mnx_getdrvsinfo(&drvs);
	if(local_nodeid < 0) ERROR_EXIT(local_nodeid);
SVRDEBUG("local_nodeid=%d\n",local_nodeid);

	/* Register into SYSTASK (as an autofork) */
SVRDEBUG("Register PM into SYSTASK pm_lpid=%d\n",pm_lpid);
	pm_ep = sys_bindproc(PM_PROC_NR, pm_lpid);
	if(pm_ep < 0) ERROR_EXIT(pm_ep);
	
SVRDEBUG("Get the DRVS info from SYSTASK\n");
    rcode = sys_getkinfo(&drvs);
	if(rcode) ERROR_EXIT(rcode);
	drvs_ptr = &drvs;

	SVRDEBUG(DRVS_USR_FORMAT,DRVS_USR_FIELDS(drvs_ptr));
SVRDEBUG("Get the VM info from SYSTASK\n");
	sys_getmachine(&vmu);
	if(rcode) ERROR_EXIT(rcode);
	vm_ptr = &vmu;
SVRDEBUG(VM_USR_FORMAT,VM_USR_FIELDS(vm_ptr));


	/* alloc dynamic memory for the KERNEL process table */
SVRDEBUG("Alloc dynamic memory for the Kernel process table nr_procs+nr_tasks=%d\n", (vm_ptr->vm_nr_tasks + vm_ptr->vm_nr_procs));
	kproc = malloc((vm_ptr->vm_nr_tasks + vm_ptr->vm_nr_procs)*sizeof(proc_usr_t));
	if(kproc == NULL) ERROR_EXIT(rcode);

	/* alloc dynamic memory for the PM process table */
SVRDEBUG("Alloc dynamic memory for the PM process table nr_procs=%d\n", vm_ptr->vm_nr_procs);
	mproc = malloc((vm_ptr->vm_nr_procs)*sizeof(mproc_t));
	if(mproc == NULL) ERROR_EXIT(rcode)

	/* alloc dynamic memory for the KERNEL priviledge table */
SVRDEBUG("Alloc dynamic memory for the Kernel priviledge table nr_procs+nr_tasks=%d\n", (vm_ptr->vm_nr_tasks + vm_ptr->vm_nr_procs));
	kpriv = malloc((vm_ptr->vm_nr_tasks + vm_ptr->vm_nr_procs)*sizeof(priv_usr_t));
	if(kpriv == NULL) ERROR_EXIT(rcode);

	/* alloc dynamic memory for the SYSTASK Slots allocation table  */
SVRDEBUG("Alloc dynamic memory for the SYSTASK Slots allocation table nr_procs+nr_tasks=%d\n", (vm_ptr->vm_nr_tasks + vm_ptr->vm_nr_procs));
	slots = malloc((vm_ptr->vm_nr_tasks + vm_ptr->vm_nr_procs)*sizeof(slots));
	if(slots == NULL) ERROR_EXIT(rcode);
	
 	/* Initialize PM's process table */
	for (i = 0; i < vm_ptr->vm_nr_procs; i++) 
		mproc_init(i);

 	/* Initialize PM entry in the PM's process table */
SVRDEBUG("Initialize PM entry in the PM's process table\n");
 	procs_in_use = 1;			/* start populating table */
	/* Set process details found in the PM table. */
	mp = &mproc[PM_PROC_NR];	
	mp->mp_nice = PRIO_SERVER;
	sigemptyset(&mp->mp_sig2mess);
  	sigemptyset(&mp->mp_ignore);	
  	sigemptyset(&mp->mp_sigmask);
  	sigemptyset(&mp->mp_catch);
	mp->mp_pid = PM_PID;		/* PM has magic pid */
	mp->mp_flags |= IN_USE | DONT_SWAP | PRIV_PROC; 
	for (sig_ptr = mess_sigs; 
	     sig_ptr < mess_sigs+sizeof(mess_sigs); 
	     sig_ptr++) {
			sigaddset(&mp->mp_sig2mess, *sig_ptr);
	}
	sigfillset(&mproc[PM_PROC_NR].mp_ignore); 	/* guard against signals */
SVRDEBUG(PM_PROC_FORMAT,PM_PROC_FIELDS(mp));

	/* Set the next free slot for USER processes */
	next_child = vm_ptr->vm_nr_sysprocs; 

	/* change PRIVILEGES of PM */
SVRDEBUG("change PRIVILEGES of PM\n");
	rcode = sys_privctl(pm_ep, SERVER_PRIV);
	if(rcode) ERROR_EXIT(rcode);

	/* get kernel PROC TABLE */
	tab_len =  ((vm_ptr->vm_nr_tasks + vm_ptr->vm_nr_procs)*sizeof(proc_usr_t));
	rcode = sys_proctab(kproc, tab_len);

	/* Fetch clock ticks */
	clockTicks = sysconf(_SC_CLK_TCK);
	if (clockTicks == -1)	ERROR_EXIT(errno);
SVRDEBUG("clockTicks =%ld\n",clockTicks );

	if ( (rcode=sys_getuptime(&boottime)) != OK) 
  		ERROR_EXIT(rcode);
	boottime /= clockTicks;
SVRDEBUG("boottime =%ld\n",boottime );

	if(rcode) ERROR_EXIT(rcode);
	
}

/*===========================================================================*
 *				setreply				     *
/* proc_nr: process to reply to 					     *
/* result: result of call (usually OK or error #) 			     *	
 *===========================================================================*/
void setreply(int proc_nr, int result)
{
/* Fill in a reply message to be sent later to a user process.  System calls
 * may occasionally fill in other fields, this is only for the main return
 * value, and for setting the "must send reply" flag.
 */
  struct mproc *rmp = &mproc[proc_nr];

SVRDEBUG("proc_nr=%d result=%d\n",proc_nr, result);

  if(proc_nr < 0 || proc_nr >= vm_ptr->vm_nr_procs)
	ERROR_EXIT(EMOLBADPROC);

  rmp->mp_reply.reply_res = result;
  rmp->mp_flags |= REPLYPENDING;	/* reply pending */
}




#ifdef TEMPORAL

  message mess;

  int s;
  static struct boot_image image[NR_BOOT_PROCS];
  register struct boot_image *ip;
  static char core_sigs[] = { SIGQUIT, SIGILL, SIGTRAP, SIGABRT,
			SIGEMT, SIGFPE, SIGUSR1, SIGSEGV, SIGUSR2 };
  static char ign_sigs[] = { SIGCHLD, SIGWINCH, SIGCONT };
  register struct mproc *rmp;


  /* Initialize process table, including timers. */
  for (rmp=&mproc[0]; rmp<&mproc[NR_PROCS]; rmp++) {
	tmr_inittimer(&rmp->mp_timer);
  }

  /* Build the set of signals which cause core dumps, and the set of signals
   * that are by default ignored.
   */
  sigemptyset(&core_sset);
  for (sig_ptr = core_sigs; sig_ptr < core_sigs+sizeof(core_sigs); sig_ptr++)
	sigaddset(&core_sset, *sig_ptr);
  sigemptyset(&ign_sset);
  for (sig_ptr = ign_sigs; sig_ptr < ign_sigs+sizeof(ign_sigs); sig_ptr++)
	sigaddset(&ign_sset, *sig_ptr);

  /* Obtain a copy of the boot monitor parameters and the kernel info struct.  
   * Parse the list of free memory chunks. This list is what the boot monitor 
   * reported, but it must be corrected for the kernel and system processes.
   */
  if ((s=sys_getmonparams(monitor_params, sizeof(monitor_params))) != OK)
      panic(__FILE__,"get monitor params failed",s);
  get_mem_chunks(mem_chunks);
  if ((s=sys_getkinfo(&kinfo)) != OK)
      panic(__FILE__,"get kernel info failed",s);

  /* Get the memory map of the kernel to see how much memory it uses. */
  if ((s=get_mem_map(SYSTASK, mem_map)) != OK)
  	panic(__FILE__,"couldn't get memory map of SYSTASK",s);
  minix_clicks = (mem_map[S].mem_phys+mem_map[S].mem_len)-mem_map[T].mem_phys;
  patch_mem_chunks(mem_chunks, mem_map);

  /* Initialize PM's process table. Request a copy of the system image table 
   * that is defined at the kernel level to see which slots to fill in.
   */
  if (OK != (s=sys_getimage(image))) 
  	panic(__FILE__,"couldn't get image table: %d\n", s);
  procs_in_use = 0;				/* start populating table */
  printf("Building process table:");		/* show what's happening */
  for (ip = &image[0]; ip < &image[NR_BOOT_PROCS]; ip++) {		
  	if (ip->proc_nr >= 0) {			/* task have negative nrs */
  		procs_in_use += 1;		/* found user process */

		/* Set process details found in the image table. */
		rmp = &mproc[ip->proc_nr];	
  		strncpy(rmp->mp_name, ip->proc_name, PROC_NAME_LEN); 
		rmp->mp_parent = RS_PROC_NR;
		rmp->mp_nice = get_nice_value(ip->priority);
  		sigemptyset(&rmp->mp_sig2mess);
  		sigemptyset(&rmp->mp_ignore);	
  		sigemptyset(&rmp->mp_sigmask);
  		sigemptyset(&rmp->mp_catch);
		if (ip->proc_nr == INIT_PROC_NR) {	/* user process */
  			rmp->mp_procgrp = rmp->mp_pid = INIT_PID;
			rmp->mp_flags |= IN_USE; 
		}
		else {					/* system process */
  			rmp->mp_pid = get_free_pid();
			rmp->mp_flags |= IN_USE | DONT_SWAP | PRIV_PROC; 
  			for (sig_ptr = mess_sigs; 
				sig_ptr < mess_sigs+sizeof(mess_sigs); 
				sig_ptr++)
			sigaddset(&rmp->mp_sig2mess, *sig_ptr);
		}

		/* Get kernel endpoint identifier. */
		rmp->mp_endpoint = ip->endpoint;

  		/* Get memory map for this process from the kernel. */
		if ((s=get_mem_map(ip->proc_nr, rmp->mp_seg)) != OK)
  			panic(__FILE__,"couldn't get process entry",s);
		if (rmp->mp_seg[T].mem_len != 0) rmp->mp_flags |= SEPARATE;
		minix_clicks += rmp->mp_seg[S].mem_phys + 
			rmp->mp_seg[S].mem_len - rmp->mp_seg[T].mem_phys;
  		patch_mem_chunks(mem_chunks, rmp->mp_seg);

		/* Tell FS about this system process. */
		mess.PR_SLOT = ip->proc_nr;
		mess.PR_PID = rmp->mp_pid;
		mess.PR_ENDPT = rmp->mp_endpoint;
  		if (OK != (s=send(FS_PROC_NR, &mess)))
			panic(__FILE__,"can't sync up with FS", s);
  		printf(" %s", ip->proc_name);	/* display process name */
  	}
  }
  printf(".\n");				/* last process done */

  /* Override some details. INIT, PM, FS and RS are somewhat special. */
  mproc[PM_PROC_NR].mp_pid = pm_lpid;		/* PM has magic pid */
  mproc[RS_PROC_NR].mp_parent = INIT_PROC_NR;	/* INIT is root */
  sigfillset(&mproc[PM_PROC_NR].mp_ignore); 	/* guard against signals */

  /* Tell FS that no more system processes follow and synchronize. */
  mess.PR_ENDPT = NONE;
  if (sendrec(FS_PROC_NR, &mess) != OK || mess.m_type != OK)
	panic(__FILE__,"can't sync up with FS", NO_NUM);

#if ENABLE_BOOTDEV
  /* Possibly we must correct the memory chunks for the boot device. */
  if (kinfo.bootdev_size > 0) {
      mem_map[T].mem_phys = kinfo.bootdev_base >> CLICK_SHIFT;
      mem_map[T].mem_len = 0;
      mem_map[D].mem_len = (kinfo.bootdev_size+CLICK_SIZE-1) >> CLICK_SHIFT;
      patch_mem_chunks(mem_chunks, mem_map);
  }
#endif /* ENABLE_BOOTDEV */

  /* Withhold some memory from x86 VM */
  do_x86_vm(mem_chunks);

  /* Initialize tables to all physical memory and print memory information. */
  printf("Physical memory:");
  mem_init(mem_chunks, &free_clicks);
  total_clicks = minix_clicks + free_clicks;
  printf(" total %u KB,", click_to_round_k(total_clicks));
  printf(" system %u KB,", click_to_round_k(minix_clicks));
  printf(" free %u KB.\n", click_to_round_k(free_clicks));
}


/*===========================================================================*
 *				TEMPO				     *
 *===========================================================================*/
void templ()
{
/* Main routine of the process manager. */
  int result, s, proc_nr;
  struct mproc *rmp;
  sigset_t sigset;
  
  	/* This is PM's main loop-  get work and do it, forever and forever. */
  	while (TRUE) {
		get_work();		/* wait for an PM system call */

		/* Check for system notifications first. Special cases. */
		if (call_nr == SYN_ALARM) {
			pm_expire_timers(m_in.NOTIFY_TIMESTAMP);
			result = SUSPEND;		/* don't reply */
		} else if (call_nr == SYS_SIG) {	/* signals pending */
			sigset = m_in.NOTIFY_ARG;
			if (sigismember(&sigset, SIGKSIG))  {
				(void) ksig_pending();
			} 
			result = SUSPEND;		/* don't reply */
		}/* Else, if the system call number is valid, perform the call. */
		else if ((unsigned) call_nr >= NCALLS) {
			result = ENOSYS;
		} else {
			result = (*call_vec[call_nr])();
		}

		/* Send the results back to the user to indicate completion. */
		if (result != SUSPEND) setreply(who_p, result);


		/* Send out all pending reply messages, including the answer to
	 	* the call just made above.  
	 	*/
		for (proc_nr=0, rmp=mproc; proc_nr < NR_PROCS; proc_nr++, rmp++) {
			/* In the meantime, the process may have been killed by a
			 * signal (e.g. if a lethal pending signal was unblocked)
		 	* without the PM realizing it. If the slot is no longer in
		 	* use or just a zombie, don't try to reply.
		 	*/
			if ((rmp->mp_flags & (REPLYPENDING | ONSWAP | IN_USE | ZOMBIE)) ==
			   (REPLYPENDING | IN_USE)) {
				if ((s=send(rmp->mp_endpoint, &rmp->mp_reply)) != OK) {
					printf("PM can't reply to %d (%s)\n",
						rmp->mp_endpoint, rmp->mp_name);
					panic(__FILE__, "PM can't reply", NO_NUM);
				}
				rmp->mp_flags &= ~REPLYPENDING;
			}
		}	
	}
}
#endif /* TEMPORAL */
