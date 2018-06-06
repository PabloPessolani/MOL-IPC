#include <mollib.h>
/* Wait until PM bound the caller (the child)				*/
int mol_wait4fork(void)
{
	message m __attribute__((aligned(0x1000)));
  	int ret;
	int child_pid;

	m.M3_LPID  = getpid();
 	ret = mnx_getep(m.M3_LPID);
	if( ret < EMOLERRCODE)
		ERROR_RETURN(ret);
	m.M3_ENDPT = ret;
	LIBDEBUG("M3_LPID=%d M3_ENDPT=%d\n", m.M3_LPID, m.M3_ENDPT );
		
	strncpy(m.M3_NAME, program_invocation_short_name, (M3_STRING-1));
	ret = molsyscall(PM_PROC_NR, MOLWAIT4FORK, &m);
	child_pid = m.m_type; /* MINIX PID */
	if( child_pid < 0 || ret < 0 ){                                          
        	/* mnx_errno= child_pid; */
	        return(-1);
	}
	LIBDEBUG("child_pid=%d ret=%d\n", child_pid, ret);
	return(child_pid);
}