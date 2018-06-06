#include <mollib.h>

pid_t mol_wait(int *status)
{
	message m __attribute__((aligned(0x1000)));

  if (molsyscall(PM_PROC_NR, MOLWAIT, &m) < 0) return(-1);
  if (status != 0) *status = m.m2_i1;
  return(m.m_type);
}
