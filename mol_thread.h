/*----------------------------------------------------------------------------------------------*/
/*			MULTI-THREADED PROCESS PRIMITIVES					*/
/*----------------------------------------------------------------------------------------------*/

extern pthread_mutex_t mol_mutex;

long long mol_dc_init(int vmid);
long lock_dc_end(int vmid);
long lock_bind(int vmid,int p_nr);
long lock_unbind(int vmid, int proc_ep);
long lock_proc_dump(int vmid);
long lock_dc_dump(void); 
long lock_getep(int pid);
long lock_send(int dst_ep, message *m_ptr);
long lock_receive(int src_ep, message *m_ptr);
long lock_sendrec(int srcdst_ep, message *m_ptr);
long lock_notify(int dst_ep);
long lock_setpriv(int priv);
long lock_vcopy(int src_ep,char *src_addr, int dst_ep,char *dst_addr, int bytes);
//long lock_getvmtab(VM_user_t *(dc_tab[]););
//long lock_getproctab(int vmid, void *ptab);
long lock_rmtbind(int vmid, int p_nr, int nodeid);
long lock_relay(int dst_ep, message *m_ptr);
int lock_proxy_bind(int spid, int rpid);
int lock_proxy_unbind(void);
long lock_rmtunbind(int vmid, int proc_ep, int nodeid);
long lock_put2lcl(proxy_hdr *header, proxy_payload *payload);
long lock_get2rmt(proxy_hdr *header, proxy_payload *payload);
long lock_add_node(int vmid, int nodeid);
long lock_del_node(int vmid, int nodeid);
long lock_dvs_init(int nodeid);
long lock_dvs_end(void);

