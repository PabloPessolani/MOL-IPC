#include <mollib.h>

int mol_pm_findproc(char *proc_name, int *proc_nr)
/* char *proc_name;		 name of process to search for */
/* int *proc_nr;			 return process number here */
{
	message m __attribute__((aligned(0x1000)));
  int ret;

  m.m1_p1 = proc_name;
  m.m1_i1 = -1;			/* search by name */
  m.m1_i2 = strlen(proc_name) + 1;
  if ((ret = molsyscall(PM_PROC_NR, MOLGETPROCNR, &m) < 0)) return(ret);
  *proc_nr = m.m1_i1;
  return(0);
}

