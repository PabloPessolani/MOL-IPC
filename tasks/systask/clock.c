/******************************************************************/
/* 				CLOCK				*/
/* Unlike MINIX where CLOCK is a task, CLOCK is a set of 	*/
/* functions related to system time and alarms managed instead of*/
/* a Timer interrupt by a Linux ALARM Signal 			*/ 
/******************************************************************/

#include "systask.h"

moltimer_t *clock_timers;		/* queue of CLOCK timers */

/**===========================================================================*
*                             set_shot	                                    *
*===========================================================================**/
int  set_shot(struct timeval *p_new)
{
	struct itimerval old_itv, new_itv;
	int rcode; 

TASKDEBUG("\n");

	rcode = getitimer(ITIMER_REAL, &new_itv);
	if ( rcode) ERROR_RETURN(rcode);

	new_itv.it_interval.tv_sec 	= 0;
	new_itv.it_interval.tv_usec 	= 0;
	new_itv.it_value.tv_sec 	= p_new->tv_sec;
	new_itv.it_value.tv_usec 	= p_new->tv_usec;

TASKDEBUG("new_sec=%lu new_usec=%lu\n", p_new->tv_sec,p_new->tv_usec);  

	rcode = setitimer(ITIMER_REAL, &new_itv, 0);
	if ( rcode) ERROR_RETURN(errno);
		
	return(OK);
}

/**===========================================================================*
*                             set_interval                                    *
*===========================================================================**/
int  set_interval(struct timeval *p_new)
{
	struct itimerval old_itv, new_itv;
	int rcode; 

TASKDEBUG("\n");

	new_itv.it_interval.tv_sec 	= p_new->tv_sec;
	new_itv.it_interval.tv_usec 	= p_new->tv_usec;
	new_itv.it_value.tv_sec 	= p_new->tv_sec;
	new_itv.it_value.tv_usec 	= p_new->tv_usec;

TASKDEBUG("new_sec=%lu new_usec=%lu\n", p_new->tv_sec,p_new->tv_usec);  

	rcode = setitimer(ITIMER_REAL, &new_itv, 0);
	if ( rcode) ERROR_RETURN(errno);
		
	return(OK);
}

/**===========================================================================*
*                             init_clock                                    *
*===========================================================================**/
int init_clock(int dcid)
{
	int rcode;
	struct proc_usr *proc_ptr;
	pid_t tid=0;
	struct timeval new_interval;

	/* keep track of elapsed time */
	/* initial value should be taken from host OS. 	*/
	realtime = 0;

	/* Fetch clock ticks */
	clockTicks = sysconf(_SC_CLK_TCK);
	if (clockTicks == -1)	return(errno);
TASKDEBUG("clockTicks =%ld\n",clockTicks );
	
	/* some random test alarm deadline 			*/
	next_timeout = TMR_NEVER; 
	
	/* syncronization period with LINUX timer		*/
	init_time = get_uptime();
	
	/* Initialize the CLOCK's interrupt hook. 	*/
    	/* Set timer to run every second */
	new_interval.tv_sec  = INTERVAL_SECS;
	new_interval.tv_usec = INTERVAL_USECS;
	set_clock_handler(&new_interval);
	return(OK);
}

/*============================================================================*
 *                              clock_handler                                	*
* this routine is equivalent to the clock interrupt handler 			*
 *============================================================================*/

