/* PM watchdog timer management. These functions in this file provide
 * a convenient interface to the timers library that manages a list of
 * watchdog timers. All details of scheduling an alarm at the CLOCK task 
 * are hidden behind this interface.
 * Only system processes are allowed to set an alarm timer at the kernel. 
 * Therefore, the PM maintains a local list of timers for user processes
 * that requested an alarm signal. 
 * 
 * The entry points into this file are:
 *   pm_set_timer:      reset and existing or set a new watchdog timer
 *   pm_expire_timers:  check for expired timers and run watchdog functions
 *   pm_cancel_timer:   remove a time from the list of timers
 *
 */

#include "pm.h"

moltimer_t *pm_timers = NULL;

/*===========================================================================*
 *				pm_set_timer				     *
 *===========================================================================*/
int pm_set_timer(moltimer_t *tp, int ticks, tmr_func_t watchdog, int arg)
{
	int rcode;
	molclock_t now, prev_time = 0, next_time;

	rcode = sys_getuptime(&now);
	if( rcode < 0) ERROR_RETURN(rcode);

SVRDEBUG("now=%ld ticks=%ld\n",now, ticks);

	/* Set timer argument and add timer to the list. */
	tmr_arg(tp)->ta_int = arg;
	prev_time = tmrs_settimer(&pm_timers,tp,now+ticks,watchdog,&next_time);

	/* Reschedule our synchronous alarm if necessary. */
	if (! prev_time || prev_time > next_time) {
		if ((rcode = sys_setalarm(next_time, 1)) != OK)
			ERROR_RETURN(rcode);
	}

	return(OK);
}

/*===========================================================================*
 *				pm_expire_timers			     *
 *===========================================================================*/
int pm_expire_timers(molclock_t now)
{
	int rcode;
	molclock_t next_time;

SVRDEBUG("now=%ld\n",now);

	/* Check for expired timers and possibly reschedule an alarm. */
	tmrs_exptimers(&pm_timers, now, &next_time);
	if (next_time > 0) {
		if ((rcode = sys_setalarm(next_time, 1)) != OK)
			ERROR_RETURN(rcode);
	}
	return(OK);

}

/*===========================================================================*
 *				pm_cancel_timer				     *
 *===========================================================================*/
int pm_cancel_timer(moltimer_t *tp)
{
	int rcode;
	molclock_t next_time, prev_time;
	prev_time = tmrs_clrtimer(&pm_timers, tp, &next_time);

SVRDEBUG("prev_time=%ld\n",prev_time);

	/* If the earliest timer has been removed, we have to set the alarm to  
     	* the next timer, or cancel the alarm altogether if the last timer has 
     	* been cancelled (next_time will be 0 then).
	 */
	if (prev_time < next_time || ! next_time) {
		if ((rcode = sys_setalarm(next_time, 1)) != OK)
			ERROR_RETURN(rcode);
	}
	return(OK);
}
