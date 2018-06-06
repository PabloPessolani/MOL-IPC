/* This library provides generic watchdog timer management functionality.
 * The functions operate on a timer queue provided by the caller. Note that
 * the timers must use absolute time to allow sorting. The library provides:
 *
 *    tmrs_settimer:     (re)set a new watchdog timer in the timers queue 
 *    tmrs_clrtimer:     remove a timer from both the timers queue 
 *    tmrs_exptimers:    check for expired timers and run watchdog functions
 *
 * Author:
 *    Jorrit N. Herder <jnherder@cs.vu.nl>
 *    Adapted from tmr_settimer and tmr_clrtimer in src/kernel/clock.c. 
 *    Last modified: September 30, 2004.
 */

#ifndef _TIMERS_H
#define _TIMERS_H

typedef unsigned long molclock_t;	

struct moltimer;
typedef void (*tmr_func_t)(struct moltimer *tp);
typedef union { int ta_int; long ta_long; void *ta_ptr; } tmr_arg_t;



/* A timer_t variable must be declare for each distinct timer to be used.
 * The timers watchdog function and expiration time are automatically set
 * by the library function tmrs_settimer, but its argument is not.
 */
typedef struct moltimer
{
  struct moltimer	*tmr_next;	/* next in a timer chain */
  molclock_t 	tmr_exp_time;	/* expiration time */
  tmr_func_t	tmr_func;	/* function to call when expired */
  tmr_arg_t	tmr_arg;	/* random argument */
} moltimer_t;


molclock_t tmrs_clrtimer(moltimer_t **tmrs, moltimer_t *tp, molclock_t *next_time);
void tmrs_exptimers(moltimer_t **tmrs, molclock_t now, molclock_t *new_head);
molclock_t tmrs_settimer(moltimer_t **tmrs, moltimer_t *tp, molclock_t exp_time, tmr_func_t watchdog, molclock_t *new_head);


/* Used when the timer is not active. */
#define TMR_NEVER    ((molclock_t) -1 < 0) ? ((molclock_t) MOL_MOL_LONG_MAX) : ((molclock_t) -1)
#undef TMR_NEVER
#define TMR_NEVER	((molclock_t) MOL_LONG_MAX)

/* These definitions can be used to set or get data from a timer variable. */ 
#define tmr_arg(tp) (&(tp)->tmr_arg)
#define tmr_exp_time(tp) (&(tp)->tmr_exp_time)

/* Timers should be initialized once before they are being used. Be careful
 * not to reinitialize a timer that is in a list of timers, or the chain
 * will be broken.
 */
#define tmr_inittimer(tp) (void)((tp)->tmr_exp_time = TMR_NEVER, \
	(tp)->tmr_next = NULL)

#endif /* _TIMERS_H */

