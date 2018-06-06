/*
inet/generic/event.c

Created:	April 1995 by Philip Homburg <philip@f-mnx.phicoh.com>

Implementation of an event queue.

Copyright 1995 Philip Homburg
*/

#include "../inet.h"
#include "assert.h"
#include "event.h"

EXTERN event_t *ev_head;

void ev_init(event_t *ev)
{
	ev->ev_func= 0;
	ev->ev_next= NULL;
}

void ev_enqueue(event_t *ev,ev_func_t func,ev_arg_t ev_arg)
{
	assert(ev->ev_func == 0);
	ev->ev_func= func;
	ev->ev_arg= ev_arg;
	ev->ev_next= NULL;
	if (ev_head == NULL)
		ev_head= ev;
	else
		ev_tail->ev_next= ev;
	ev_tail= ev;
}

void ev_process(void)
{
	ev_func_t func;
	event_t *curr;

	while (ev_head) {
		curr= ev_head;
		ev_head= curr->ev_next;
		func= curr->ev_func;
		curr->ev_func= 0;

		assert(func != 0);
		func(curr, curr->ev_arg);
	}
}

int ev_in_queue(event_t *ev)
{
	return ev->ev_func != 0;
}
