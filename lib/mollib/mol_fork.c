#include <mollib.h>
/* Tell PM that the a process has forked				*/
/* Do RFORK to PM-->SYSTASK 						*/
int _mol_fork(int child_lpid, int child_ep)
{
	message m __attribute__((aligned(0x1000)));
  	int ret;
	int child_pid;

	LIBDEBUG("child_lpid=%d child_ep=%d\n", child_lpid, child_ep);

	m.M3_LPID  = child_lpid;
	m.M3_ENDPT = child_ep;

	strncpy(m.m3_ca1, program_invocation_short_name, (M3_STRING-1));
	ret = molsyscall(PM_PROC_NR, MOLFORK, &m);
	child_pid = m.m_type; /* MINIX PID */
	if( child_pid < 0 || ret < 0 ){                                          
        	/* mnx_errno= child_pid; */
	        return(-1);
	}
	LIBDEBUG("child_pid=%d ret=%d\n", child_pid, ret);
	return(child_pid);
}