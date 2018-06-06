#include "syslib.h"

/*===========================================================================*
 *                                _sys_exit			     	     *
 *===========================================================================*/
int _sys_exit(int proc, int nodeid)
/* int proc;			 which process has exited */
{
/* A process has exited. PM tells the kernel. In addition this call can be
 * used by system processes to directly exit without passing through the
 * PM. This should be used with care to prevent inconsistent PM tables. 
 */
	message m __attribute__((aligned(0x1000)));
	int rcode;
	
	LIBDEBUG("SYS_EXIT request to SYSTEM(%d) proc=%d nodeid=%d\n",
		SYSTASK(nodeid), proc, nodeid);

	m.PR_ENDPT = proc;
	rcode = _taskcall(SYSTASK(nodeid), SYS_EXIT, &m);
	if(rcode) return(rcode);

    return(OK);
}
