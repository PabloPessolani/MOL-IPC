#include <mollib.h>


unsigned int mol_alarm(unsigned int sec)
{
	message m __attribute__((aligned(0x1000)));

  m.m1_i1 = (int) sec;
  return( (unsigned) molsyscall(PM_PROC_NR, MOLALARM, &m));
}
