#include <mollib.h>
/* Tell PM that the process exits					*/
/* int status;			the statuts of the process		*/
void mol_exit(status)
{
  message m __attribute__((aligned(0x1000)));
  int ret;
  m.m1_i1 = status;
  m.m1_i2 = (-1);
  ret = molsyscall(PM_PROC_NR, MOLEXIT, &m);
}

