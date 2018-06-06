/****************************************************************/
/*			MOL FUNCTION PROTOTYPES 			*/
/****************************************************************/

//int check_lock_caller(struct task_struct **t_ptr, struct proc **c_ptr, int *c_pid, dc_desc_t *dc_ptr);
int check_caller(struct task_struct **t_ptr, struct proc **c_ptr, int *c_pid);


void init_proc_desc(struct proc *proc_ptr, int dcid, int index);
long do_dc_end(dc_desc_t *dc_ptr);
asmlinkage long do_unbind(dc_desc_t *dc_ptr, struct proc *proc_ptr);

asmlinkage long send_rmt2lcl(proxy_hdr_t *usr_h_ptr, proxy_hdr_t *h_ptr, struct proc *src_ptr, struct proc *dst_ptr);
asmlinkage long notify_rmt2lcl(proxy_hdr_t *usr_h_ptr, proxy_hdr_t *h_ptr, struct proc *src_ptr,struct proc *dst_ptr, int update_proc);
asmlinkage long send_ack_rmt2lcl(struct proc *src_ptr,struct proc *dst_ptr, int rcode);
asmlinkage long copyout_data_rmt2lcl(struct proc *caller_ptr, struct proc *src_ptr,struct proc *dst_ptr, 
		proxy_hdr_t *h_ptr, proxy_payload_t *usr_pay_ptr);
asmlinkage long copyin_data_rmt2lcl(struct proc *caller_ptr, struct proc *src_ptr, struct proc *dst_ptr, 
		proxy_hdr_t *h_ptr, proxy_payload_t *usr_pay_ptr);
asmlinkage long copyout_rqst_rmt2lcl( struct proc *caller_ptr, struct proc *src_ptr, struct proc *dst_ptr, 
		proxy_hdr_t *h_ptr, proxy_payload_t *usr_pay_ptr);
asmlinkage long copylcl_rqst_lcl2lcl(struct proc *rproxy_ptr, struct proc *rmt_ptr, 
		struct proc *lcl_ptr, proxy_hdr_t *h_ptr, proxy_payload_t *usr_pay_ptr);
asmlinkage int k_proxies_bind(char *px_name, int px_nr, int spid, int rpid , int maxcopybuf);
asmlinkage int mm_proxy_conn(int px_nr, int status);
asmlinkage long mm_put2lcl(proxy_hdr_t *usr_hdr_ptr, proxy_payload_t *usr_pay_ptr);
asmlinkage long mm_get2rmt(proxy_hdr_t *usr_hdr_ptr, proxy_payload_t *usr_pay_ptr, long timeout_ms);
asmlinkage long mm_node_up(char *node_name, int nodeid, int px_nr);
asmlinkage long mm_exit_unbind(long code);
asmlinkage long mm_wakeup(int dcid, int proc_ep);

long generic_ack_lcl2rmt(int ack, struct proc *rmt_ptr, struct proc *lcl_ptr, int rcode);
long generic_ack_rmt2lcl(int ack, struct proc *rmt_ptr, struct proc *lcl_ptr, int rcode);

long copyin_rqst_lcl2rmt(struct proc *dst_ptr, struct proc *lcl_ptr);	
long copyout_data_lcl2rmt(struct proc *rmt_ptr, struct proc *lcl_ptr, int rcode);
long error_lcl2rmt(int ack, struct proc *rmt_ptr, proxy_hdr_t *h_ptr,   int rcode);

long sproxy_enqueue(struct proc *proc_ptr);
long kill_unbind(struct proc *dst_ptr, struct proc *src_ptr);
// long kernel_sendrec(int srcdst_ep, message* m_ptr);

#ifdef MOLAUTOFORK
struct proc* fork_bind(struct proc *proc_ptr, int lpid);
long kernel_lclbind(int dcid, int pid, int p_nr);
#endif /*MOLAUTOFORK */

int do_proxies_unbind(struct proc *proc_ptr, struct proc *sproxy_ptr,  struct proc  *rproxy_ptr);
int do_node_end(cluster_node_t *n_ptr);
void clear_node(cluster_node_t *nptr);
void init_node(int nodeid);
void clear_proxies(proxies_t *px_ptr);
void init_proxies(int pxid);
long do_autobind(dc_desc_t *dc_ptr, struct proc *rmt_ptr, int endpoint, int nodeid);

