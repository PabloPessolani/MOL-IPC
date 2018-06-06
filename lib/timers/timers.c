#include "timers.h"

/*===========================================================================*
 *				tmrs_clrtimer				     *
 *===========================================================================*/
molclock_t tmrs_clrtimer(moltimer_t **tmrs, moltimer_t *tp, molclock_t *next_time)
{
	/* Deactivate a timer and remove it from the timers queue. */
 	moltimer_t **atp;
  	molclock_t prev_time;

  	if(*tmrs)
  		prev_time = (*tmrs)->tmr_exp_time;
  	else
  		prev_time = 0;

LIBDEBUG("prev_time=%ld\n",prev_time);

  	tp->tmr_exp_time = TMR_NEVER;

  	for (atp = tmrs; *atp != NULL; atp = &(*atp)->tmr_next) {
		if (*atp == tp) {
			*atp = tp->tmr_next;
			break;
		}
  	}

  	if(next_time) {
  		if(*tmrs)
  			*next_time = (*tmrs)->tmr_exp_time;
  		else	
  			*next_time = 0;
		LIBDEBUG("next_time=%ld\n",*next_time);
  	}	

  return prev_time;
}



/*===========================================================================*
 *				tmrs_exptimers				     *
 *===========================================================================*/
void tmrs_exptimers(moltimer_t **tmrs, molclock_t now, molclock_t *new_head)
{
/* Use the current time to check the timers queue list for expired timers. 
 * Run the watchdog functions for all expired timers and deactivate them.
 * The caller is responsible for scheduling a new alarm if needed.
 */
  	moltimer_t *tp;

LIBDEBUG("now=%ld\n", now);

  	while ((tp = *tmrs) != NULL && tp->tmr_exp_time <= now) {
		*tmrs = tp->tmr_next;
		tp->tmr_exp_time = TMR_NEVER;
		(*tp->tmr_func)(tp);
  	}

  	if(new_head) {
  		if(*tmrs)
  			*new_head = (*tmrs)->tmr_exp_time;
  		else
  			*new_head = 0;
		LIBDEBUG("new_head=%ld\n",*new_head);
  	}
}


/*===========================================================================*
 *				tmrs_settimer				     *
 *===========================================================================*/
molclock_t tmrs_settimer(moltimer_t **tmrs, moltimer_t *tp, molclock_t exp_time, tmr_func_t watchdog, molclock_t *new_head)
{
/* Activate a timer to run function 'fp' at time 'exp_time'. If the timer is
 * already in use it is first removed from the timers queue. Then, it is put
 * in the list of active timers with the first to expire in front.
 * The caller responsible for scheduling a new alarm for the timer if needed. 
 */
  	moltimer_t **atp;
  	molclock_t old_head = 0;

LIBDEBUG("exp_time=%ld\n",exp_time);

 	if(*tmrs != NULL )
  		old_head = (*tmrs)->tmr_exp_time;

  	/* Set the timer's variables. */
  	(void) tmrs_clrtimer(tmrs, tp, NULL);
  	tp->tmr_exp_time = exp_time;
  	tp->tmr_func = watchdog;

  	/* Add the timer to the active timers. The next timer due is in front. */
 	for (atp = tmrs; *atp != NULL; atp = &(*atp)->tmr_next) {
		if (exp_time < (*atp)->tmr_exp_time) break;
  	}
  	tp->tmr_next = *atp;
  	*atp = tp;
  	if(new_head != NULL) {
  		(*new_head) = (*tmrs)->tmr_exp_time;
		LIBDEBUG("new_head=%ld\n",*new_head);
	}

  return old_head;
}


