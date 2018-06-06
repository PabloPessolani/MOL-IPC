#include <mollib.h>


molclock_t mol_times(struct tms *buf)
{
	message m __attribute__((aligned(0x1000)));

  m.m4_l5 = 0;			/* return this if system is pre-1.6 */
  if (molsyscall(PM_PROC_NR, MOLTIMES, &m) < 0) return( (molclock_t) -1);
  buf->tms_utime = m.m4_l1;
  buf->tms_stime = m.m4_l2;
  buf->tms_cutime = m.m4_l3;
  buf->tms_cstime = m.m4_l4;
  return(m.m4_l5);
}