long flush_sending_procs(int nodeid, struct proc *sproxy_ptr);
long flush_receiving_procs(int nodeid, struct proc *rproxy_ptr);

long copy_usr2usr(int source, struct proc *src_proc, char *src_addr, struct proc *dst_proc, char *dst_addr, int bytes);
#define copy_msg(src_ptr, src_msg, dst_ptr, dst_msg) \
	copy_usr2usr(src_ptr->p_endpoint, src_ptr, (char*) src_msg, dst_ptr, (char*)dst_msg, sizeof(message));
long copy_krn2usr(int source, char *src_addr, struct proc *dst_proc, char *dst_addr, int bytes);
long copy_usr2krn(int source, struct proc *src_proc, char *src_addr, char *dst_addr, int bytes);
	
void dc_release(struct kref *kref);
void dvs_release(struct kref *kref);
int sleep_proc(struct proc *proc, long timeout_ms);
int sleep_proc2(struct proc *proc, struct proc *other, long timeout_ms);	
int sleep_proc3(struct proc *proc, struct proc *other1, struct proc *other2, long timeout_ms);	
void bm2ascii( char *buf, unsigned long int bitmap);
void inherit_cpu(struct proc *proc_ptr);
void resume_migration( struct proc *proc_ptr);
proc_t *get_sproxy(int nodeid); 
proc_t *get_rproxy(int nodeid);
int lock_sr_proxies(int px_nr,  struct proc **sproxy_ptr,  struct proc  **rproxy_ptr);
int unlock_sr_proxies(int px_nr);

/* /procs read functions */
int info_read( char *page, char **start, off_t off, int count, int *eof, void *data );
int dc_info_read( char *page, char **start, off_t off, int count, int *eof, void *data );
int dc_procs_read( char *page, char **start, off_t off, int count, int *eof, void *data );
int dc_stats_read( char *page, char **start, off_t off, int count, int *eof, void *data );
int nodes_read( char *page, char **start, off_t off, int count, int *eof, void *data );
int node_info_read( char *page, char **start, off_t off, int count, int *eof, void *data );
int node_stats_read( char *page, char **start, off_t off, int count, int *eof, void *data );
int proxies_info_read( char *page, char **start, off_t off, int count, int *eof, void *data );
int proxies_procs_read( char *page, char **start, off_t off, int count, int *eof, void *data );

/* debugfs functions */
int proc_dbg_mmap(struct file *filp, struct vm_area_struct *vma);
int proc_dbg_close(struct inode *inode, struct file *filp);
int proc_dbg_open(struct inode *inode, struct file *filp);


#define copylcl_ack_rmt2lcl(x,y,z)	generic_ack_rmt2lcl(CMD_COPYLCL_ACK,x,y,z) 
#define copyrmt_ack_rmt2lcl(x,y,z) 	generic_ack_rmt2lcl(CMD_COPYRMT_ACK,x,y,z)
#define copyin_ack_rmt2lcl(x,y,z) 	generic_ack_rmt2lcl(CMD_COPYIN_ACK,x,y,z)

#define send_ack_lcl2rmt(r,l,e)   	generic_ack_lcl2rmt(CMD_SEND_ACK, r,l,e) 
#define copylcl_ack_lcl2rmt(r,l,e) 	generic_ack_lcl2rmt(CMD_COPYLCL_ACK, r,l,e)
#define copyin_ack_lcl2rmt(r,l,e) 	generic_ack_lcl2rmt(CMD_COPYIN_ACK, r,l,e)
#define copyrmt_ack_lcl2rmt(r,l,e) 	generic_ack_lcl2rmt(CMD_COPYRMT_ACK, r,l,e)

/* protos of routines to access non exported symbols */
asmlinkage long mm_sys_wait4(pid_t, int __user *,int , struct rusage __user *);
void lock_tasklist(void);
void unlock_tasklist(void);
void mm_put_task_struct(struct task_struct *);
long mm_sched_setaffinity(pid_t , const struct cpumask *);
//int k_proxies_bind(char *px_name, int px_nr, int spid, int rpid);

#ifndef MOL_MODULE
EXPORT_SYMBOL(unlock_tasklist);
EXPORT_SYMBOL(lock_tasklist);
EXPORT_SYMBOL(mm_sys_wait4);
EXPORT_SYMBOL(mm_put_task_struct);
EXPORT_SYMBOL(mm_sched_setaffinity);
//EXPORT_SYMBOL(k_proxies_bind);
#endif

