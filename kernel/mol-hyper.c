/****************************************************************/
/*			MINIX OVER LINUX HYPERVISOR 	 	*/
/****************************************************************/

#include "mol.h"
#include "mol-proto.h"

extern int send_sig_info(int, struct siginfo *, struct task_struct *);
extern struct cpuinfo_x86 boot_cpu_data;

/* space for global variables are defined here */
#ifdef EXTERN
#undef EXTERN
#define EXTERN
#define MOLHYPER
#endif
#include "mol-glo.h"
#include "mol-macros.h"


/*--------------------------------------------------------------*/
/*			mol_wait4bind				*/
/*--------------------------------------------------------------*/
asmlinkage long mol_wait4bind(long timeout_ms)
{
    return EMOLNOSYS;
}

/*--------------------------------------------------------------*/
/*			mol_dvs_init				*/
/* Initialize the DVS system					*/
/* du_addr = NULL => already loaded DVS parameters 	*/ 
/* returns the local_node id if OK or negative on error	*/
/*--------------------------------------------------------------*/
asmlinkage long mol_dvs_init(int nodeid, dvs_usr_t *du_addr)
{
    return EMOLNOSYS;
}


/*--------------------------------------------------------------*/
/*			mol_add_node				*/
/* Initializa a cluster node to be used by a DC 		*/
/*--------------------------------------------------------------*/
asmlinkage long mol_add_node(int dcid, int nodeid)
{
    return EMOLNOSYS;
}

/*--------------------------------------------------------------*/
/*			mol_del_node				*/
/* Delete a cluster node from a DC 		 		*/
/*--------------------------------------------------------------*/
asmlinkage long mol_del_node(int dcid, int nodeid)
{
    return EMOLNOSYS;
}

/*--------------------------------------------------------------*/
/*			mol_dc_init				*/
/* Initializa a DC and all of its processes			*/
/* returns the local_node ID					*/
/*--------------------------------------------------------------*/
asmlinkage long mol_dc_init(dc_usr_t *dcu_addr)
{
    return EMOLNOSYS;
}

/*--------------------------------------------------------------*/
/*			mol_dc_dmp				*/
/* Dumps a table of all DCs into dmesg buffer			*/
/*--------------------------------------------------------------*/
asmlinkage long mol_dc_dump(void)
{
    return EMOLNOSYS;
}
/*--------------------------------------------------------------*/
/*			mol_getdvsinfo				*/
/* On return: if (ret >= 0 ) return local_nodeid 		*/
/*         and the DVS configuration  parameters 		*/
/* if ret == -1, the DVS has not been initialized		*/
/* if ret < -1, a copy_to_user error has ocurred		*/
/*--------------------------------------------------------------*/
asmlinkage long mol_getdvsinfo(dvs_usr_t *dvs_usr_ptr)
{
    return EMOLNOSYS;
}


/*--------------------------------------------------------------*/
/*			mol_getdcinfo				*/
/* Copies the DC entry to userspace				*/
/*--------------------------------------------------------------*/
asmlinkage long mol_getdcinfo(int dcid, dc_usr_t *dc_usr_ptr)
{
    return EMOLNOSYS;
}

/*--------------------------------------------------------------*/
/*			mol_getnodeinfo				*/
/* Copies the NODE entry to userspace				*/
/*--------------------------------------------------------------*/
asmlinkage long mol_getnodeinfo(int nodeid, node_usr_t *node_usr_ptr)
{
    return EMOLNOSYS;
}

/*--------------------------------------------------------------*/
/*			mol_proc_dmp				*/
/* Dumps a table of process fields of a DC into dmesg buffer	*/
/*--------------------------------------------------------------*/
asmlinkage long mol_proc_dump(int dcid)
{
    return EMOLNOSYS;
}
/*--------------------------------------------------------------*/
/*			mol_getprocinfo				*/
/* Copies a proc descriptor to userspace			*/
/*--------------------------------------------------------------*/
asmlinkage long mol_getprocinfo(int dcid, int p_nr, struct proc_usr *proc_usr_ptr)
{
    return EMOLNOSYS;
}

/*--------------------------------------------------------------*/
/*			mol_getproxyinfo					*/
/* Copies a sproxy and rproxy proc descriptor to userspace  */
/*--------------------------------------------------------------*/
asmlinkage long mol_getproxyinfo(int px_nr,  struct proc_usr *sproc_usr_ptr, struct proc_usr *rproc_usr_ptr)
{
    return EMOLNOSYS;
}

