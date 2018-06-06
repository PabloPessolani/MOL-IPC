#include <mollib.h>

pid_t mol_waitpid(pid_t pid,int *status,int options)
{
	message m __attribute__((aligned(0x1000)));

  m.m1_i1 = pid;
  m.m1_i2 = options;
  if (molsyscall(PM_PROC_NR, MOLWAITPID, &m) < 0) return(-1);
  if (status != 0) *status = m.m2_i1;
  return m.m_type;
}
