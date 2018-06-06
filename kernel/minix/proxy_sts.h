
#define MIS_BIT_PROXY		0
#define MIS_BIT_CONNECTED	1
#define MIS_BIT_NOTIFY		2
#define MIS_BIT_NEEDMIGR	3

#define MIS_BIT_RMTBACKUP	4
#define MIS_BIT_GRPLEADER	5
#define MIS_BIT_KTHREAD		6
#define MIS_BIT_REPLICATED 	7

#define MIS_BIT_KILLED	 	8
#define MIS_BIT_WOKENUP	9

enum mis_status {
		MIS_PROXY		= (1<<MIS_BIT_PROXY),		/* the process is a proxy 			*/
		MIS_CONNECTED	= (1<<MIS_BIT_CONNECTED),	/* The proxy is connected 			*/
		MIS_NOTIFY		= (1<<MIS_BIT_NOTIFY),		/* A notify is pending 	 			*/
		MIS_NEEDMIG		= (1<<MIS_BIT_NEEDMIGR), 	/* The proccess need to migrate		*/
		MIS_RMTBACKUP	= (1<<MIS_BIT_RMTBACKUP), 	/* The proccess is a remote process' backup	*/
		MIS_GRPLEADER	= (1<<MIS_BIT_GRPLEADER), 	/* The proccess is the thread group leader 	*/	
		MIS_KTHREAD		= (1<<MIS_BIT_KTHREAD), 	/* The proccess is a KERNEL thread 	*/	
		MIS_REPLICATED	= (1<<MIS_BIT_REPLICATED), 	/* The ep is LOCAL but it is replicated on other nodes */	
		MIS_KILLED		= (1<<MIS_BIT_KILLED), 		/* The process has been killed 		*/
		MIS_WOKENUP 	= (1<<MIS_BIT_WOKENUP), 	/* The process has been signaled 	*/
};

#define PX_BIT_INUSE		0
#define PX_BIT_SCONNECTED	1
#define PX_BIT_RCONNECTED	2

#define PROXIES_FREE		0x0000
enum px_status {
		PROXIES_INUSE		= (1<<PX_BIT_INUSE),		/* the proxy pair is in use 		*/
		PROXIES_SCONNECTED	= (1<<PX_BIT_SCONNECTED),	
		PROXIES_RCONNECTED	= (1<<PX_BIT_RCONNECTED),	
};




