// #include <sys/select.h>

/* This is the per-process information.  A slot is reserved for each potential
 * process. Thus NR_PROCS must be the same as in the kernel. It is not 
 * possible or even necessary to tell when a slot is free here.
 */
struct fproc {
  mnx_mode_t fp_umask;		/* mask set by umask system call */
  struct inode *fp_workdir;	/* pointer to working directory's inode */
  struct inode *fp_rootdir;	/* pointer to current root dir (see chroot) */
  struct filp *fp_filp[MNX_OPEN_MAX];/* the file descriptor table */
  mol_fd_set fp_filp_inuse;		/* which fd's are in use? */
  mnx_uid_t fp_realuid;		/* real user id */
  mnx_uid_t fp_effuid;		/* effective user id */
  mnx_gid_t fp_realgid;		/* real group id */
  mnx_gid_t fp_effgid;		/* effective group id */
  mnx_dev_t fp_tty;			/* major/minor of controlling tty */
  int fp_fd;			/* place to save fd if rd/wr can't finish */
  char *fp_buffer;		/* place to save buffer if rd/wr can't finish*/
  int  fp_nbytes;		/* place to save bytes if rd/wr can't finish */
  int  fp_cum_io_partial;	/* partial byte count if rd/wr can't finish */
  char fp_suspended;		/* set to indicate process hanging */
  char fp_revived;		/* set to indicate process being revived */
  int fp_task;			/* which task is proc suspended on */
  char fp_sesldr;		/* true if proc is a session leader */
  char fp_execced;		/* true if proc has exec()ced after fork */
  mnx_pid_t fp_pid;			/* process id */
  long fp_cloexec;		/* bit map for POSIX Table 6-2 FD_CLOEXEC */
  int fp_endpoint;		/* kernel endpoint number of this process */
};
typedef struct fproc fproc_t;

#define FS_PROC_FORMAT "fp_pid=%d fp_endpoint=%d fp_tty=%d fp_fd=%d\n"
#define FS_PROC_FIELDS(p) p->fp_pid,p->fp_endpoint, p->fp_tty, p->fp_fd


/* Field values. */
#define NOT_SUSPENDED      0	/* process is not suspended on pipe or task */
#define SUSPENDED          1	/* process is suspended on pipe or task */
#define NOT_REVIVING       0	/* process is not being revived */
#define REVIVING           1	/* process is being revived from suspension */
#define PID_FREE	   0	/* process slot free */

/* Check is process number is acceptable - includes system processes. */
#define isokprocnr(n)	((unsigned)((n)+dc_ptr->dc_nr_tasks) < dc_ptr->dc_nr_procs + dc_ptr->dc_nr_tasks)

