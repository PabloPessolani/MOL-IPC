#include "syslib.h"

/*===========================================================================*
 *				sys_fork				     *
 * DUALITY: on input "*child" has the linux PID			*
 *      	on output "*child" has de process ENDPOINT       *
 *===========================================================================*/

/*===========================================================================*
 *				sys_rfork				   					*
 * Tell SYSTASK to bind a new process with Linux PID LPID 				*
 * It returns the endpoint allocated to the process						*
 *===========================================================================*/
int sys_rfork(int child_lpid, int st_nodeid)
{
	message m __attribute__((aligned(0x1000)));
	message *m_ptr;
	int rcode;

	LIBDEBUG("Sending SYS_FORK to SYSTASK child_lpid=%d st_nodeid=%d\n",
		child_lpid,st_nodeid);
	m_ptr = &m;
	m_ptr->PR_LPID  = child_lpid;
 	rcode = _taskcall(SYSTASK(st_nodeid), SYS_FORK, &m);
	if(rcode) return(rcode);

	LIBDEBUG("child_ep=%d\n",m_ptr->PR_ENDPT);
	return(m_ptr->PR_ENDPT);
}
