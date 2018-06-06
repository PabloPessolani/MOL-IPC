
#define NCALLS		  102	/* number of system calls allowed */

#define MOLUNUSED		0
#define MOLEXIT		   1 
#define MOLFORK		   2 
#define MOLREAD		   3 
#define MOLWRITE		   4 
#define MOLOPEN		   5 
#define MOLCLOSE		   6 
#define MOLWAIT		   7
#define MOLCREAT		   8 
#define MOLLINK		   9 
#define MOLUNLINK		  10 
#define MOLWAITPID		  11
#define MOLCHDIR		  12 
#define MOLTIME		  13
#define MOLMKNOD		  14 
#define MOLCHMOD		  15 
#define MOLCHOWN		  16 
#define MOLBRK		  17
#define MOLSTAT		  18 
#define MOLLSEEK		  19
#define MOLGETPID		  20
#define MOLMOUNT		  21 
#define MOLUMOUNT		  22 
#define MOLSETUID		  23
#define MOLGETUID		  24
#define MOLSTIME		  25
#define MOLPTRACE		  26
#define MOLALARM		  27
#define MOLFSTAT		  28 
#define MOLPAUSE		  29
#define MOLUTIME		  30 
#define MOLACCESS		  33 
#define MOLSYNC		  36 
#define MOLKILL		  37
#define MOLRENAME		  38
#define MOLMKDIR		  39
#define MOLRMDIR		  40
#define MOLDUP		  41 
#define MOLPIPE		  42 
#define MOLTIMES		  43
#define MOLSYMLINK		  45
#define MOLSETGID		  46
#define MOLGETGID		  47
#define MOLSIGNAL		  48
#define MOLRDLNK		  49
#define MOLLSTAT		  50
#define MOLIOCTL		  54
#define MOLFCNTL		  55
#define MOLEXEC		  59
#define MOLUMASK		  60 
#define MOLCHROOT		  61 
#define MOLSETSID		  62
#define MOLGETPGRP		  63

/* The following are not system calls, but are processed like them. */
#define MOLUNPAUSE		  65	/* to MM or FS: check for EINTR */
#define MOLREVIVE	 	  67	/* to FS: revive a sleeping process */
#define MOLTASK_REPLY	  68	/* to FS: reply code from tty task */

/* Posix signal handling. */
#define MOLSIGACTION	  71
#define MOLSIGSUSPEND	  72
#define MOLSIGPENDING	  73
#define MOLSIGPROCMASK	  74
#define MOLSIGRETURN	  75

#define MOLREBOOT		  76	/* to PM */

/* MINIX specific calls, e.g., to support system services. */
#define MOLSVRCTL		  77
#define MOLPROCSTAT          78    /* to PM */
#define MOLGETSYSINFO	  79	/* to PM or FS */
#define MOLGETPROCNR         80    /* to PM */
#define MOLDEVCTL		  81    /* to FS */
#define MOLFSTATFS	 	  82	/* to FS */
#define MOLALLOCMEM	  83	/* to PM */
#define MOLFREEMEM		  84	/* to PM */
#define MOLSELECT            85	/* to FS */
#define MOLFCHDIR            86	/* to FS */
#define MOLFSYNC             87	/* to FS */
#define MOLGETPRIORITY       88	/* to PM */
#define MOLSETPRIORITY       89	/* to PM */
#define MOLGETTIMEOFDAY      90	/* to PM */
#define MOLSETEUID		  91	/* to PM */
#define MOLSETEGID		  92	/* to PM */
#define MOLTRUNCATE	  	93	/* to FS */
#define MOLFTRUNCATE	  94	/* to FS */

#define MOLFREEPROC	  95	/* to PM: Sent by the parent process when it does fork() */
#define MOLBINDPROC	  96	/* to PM: Sent by the parent process when it does fork() */
#define MOLMIGRPROC   97	/* to PM: When a registered system process needs to register to PM and SYSTASK  */
#define MOLREXEC	  98	/* to PM: When a process want to execute a program on a REMOTE node (Remote means other nodes than PM's node) */
#define MOLSETPNAME    99	/* to PM: To change the name of a (local?) process in kernel  */
#define MOLWAIT4FORK   100	/* to PM: incomming Child send a  sendrec() to wait for it to bound child */
#define MOLUNBIND 	   101	/* to PM: Some process unbind another process or itself */



