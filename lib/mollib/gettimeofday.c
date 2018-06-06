/*
gettimeofday.c
*/

#include <mollib.h>

int mol_gettimeofday(struct timeval *_RESTRICT tp, void *_RESTRICT tzp)
{
	message m __attribute__((aligned(0x1000)));

  if (molsyscall(PM_PROC_NR, MOLGETTIMEOFDAY, &m) < 0)
  	return -1;

  tp->tv_sec = m.m2_l1;
  tp->tv_usec = m.m2_l2;

  return 0;
}

