#include "syslib.h"

/*===========================================================================*
 *				sys_setpname				     *
 * m_in.M3_ENDPT
 * m_in.M3_NAME
 *===========================================================================*/

int sys_rsetpname(int p_ep,  char *p_name, int st_nodeid)
{
	message m __attribute__((aligned(0x1000)));
	message *m_ptr;
	int rcode;

	LIBDEBUG("Sending SYS_SETPNAME request to SYSTEM p_ep=%d p_name=%s\n",
		p_ep, p_name);
	m_ptr = &m;
	m_ptr->M3_ENDPT = p_ep;
	strncpy(m_ptr->M3_NAME,p_name, (M3_STRING-1));
	rcode = _taskcall(SYSTASK(st_nodeid), SYS_SETPNAME, &m);
	if( rcode < 0) return(rcode);
	return(rcode);
}
