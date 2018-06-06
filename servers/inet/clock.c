/*
clock.c

Copyright 1995 Philip Homburg
*/

#include "inet.h"
#include "proto.h"
#include "generic/assert.h"
#include "generic/buf.h"
#include "generic/clock.h"
#include "generic/type.h"

 int clck_call_expire;

 mnx_time_t curr_time;
 mnx_time_t prev_time;
 inet_timer_t *timer_chain;
 mnx_time_t next_timeout;

void clck_fast_release(inet_timer_t *timer);
void set_timer(void);

void clck_init(void)
{
	int r;
	SVRDEBUG("\n");

	clck_call_expire= 0;
	curr_time= 0;
	prev_time= 0;
	next_timeout= 0;
	timer_chain= 0;
}

mnx_time_t get_time(void)
{
	if (!curr_time)
	{
		int s;
		if ((curr_time = mol_time(&curr_time)) < 0)
			ERROR_EXIT(curr_time);
		assert(curr_time >= prev_time);
	}
	return curr_time;
}

void set_time (mnx_time_t tim)
{
	if (!curr_time && tim >= prev_time)
	{
		/* Some code assumes that no time elapses while it is
		 * running.
		 */
		curr_time= tim;
	}
	else if (!curr_time)
	{
		DBLOCK(0x20, printf("set_time: new time %ld < prev_time %ld\n",
			tim, prev_time));
	}
}

void reset_time(void)
{
	prev_time= curr_time;
	curr_time= 0;
}

void clck_timer(inet_timer_t *timer,mnx_time_t timeout,timer_func_t func,int fd)
{
	inet_timer_t *timer_index;

	if (timer->tim_active)
		clck_fast_release(timer);
	assert(!timer->tim_active);

	timer->tim_next= 0;
	timer->tim_func= func;
	timer->tim_ref= fd;
	timer->tim_time= timeout;
	timer->tim_active= 1;

	if (!timer_chain)
		timer_chain= timer;
	else if (timeout < timer_chain->tim_time)
	{
		timer->tim_next= timer_chain;
		timer_chain= timer;
	}
	else
	{
		timer_index= timer_chain;
		while (timer_index->tim_next &&
			timer_index->tim_next->tim_time < timeout)
			timer_index= timer_index->tim_next;
		timer->tim_next= timer_index->tim_next;
		timer_index->tim_next= timer;
	}
	if (next_timeout == 0 || timer_chain->tim_time < next_timeout)
		set_timer();
}

void clck_tick (message *mess)
{
	next_timeout= 0;
	set_timer();
}

void clck_fast_release (inet_timer_t *timer)
{
	inet_timer_t *timer_index;

	if (!timer->tim_active)
		return;

	if (timer == timer_chain)
		timer_chain= timer_chain->tim_next;
	else
	{
		timer_index= timer_chain;
		while (timer_index && timer_index->tim_next != timer)
			timer_index= timer_index->tim_next;
		assert(timer_index);
		timer_index->tim_next= timer->tim_next;
	}
	timer->tim_active= 0;
}

 void set_timer(void)
{
	mnx_time_t new_time;
	mnx_time_t curr_time;

	if (!timer_chain)
		return;

	curr_time= get_time();
	new_time= timer_chain->tim_time;
	if (new_time <= curr_time)
	{
		clck_call_expire= 1;
		return;
	}

	if (next_timeout == 0 || new_time < next_timeout)
	{
		next_timeout= new_time;
		new_time -= curr_time;

		if (sys_setalarm(new_time, 0) != OK)
  			ip_panic(("can't set timer"));
	}
}

 void clck_untimer (inet_timer_t *timer)
{
	clck_fast_release (timer);
	set_timer();
}

 void clck_expire_timers(void)
{
	mnx_time_t curr_time;
	inet_timer_t *timer_index;

	clck_call_expire= 0;

	if (timer_chain == NULL)
		return;

	curr_time= get_time();
	while (timer_chain && timer_chain->tim_time<=curr_time)
	{
		assert(timer_chain->tim_active);
		timer_chain->tim_active= 0;
		timer_index= timer_chain;
		timer_chain= timer_chain->tim_next;
		(*timer_index->tim_func)(timer_index->tim_ref, timer_index);
	}
	set_timer();
}

/*
 * $PchId: clock.c,v 1.10 2005/06/28 14:23:40 philip Exp $
 */
