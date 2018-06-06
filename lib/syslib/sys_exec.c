#include "syslib.h"

PUBLIC int sys_exec(proc, ptr, prog_name, initpc)
int proc;			/* process that did exec */
char *ptr;			/* new stack pointer */
char *prog_name;		/* name of the new program */
vir_bytes initpc;
{
/* A process has exec'd.  Tell the kernel. */

	message m __attribute__((aligned(0x1000)));
	
  m.PR_ENDPT = proc;
  m.PR_STACK_PTR = ptr;
  m.PR_NAME_PTR = prog_name;
  m.PR_IP_PTR = (char *)initpc;
  return(_taskcall(SYSTASK(local_nodeid), SYS_EXEC, &m));
}
