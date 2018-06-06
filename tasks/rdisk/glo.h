/* EXTERN should be extern except for the table file */
#ifdef _TABLE
#define EXTERN
#else
#define EXTERN extern
#endif


/* The parameters of the call are kept here. */
EXTERN message m_in;		/* the input message itself */
EXTERN message m_out;		/* the output message used for reply */
EXTERN message *m_ptr;		/* pointer to message */

EXTERN int who_p, who_e;	/* caller's proc number, endpoint */
EXTERN int call_nr;		/* system call number */

EXTERN int img_size;		/* testing image file size */
EXTERN char *img_ptr;		/* pointer to the first byte of the ram disk image */

//EXTERN fproc_t *fproc;		/* FS process table			*/

EXTERN int rd_lpid;		
EXTERN int rd_ep;		
//EXTERN int fs_nr;	
EXTERN int img_p; /*pointer to the first byte of the ram disk image*/

EXTERN int mc_lpid;
EXTERN int mc_pid;
EXTERN int mc_ep;

EXTERN int sc_lpid;
EXTERN int sc_pid;
EXTERN int sc_ep;

EXTERN dvs_usr_t dvs, *dvs_ptr;
EXTERN DC_usr_t  dcu, *dc_ptr;
EXTERN proc_usr_t proc_rd, *rd_ptr;	

EXTERN int err_code;		/* temporary storage for error number */
//EXTERN char user_path[PATH_MAX];/* storage for user path name */

/*
Ver que pasa con esto porque originalmente es un Dev_t con mayusculas
que es un int y aca lo estoy definiendo con minusculas que es un short
 */

EXTERN mnx_dev_t root_dev;		/* device number of the root device */ 

EXTERN char *img_name;		/* name of the ram disk image file*/
EXTERN int local_nodeid;

// EXTERN int d_minor;		/* minor number of device*/

struct super_block *sb_ptr; /*Super block pointer*/

// EXTERN mnx_time_t boottime;		/* time in seconds at system boot */

/* The following variables are used for returning results to the caller. */
EXTERN int rdwt_err;		/* status of last disk i/o request */


EXTERN  pthread_t replicate_thread;
EXTERN  pthread_t mastercopy_thread;
EXTERN  pthread_t slavecopy_thread;
EXTERN int dcid;
EXTERN char replica_name[MAXNODENAME]; /* RDISKnn.vv  nn=nodeid vv=dcidi*/

EXTERN mailbox sysmbox;		/* SPREAD MAILBOX for SYSTASK */	
EXTERN  char	User[80];
EXTERN char		 mess_in[MAX_MESSLEN];
EXTERN char		 mess_out[MAX_MESSLEN];
EXTERN  char    Spread_name[80];
EXTERN  char    Private_group[MAX_GROUP_NAME];
EXTERN  char    first_name[MAX_GROUP_NAME];

EXTERN 		int synchronized;
EXTERN		int	FSM_state;		/* Finite State Machine state */
EXTERN 		int primary_mbr;

EXTERN    membership_info  memb_info;
EXTERN    vs_set_info      vssets[MAX_VSSETS];
EXTERN    unsigned int     my_vsset_index;
EXTERN    int              num_vs_sets;
EXTERN    char             members[MAX_MEMBERS][MAX_GROUP_NAME];
EXTERN	  char		   sp_members[MAX_MEMBERS][MAX_GROUP_NAME];
EXTERN	  int		   sp_nr_mbrs;

EXTERN 		int nr_nodes;
// EXTERN		int active_nr_nodes; 
// EXTERN		int active_nodes; /*ver si sólo dejo este, este se cuenta en la systask*/
EXTERN 		int nr_sync;
EXTERN 		int count_availables; /*numbers minor devices availables*/
EXTERN 		unsigned long int nr_optrans; /*numers transfer operations*/
EXTERN	  	unsigned long int	bm_acks;
EXTERN	  	unsigned long int	bm_nodes;
EXTERN	  	unsigned long int	bm_sync;


#ifdef _RDISK
pthread_mutex_t rd_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t rd_barrier = PTHREAD_COND_INITIALIZER;
pthread_mutex_t primary_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t primary_barrier = PTHREAD_COND_INITIALIZER;
pthread_mutex_t update_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t update_barrier = PTHREAD_COND_INITIALIZER;

#else
extern pthread_mutex_t rd_mutex;
extern pthread_cond_t rd_barrier;
extern pthread_mutex_t primary_mutex;
extern pthread_cond_t primary_barrier;
extern pthread_mutex_t update_mutex;
extern pthread_cond_t update_barrier;
#endif

extern _PROTOTYPE (int (*call_vec[]), (void) ); /* sys call table */


