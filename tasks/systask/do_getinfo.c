/* The kernel call implemented in this file:
 *   m_type:	SYS_GETINFO
 *
 * The parameters for this kernel call are:
 *    m1_i3:	I_REQUEST	(what info to get)	
 *    m1_p1:	I_VAL_PTR 	(where to put it)	
 *    m1_i1:	I_VAL_LEN 	(maximum length expected, optional)	
 *    m1_p2:	I_VAL_PTR2	(second, optional pointer)	
 *    m1_i2:	I_VAL_LEN2_E	(second length or process nr)	
 */

#include "systask.h"

#if USE_GETINFO

extern int who_e, who_p;		/* message source endpoint and proc */
extern dc_usr_t dcu, *dc_ptr;
extern proc_usr_t *proc;
extern priv_usr_t *priv;
extern dvs_usr_t dvs, *dvs_ptr;
extern slot_t *slot, *slot_ptr;

/*===========================================================================*
 *			        do_getinfo				     *
 *===========================================================================*/
int do_getinfo(message *m_ptr)	/* pointer to request message */
{
/* Request system information to be copied to caller's address space. This
 * call simply copies entire data structures to the caller.
 */
  	size_t length;
  	void *src_addr; 
	slot_t *s_ptr;
  	proc_usr_t *caller_ptr, *proc_ptr, *lcl_ptr;
 	int ret, p_nr, p_ep, i;

TASKDEBUG("caller=%d rqst=%d addr=%X len=%d\n", 
		m_ptr->m_source, m_ptr->I_REQUEST, m_ptr->I_VAL_PTR, m_ptr->I_VAL_LEN);

	/* Set source address and length based on request type. */
  	ret = OK;
  	switch (m_ptr->I_REQUEST) {
	case GET_KINFO:  {
        	length = sizeof(dvs_usr_t);
        	src_addr = (void*)&dvs;
        	break;
    		}
    	case GET_MACHINE: {
        	length = sizeof(dc_usr_t);
        	src_addr = (void*)dc_ptr;
TASKDEBUG("dc_id=%d \n", dc_ptr->dc_dcid);
//	SYSTASK has already get the DC info at startup
//			ret = mnx_getdcinfo(dc_ptr->dc_dcid, dc_ptr);
TASKDEBUG(dc_USR_FORMAT, dc_USR_FIELDS(dc_ptr));
        	break;
    		}
    	case GET_PROCTAB: {
#ifdef ALLOC_LOCAL_TABLE 			
        	length = (sizeof(proc_usr_t) * (dc_ptr->dc_nr_tasks + dc_ptr->dc_nr_procs)); 
        	src_addr = (void *) proc;
#else /* ALLOC_LOCAL_TABLE */	
			/* it must be copied to user-space buffer because the mnx_vcopy some lines down*/
			/* is prepared for usr-usr copy */
        	length = (sizeof(proc_usr_t) * (dc_ptr->dc_nr_tasks + dc_ptr->dc_nr_procs)); 
			posix_memalign( (void**) &src_addr, getpagesize(), length);
			if( src_addr == NULL) return (EMOLNOMEM);
			lcl_ptr = (proc_usr_t *) src_addr;
			for(i = 0; i < (dc_ptr->dc_nr_tasks + dc_ptr->dc_nr_procs); i++){
				proc_ptr = (proc_usr_t *) PROC_MAPPED(i - dc_ptr->dc_nr_tasks);
				memcpy( &lcl_ptr[i], (void*) proc_ptr, sizeof(proc_usr_t));
			}
#endif /* ALLOC_LOCAL_TABLE */			
        	break;
    		}
    	case GET_SLOTSTAB: {
        	length = (sizeof(slot_t) * (dc_ptr->dc_nr_tasks + dc_ptr->dc_nr_procs)); 
        	src_addr = (void *) slot;
        	break;
    		}			
    	case GET_PRIVTAB: {
        	length = (sizeof(priv_usr_t) * (dc_ptr->dc_nr_tasks + dc_ptr->dc_nr_procs)); 
        	src_addr = (void *) priv;
        	break;
    		}
    	case GET_PROC: {
        	p_ep = (m_ptr->I_VAL_LEN2_E == SELF) ? m_ptr->m_source : m_ptr->I_VAL_LEN2_E;
			p_nr = _ENDPOINT_P(p_ep);
			CHECK_P_NR(p_nr); 
			TASKDEBUG("p_ep=%d p_nr=%d \n", p_ep, p_nr);
        	length = sizeof(proc_usr_t);
#ifdef ALLOC_LOCAL_TABLE 
        	src_addr = PROC2PTR(p_nr);
			proc_ptr = (proc_usr_t *) src_addr;
			TASKDEBUG("BEFORE" PROC_USR_FORMAT, PROC_USR_FIELDS(proc_ptr));	
			ret = mnx_getprocinfo(dc_ptr->dc_dcid, p_nr, src_addr);
			if( ret < 0) ERROR_RETURN(ret);
#else /* ALLOC_LOCAL_TABLE */	
			/* it must be copied to user-space buffer because the mnx_vcopy some lines down*/
			/* is prepared for usr-usr copy */
			posix_memalign( (void**) &src_addr, getpagesize(), length);
			if( src_addr == NULL) return (EMOLNOMEM);
			proc_ptr = (proc_usr_t *) PROC_MAPPED(p_nr);
			memcpy( src_addr, (void*) proc_ptr, sizeof(proc_usr_t));
			ret = OK;			
#endif /* ALLOC_LOCAL_TABLE */
			TASKDEBUG("AFTER" PROC_USR_FORMAT, PROC_USR_FIELDS(proc_ptr));
        	break;
    		}
    	case GET_SLOT: {
        	p_ep = (m_ptr->I_VAL_LEN2_E == SELF) ? m_ptr->m_source : m_ptr->I_VAL_LEN2_E;
			p_nr = _ENDPOINT_P(p_ep);
			CHECK_P_NR(p_nr); 
        	length = sizeof(slot_t);
        	s_ptr = &slot[p_nr+ dc_ptr->dc_nr_procs];
			TASKDEBUG(SLOTS_FORMAT, SLOTS_FIELDS(s_ptr));
			src_addr = (void *)s_ptr;
        	break;
    		}
		default:
        	ERROR_RETURN(EMOLINVAL);
 	}

	if(ret) ERROR_RETURN(ret);
  	/* Try to make the actual copy for the requested data. */
  	if (m_ptr->I_VAL_LEN > 0 && length > m_ptr->I_VAL_LEN) ERROR_RETURN(EMOL2BIG);

//	caller_ptr = ENDPOINT2PTR(m_ptr->m_source);
//	if( caller_ptr->p_rts_flags == SLOT_FREE) 	ERROR_RETURN(EMOLNOTBIND); 
	ret =	mnx_vcopy(SYSTASK(local_nodeid), src_addr, m_ptr->m_source, m_ptr->I_VAL_PTR, length);
#ifndef ALLOC_LOCAL_TABLE 
	if( (m_ptr->I_REQUEST == GET_PROCTAB) 
	 || (m_ptr->I_REQUEST == GET_PROC)){
		free(src_addr);
	}
#endif /* ALLOC_LOCAL_TABLE */
	if(ret) ERROR_RETURN(ret);

  return(OK);
}

#endif /* USE_GETINFO */

