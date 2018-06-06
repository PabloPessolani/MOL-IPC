#include "syslib.h"

/*===========================================================================*
 *				sys_rfork				     *
 *===========================================================================*/
int sys_rfork(int endpoint, int nodeid, char *name)
{
	message m, *m_ptr;
	int rcode;

LIBDEBUG("Sending SYS_RFORK request to SYSTEM endpoint=%d nodeid=%d name=%s\n"
		,endpoint, nodeid, name);
	m_ptr = &m;
	m_ptr->m3_i1 = endpoint;
	m_ptr->m3_i2 = nodeid;
	strncpy(m_ptr->m3_ca1,name, (M3_STRING-1));
 	rcode = _taskcall(SYSTASK, SYS_RFORK, &m);
	if(rcode) return(rcode);

LIBDEBUG("child_ep=%d\n",m_ptr->PR_ENDPT);
	return(m_ptr->PR_ENDPT);
}

