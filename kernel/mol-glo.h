/*----------------------------------------------------------------*/
/*				GLOBAL VARIABLES		*/
/*----------------------------------------------------------------*/

EXTERN dc_desc_t dc[NR_DCS];		/* an array of DC structs */
EXTERN cluster_node_t node[NR_NODES];	/* an array of NODE structs */
EXTERN proxies_t proxies[NR_NODES];		/* an array of PROXY PAIR structs */
EXTERN struct kref dvs_kref;		/* reference counter for DVS */
EXTERN int sizeof_proc_aligned; 	/* size of the struct proc cache line aligned */
EXTERN int log2_proc_size;		/* number of bits to left shift 			 */

EXTERN struct proc_dir_entry *info_entry;
EXTERN struct proc_dir_entry *proc_dvs_dir;
EXTERN struct proc_dir_entry *nodes_entry;
EXTERN struct proc_dir_entry *proxies_dir;
EXTERN struct proc_dir_entry *proxies_info_entry;
EXTERN struct proc_dir_entry *proxies_info_procs;

EXTERN struct dentry *dbg_dvs; /* directory entry of "dvs" directory on debugfs */

extern atomic_t	local_nodeid; 

#ifdef MOLHYPER

static const struct file_operations proc_dbg_fops = {
	.open = proc_dbg_open,
	.release = proc_dbg_close,
	.mmap = proc_dbg_mmap,
};

dvs_usr_t dvs = {
		NR_DCS,
		NR_NODES,
		NR_PROCS,
		NR_TASKS,
		NR_SYS_PROCS,

		MAXCOPYBUF,
		MAXCOPYLEN,

		(GENERIC|INTERNAL|DBGPROCLOCK|DBGDCLOCK|DBGMESSAGE|DBGCMD|DBGVCOPY|DBGPARAMS|DBGPROC|\
		 DBGPRIV|DBGPROCSEM),
		DVS_VERSION,
		DVS_SUBVER
		};

#if LOCK_DVS_TYPE == USE_DVS_RWLOCK
rwlock_t dvs_rwlock = RW_LOCK_UNLOCKED;		
#elif LOCK_DVS_TYPE == USE_DVS_MUTEX
DEFINE_MUTEX(dvs_mutex);				/* LINUX mutex to protect DVS 	*/	
#elif LOCK_DVS_TYPE == USE_DVS_RWSEM
DECLARE_RWSEM(dvs_rwsem);				/* LINUX RW Semaphore to protect DVS 	*/
#elif LOCK_DVS_TYPE == USE_DVS_RCU
spinlock_t dvs_spinlock = SPIN_LOCK_UNLOCKED;	
#endif


#else /* MOLHYPER */

EXTERN const struct file_operations proc_dbg_fops;

EXTERN dvs_usr_t dvs;

#if LOCK_DVS_TYPE == USE_DVS_RWLOCK
EXTERN rwlock_t   	dvs_rwlock;
#elif LOCK_DVS_TYPE == USE_DVS_MUTEX
EXTERN struct mutex dvs_mutex;
#elif LOCK_DVS_TYPE == USE_DVS_RWSEM
EXTERN struct rw_semaphore dvs_rwsem;
#elif LOCK_DVS_TYPE == USE_DVS_RCU
EXTERN spinlock_t dvs_spinlock;
#endif



#endif /* MOLHYPER */