/*--------------------------------------------------------------*/
/*			mol_bind				*/
/* Binds (an Initialize) a Linux process to the IPC Kernel	*/
/* Who can call bind?:						*/
/* - The main thread of a process to bind itself (mnx_bind)	*/
/* - The child thread of a process to bind itself (mnx_bind)	*/
/* - A MOL process that bind a local process (mnx_lclbind)	*/
/* - A MOL process that bind a remote process (mnx_rmtbind)	*/
/* - A MOL process that bind a local process that is a backup of*/
/*	a remote active process (mnx_bkupbind). Then, with	*/
/*	mol_migrate, the backup process can be converted into   */
/*	the primary process					*/
/* Local process: proc = proc number				*/
/* Remote  process: proc = endpoint 				*/
/*--------------------------------------------------------------*/
asmlinkage long mol_bind(int oper, int dcid, int pid, int proc, int nodeid)
{
    return EMOLNOSYS;
}


/*----------------------------------------------------------------*/
/*			mol_void3				*/
/*----------------------------------------------------------------*/
asmlinkage long mol_void3(void)
{
    return EMOLNOSYS;
}

/*----------------------------------------------------------------*/
/*			mol_void18				*/
/*----------------------------------------------------------------*/
asmlinkage long mol_void18(void)
{
    return EMOLNOSYS;
}

/*----------------------------------------------------------------*/
/*			mol_dc_end				*/
/*----------------------------------------------------------------*/

asmlinkage long mol_dc_end(int dcid)
{
    return EMOLNOSYS;
}
/*--------------------------------------------------------------*/
/*			mol_getep				*/
/*--------------------------------------------------------------*/

asmlinkage long mol_getep(int pid)
{
    return EMOLNOSYS;
}

/*--------------------------------------------------------------*/
/*			mol_unbind				*/
/* Unbind a  LOCAL/REMOTE  process from the DVS */
/* Who can mol_unbind a process?:				*/
/* - The main thread of a LOCAL process itself		*/
/* - The child thread of a LOCAL process itself		*/
/* - A system process that unbind a remote process	*/
/*--------------------------------------------------------------*/
asmlinkage long mol_unbind(int dcid, int proc_ep) 
{
    return EMOLNOSYS;
}
 
/*----------------------------------------------------------------*/
/*			mol_getpriv				*/
/*----------------------------------------------------------------*/

asmlinkage long mol_getpriv(int dcid, int proc_ep, priv_t *u_priv)
{
    return EMOLNOSYS;
}

/*----------------------------------------------------------------*/
/*			mol_setpriv				*/
/*----------------------------------------------------------------*/

asmlinkage long mol_setpriv(int dcid, int proc_ep, priv_t *u_priv)
{
    return EMOLNOSYS;
}

/*----------------------------------------------------------------*/
/*			mol_proxies_bind			*/
/* it returns the proxies ID or ERROR				*/
/*----------------------------------------------------------------*/
asmlinkage int mol_proxies_bind(char *px_name, int px_nr, int spid, int rpid)
{
    return EMOLNOSYS;
}

/*----------------------------------------------------------------*/
/*			mol_proxies_unbind			*/
/*----------------------------------------------------------------*/
asmlinkage int mol_proxies_unbind(int px_nr)
{
    return EMOLNOSYS;
}

/*----------------------------------------------------------------*/
/*			mol_proxy_conn				*/
/* Its is used by the proxies to signal that that they are 	*/
/* connected/disconnected to proxies on remote nodes		*/
/* status can be: 						*/
/* CONNECT_PROXIES or DISCONNECT_PROXIES 			*/
/*----------------------------------------------------------------*/
asmlinkage int mol_proxy_conn(int px_nr, int status)
{
    return EMOLNOSYS;
}


/*--------------------------------------------------------------*/
/*			mol_dvs_end				*/
/* End the DVS system						*/
/* - Unbind Proxies and delete Remote Nodes and its processes	*/
/* - End al DCs: unbind local process and remove commands	*/
/* - Unbind Proxies						*/
/*--------------------------------------------------------------*/
asmlinkage long mol_dvs_end(void)
{
    return EMOLNOSYS;
}

/*--------------------------------------------------------------*/
/*			mol_node_up				*/
/* Link a node withe a proxy pair				*/
/*--------------------------------------------------------------*/
asmlinkage long mol_node_up(char *node_name, int nodeid, int px_nr)
{
    return EMOLNOSYS;
}

