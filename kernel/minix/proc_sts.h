#ifndef PROC_STS_H
#define PROC_STS_H

/* =================================*/
/* PROCESS DESCRIPTOR STATUS        */
/* =================================*/

/* Bits for the runtime flags. A process is runnable iff p_rts_flags == 0. 	*/
#define PROC_RUNNING	0x00000000

#define BIT_SLOT_FREE	0
#define BIT_NO_MAP		1
#define BIT_SENDING		2
#define BIT_RECEIVING	3

#define BIT_SIGNALED	4	
#define BIT_SIG_PENDING	5
#define BIT_P_STOP		6
#define BIT_NO_PRIV		7

#define BIT_NO_PRIORITY 8
#define BIT_NO_ENDPOINT 9
#define BIT_ONCOPY		10
#define BIT_MIGRATE		11

#define BIT_REMOTE		12
#define BIT_RMTOPER		13		
#define BIT_FREE14		14
#define BIT_WAITMIGR	15
		
#define BIT_FREE16		16
#define BIT_WAITUNBIND	17
#define BIT_WAITBIND	18

enum proc_status {
		SLOT_FREE	= (1<<BIT_SLOT_FREE),	/* process slot is free 				*/
		NO_MAP		= (1<<BIT_NO_MAP),		/* keeps unmapped forked child from running 	*/
		SENDING		= (1<<BIT_SENDING),		/* process blocked trying to send 			*/
		RECEIVING	= (1<<BIT_RECEIVING), 	/* process blocked trying to receive 		*/
		
		SIGNALED	= (1<<BIT_SIGNALED),	/* set when new kernel signal arrives 		*/
		SIG_PENDING	= (1<<BIT_SIG_PENDING),	/* unready while signal being processed 	*/
		P_STOP		= (1<<BIT_P_STOP),		/* set when process is being traced 		*/
		NO_PRIV		= (1<<BIT_NO_PRIV),		/* keep forked system process from running 	*/
		
		NO_PRIORITY	= (1<<BIT_NO_PRIORITY),	/* process has been stopped 			*/
		NO_ENDPOINT = (1<<BIT_NO_ENDPOINT),	/* process cannot send or receive messages 	*/
		ONCOPY	    = (1<<BIT_ONCOPY), 		/* A copy request is pending				*/
		MIGRATING   = (1<<BIT_MIGRATE),		/* the process is waiting that it ends its 	*/
											/* MIGRATION 								*/
		
		REMOTE	    = (1<<BIT_REMOTE),		/* the process is running on a remote host	*/
		RMTOPER		= (1<<BIT_RMTOPER),		/* a process descriptor is just used for a 	*/
											/* remote operation until the Sender PROXY  */
											/* completes the request 					*/
		WAITMIGR	= (1<<BIT_WAITMIGR),	/* a destination process is MIGRATING, the 	*/
											/* sender must be blocked and enqueued into */
											/* the migrating queue until the process	*/
											/* finish the migration 					*/
		WAITUNBIND	= (1<<BIT_WAITUNBIND),	/* a process is waiting another unbinding 	*/
											/* it must be blocked and enqueued into 	*/
											/* the waiting queue until the other process*/
											/* finish the unbinding 					*/
		WAITBIND	= (1<<BIT_WAITBIND),	/* a process is waiting another binding 	*/
											/* it must be blocked and enqueued into 	*/
											/* the waiting queue until the other process*/
											/* is bound 					*/
};

/* Bits for the p_misc_flags  */
#define GENERIC_PROC	0

#endif /* PROC_STS_H */
