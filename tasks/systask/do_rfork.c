/* The kernel call implemented in this file:
 *   m_type:	SYS_RFORK
 *
 * The parameters for this kernel call are:
 *    m3_i1:	child endpoint
 *    m3_i2:	child nodeid
 *   m3_ca1: 	child name
 OUTPUT:
 *    m1_i2:	PR_ENDPT child endpoint
 */
#include "systask.h"


#if USE_RFORK

/*===========================================================================*
 *				do_rfork					     *
 *===========================================================================*/
int do_rfork(message *m_ptr)	
{
/* Handle sys_rfork().  PR_ENDPT has forked.  The child is PR_SLOT. */
  proc_usr_t *child_ptr;		/* child process pointer */
  priv_usr_t *cpriv_ptr;		/* child privileges pointer */
  int child_nr, child_lpid, child_ep, child_nodeid;
  char *child_name;
  int rcode;

	child_ep 		= m_ptr->m3_i1;
	child_nodeid 	= m_ptr->m3_i2;
	child_name	 	= m_ptr->m3_ca1;
TASKDEBUG("child_ep=%d child_nodeid=%d child_name=%s\n", child_ep, child_nodeid, child_name);
		
	child_nr = _ENDPOINT_P(child_ep);
	if( (child_nr+vm_ptr->vm_nr_tasks) >= vm_ptr->vm_nr_sysprocs)
		if( slot[child_nr+vm_ptr->vm_nr_tasks].s_owner != child_nodeid)
			ERROR_RETURN(EMOLBADNODEID);
		
	/* Register the process to the kernel */
	child_ep  = mnx_rmtbind(vm_ptr->vm_vmid, child_name , child_ep, child_nodeid);
	TASKDEBUG("child_ep=%d\n", child_ep);
	if(child_ep < 0 && child_ep != EMOLBUSY) ERROR_RETURN(child_ep);
	
	/* Get process information from kernel */
	child_ptr = PROC2PTR(child_nr);
	rcode = mnx_getprocinfo(vm_ptr->vm_vmid, child_nr, child_ptr);
	if( rcode < 0) ERROR_RETURN(rcode);
TASKDEBUG(PROC_USR_FORMAT,PROC_USR_FIELDS(child_ptr));

#ifdef ANULADO
	/* Get process privileges from kernel */
	child_ep = child_ptr->p_endpoint;
TASKDEBUG("child_ep=%d\n", child_ep);
	cpriv_ptr =  PROC2PRIV(child_nr);
	rcode = mnx_getpriv(vm_ptr->vm_vmid, child_ep, cpriv_ptr);
	if( rcode < 0) ERROR_RETURN(rcode);
TASKDEBUG(PRIV_USR_FORMAT,PRIV_USR_FIELDS(cpriv_ptr));
#endif /* ANULADO */

	m_ptr->PR_ENDPT = child_ep;
	m_ptr->PR_SLOT  = child_nr;

TASKDEBUG("REPLY msg:"MSG1_FORMAT,MSG1_FIELDS(m_ptr));

  return(OK);
}

#endif /* USE_FORK */

