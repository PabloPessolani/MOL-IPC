/* The <unistd.h> header contains a few miscellaneous manifest constants. */

/* Values used by access().  POSIX Table 2-8. */
#define F_OK               0	/* test if file exists */
#define X_OK               1	/* test if file is executable */
#define W_OK               2	/* test if file is writable */
#define R_OK               4	/* test if file is readable */

/* Values used for whence in lseek(fd, offset, whence).  POSIX Table 2-9. */
#define SEEK_SET           0	/* offset is absolute  */
#define SEEK_CUR           1	/* offset is relative to current position */
#define SEEK_END           2	/* offset is relative to end of file */

#ifdef _MINIX
/* How to exit the system or stop a server process. */
#define RBT_HALT	   0	/* shutdown and return to monitor */
#define RBT_REBOOT	   1	/* reboot the system through the monitor */
#define RBT_PANIC	   2	/* a server panics */
#define RBT_MONITOR	   3	/* let the monitor do this */
#define RBT_RESET	   4	/* hard reset the system */
#define RBT_INVALID	   5	/* first invalid reboot flag */

#define _PM_SEG_FLAG (1L << 30)	/* for read() and write() to FS by PM */
#endif




#ifdef ANULADO
/* This value is required by POSIX Table 2-10. */
#define _POSIX_VERSION 199009L	/* which standard is being conformed to */

/* These three definitions are required by POSIX Sec. 8.2.1.2. */
#define STDIN_FILENO       0	/* file descriptor for stdin */
#define STDOUT_FILENO      1	/* file descriptor for stdout */
#define STDERR_FILENO      2	/* file descriptor for stderr */

/* NULL must be defined in <unistd.h> according to POSIX Sec. 2.7.1. */
#define NULL    ((void *)0)

/* The following relate to configurable system variables. POSIX Table 4-2. */
#define _SC_ARG_MAX	   1
#define _SC_CHILD_MAX	   2
#define _SC_CLOCKS_PER_SEC 3
#define _SC_CLK_TCK	   3
#define _SC_NGROUPS_MAX	   4
#define _SC_OPEN_MAX	   5
#define _SC_JOB_CONTROL	   6
#define _SC_SAVED_IDS	   7
#define _SC_VERSION	   8
#define _SC_STREAM_MAX	   9
#define _SC_TZNAME_MAX    10
#define _SC_PAGESIZE	  11
#define _SC_PAGE_SIZE	  _SC_PAGESIZE

/* The following relate to configurable pathname variables. POSIX Table 5-2. */
#define _PC_LINK_MAX	   1	/* link count */
#define _PC_MAX_CANON	   2	/* size of the canonical input queue */
#define _PC_MAX_INPUT	   3	/* type-ahead buffer size */
#define _PC_NAME_MAX	   4	/* file name size */
#define _PC_PATH_MAX	   5	/* pathname size */
#define _PC_PIPE_BUF	   6	/* pipe size */
#define _PC_NO_TRUNC	   7	/* treatment of long name components */
#define _PC_VDISABLE	   8	/* tty disable */
#define _PC_CHOWN_RESTRICTED 9	/* chown restricted or not */

/* POSIX defines several options that may be implemented or not, at the
 * implementer's whim.  This implementer has made the following choices:
 *
 * _POSIX_JOB_CONTROL	    not defined:	no job control
 * _POSIX_SAVED_IDS 	    not defined:	no saved uid/gid
 * _POSIX_NO_TRUNC	    defined as -1:	long path names are truncated
 * _POSIX_CHOWN_RESTRICTED  defined:		you can't give away files
 * _POSIX_VDISABLE	    defined:		tty functions can be disabled
 */
#define _POSIX_NO_TRUNC       (-1)
#define _POSIX_CHOWN_RESTRICTED  1
#endif // ANULADO


extern char *optarg;
extern int optind, opterr, optopt;

#define DEV_MAP 1
#define DEV_UNMAP 2
#define mapdriver(driver, device, style) devctl(DEV_MAP, driver, device, style)
#define unmapdriver(device) devctl(DEV_UNMAP, 0, device, 0)


/* Function Prototypes. */
_PROTOTYPE( void mol_exit, (int _status)					);
_PROTOTYPE( int mol_access, (const char *_path, int _amode)			);
_PROTOTYPE( unsigned int  mol_alarm, (unsigned int _seconds)			);
_PROTOTYPE( int mol_chdir, (const char *_path)				);
_PROTOTYPE( int mol_fchdir, (int fd)					);
_PROTOTYPE( int mol_chown, (const char *_path, _mnx_Uid_t _owner, _mnx_Gid_t _group)	);
_PROTOTYPE( int mol_close, (int _fd)					);
_PROTOTYPE( char *  mol_ctermid, (char *_s)					);
_PROTOTYPE( char *  mol_cuserid, (char *_s)					);
_PROTOTYPE( int mol_dup, (int _fd)						);
_PROTOTYPE( int mol_dup2, (int _fd, int _fd2)				);
_PROTOTYPE( int mol_execl, (const char *_path, const char *_arg, ...)	);
_PROTOTYPE( int mol_execle, (const char *_path, const char *_arg, ...)	);
_PROTOTYPE( int mol_execlp, (const char *_file, const char *arg, ...)	);
_PROTOTYPE( int mol_execv, (const char *_path, char *const _argv[])		);
_PROTOTYPE( int mol_execve, (const char *_path, char *const _argv[], 
						char *const _envp[])	);
