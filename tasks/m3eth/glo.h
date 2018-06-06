/* EXTERN should be extern except for the table file */
#ifdef _TABLE
#define EXTERN
#else
#define EXTERN extern
#endif

EXTERN int local_nodeid;
EXTERN int dcid;
EXTERN drvs_usr_t drvs, *drvs_ptr;
EXTERN pid_t eth_lpid;
EXTERN pid_t main_lpid;
EXTERN DC_usr_t  vmu, *dc_ptr;

EXTERN proc_usr_t eth, *eth_ptr;	
EXTERN proc_usr_t inet, *inet_ptr;
EXTERN message *mi_ptr, *mo_ptr;

EXTERN eth_card_t ec_table[EC_PORT_NR_MAX];

/* =============== global variables =============== */
EXTERN struct eth_interface  *ei_ptr;
EXTERN char eth_iface[sizeof(struct eth_interface)];
EXTERN int rx_slot_nr = 0;          /* Rx-slot number */
EXTERN int tx_slot_nr = 0;          /* Tx-slot number */
EXTERN int cur_tx_slot_nr = 0;      /* Tx-slot number */
EXTERN char isstored[TX_RING_SIZE]; /* Tx-slot in-use */
EXTERN char *progname;
EXTERN long clockTicks;
EXTERN unsigned int iface_rqst;
EXTERN unsigned int rqst_source;

EXTERN pthread_mutex_t 	m3ipc_mutex;
EXTERN pthread_mutex_t 	wakeup_mutex;
EXTERN pthread_mutex_t 	main_mutex;
EXTERN pthread_cond_t 	main_barrier;
EXTERN pthread_cond_t 	inet2eth_barrier;

EXTERN pthread_cond_t 	receive_barrier[EC_PORT_NR_MAX];

EXTERN pthread_mutex_t 	send_mutex[EC_PORT_NR_MAX];
EXTERN pthread_mutex_t 	smain_mutex[EC_PORT_NR_MAX];
EXTERN pthread_cond_t 	smain_barrier[EC_PORT_NR_MAX];
EXTERN pthread_cond_t 	send_barrier[EC_PORT_NR_MAX];

	