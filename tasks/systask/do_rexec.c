/* The kernel call implemented in this file:
 *   m_type:	SYS_REXEC
 */
#include "systask.h"

#if USE_REXEC
#define FORK_WAIT_MS 1000
/*===========================================================================*
 *				do_rexec					     *
* 	m_ptr->M7_NODEID = rmt_nodeid;				*
*	m_ptr->M7_ENDPT1 = rmt_ep;					*
*	m_ptr->M7_BIND_TYPE = bind_type;				*
*	m_ptr->M7_LEN    = arg_len;					*
*	m_ptr->M7_ARGV_PTR = (char *) argv_ptr;			*
*	m_ptr->M7_THROWER = (char *) thrower_ep;			*
 *===========================================================================*/
int do_rexec(message *m_ptr)	
{
	int rcode, fork_lpid, fork_ep, arg_c, fork_nr, arg_len;
	int rmt_nodeid, bind_type, thrower_ep;
	proc_usr_t *pm_ptr, *d_ptr, *f_ptr;
	static proc_usr_t pm_desc, d_desc, f_desc;		
	char *argv_ptr, *argv_buf, *ptr;
	char *arg_v[MNX_MAX_ARGS+1];
	message *fork_ptr;
 	slot_t   *sp; 		/* PST from the merged partition  */
	static	message fork_msg __attribute__((aligned(0x1000)));
	
	TASKDEBUG(MSG7_FORMAT, MSG7_FIELDS(m_ptr));	

	rmt_nodeid 	= m_ptr->M7_NODEID;
	fork_ep		= m_ptr->M7_ENDPT1; 
	bind_type   = m_ptr->M7_BIND_TYPE;
	arg_len   	= m_ptr->M7_LEN;
	thrower_ep  = m_ptr->M7_THROWER;
	
	if( arg_len > MAXCOPYBUF)
		ERROR_RETURN(EMOL2BIG);
	
	/* Get info about the caller from KERNEL */	
	if( who_p != PM_PROC_NR) ERROR_RETURN(EMOLPERM);
	
#ifdef ALLOC_LOCAL_TABLE 			
	pm_ptr = &pm_desc;
	rcode = mnx_getprocinfo(dc_ptr->dc_dcid, PM_PROC_NR, (long int) pm_ptr);
	if( rcode != OK) ERROR_RETURN(rcode);
#else /* ALLOC_LOCAL_TABLE */			
	pm_ptr = (proc_usr_t *) PROC_MAPPED(PM_PROC_NR);	
	rcode = OK;	
#endif /* ALLOC_LOCAL_TABLE */
	TASKDEBUG(PROC_USR_FORMAT,PROC_USR_FIELDS(pm_ptr));
	
#ifdef ALLOC_LOCAL_TABLE 			
	d_ptr = &d_desc;
	rcode = mnx_getprocinfo(dc_ptr->dc_dcid, thrower_ep, (long int) d_ptr);
	if( rcode != OK) ERROR_RETURN(rcode);
#else /* ALLOC_LOCAL_TABLE */			
	d_ptr = (proc_usr_t *) PROC_MAPPED(_ENDPOINT_P(thrower_ep));	
	rcode = OK;
#endif /* ALLOC_LOCAL_TABLE */
	TASKDEBUG(PROC_USR_FORMAT,PROC_USR_FIELDS(d_ptr));

	/* if caller (PM) is remote, execute a local process */
	if(pm_ptr->p_nodeid != local_nodeid) {
		
		posix_memalign( (void**) &argv_buf, getpagesize(), MAXCOPYBUF+1);

		/* Get argv from requester  */
		rcode = mnx_vcopy(thrower_ep, m_ptr->M7_ARGV_PTR, 
						SYSTASK(local_nodeid), argv_buf, arg_len);
		TASKDEBUG("mnx_vcopy rcode=%d\n", rcode);
		if( rcode < 0) 
			ERROR_RETURN(rcode);
		
		TASKDEBUG("arg_len=%d argv_buf >%s<\n", arg_len, argv_buf);
		ptr = argv_buf;
		arg_c = 0;
		do {
			arg_v[arg_c] = strtok(ptr, " ");
			TASKDEBUG("arg_v[%d]=%s\n", arg_c, arg_v[arg_c]);
			if( arg_v[arg_c] == NULL) break;
			arg_c++;
			ptr = NULL;
		}while(TRUE);
		TASKDEBUG("arg_c=%d\n",arg_c);

		if(arg_c >= MNX_MAX_ARGS) ERROR_RETURN(EMOL2BIG);

		fork_ptr = &fork_msg;

		/* Create child process to execute the program */
		if ((fork_lpid = fork()) == 0) {     
			/*----------------------------------------------------*/
			/*			CHILD				*/
			/*----------------------------------------------------*/
			TASKDEBUG("CHILD\n");
			
			/* Wait for local SYSTASK to bind CHILD */
			do { 
				rcode = mnx_wait4bind_T(FORK_WAIT_MS);
				TASKDEBUG("CHILD: mnx_wait4bind_T rcode=%d\n", rcode);
				if (rcode == EMOLTIMEDOUT) {
					TASKDEBUG("CHILD: mnx_wait4bind_T TIMEOUT\n");
					continue ;
				}else if( rcode < 0) 
					ERROR_EXIT(EXIT_FAILURE);
			} while	(rcode < OK); 
			
			fork_lpid = getpid();
			fork_ep = mnx_getep(fork_lpid);

			/* Wait a NOTIFY message from PM  */
			do { 		
				rcode = mnx_receive_T(PM_PROC_NR, fork_ptr, FORK_WAIT_MS);	
				TASKDEBUG("CHILD: mnx_receive_T from PM rcode=%d m_type=%d\n", rcode, fork_ptr->m_type);
				if (rcode == EMOLTIMEDOUT) {
					TASKDEBUG("CHILD: mnx_receive_T TIMEOUT\n");
					continue ;
				}else if( rcode < 0) {
					ERROR_EXIT(EXIT_FAILURE);
				}
				TASKDEBUG(MSG9_FORMAT, MSG9_FIELDS(fork_ptr));	
				if(fork_ptr->m_type != NOTIFY_FROM(PM_PROC_NR) ) {
					ERROR_EXIT(EXIT_FAILURE);					
				}
			} while	(rcode == EMOLTIMEDOUT);

			setsid();

			/* Finally, execute the program */
			TASKDEBUG("pid=%d argv_ptr[0]=%s\n", fork_ptr->M7_PID, arg_v[0]);
//			rcode = execvp(argv_ptr, &argv_ptr);
			rcode = execvpe(arg_v[0], arg_v, NULL);

			/* this code only executes if execvp() failure */
			TASKDEBUG("execvpe rcode=%d errno=%d\n", rcode, errno);
			fork_ptr->PR_ENDPT = fork_ep;
			rcode = do_exit(fork_ptr);
			ERROR_EXIT(EXIT_FAILURE);					
		}  
		
		/*----------------------------------------------------*/
		/*			PARENT				*/
		/*----------------------------------------------------*/
		if( fork_lpid < 0) {
			free(argv_buf); 
			ERROR_RETURN(fork_lpid);				
		}
		if( fork_ep != ANY ) {
			/* BIND the program to kernel and SYSTASK */
			TASKDEBUG("BIND the program to kernel and SYSTASK: fork_ep=%d\n", fork_ep);

			/* GET process information from kernel */
#ifdef ALLOC_LOCAL_TABLE 	
			f_ptr = &f_desc;
			rcode = mnx_getprocinfo(dc_ptr->dc_dcid, fork_ep, f_ptr);
			if( rcode < 0) ERROR_RETURN(rcode);
#else /* ALLOC_LOCAL_TABLE */			
			f_ptr = (proc_usr_t *) PROC_MAPPED(_ENDPOINT_P(fork_ep));		
			rcode = OK;
#endif /* ALLOC_LOCAL_TABLE */			
			TASKDEBUG(PROC_USR_FORMAT,PROC_USR_FIELDS(f_ptr));
	
			if( rmt_nodeid != local_nodeid) {
				free(argv_buf); 
				ERROR_RETURN(EMOLBADNODEID);				
			}
			
			fork_nr = _ENDPOINT_P(fork_ep);
			/* Check if local node is the owner of the slot */
			sp = &slot[fork_nr+dc_ptr->dc_nr_tasks];
			if( sp->s_owner != NO_PRIMARY_MBR){ 
				if(sp->s_owner != local_nodeid ) {
					TASKDEBUG("ERROR: slot=%d owned by node=%d\n", 
						fork_nr+dc_ptr->dc_nr_tasks, sp->s_owner);
					free(argv_buf); 
					ERROR_RETURN(EMOLBADOWNER);
				}
			}
			
			if( !TEST_BIT(f_ptr->p_rts_flags, BIT_SLOT_FREE)) {
				TASKDEBUG("ERROR: slot=%d used by %s\n", 
					fork_nr+dc_ptr->dc_nr_tasks, f_ptr->p_name);
				free(argv_buf); 
				ERROR_RETURN(EMOLSLOTUSED);
			}
			
			TASKDEBUG("Bound endpoint=%d for pid=%d\n", fork_ep, fork_lpid);
			rcode = mnx_lclbind(dc_ptr->dc_dcid,fork_lpid,fork_ep);
			if( rcode < 0) ERROR_RETURN(rcode);
			/* Return the endpoint to PM */
			m_ptr->M7_ENDPT1 = fork_ep;
			strncpy(f_ptr->p_name,basename(arg_v[0]),(MAXPROCNAME-1));
			TASKDEBUG("p_name=%s\n", f_ptr->p_name);
		}else{
			fork_ptr->PR_LPID = fork_lpid;	
			rcode = do_fork(fork_ptr);
			if(rcode) {
				kill(fork_lpid, SIGSTOP);
				free(argv_buf); 
				ERROR_RETURN(rcode);				
			}
			/* Return the endpoint to PM */
			m_ptr->M7_ENDPT1 = fork_ptr->PR_ENDPT;
		}
		
		free(argv_buf);		
	}else { /* if caller (PM) is local, bind the remote process */
		posix_memalign( (void**) &argv_buf, getpagesize(), MAXCOPYBUF+1);

		/* Get filename from requester  */
		rcode = mnx_vcopy(thrower_ep, m_ptr->M7_ARGV_PTR, 
						SYSTASK(local_nodeid), argv_buf, arg_len);
		TASKDEBUG("rcode=%d\n",rcode);
		if( rcode < 0) {
			free(argv_buf);	
			ERROR_RETURN(rcode);			
		}
	
		TASKDEBUG("arg_len=%d argv_buf >%s<\n", arg_len, argv_buf);
		ptr = argv_buf;
		arg_c = 0;
		do {
			arg_v[arg_c] = strtok(ptr, " ");
			TASKDEBUG("arg_v[%d]=%s\n", arg_c, arg_v[arg_c]);
			if( arg_v[arg_c] == NULL) break;
			arg_c++;
			ptr = NULL;
		}while(TRUE);
		TASKDEBUG("arg_c=%d\n",arg_c);

		if(arg_c >= MNX_MAX_ARGS) ERROR_RETURN(EMOL2BIG);
				
		rcode = mnx_rmtbind(dc_ptr->dc_dcid, basename(arg_v[0]), 
					fork_ep, rmt_nodeid);
		if( rcode < 0) {
			free(argv_buf);	
			ERROR_RETURN(rcode);			
		}

		free(argv_buf);	
	}
	
	return(OK);
}

#endif /* USE_REXEC */

