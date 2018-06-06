#include "syslib.h"

/*===========================================================================*
 *				sys_migrproc				     
 * A system process has migrate to local node (or a local backup node is the new primary 
 * m_in.m3_i1:  local Linux PID
 * m_in.m3_i2: p_endpoint
 *===========================================================================*/
int sys_migrproc(int sysproc_lpid, int sysproc_ep)
{
	message m __attribute__((aligned(0x1000)));
	message *m_ptr;
	int rcode;

	LIBDEBUG("Sending SYS_MIGRPROC request to SYSTEM sysproc_lpid=%d sysproc_ep=%d\n",
		sysproc_lpid, sysproc_ep);
	m_ptr = &m;
	m_ptr->M3_LPID	= sysproc_lpid;	
	m_ptr->M3_ENDPT	= sysproc_ep;
	rcode = _taskcall(SYSTASK(local_nodeid), SYS_MIGRPROC, &m);
	if( rcode < 0) return(rcode);
	return(OK);
}

