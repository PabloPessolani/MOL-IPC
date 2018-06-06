#include <mollib.h>

pid_t mol_getpid(void)
{
	message m __attribute__((aligned(0x1000)));
  	
  return(molsyscall(PM_PROC_NR, MOLGETPID, &m));
}