static void clock_handler(int sig)
{
	int rcode;
	unsigned long delta_sec, delta_usec;
	struct timeval new_interval;

	/* update realtime */
	realtime = get_uptime();

TASKDEBUG("clockTicks=%d next_timeout=%ld realtime=%ld \n",clockTicks, next_timeout, realtime );

	if( 	((next_timeout-realtime) > 0) &&
		((next_timeout-realtime) < (INTERVAL_SECS*clockTicks))) {

		delta_sec = (next_timeout-realtime);		/* ticks */
TASKDEBUG("delta=%lu\n", delta_sec);  
		new_interval.tv_sec  = delta_sec/clockTicks; 	/* secs  */

		delta_usec = delta_sec%clockTicks; 		/* remaining ticks */
		delta_usec = delta_usec*1000/clockTicks;	/* milisecs */ 
		new_interval.tv_usec = delta_usec * 1000; 	/* microsecs */

TASKDEBUG("new_sec=%lu new_usec=%lu\n", new_interval.tv_sec,new_interval.tv_usec); 
 
		set_shot(&new_interval);
		return;	
	}

  	if (next_timeout > realtime) return;

	tmrs_exptimers(&clock_timers, realtime, NULL);
	
	next_timeout = (clock_timers == NULL) ? 
			TMR_NEVER : clock_timers->tmr_exp_time;

	if( next_timeout != TMR_NEVER) {
		if( 	((next_timeout-realtime) > 0) &&
			((next_timeout-realtime) < (INTERVAL_SECS*clockTicks))) { 
			new_interval.tv_sec  = (next_timeout-realtime)/clockTicks; 
			new_interval.tv_usec = (next_timeout-realtime)%clockTicks*1000;
			set_shot(&new_interval);
			return;
		}

	}

    	/* Set timer to run every second */
	new_interval.tv_sec  = INTERVAL_SECS;
	new_interval.tv_usec = INTERVAL_USECS;
	set_interval(&new_interval);
}

/*===========================================================================*
 *                              set_clock_handler                            *
 * Sets the ALARM signal handler 					     *
 *===========================================================================*/
int set_clock_handler(struct timeval *p_itv)
{
	struct sigaction sa;
	int rcode;

TASKDEBUG("\n");
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	sa.sa_handler = clock_handler;
	sa.sa_flags = SA_RESTART;
	if ((rcode=sigaction(SIGALRM, &sa, NULL)) == -1)
      		ERROR_RETURN(rcode);

	rcode = set_interval(p_itv);
	if(rcode) ERROR_RETURN(rcode);
	return(OK);
}

/*===========================================================================*
 *                              walltime		                     *
 * Returns time of day							     *
 *===========================================================================*/
long walltime()
{
	struct timeval tv;
	gettimeofday(&tv,NULL);
	return((unsigned long int) tv.tv_sec);
}

/*===========================================================================*
 *				get_uptime				     *
 * Returns the number of CLOCK TICKS (jiffies) since UPTIME 		     *
 *===========================================================================*/
molclock_t get_uptime(void)
{
	int rcode;
	 struct timespec t; 
	long long tt, td;

//	rcode = sysinfo(&info); 
//	if( rcode) ERROR_EXIT(errno);
//	return(info.uptime); 

	clock_gettime(CLOCK_MONOTONIC, &t);
	td = 1000000000/clockTicks;
	tt =  t.tv_nsec / td;
	tt += (t.tv_sec * clockTicks);

TASKDEBUG("tv_sec=%llu tv_nsec=%llu uptime=%lld \n", (unsigned long long)t.tv_sec, 
	(unsigned long long)t.tv_nsec,
	tt);
	return(tt);
}

/*===========================================================================*
 *				set_timer				     *
 *===========================================================================*/
void set_timer(moltimer_t *tp, molclock_t exp_time, tmr_func_t watchdog)
{
/* Insert the new timer in the active timers list. Always update the 
 * next timeout time by setting it to the front of the active list.
 */
TASKDEBUG("exp_time=%d\n",exp_time);
  tmrs_settimer(&clock_timers, tp, exp_time, watchdog, NULL);
  next_timeout = clock_timers->tmr_exp_time;
TASKDEBUG("realtime=%d exp_time=%d next_timeout=%ld \n",(get_uptime()-init_time), exp_time, next_timeout);

}

/*===========================================================================*
 *				reset_timer				     *
 *===========================================================================*/
void reset_timer(moltimer_t *tp)		/* pointer to timer structure */
{
/* The timer pointed to by 'tp' is no longer needed. Remove it from both the
 * active and expired lists. Always update the next timeout time by setting
 * it to the front of the active list.
 */
  tmrs_clrtimer(&clock_timers, tp, NULL);
  next_timeout = (clock_timers == NULL) ? 
	TMR_NEVER : clock_timers->tmr_exp_time;
}




