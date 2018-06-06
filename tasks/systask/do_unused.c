/* This file provides a catch-all handler for unused kernel calls. A kernel 
 * call may be unused when it is not defined or when it is disabled in the
 * kernel's configuration.
 */
#include "systask.h"

/*===========================================================================*
 *			          do_unused				     *
 *===========================================================================*/
int do_unused(message *m)
{
  printf("SYSTASK: got unused request %d from %d\n", m->m_type, m->m_source);
  return(EMOLBADREQUEST);			/* illegal message type */
}