_PROTOTYPE( int mol_execvp, (const char *_file, char *const _argv[])	);

_PROTOTYPE( pid_t mol_fork, (void)						);
_PROTOTYPE( pid_t _mol_fork, (int child_lpid, int child_ep));	// LOW LEVEL FUNCTION

_PROTOTYPE( long  mol_fpathconf, (int _fd, int _name)			);
_PROTOTYPE( char *  mol_getcwd, (char *_buf, mnx_size_t _size)			);
_PROTOTYPE( gid_t  mol_getegid, (void)					);
_PROTOTYPE( uid_t  mol_geteuid, (void)					);
_PROTOTYPE( gid_t  mol_getgid, (void)					);
_PROTOTYPE( int mol_getgroups, (int _gidsetsize, gid_t _grouplist[])	);
_PROTOTYPE( char *  mol_getlogin, (void)					);
_PROTOTYPE( pid_t mol_getpgrp, (void)					);
_PROTOTYPE( pid_t mol_getpid, (void)					);
_PROTOTYPE( pid_t mol_getnpid, (int proc_nr)				);
_PROTOTYPE( pid_t mol_getppid, (void)					);
_PROTOTYPE( uid_t  mol_getuid, (void)					);
_PROTOTYPE( int mol_isatty, (int _fd)					);
_PROTOTYPE( int mol_link, (const char *_existing, const char *_new)		);
_PROTOTYPE( mnx_off_t mol_lseek, (int _fd, mnx_off_t _offset, int _whence)		);
_PROTOTYPE( long  mol_pathconf, (const char *_path, int _name)		);
_PROTOTYPE( int mol_pause, (void)						);
_PROTOTYPE( int mol_pipe, (int _fildes[2])					);
_PROTOTYPE( ssize_t mol_read, (int _fd, void *_buf, mnx_size_t _n)		);
_PROTOTYPE( ssize_t mol_write, (int _fd, void *_buf, mnx_size_t _n)		);
_PROTOTYPE( int mol_rmdir, (const char *_path)				);
_PROTOTYPE( int mol_setgid, (_mnx_Gid_t _gid)				);
_PROTOTYPE( int mol_setegid, (_mnx_Gid_t _gid)				);
_PROTOTYPE( int mol_setpgid, (pid_t _pid, pid_t _pgid)			);
_PROTOTYPE( pid_t mol_setsid, (void)					);
_PROTOTYPE( int mol_setuid, (_mnx_Uid_t _uid)				);
_PROTOTYPE( int mol_seteuid, (_mnx_Uid_t _uid)				);
_PROTOTYPE( unsigned int  mol_sleep, (unsigned int _seconds)			);
_PROTOTYPE( long  mol_sysconf, (int _name)					);
_PROTOTYPE( pid_t mol_tcgetpgrp, (int _fd)					);
_PROTOTYPE( int mol_tcsetpgrp, (int _fd, pid_t _pgrp_id)			);
_PROTOTYPE( char *  mol_ttyname, (int _fd)					);
_PROTOTYPE( int mol_unlink, (const char *_path)				);
_PROTOTYPE( ssize_t write, (int _fd, const void *_buf, mnx_size_t _n)	);
_PROTOTYPE( mnx_off_t mol_truncate, (const char *_path, mnx_off_t _length)		);
_PROTOTYPE( mnx_off_t mol_ftruncate, (int _fd, mnx_off_t _length)			);

/* Open Group Base Specifications Issue 6 (not complete) */
_PROTOTYPE( int mol_symlink, (const char *path1, const char *path2)		);
_PROTOTYPE( int mol_readlink, (const char *, char *, mnx_size_t)		);
_PROTOTYPE( int mol_getopt, (int _argc, char * const _argv[], char const *_opts)		);
_PROTOTYPE( int mol_usleep, (useconds_t _useconds)				);
_PROTOTYPE( void mol_loadname, (const char *name, message *msgptr));
_PROTOTYPE( int mol_stat,(const char *name, struct mnx_stat *buffer));
_PROTOTYPE( int mol_open, (const char *name, int flags, ...));
_PROTOTYPE( int mol_fstat, (int fd, struct mnx_stat *buffer));
_PROTOTYPE( int mol_fcntl, (int fd, int cmd, ...));

