/* EXTERN should be extern except for the table file */
#ifdef _TABLE
#define EXTERN
#else
#define EXTERN extern
#endif

EXTERN int tty_lpid;		
EXTERN int tty_ep;		
EXTERN int who_e, who_p;
EXTERN drvs_usr_t drvs, *drvs_ptr;
EXTERN DC_usr_t  vmu, *dc_ptr;
EXTERN proc_usr_t proc_tty, *tty_ptr;	
EXTERN int local_nodeid;
EXTERN int cfg_tty_nr;
EXTERN unsigned int mandatory;
EXTERN message tty_rqst, *rqst_ptr;
EXTERN message tty_rply, *rply_ptr ;
EXTERN tty_t tty_table[NR_VTTYS];
EXTERN moltimer_t *tty_timers;		/* queue of TTY timers */
EXTERN molclock_t tty_next_timeout;	/* next TTY timeout */

#ifdef _TTY
pthread_mutex_t tty_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t tty_barrier = PTHREAD_COND_INITIALIZER;
pthread_mutex_t primary_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t primary_barrier = PTHREAD_COND_INITIALIZER;
pthread_mutex_t update_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t update_barrier = PTHREAD_COND_INITIALIZER;

#else
extern pthread_mutex_t tty_mutex;
extern pthread_cond_t tty_barrier;
extern pthread_mutex_t primary_mutex;
extern pthread_cond_t primary_barrier;
extern pthread_mutex_t update_mutex;
extern pthread_cond_t update_barrier;
#endif



