#ifdef _SYSTEM
#define EXTERN
#else
#define EXTERN extern
#endif


EXTERN dvs_usr_t dvs, *dvs_ptr;
EXTERN dc_usr_t dcu, dcu_primary, *dc_ptr, *dcf_ptr;
EXTERN node_usr_t node, *node_ptr;
EXTERN int local_nodeid;

EXTERN molclock_t realtime;               	/* Minix real time clock */
EXTERN long clockTicks;
EXTERN molclock_t realtime;               	/* Minix real time clock 			*/
EXTERN molclock_t next_timeout;		/* realtime that next timer expires 		*/
EXTERN long init_time;
EXTERN struct sysinfo info;
EXTERN int pagesize;

EXTERN int who_e, who_p;		/* message source endpoint and proc */
EXTERN int sys_nr, sys_ep, sys_pid;
EXTERN int clock_ep, clock_pid;

#ifdef ALLOC_LOCAL_TABLE 			
EXTERN proc_usr_t *proc;		/* SYSTASK LOCAL process table 	*/
#else /* ALLOC_LOCAL_TABLE */			
EXTERN char     *kproc;		/* KERNEL memory mapped process table	*/
#endif /* ALLOC_LOCAL_TABLE */

EXTERN priv_usr_t *priv;		/* Privileges table		*/
EXTERN slot_t  	  *slot; 		/* slot descriptor table */
EXTERN slot_t  	  *slot_merge; 	/* slot descriptor table for merging */

EXTERN int debug_fd;
EXTERN char *proc_addr;

EXTERN message m, *m_ptr;
EXTERN sigjmp_buf senv;
EXTERN int next_child;

EXTERN int active_nodes;	/* current number of nodes that conform the DC (0 < active_nodes <= dc_nr_nodes)*/
EXTERN int init_nodes;		/* current number of nodes that conform the DC (0 <  init_nodes <= active_nodes <= dc_nr_nodes)*/

EXTERN int free_slots;		/* current number of free_slots for non system processes in local node (dynamic)*/
EXTERN int free_slots_low;	/* Threadhold to trigger the slot donation protocol (free_slots<=free_lots_low) */
EXTERN volatile int owned_slots;		/* current number of owned slots for non system processes in local node (dynamic)*/

EXTERN int total_slots;		/*Total number of slots for non system processes for all DC nodes */
EXTERN int min_owned_slots;		/*Minimum number of slots for non system processes for local node */
EXTERN int max_owned_slots;		/*Maximum number of slots for non system processes for local node */
EXTERN int don_loops;

EXTERN mailbox sysmbox;		/* SPREAD MAILBOX for SYSTASK */	
EXTERN char		 mess_in[MAX_MESSLEN];
EXTERN char		 mess_out[MAX_MESSLEN];
EXTERN  char    Spread_name[80];
EXTERN  char    Private_group[MAX_GROUP_NAME];
EXTERN  char    first_name[MAX_GROUP_NAME];
EXTERN  char	User[80];
// EXTERN 	int		first_act_mbr;			/* first active member  */
EXTERN  int		primary_mbr;			/* primary member  */
EXTERN 	int 	waitsts_mbr;		/* next member to send STS_WAIT_STATUS */

EXTERN  pthread_t slots_thread; 		
EXTERN	int		FSM_state;		/* Finite State Machine state */

EXTERN	int		SP_bytes;		/* bytes returned by SP_receive */

EXTERN    membership_info  memb_info;
EXTERN    vs_set_info      vssets[MAX_VSSETS];
EXTERN    unsigned int     my_vsset_index;
EXTERN    int              num_vs_sets;
EXTERN    char             members[MAX_MEMBERS][MAX_GROUP_NAME];
EXTERN	  char		   sp_members[MAX_MEMBERS][MAX_GROUP_NAME];
EXTERN	  int		   sp_nr_mbrs;

EXTERN	  unsigned long int			bm_donors;		/* bitmap  of donors  members */
EXTERN	  unsigned long int			bm_init;		/* bitmap of initialized  members */
EXTERN	  unsigned long int			bm_active;		/* bitmap of active members */
EXTERN	  unsigned long int			bm_pending;		/* bitmap of pending donating ZERO replies */
EXTERN	  unsigned long int			bm_waitsts;		/* bitmap of members waiting SYS_PUT_STATUS */
EXTERN	  unsigned long int			bm_waitsts_mrg;	/* Temporal bitmap of initialized  members during merge */
EXTERN	  unsigned long int			bm_init_mrg;	/* Temporal bitmap of members waiting SYS_PUT_STATUS during merge */	

EXTERN 	int	sts_size;
EXTERN	char *sts_ptr;			/* pointer to the start of the Global status */
EXTERN	char *msg_ptr;			/* pointer to the header message before DC status and GST */
EXTERN	char *rst_ptr;			/* pointer to the shared slot table within Global status */

EXTERN 	time_t 	last_rqst;		/* time of last slot request */

#ifdef _SYSTEM
pthread_mutex_t sys_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t sys_barrier = PTHREAD_COND_INITIALIZER;
pthread_cond_t init_barrier = PTHREAD_COND_INITIALIZER;
pthread_cond_t sys_syscall = PTHREAD_COND_INITIALIZER;
#else
extern pthread_mutex_t sys_mutex;
extern pthread_cond_t sys_barrier;
extern pthread_cond_t fork_barrier;
extern pthread_cond_t init_barrier;
extern pthread_cond_t sys_syscall;
#endif

