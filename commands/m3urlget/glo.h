/* EXTERN should be extern except for the table file */
#ifdef _TABLE
#define EXTERN
#else
#define EXTERN extern
#endif

EXTERN int web_lpid;		
EXTERN int web_ep;		
EXTERN int who_e, who_p;
EXTERN dvs_usr_t dvs, *dvs_ptr;
EXTERN VM_usr_t  vmu, *dc_ptr;
EXTERN proc_usr_t proc_web, *web_ptr;	
EXTERN int local_nodeid;
EXTERN int cfg_web_nr;
EXTERN unsigned int mandatory;
EXTERN message web_rqst, *rqst_ptr;
EXTERN message web_rply, *rply_ptr ;
EXTERN web_t web_table[NR_WEBSRVS];
EXTERN sess_t sess_table[NR_SESSIONS];
EXTERN moltimer_t *web_timers;		/* queue of WEBSRV timers */
EXTERN molclock_t web_next_timeout;	/* next WEBSRV timeout */
EXTERN int sess_reset;

#ifdef _TTY
pthread_mutex_t web_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t web_barrier = PTHREAD_COND_INITIALIZER;
pthread_mutex_t primary_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t primary_barrier = PTHREAD_COND_INITIALIZER;
pthread_mutex_t update_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t update_barrier = PTHREAD_COND_INITIALIZER;

#else
extern pthread_mutex_t web_mutex;
extern pthread_cond_t web_barrier;
extern pthread_mutex_t primary_mutex;
extern pthread_cond_t primary_barrier;
extern pthread_mutex_t update_mutex;
extern pthread_cond_t update_barrier;
#endif



