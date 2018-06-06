
#define s_nr_to_id(n)	(dcu.dc_nr_tasks + (n) + 1)

/* Define flags for the various process types. */
#define IDL_F 	(SYS_PROC | PREEMPTIBLE | BILLABLE)	/* idle task */
#define TSK_F 	(SYS_PROC)				/* kernel tasks */
#define SRV_F 	(SYS_PROC | PREEMPTIBLE)		/* system services */
#define USR_F	(BILLABLE | PREEMPTIBLE)		/* user processes */

/* Define system call traps for the various process types. These call masks
 * determine what system call traps a process is allowed to make.
 */
#define TSK_T	(1 << RECEIVE)			/* clock and system */
#define SRV_T	(~0)				/* system services */
#define USR_T   ((1 << SENDREC) | (1 << ECHO))	/* user processes */

/* Send masks determine to whom processes can send messages or notifications. 
 * The values here are used for the processes in the boot image. We rely on 
 * the initialization code in main() to match the s_nr_to_id() mapping for the
 * processes in the boot image, so that the send mask that is defined here 
 * can be directly copied onto map[0] of the actual send mask. Privilege
 * structure 0 is shared by user processes. 
 */
#define s(n)		(1 << s_nr_to_id(n))
#define SRV_M	(~0)
#define SYS_M	(~0)
#define USR_M (s(PM_PROC_NR) | s(FS_PROC_NR) | s(RS_PROC_NR) | s(SYSTEM))
#define DRV_M (USR_M | s(SYSTEM) | s(CLOCK) | s(DS_PROC_NR) | s(LOG_PROC_NR) | s(TTY_PROC_NR))

/* Define kernel calls that processes are allowed to make. This is not looking
 * very nice, but we need to define the access rights on a per call basis. 
 * Note that the reincarnation server has all bits on, because it should
 * be allowed to distribute rights to services that it starts. 
 */
#define c(n)	(1 << ((n)-KERNEL_CALL))
#define RS_C	~0	
#define DS_C	~0	
#define PM_C	~(c(SYS_DEVIO) | c(SYS_SDEVIO) | c(SYS_VDEVIO) | c(SYS_IRQCTL) | c(SYS_UNBIND)) 
#define FS_C	(c(SYS_KILL) | c(SYS_VIRCOPY) | c(SYS_VIRVCOPY) | c(SYS_UMAP) | c(SYS_GETINFO) | c(SYS_EXIT) | c(SYS_TIMES) | c(SYS_SETALARM))
#define DRV_C (FS_C | c(SYS_SEGCTL) | c(SYS_IRQCTL) | c(SYS_INT86) | c(SYS_DEVIO) | c(SYS_SDEVIO) | c(SYS_VDEVIO))
#define TTY_C (DRV_C | c(SYS_ABORT) | c(SYS_dc_MAP) | c(SYS_IOPENABLE))
#define MEM_C	(DRV_C | c(SYS_PHYSCOPY) | c(SYS_PHYSVCOPY) | c(SYS_dc_MAP) | \
	c(SYS_IOPENABLE))

