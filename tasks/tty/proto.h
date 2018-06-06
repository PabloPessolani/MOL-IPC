/* Function prototypes. */

u64_t cvul64(unsigned long x32);

unsigned long cv64ul(u64_t i);

unsigned long div64u(u64_t i, unsigned j);

int init_replicate(void);
void *replicate_main(void *arg);
int replica_loop(int *mtype, char *source);

int sp_join( int new_mbr);
int sp_disconnect(int  disc_mbr);
int sp_net_partition(void);
int sp_net_merge(void);

int rep_dev_scatter( message *sp_ptr);
int rep_dev_write(SP_message *sp_ptr);
int rep_task_reply( message *sp_ptr);
int rep_dev_open( message *sp_ptr);
int rep_dev_close( message *sp_ptr);
int rep_dev_ioctl( message *sp_ptr);
int rep_cancel( message *sp_ptr);
int rep_select( message *sp_ptr);

int mc_status_info( message *sp_ptr);
int mc_synchronized( message *sp_ptr);

int get_nodeid(char *mbr_string);
int replica_updated(int localnodeid);
int send_synchronized(void);
int send_status_info(void);



