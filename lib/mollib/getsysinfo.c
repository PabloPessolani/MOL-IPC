#include <mollib.h>

/* int who;			from whom to request info */
/* int what;			what information is requested */
/* void *where;			where to put it */
int getsysinfo(int who, int what, void *where)
{
	message m __attribute__((aligned(0x1000)));
	m.m1_i1 = what;
  m.m1_p1 = where;
  if (molsyscall(who, MOLGETSYSINFO, &m) < 0) return(-1);
  return(0);
}

