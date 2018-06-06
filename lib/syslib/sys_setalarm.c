#include "syslib.h"


/*===========================================================================*
 *                               sys_setalarm		     	     	     *
 *===========================================================================*/
int sys_setalarm(molclock_t exp_time, int abs_time)
/* molclock_t exp_time;	 expiration time for the alarm */
/* int abs_time;	 use absolute or relative expiration time */
{
/* Ask the SYSTEM schedule a synchronous alarm for the caller. The process
 * number can be SELF if the caller doesn't know its process number.
 */
	message m __attribute__((aligned(0x1000)));
    m.ALRM_EXP_TIME = exp_time;		/* the expiration time */
    m.ALRM_ABS_TIME = abs_time;		/* time is absolute? */
    return _taskcall(SYSTASK(local_nodeid), SYS_SETALARM, &m);
}

