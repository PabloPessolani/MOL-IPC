/* Prototypes for system library functions. */

#ifndef _SYSLIB_H
#define _SYSLIB_H

/*==========================================================================* 
 * Minix system library. 						    *
 *==========================================================================*/ 
int _taskcall(int who, int syscallnr, message *msgptr);
int sys_rfork(int child_lpid, int nodeid);
int sys_exit(int proc);
int sys_privctl(int endpoint, int priv_type);
int sys_getinfo(int request, void *ptr, int len, void *ptr2, int len2);
int sys_rbindproc(int sysproc_ep, int lpid, int oper, int sy_nodeid);
int sys_rtimes(int endp, molclock_t *ptr, int st_nodeid);
int sys_rendksig(int endp, int st_nodeid);

int sys_rexec( int rmt_nodeid, int rmt_ep, int bind_type, int thrower_ep, int arg_len, char **argv_ptr);
int sys_setalarm(molclock_t exp_time, int abs_time);
int sys_vircopy(int src_proc, void *src_vir, int dst_proc, void *dst_vir, int bytes);
int sys_virvcopy(int vect_size, void *vect_addr);
int sys_getuptime(molclock_t *ticks);
int sys_memset(unsigned long pattern, char *base, int bytes);
int sys_migrproc(int sysproc_lpid, int sysproc_ep);
int sys_rsetpname(int p_ep,  char *p_name, int st_nodeid);
		
/* Shorthands for sys_getinfo() system call. */
#define sys_fork(child_lpid) 					sys_rfork(child_lpid, local_nodeid)
#define sys_bindproc(sysproc_ep, lpid, oper) 	sys_rbindproc(sysproc_ep, lpid, oper, local_nodeid)
#define sys_times(ep, ptr)						sys_rtimes(ep , ptr, local_nodeid)
#define sys_endksig(endp)						sys_rendksig(endp, local_nodeid)
#define sys_setpname(p_ep, p_name)				sys_rsetpname(p_ep,  p_name, local_nodeid)

#define sys_exit(p_ep)							_sys_exit(p_ep, local_nodeid)
#define sys_rexit(p_ep, nodeid)					_sys_exit(p_ep, nodeid)

#define sys_getkinfo(dst)	sys_getinfo(GET_KINFO, dst, 0,0,0)
#define sys_getproctab(dst)	sys_getinfo(GET_PROCTAB, dst, 0,0,0)
#define sys_getprivtab(dst)	sys_getinfo(GET_PRIVTAB, dst, 0,0,0)
#define sys_getmachine(dst)	sys_getinfo(GET_MACHINE, dst, 0,0,0)

#define sys_getkmessages(dst)	sys_getinfo(GET_KMESSAGES, dst, 0,0,0)
#define sys_getkinfo(dst)	sys_getinfo(GET_KINFO, dst, 0,0,0)
#define sys_getloadinfo(dst)	sys_getinfo(GET_LOADINFO, dst, 0,0,0)
#define sys_getproc(dst,nr)	sys_getinfo(GET_PROC, dst, 0,0, nr)
#define sys_getrandomness(dst)	sys_getinfo(GET_RANDOMNESS, dst, 0,0,0)
#define sys_getimage(dst)	sys_getinfo(GET_IMAGE, dst, 0,0,0)
#define sys_getirqhooks(dst)	sys_getinfo(GET_IRQHOOKS, dst, 0,0,0)
#define sys_getirqactids(dst)	sys_getinfo(GET_IRQACTIDS, dst, 0,0,0)
#define sys_getmonparams(v,vl)	sys_getinfo(GET_MONPARAMS, v,vl, 0,0)
#define sys_getschedinfo(v1,v2)	sys_getinfo(GET_SCHEDINFO, v1,0, v2,0)
#define sys_getlocktimings(dst)	sys_getinfo(GET_LOCKTIMING, dst, 0,0,0)
#define sys_getbiosbuffer(virp, sizep) sys_getinfo(GET_BIOSBUFFER, virp, \
	sizeof(*virp), sizep, sizeof(*sizep))

#define sys_getslot(dst,ep)	sys_getinfo(GET_SLOT, dst, 0,0, ep)

#endif /* _SYSLIB_H */


