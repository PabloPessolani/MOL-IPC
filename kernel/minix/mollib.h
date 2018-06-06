int getsysinfo(int who, int what, void *where);
int molsyscall(int who, int syscallnr, message *msgptr);

/* What system info to retrieve with sysgetinfo(). */
#define SI_KINFO	   0	/* get kernel info via PM */
#define SI_PMPROC_ADDR 1	/* address of process table */
#define SI_PMPROC_TAB  2	/* copy of entire PM process table */
#define SI_DMAP_TAB	   3	/* get device <-> driver mappings */
#define SI_MEM_ALLOC   4	/* get memory allocation data */
#define SI_DATA_STORE  5	/* get copy of data store */
#define SI_LOADINFO	   6	/* get copy of load average structure */
#define SI_KPROC_TAB   7	/* copy of kernel process table */
#define SI_MACHINE     8	
#define SI_PRIV_TAB    9
#define SI_SLOTS_TAB   10
#define SI_FSPROC_ADDR 11	
#define SI_FSPROC_TAB  12
	

#define mol_getkinfo(ptr) 	getsysinfo(PM_PROC_NR, SI_KINFO, ptr)
#define mol_getmachine(ptr) 	getsysinfo(PM_PROC_NR, SI_MACHINE, ptr)
#define mol_getkproctab(ptr) 	getsysinfo(PM_PROC_NR, SI_KPROC_TAB, ptr)
#define mol_getpmproctab(ptr) getsysinfo(PM_PROC_NR, SI_PMPROC_TAB, ptr)
#define mol_getprivtab(ptr)   getsysinfo(PM_PROC_NR, SI_PRIV_TAB, ptr)
#define mol_getslotstab(ptr)   getsysinfo(PM_PROC_NR, SI_SLOTS_TAB, ptr)

mnx_time_t mol_time(mnx_time_t *tp);
