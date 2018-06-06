#include "syslib.h"

int sys_rtimes(int endp, molclock_t *ptr, int st_nodeid)
/* int endp;			endp whose times are needed */
/* molclock_t ptr[5];		pointer to time buffer */
{
/* Fetch the accounting info for a endp. */
	message m __attribute__((aligned(0x1000)));
	int r;

  m.T_ENDPT = endp;
  r = _taskcall(SYSTASK(st_nodeid), SYS_TIMES, &m);
  ptr[0] = m.T_USER_TIME;
  ptr[1] = m.T_SYSTEM_TIME;
  ptr[2] = m.T_CHILD_UTIME;
  ptr[3] = m.T_CHILD_STIME;
  ptr[4] = m.T_BOOT_TICKS;
  return(r);
}
