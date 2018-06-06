#include <mollib.h>
void mol_unbind(int p_ep)
{
  message m __attribute__((aligned(0x1000)));
  int ret;
  m.m1_i1 = p_ep;
  ret = molsyscall(PM_PROC_NR, MOLUNBIND, &m);
}

