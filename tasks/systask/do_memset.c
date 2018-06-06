/* The kernel call implemented in this file:
 *   m_type:	SYS_MEMSET
 *
 * The parameters for this kernel call are:
 *    m2_p1:	MEM_PTR	(virtual address)	
 *    m2_l1:	MEM_COUNT	(returns physical address)	
 *    m2_l2:	MEM_PATTERN	(size of datastructure) 	
 */

#include "systask.h"

#if USE_MEMSET

/*===========================================================================*
 *				do_memset				     *
 *===========================================================================*/
int do_memset(message *m_ptr)
{
/* Handle sys_memset(). This writes a pattern into the specified memory. */

	char *lclbuf;
	int count, size, remain, ret;
	char *dst_addr;

	unsigned char c;

	c = m_ptr->MEM_PATTERN;
	count = m_ptr->MEM_COUNT;
	dst_addr = m_ptr->MEM_PTR;
	TASKDEBUG( "pattern=%d count=%d\n", c, count);

	if( count <= 0) ERROR_RETURN(EINVAL);

  	lclbuf = memalign(pagesize, pagesize);
	if( lclbuf == NULL) ERROR_EXIT(errno);

	memset(lclbuf, c, pagesize);
	remain = count; 
	while( remain > 0){
		size = ( remain > pagesize)? pagesize:remain;
		ret = mnx_vcopy(SYSTEM, lclbuf, who_e, dst_addr , size);
		if(ret) break;
		dst_addr+=size;
		remain -= size;
	} 
	free(lclbuf);
	return(ret);
}

#endif /* USE_MEMSET */

