/* stub_syscall.h */

/*	permite realizar llamadas al sistema
 *	utilizando la int 0x21
 */

inline long mol_stub_syscall0(long syscall);

inline long mol_stub_syscall1(long syscall, long arg1);

inline long mol_stub_syscall2(long syscall, long arg1, long arg2);

inline long mol_stub_syscall3(long syscall, long arg1, long arg2, long arg3);

inline long mol_stub_syscall4(long syscall, long arg1, long arg2, long arg3, long arg4);

inline long mol_stub_syscall5(long syscall, long arg1, long arg2, long arg3, long arg4, long arg5);

/*----------------------------------------------------------------------------------------------*/
/*			SINGLE THREADED PROCESS PRIMITIVES					*/
/*----------------------------------------------------------------------------------------------*/
#define mnx_dc_init(dcu) 			mol_stub_syscall1(DCINIT, (long) dcu)
#define mnx_dc_end(dcid) 			mol_stub_syscall1(DCEND, dcid)

#define mnx_bind(dcid,endpoint) 			mol_stub_syscall5(BIND, SELF_BIND, dcid, getpid(), endpoint, LOCALNODE)
#define mnx_tbind(dcid,endpoint) 			mol_stub_syscall5(BIND, SELF_BIND, dcid, (pid_t) syscall (SYS_gettid), endpoint, LOCALNODE)
#define mnx_lclbind(dcid,pid,endpoint) 		mol_stub_syscall5(BIND, LCL_BIND, dcid, pid, endpoint, LOCALNODE)
#define mnx_rmtbind(dcid,name,endpoint,nodeid) 	mol_stub_syscall5(BIND, RMT_BIND, dcid, (int) name, endpoint, nodeid)
#define mnx_bkupbind(dcid,pid,endpoint,nodeid) 	mol_stub_syscall5(BIND, BKUP_BIND, dcid, pid, endpoint, nodeid)
#define mnx_replbind(dcid,pid,endpoint) 	mol_stub_syscall5(BIND, REPLICA_BIND, dcid, pid, endpoint, LOCALNODE)
#define mnx_wait4bind()						mol_stub_syscall3(WAIT4BIND, WAIT_BIND,  SELF,  TIMEOUT_FOREVER)
#define mnx_wait4bindep(endpoint)			mol_stub_syscall3(WAIT4BIND, WAIT_BIND,  endpoint,  TIMEOUT_FOREVER)
#define mnx_wait4unbind(endpoint)			mol_stub_syscall3(WAIT4BIND, WAIT_UNBIND, endpoint, TIMEOUT_FOREVER)

#define mnx_unbind(dcid,p_ep) 		  		mol_stub_syscall3(UNBIND, dcid, p_ep, TIMEOUT_FOREVER)
#define mnx_unbind_T(dcid,p_ep, to_ms)   	mol_stub_syscall3(UNBIND, dcid, p_ep, to_ms)
/* #define mnx_rmtunbind(dcid,p_ep,nodeid)   mol_stub_syscall3(UNBIND, dcid, p_ep, nodeid) */

#define mnx_proc_dump(dcid)			mol_stub_syscall1(PROCDUMP,dcid)
#define mnx_dc_dump()				mol_stub_syscall0(DCDUMP)
#define mnx_getep(pid)				mol_stub_syscall1(GETEP, pid)

#define mnx_send(dst_ep,m)			mol_stub_syscall3(SEND, dst_ep, (int) m, TIMEOUT_FOREVER)
#define mnx_receive(src_ep,m)   	mol_stub_syscall3(RECEIVE, src_ep, (int) m, TIMEOUT_FOREVER)
#define mnx_sendrec(srcdst_ep,m) 	mol_stub_syscall3(SENDREC, srcdst_ep, (int) m, TIMEOUT_FOREVER)
#define mnx_notify(dst_ep)			mol_stub_syscall3(NOTIFY, SELF, dst_ep, HARDWARE)
#define mnx_hdw_notify(dcid, dst_ep) mol_stub_syscall3(NOTIFY, HARDWARE, dst_ep, dcid)
#define mnx_ntfy_value(src_nr, dst_ep, value)	mol_stub_syscall3(NOTIFY, src_nr, dst_ep, value)

#define mnx_rcvrqst(m) 				mol_stub_syscall2(RCVRQST, (int) m, TIMEOUT_FOREVER)
#define mnx_reply(dst_ep,m)			mol_stub_syscall3(REPLY, dst_ep, (int) m, TIMEOUT_FOREVER)
#define mnx_vcopy(src_ep,src_addr,dst_ep, dst_addr, bytes) \
					mol_stub_syscall5(VCOPY, src_ep,(long int)src_addr,dst_ep,(long int) dst_addr, bytes)

