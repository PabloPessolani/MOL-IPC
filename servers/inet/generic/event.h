#ifndef INET__GENERIC__EVENT_H
#define INET__GENERIC__EVENT_H
 
struct event;

typedef union ev_arg
{
	int ev_int;
	void *ev_ptr;
} ev_arg_t;

typedef void (*ev_func_t) ( struct event *ev, union ev_arg eva );

typedef struct event
{
	ev_func_t ev_func;
	ev_arg_t ev_arg;
	struct event *ev_next;
} event_t;

extern event_t *ev_head;

void ev_init( event_t *ev );
void ev_enqueue( event_t *ev, ev_func_t func, ev_arg_t ev_arg );
void ev_process( void );
int ev_in_queue( event_t *ev );

#endif /* INET__GENERIC__EVENT_H */
