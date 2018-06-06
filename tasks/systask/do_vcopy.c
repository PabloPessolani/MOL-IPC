/* The kernel call implemented in this file:
 *   m_type:	SYS_VIRVCOPY, 
 *
 * The parameters for this kernel call are:
 *    m1_i3:	VCP_VEC_SIZE		size of copy request vector 
 *    m1_p1:	VCP_VEC_ADDR		address of vector at caller 
 *    m1_i2:	VCP_NR_OK		number of successfull copies	
 */

#include "systask.h"

// CPVEC_NR definde in kernel/minix/const.h
#if (USE_VIRVCOPY || USE_PHYSVCOPY)

/* Buffer to hold copy request vector from user. */
struct vir_cp_req cp_req[VCOPY_VEC_SIZE];

/*===========================================================================*
 *				do_vcopy					     *
 *===========================================================================*/
int do_vcopy(register message *m_ptr)
{
	int nr_req;
	int bytes;
	int i,rcode;
	struct vir_cp_req *req;

	TASKDEBUG("VCP_VEC_SIZE=%d, VCP_VEC_ADDR=%X, VCP_NR_OK=%d\n",		
				m_ptr->VCP_VEC_SIZE,
				m_ptr->VCP_VEC_ADDR,
				m_ptr->CP_DST_ENDPT);
	
	/*!!!!!!!!!!!!!! VERIFICAR QUE EL M_SOURCE/REQUESTER TENGA PRIVILEGIOS !!!!!!!!!!!!!!!!!!!!*/
	
	/* Check if request vector size is ok. */
	nr_req = (unsigned) m_ptr->VCP_VEC_SIZE;
	if (nr_req > VCOPY_VEC_SIZE) return(EINVAL);
	bytes = nr_req * sizeof(struct vir_cp_req);

	/* copy request vector  */
	rcode = mnx_vcopy(who_e, m_ptr->VCP_VEC_ADDR, 
						SELF, cp_req ,bytes);
	if(rcode) ERROR_RETURN(rcode);	
	
	/* Assume vector with requests is correct. Try to copy everything. */
	m_ptr->VCP_NR_OK = 0;
	for (i=0; i<nr_req; i++) {

		req = &cp_req[i];

		TASKDEBUG(VIRCP_FORMAT, VIRCP_FIELDS(req));
					
		rcode = mnx_vcopy(req->src.proc_nr_e, req->src.offset,
							req->dst.proc_nr_e, req->dst.offset,
							req->count);
		if(rcode) ERROR_RETURN(rcode);
		m_ptr->VCP_NR_OK ++;
	}
	TASKDEBUG("VCP_NR_OK=%d\n",	m_ptr->VCP_NR_OK);	
	return(OK);
}

#endif /* (USE_VIRVCOPY || USE_PHYSVCOPY) */