/*--------------------------------------------------------------*/
/*			mol_node_down				*/
/* Unlink a node from a proxy pair				*/
/*--------------------------------------------------------------*/
asmlinkage long mol_node_down(int nodeid)
{
    return EMOLNOSYS;
}

/*--------------------------------------------------------------*/
/*			mm_exit_unbind				*/
/* Called by LINUX exit() function when a process exits */
/*--------------------------------------------------------------*/
asmlinkage long mm_exit_unbind(long code)
{
	return EMOLNOSYS;
}

/*--------------------------------------------------------------*/
/*			mol_wakeup				*/
/*--------------------------------------------------------------*/
asmlinkage long mol_wakeup(int dcid, int proc_ep)
{
	MOLDEBUG(1,"dcid=%d proc_ep=%d\n",dcid, proc_ep);
	return EMOLNOSYS;
}

#ifdef MOLAUTOFORK
/*--------------------------------------------------------------*/
/*			kernel_lclbind				*/
/* A parent process has forked and the PM returns de p_nr	*/
/* Now the parent must bind the child				*/
/*--------------------------------------------------------------*/
long kernel_lclbind(int dcid, int pid, int p_nr)
{
	struct proc *proc_ptr;
	dc_desc_t *dc_ptr;
	int g, i, endpoint, rcode, nodeid;
	struct task_struct *task_ptr;	

	if( DVS_NOT_INIT() )   			ERROR_RETURN(EMOLDVSINIT);
	nodeid = atomic_read(&local_nodeid);
MOLDEBUG(DBGLVL1,"dcid=%d pid=%d p_nr=%d nodeid=%d\n",dcid, pid, p_nr, nodeid);

	dc_ptr 		= &dc[dcid];
	WLOCK_DC(dc_ptr);	
	if( dc_ptr->dc_usr.dc_flags) 			ERROR_WUNLOCK_DC(dc_ptr,EMOLDCNOTRUN);

	i = p_nr+dc_ptr->dc_usr.dc_nr_tasks;
	proc_ptr = dc_PROC(dc_ptr,i);
	if(proc_ptr->p_usr.p_rts_flags != SLOT_FREE) 	ERROR_WUNLOCK_DC(dc_ptr,EMOLBUSY);

	init_proc_desc(proc_ptr, dcid, i);		/* Initialize all process descriptor fields	*/

// 	strncpy((char* )&proc_ptr->p_usr.p_name, (char*)task_ptr->comm, MAXPROCNAME-1);
	proc_ptr->p_name_ptr = (char*)task_ptr->comm;

// MOLDEBUG(DBGLVL1,"process p_name=%s *p_name_ptr=%s\n", (char*)&proc_ptr->p_usr.p_name, proc_ptr->p_name_ptr);
		
	proc_ptr->p_usr.p_rts_flags	= PROC_RUNNING;		/* set to RUNNING STATE	*/
	proc_ptr->p_usr.p_nodeid	= nodeid;	
	proc_ptr->p_usr.p_lpid 		= pid;			/* Update PID		*/
	if( i < dc_ptr->dc_usr.dc_nr_sysprocs) {
		proc_ptr->p_usr.p_endpoint = _ENDPOINT(0,proc_ptr->p_usr.p_nr);
		proc_ptr->p_priv.s_usr.s_id = i;
	}else{
		g = _ENDPOINT_G(proc_ptr->p_usr.p_endpoint);	/* Update endpoint 	*/
		if(++g >= _ENDPOINT_MAX_GENERATION)		/* increase generation */
			g = 1;					/* generation number wraparound */
		proc_ptr->p_usr.p_endpoint = _ENDPOINT(g,proc_ptr->p_usr.p_nr);
		proc_ptr->p_priv.s_usr.s_id = 0;
	}

	
MOLDEBUG(DBGLVL1,"i=%d p_nr=%d dcid=%d lpid=%d endpoint=%d nodeid=%d name=%s\n",
		i,
		proc_ptr->p_usr.p_nr, 
		proc_ptr->p_usr.p_dcid,
		proc_ptr->p_usr.p_lpid,
		proc_ptr->p_usr.p_endpoint,
		proc_ptr->p_usr.p_nodeid,
		proc_ptr->p_usr.p_name
		);

	endpoint = proc_ptr->p_usr.p_endpoint;
	
	dc_INCREF(dc_ptr);
	WUNLOCK_DC(dc_ptr);
	return(endpoint);
}

#endif /*MOLAUTOFORK */
