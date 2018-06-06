#include <mollib.h>

int mol_getnprocnr(pid_t pid)
{
	message m __attribute__((aligned(0x1000)));
  int ret;

  m.m1_i1 = pid;		/* pass pid >=0 to search for */
  m.m1_i2 = 0;			/* don't pass name to search for */
  if ((ret = molsyscall(PM_PROC_NR, MOLGETPROCNR, &m) < 0)) return(ret);
  return(m.m1_i1);		/* return search result */
}

