/* The kernel call implemented in this file:
 *   m_type:	SYS_SETALARM 
 *
 * The parameters for this kernel call are:
 *    m2_l1:	ALRM_EXP_TIME		(alarm's expiration time)
 *    m2_i2:	ALRM_ABS_TIME		(expiration time is absolute?)
 *    m2_l1:	ALRM_TIME_LEFT		(return seconds left of previous)
 */

#include "systask.h"

#if USE_SETALARM

/*===========================================================================*
 *				do_setalarm				     *
 *===========================================================================*/
int do_setalarm(message *m_ptr)		/* pointer to request message */
{
	/* A process requests a synchronous alarm, or wants to cancel its alarm. */
  	proc_usr_t *caller_ptr;
	priv_usr_t  *priv_ptr;
  	long exp_time;			/* expiration time for this alarm */
  	int use_abs_time;		/* use absolute or relative time */
  	moltimer_t *tp;			/* the process' timer structure */
  	molclock_t uptime, now;		/* placeholder for current uptime */
	int rcode;

  	/* Extract shared parameters from the request message. */
  	exp_time = m_ptr->ALRM_EXP_TIME;	/* alarm's expiration time */
  	use_abs_time = m_ptr->ALRM_ABS_TIME;	/* flag for absolute time */
  	caller_ptr = PROC2PTR(who_p);
TASKDEBUG("who_p=%d who_e=%d exp_time=%d, use_abs_time=%d\n",who_p,who_e,exp_time,use_abs_time);

	priv_ptr = PROC2PRIV(who_p);
	rcode = mnx_getpriv(dc_ptr->dc_dcid, caller_ptr->p_endpoint, priv_ptr);
	if( rcode ) ERROR_RETURN(rcode);

//  	if (priv_ptr->s_level == USER_PRIV) ERROR_RETURN(EMOLPERM);

TASKDEBUG("PRIV "PRIV_USR_FORMAT, PRIV_USR_FIELDS(priv_ptr));

  	/* Get the timer structure and set the parameters for this alarm. */
  	tp = &priv_ptr->s_alarm_timer;	
  	tp->tmr_arg.ta_int = who_e;
  	tp->tmr_func = cause_alarm; 

  	/* Return the ticks left on the previous alarm. */
	now = get_uptime();
TASKDEBUG("now=%ld init_time=%ld exp_time=%ld\n",now, init_time,exp_time);
  	if ((tp->tmr_exp_time != TMR_NEVER) && ( (now) < tp->tmr_exp_time) ) {
      		m_ptr->ALRM_TIME_LEFT = (tp->tmr_exp_time - (now));
  	} else {
      		m_ptr->ALRM_TIME_LEFT = 0;
  	}

  	/* Finally, (re)set the timer depending on the expiration time. */
  	if (exp_time == 0) {
      		reset_timer(tp);
  	} else {
      		tp->tmr_exp_time = (use_abs_time) ? exp_time : exp_time + (now);
      		set_timer(tp, tp->tmr_exp_time, tp->tmr_func);
  	}
  return(OK);
}



/*===========================================================================*
 *				cause_alarm				     *
 *===========================================================================*/
void cause_alarm(moltimer_t *tp)
{
	/* Routine called if a timer goes off and the process requested a synchronous
	 * alarm. The process number is stored in timer argument 'ta_int'. Notify that
 	* process with a notification message from CLOCK.
 	*/
  	int proc_nr_e = tp->tmr_arg.ta_int;	/* get process endpoint */
TASKDEBUG("proc_nr_e=%d\n",proc_nr_e);

  	mnx_src_notify(CLOCK, proc_nr_e);	/* notify process as CLOCK */
}

#endif /* USE_SETALARM */
