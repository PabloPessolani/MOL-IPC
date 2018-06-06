/* The kernel call implemented in this file:
 *   m_type:	SYS_VIRCOPY
 *
 * The parameters for this kernel call are:
 *    m5_l1:	CP_SRC_ADDR		source offset within userspace
 *    m5_i1:	CP_SRC_ENDPT	source process endpoint
 *    m5_l2:	CP_DST_ADDR		destination offset within userspace
 *    m5_i2:	CP_DST_ENDPT	destination process endpoint
 *    m5_l3:	CP_NR_BYTES		number of bytes to copy
 */

#include "systask.h"

#if (USE_VIRCOPY || USE_PHYSCOPY)

/*===========================================================================*
 *				do_copy					     *
 *===========================================================================*/
int do_copy(message *m_ptr)	/* pointer to request message */
{

	proc_usr_t *src_ptr, *dst_ptr;
	int src_nr, dst_nr;
	struct vir_cp_req req, *req_ptr;


	TASKDEBUG(MSG5_FORMAT, MSG5_FIELDS(m_ptr));

/*!!!!!!!!!!!!!! VERIFICAR QUE EL M_SOURCE/REQUESTER TENGA PRIVILEGIOS !!!!!!!!!!!!!!!!!!!!*/

	req_ptr = &req;
	req_ptr->src.proc_nr_e 	= m_ptr->CP_SRC_ENDPT;
	req_ptr->dst.proc_nr_e 	= m_ptr->CP_DST_ENDPT;
	req_ptr->count  		= m_ptr->CP_NR_BYTES;	
	if( req_ptr->src.proc_nr_e == SELF) req_ptr->src.proc_nr_e = who_e;
	if( req_ptr->dst.proc_nr_e == SELF) req_ptr->dst.proc_nr_e = who_e;

	TASKDEBUG(VIRCP_FORMAT, VIRCP_FIELDS(req_ptr));

	/* Parameter checking	*/
	if( 	(req_ptr->src.proc_nr_e == req_ptr->dst.proc_nr_e) 	
		|| 	(req_ptr->src.proc_nr_e == ANY)	
		|| 	(req_ptr->dst.proc_nr_e == ANY) 
		||	(req_ptr->src.proc_nr_e == NONE)
		||	(req_ptr->dst.proc_nr_e == NONE) )		
			ERROR_RETURN(EMOLENDPOINT);

	src_nr = _ENDPOINT_P(req_ptr->src.proc_nr_e);
	dst_nr = _ENDPOINT_P(req_ptr->dst.proc_nr_e);

	CHECK_P_NR(src_nr);		/* check process number limits */
	CHECK_P_NR(dst_nr);		/* check process number limits */

	src_ptr = PROC2PTR(src_nr);
	TASKDEBUG("SRC " PROC_USR_FORMAT,PROC_USR_FIELDS(src_ptr));

	dst_ptr = PROC2PTR(dst_nr);
	TASKDEBUG("DST " PROC_USR_FORMAT,PROC_USR_FIELDS(dst_ptr));

	if( src_ptr->p_rts_flags == SLOT_FREE) ERROR_RETURN(EMOLSRCDIED);
	if( dst_ptr->p_rts_flags == SLOT_FREE) ERROR_RETURN(EMOLDSTDIED);

  	if ( req_ptr->count <= 0 ) 			ERROR_RETURN(EMOLINVAL);
  	if ( req_ptr->count > MAXCOPYLEN ) 	ERROR_RETURN(E2BIG);

  	/* Now try to make the actual virtual copy. */
	return( mnx_vcopy( req_ptr->src.proc_nr_e, m_ptr->CP_SRC_ADDR,
			req_ptr->dst.proc_nr_e, m_ptr->CP_DST_ADDR, req_ptr->count));
}

#endif /* (USE_VIRCOPY || USE_PHYSCOPY) */

