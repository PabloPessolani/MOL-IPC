/* EXTERN should be extern except in table.c */
#ifdef _TABLE
#define EXTERN
#else
#define EXTERN extern
#endif

/* Global variables. */
EXTERN mproc_t *mp;		/* ptr to 'mproc' slot of current process */
EXTERN proc_usr_t *kp;		/* ptr to 'kproc' slot of current process */
EXTERN int procs_in_use;	/* how many processes are marked as IN_USE */

/* The parameters of the call are kept here. */
EXTERN message m_in __attribute__((aligned(0x1000)));		/* the incoming message itself is kept here. */
EXTERN message m_out __attribute__((aligned(0x1000))) ;		/* the outgoing message is kept here. */
EXTERN message *m_ptr;		/* pointer to message */

EXTERN int who_p, who_e;	/* caller's proc number, endpoint */
EXTERN int call_nr;		/* system call number */

extern _PROTOTYPE (int (*call_vec[]), (void) );	/* system call handlers */
extern char core_name[];	/* file name where core images are produced */
EXTERN sigset_t core_sset;	/* which signals cause core images */
EXTERN sigset_t ign_sset;	/* which signals are by default ignored */

EXTERN mproc_t *mproc;		/* PM process table			*/
EXTERN proc_usr_t *kproc;	/* Kernel process table (in userspace)	*/
EXTERN priv_usr_t *kpriv;	/* Kernel priviledge table (in userspace)*/
EXTERN slot_t *slots;		/* systask slots allocation table (in userspace)	*/

EXTERN int pm_lpid;		
EXTERN int pm_ep;		
EXTERN int pm_nr;	

EXTERN dvs_usr_t dvs, *dvs_ptr;
EXTERN dc_usr_t  dcu, *dc_ptr;
EXTERN proc_usr_t *proc_ptr;
EXTERN priv_usr_t *priv_ptr;

EXTERN int local_nodeid;
EXTERN struct sysinfo info;
EXTERN int next_child;

EXTERN long clockTicks;
EXTERN time_t boottime;


	