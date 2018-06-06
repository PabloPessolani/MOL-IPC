#ifndef NODE_USR_H
#define NODE_USR_H

#define MAXNODENAME	16


#define NODE_BIT_ATTACHED		1	/* The node is attached to a pair of proxies 	*/
#define NODE_BIT_SCONNECTED		2	/* The proxy sender is connected		*/
#define NODE_BIT_RCONNECTED		3	/* The proxy receiver is connected		*/

#define NODE_FREE		0
#define NODE_ATTACHED	(1<<NODE_BIT_ATTACHED)
#define NODE_SCONNECTED	(1<<NODE_BIT_SCONNECTED)
#define NODE_RCONNECTED	(1<<NODE_BIT_RCONNECTED)

struct node_usr {
	int			n_nodeid;
	volatile unsigned long	n_flags;
	int			n_proxies;		/* proxies ID for this node		*/
	unsigned long int  	n_dcs; 			/* BITMAP 				*/
	struct timespec 	n_stimestamp;		/* timestamp of the last sent  msg	*/
	struct timespec 	n_rtimestamp;		/* timestamp of the last received msg	*/
	unsigned long n_pxsent; 		/* proxy message sent to that node 	*/
	unsigned long n_pxrcvd; 		/* proxy message received from the node */ 
	char			n_name[MAXNODENAME];
};

typedef struct node_usr node_usr_t;

#define NODE_USR_FORMAT "n_nodeid=%d n_proxies=%d n_flags=%lX n_dcs=%lX n_name=%s\n"
#define NODE_USR_FIELDS(p) p->n_nodeid, p->n_proxies, p->n_flags, p->n_dcs, p->n_name

#define NODE_TIME_FORMAT "n_nodeid=%d n_name=%s n_stime_sec=%ld n_stime_nsec=%ld n_rtime_sec=%ld n_rtime_nsec=%ld\n"
#define NODE_TIME_FIELDS(p) p->n_nodeid, p->n_name, p->n_stimestamp.tv_sec, p->n_stimestamp.tv_nsec, \
			p->n_rtimestamp.tv_sec, p->n_rtimestamp.tv_nsec

#endif /* NODE_USR_H */