/* The kernel call implemented in this file:
 *   m_type:	SYS_VIRCOPY, SYS_PHYSCOPY
 *
 * The parameters for this kernel call are:
 *    m5_c1:	CP_SRC_SPACE		source virtual segment
 *    m5_l1:	CP_SRC_ADDR		source offset within segment
 *    m5_i1:	CP_SRC_PROC_NR		source process number
 *    m5_c2:	CP_DST_SPACE		destination virtual segment
 *    m5_l2:	CP_DST_ADDR		destination offset within segment
 *    m5_i2:	CP_DST_PROC_NR		destination process number
 *    m5_l3:	CP_NR_BYTES		number of bytes to copy
 */

#include "../system.h"
#include <minix/type.h>

#define		MAXBUFSIZE	1432 
char	buffer[MAXBUFSIZE];
extern volatile molclock_t realtime;		/* real time clock */


#if (USE_VIRCOPY || USE_PHYSCOPY)

/*===========================================================================*
 *				do_copy					     *
 *===========================================================================*/
PUBLIC int do_copy(m_ptr)
register message *m_ptr;	/* pointer to request message */
{
/* Handle sys_vircopy() and sys_physcopy().  Copy data using virtual or
 * physical addressing. Although a single handler function is used, there 
 * are two different kernel calls so that permissions can be checked. 
 */
  struct vir_addr vir_addr[2];	/* virtual source and destination address */
  phys_bytes bytes;		/* number of bytes to copy */
  int i;

  /* Dismember the command message. */
  vir_addr[_SRC_].proc_nr_e = m_ptr->CP_SRC_ENDPT;
  vir_addr[_SRC_].segment = m_ptr->CP_SRC_SPACE;
  vir_addr[_SRC_].offset = (vir_bytes) m_ptr->CP_SRC_ADDR;
  vir_addr[_DST_].proc_nr_e = m_ptr->CP_DST_ENDPT;
  vir_addr[_DST_].segment = m_ptr->CP_DST_SPACE;
  vir_addr[_DST_].offset = (vir_bytes) m_ptr->CP_DST_ADDR;
  bytes = (phys_bytes) m_ptr->CP_NR_BYTES;

  /* Now do some checks for both the source and destination virtual address.
   * This is done once for _SRC_, then once for _DST_. 
   */
  for (i=_SRC_; i<=_DST_; i++) {
	int p;
      /* Check if process number was given implictly with SELF and is valid. */
      if (vir_addr[i].proc_nr_e == SELF)
	vir_addr[i].proc_nr_e = m_ptr->m_source;
      if (vir_addr[i].segment != PHYS_SEG &&
	! isokendpt(vir_addr[i].proc_nr_e, &p))
          return(EINVAL); 

      /* Check if physical addressing is used without SYS_PHYSCOPY. */
      if ((vir_addr[i].segment & PHYS_SEG) &&
          m_ptr->m_type != SYS_PHYSCOPY) return(EPERM);
  }

  /* Check for overflow. This would happen for 64K segments and 16-bit 
   * vir_bytes. Especially copying by the PM on do_fork() is affected. 
   */
  if (bytes != (vir_bytes) bytes) return(E2BIG);

  /* Now try to make the actual virtual copy. */
  return( virtual_copy(&vir_addr[_SRC_], &vir_addr[_DST_], bytes) );
}

/*===========================================================================*
 *				do_testcopy					     *
 *===========================================================================*/
PUBLIC int do_testcopy(m_ptr)
register message *m_ptr;	/* pointer to request message */
{
	int i, r, bytes, loops;
	message m;
	struct vir_addr vir_addr[2];	/* virtual source and destination address */
	molclock_t before, after;

/*
  kprintf("SYSTEM do_testcopy: src=%d dst=%d bytes=%d loops=%d saddr=%X daddr=%X\n",
  m_ptr->m7_i1,
  m_ptr->m7_i2,
  m_ptr->m7_i3,
  m_ptr->m7_i4,
  m_ptr->m7_p1,
  m_ptr->m7_p2);

  for(i = 0; i < MAXBUFSIZE; i++)		
	buffer[i] = ((i%10) + '0');
*/
	
	if( m_ptr->m7_i1 == SYSTEM) {
		vir_addr[_SRC_].proc_nr_e = SYSTEM;
		vir_addr[_SRC_].segment = D;
		vir_addr[_SRC_].offset = &buffer;
	} else {
		vir_addr[_SRC_].proc_nr_e = m_ptr->m7_i1;
		vir_addr[_SRC_].segment = D;
		vir_addr[_SRC_].offset = m_ptr->m7_p1;
	}
  vir_addr[_DST_].proc_nr_e =  m_ptr->m7_i2;
  vir_addr[_DST_].segment = D;
  vir_addr[_DST_].offset =  m_ptr->m7_p2;
  bytes = m_ptr->m7_i3;
 
/*  for(i = 0; i < m_ptr->m7_i4; i++) */
		r= virtual_copy(&vir_addr[_SRC_], &vir_addr[_DST_], bytes);

 return(r);
}
#endif /* (USE_VIRCOPY || USE_PHYSCOPY) */

