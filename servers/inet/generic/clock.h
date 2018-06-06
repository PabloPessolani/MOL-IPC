/*
clock.h

Copyright 1995 Philip Homburg
*/

#ifndef CLOCK_H
#define CLOCK_H

struct inet_timer;

typedef void (*timer_func_t) ( int fd, struct inet_timer *timer );

typedef struct inet_timer
{
	struct inet_timer *tim_next;
	timer_func_t tim_func;
	int tim_ref;
	mnx_time_t tim_time;
	int tim_active;
} inet_timer_t;


void clck_init ( void );
void set_time ( mnx_time_t time );
mnx_time_t get_time ( void );
void reset_time ( void );
/* set a timer to go off at the time specified by timeout */
void clck_timer ( inet_timer_t *timer, mnx_time_t timeout, timer_func_t func,
								int fd );
void clck_untimer ( inet_timer_t *timer );
void clck_expire_timers ( void );

#endif /* CLOCK_H */

/*
 * $PchId: clock.h,v 1.5 1995/11/21 06:45:27 philip Exp $
 */