/* IPC with timeout */
#define mnx_send_T(dst_ep,m,to_ms)				mol_stub_syscall3(SEND, dst_ep, (int) m, to_ms)
#define mnx_receive_T(src_ep,m, to_ms)   	mol_stub_syscall3(RECEIVE, src_ep, (int) m, to_ms)
#define mnx_sendrec_T(srcdst_ep,m, to_ms) 	mol_stub_syscall3(SENDREC, srcdst_ep, (int) m, to_ms)
#define mnx_rcvrqst_T(m, to_ms) 			mol_stub_syscall2(RCVRQST, (int) m, to_ms)
#define mnx_reply_T(dst_ep,m, to_ms)		mol_stub_syscall3(REPLY, dst_ep, (int) m, to_ms)
#define mnx_wait4bind_T(to_ms)				mol_stub_syscall3(WAIT4BIND, WAIT_BIND,  SELF, to_ms)
#define mnx_wait4bindep_T(endpoint, to_ms)	mol_stub_syscall3(WAIT4BIND, WAIT_BIND,  endpoint,  to_ms)
#define mnx_wait4unbind_T(endpoint, to_ms)	mol_stub_syscall3(WAIT4BIND, WAIT_UNBIND, endpoint, to_ms)
					
#define mnx_void(srcdst_ep,m) 			mol_stub_syscall2(MOLVOID3, srcdst_ep, (int) m)

#define mnx_setpriv(dc, ep, priv)		mol_stub_syscall3(SETPRIV,dc, ep, (long int)priv)
#define mnx_getpriv(dc, ep, priv)		mol_stub_syscall3(GETPRIV, dc, ep, (long int)priv)

#define mnx_getdvsinfo(dvs)			mol_stub_syscall1(GETDVSINFO,  (long int)dvs)
#define mnx_getdcinfo(dcid, dc_usr)		mol_stub_syscall2(GETDCINFO, dcid, (long int)dc_usr)
#define mnx_getprocinfo(dcid, p_nr, proc_usr)	mol_stub_syscall3(GETPROCINFO, dcid, p_nr, (long int) proc_usr)
#define mnx_getnodeinfo(nodeid, node_usr)	mol_stub_syscall2(GETNODEINFO, nodeid, (long int) node_usr)

#define mnx_relay(dst_ep,m)			mol_stub_syscall2(RELAY, dst_ep, (int) m)
#define mnx_wakeup(dc, dst_ep)	mol_stub_syscall2(WAKEUPEP, dc, dst_ep)

#define mnx_proxies_bind(name, pxnr, spid, rpid, maxcopybuf)	mol_stub_syscall5(PROXYBIND, (long int) name, pxnr, spid, rpid, maxcopybuf)
#define mnx_proxies_unbind(pxnr)		mol_stub_syscall1(PROXYUNBIND,pxnr)

#define mnx_put2lcl(header, payload)   		mol_stub_syscall2(PUT2LCL, (int)header,(int) payload)
#define mnx_get2rmt(header, payload)   		mol_stub_syscall3(GET2RMT, (int)header, (int)payload, HELLO_PERIOD)
#define mnx_get2rmt_T(header, payload, to_ms)   	mol_stub_syscall3(GET2RMT, (int)header, (int)payload, to_ms)

#define mnx_add_node(dcid, nodeid)		mol_stub_syscall2(ADDNODE, dcid, nodeid)
#define mnx_del_node(dcid, nodeid)		mol_stub_syscall2(DELNODE, dcid, nodeid)
#define mnx_dvs_init(nodeid, params)		mol_stub_syscall2(DVSINIT, nodeid, (int) params)
#define mnx_dvs_end()				mol_stub_syscall0(DVSEND)
#define mnx_src_notify(src_nr, dst_ep)		mol_stub_syscall3(NOTIFY, src_nr, dst_ep, HARDWARE)

#define mnx_proxy_conn(pxid, status)		mol_stub_syscall2(PROXYCONN, pxid, status)

#define mnx_migr_start(dcid, endpoint)				mol_stub_syscall5(MIGRATE, MIGR_START, PROC_NO_PID, dcid, endpoint, PROC_NO_PID)
#define mnx_migr_rollback(dcid, endpoint)				mol_stub_syscall5(MIGRATE, MIGR_ROLLBACK, PROC_NO_PID, dcid, endpoint, PROC_NO_PID)
#define mnx_migr_commit(pid, dcid, endpoint, new_node)	mol_stub_syscall5(MIGRATE, MIGR_COMMIT, pid, dcid, endpoint, new_node)

#define mnx_node_up(name, nodeid, proxy_nr)		mol_stub_syscall3(NODEUP, (long int) name, nodeid, proxy_nr)
#define mnx_node_down(nodeid)					mol_stub_syscall1(NODEDOWN, nodeid)

#define mnx_getproxyinfo(pxid, sproc_usr, rproc_usr)	mol_stub_syscall3(GETPROXYINFO, pxid, (long int) sproc_usr, (long int) rproc_usr)

























