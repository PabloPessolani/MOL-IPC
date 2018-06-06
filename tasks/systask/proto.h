/******************************************************************/
/* 				PROTO.H				*/
/******************************************************************/

int systask_init(int dcid);
int bind_rmt_systask(int node);
int unbind_rmt_sysproc(int endpoint);

int dc_init(int argc, char *argv[]);

int init_clock(int dcid);
static void clock_handler(int sig);
int set_clock_handler(struct timeval *p_itv);
long walltime(void);
molclock_t get_uptime(void);

void cause_alarm(moltimer_t *tp);

int init_spread(void);
void *slots_read_thread(void *);
int spread_read(int *mtype, char *source);
int mcast_rqst_slots(int nr_slots);




