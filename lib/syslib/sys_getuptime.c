#include "syslib.h"

/*===========================================================================*
 *                               getuptime			    	     *
 *===========================================================================*/
int sys_getuptime(molclock_t *ticks)
{
	message m __attribute__((aligned(0x1000)));
    int s;

    m.m_type = SYS_TIMES;		/* request time information */
    m.T_ENDPT = NONE;			/* ignore process times */
    s = _taskcall(SYSTASK(local_nodeid), SYS_TIMES, &m);
    *ticks = m.T_BOOT_TICKS;
    return(s);
}





