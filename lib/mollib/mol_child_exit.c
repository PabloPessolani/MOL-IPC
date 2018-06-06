#include <mollib.h>
/* The parent of a process (linux) detect that its child exit 	*/
/* Tell PM that the child exits					*/
/* int lnx_pid;			the linux pid of the child 	*/
/* int status;			the statuts of the child	*/
int mol_child_exit(lnx_pid, status)
{
	message m __attribute__((aligned(0x1000)));
  int ret;
  m.m1_i1 = status;
  m.m1_i2 = lnx_pid;
  ret = molsyscall(PM_PROC_NR, MOLEXIT, &m);
  if(ret < 0) return(-1);
  return(0);
}

