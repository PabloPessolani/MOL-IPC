#include "syslib.h"

/*===========================================================================*
 *				sys_privctl				     *
 *===========================================================================*/
int sys_privctl(int endpoint, int priv_type)
{
	message m __attribute__((aligned(0x1000)));
	message *m_ptr;
	int rcode;

LIBDEBUG("Sending SYSPRIVCTL request to SYSTEM endpoint=%d priv_type=%d\n",endpoint, priv_type);
	m_ptr = &m;
	m_ptr->m_type  = SYS_PRIVCTL;
  	m_ptr->PR_ENDPT= endpoint;
	m_ptr->PR_PRIV = priv_type;
	rcode = _taskcall(SYSTASK(local_nodeid), SYS_PRIVCTL, &m);
	if(rcode) ERROR_RETURN(rcode);
	return(rcode);
}


