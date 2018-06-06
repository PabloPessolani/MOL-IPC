/* This file contains the table used to map system call numbers onto the
 * routines that perform them. */                                                                                                                                                                          

// #define SVRDBG    1


#include "fs.h"


int (*call_vec[NCALLS])(void) = {
	no_sys,        /* 0  = unused	      -FS-*/                  
	no_sys,        /* 1  = exit	          -FS- LLAMA DESDE    PM */                                                                    
	no_sys,        /* 2  = fork	          -FS- LLAMA DESDE    PM */                                                                                 
	do_read,       /* 3  = read	          -FS-*/                                                                                
	do_write,      /* 4  = write	      -FS-*/                                                                                
	do_open,       /* 5  = open	          -FS-*/                                                                                
	do_close,      /* 6  = close	      -FS-*/                                                                                
	no_sys,        /* 7  = wait	                             -PM-*/                                                                                
	do_creat,      /* 8  = creat	      -FS-*/                                                                                
	do_link,       /* 9  = link	          -FS-*/                                                                                
	do_unlink,     /* 10 = unlink	      -FS-*/                                                                                
	no_sys,        /* 11 = waitpid	                         -PM-*/                                                                        
	do_chdir,      /* 12 = chdir	      -FS-*/                                                                                
	no_sys,        /* 13 = time	                             -PM-*/                                                                                
	no_sys,      /* 14 = mknod	      -FS-*/                                                                                
	do_chmod,      /* 15 = chmod	      -FS-*/                                                                                
	no_sys,      /* 16 = chown	      -FS-*/                                                                                
	no_sys,        /* 17 = break	                         -PM-*/                                                                                
	do_stat,       /* 18 = stat	          -FS-*/                                                                                
	do_lseek,      /* 19 = lseek	      -FS-*/                                                                                
	no_sys,        /* 20 = getpid	                         -PM-*/                                                                                
	do_mount,      /* 21 = mount	      -FS-*/                                                                                
	do_umount,     /* 22 = umount	      -FS-*/                                                                                
	do_set,        /* 23 = setuid	      -FS-*/                                                                                
	no_sys,        /* 24 = getuid	                         -PM-*/                                                                                
	do_stime,      /* 25 = stime	      -FS-*/                                                                                
	no_sys,        /* 26 = ptrace	                         -PM-*/                                                                                
	no_sys,        /* 27 = alarm	                         -PM-*/                                                                                
	do_fstat,      /* 28 = fstat	      -FS-*/                                                                                
	no_sys,        /* 29 = pause	                         -PM-*/                                                                                
	do_utime,      /* 30 = utime	      -FS-*/                                                                                
	no_sys,        /* 31 = (stty)	                         -PM-*/                                                                                
	no_sys,        /* 32 = (gtty)	                         -PM-*/                                                                                
	do_access,     /* 33 = access	      -FS-*/                                                                                
	no_sys,        /* 34 = (nice)	                         -PM-*/                                                                                
	no_sys,        /* 35 = (ftime)	                         -PM-*/                                                                        
	do_sync,       /* 36 = sync	          -FS-*/                                                                                
	no_sys,        /* 37 = kill	                             -PM-*/                                                                                
	do_rename,     /* 38 = rename	      -FS-*/                                                                                
	do_mkdir,      /* 39 = mkdir	      -FS-*/                                                                                
	do_unlink,     /* 40 = rmdir	      -FS-*/                                                                                
	no_sys,        /* 41 = dup	          -FS-*/                                                                                
	no_sys,        /* 42 = pipe	          -FS- FALTA*/                                                                                
	no_sys,        /* 43 = times	                         -PM-*/                                                                                
	no_sys,        /* 44 = (prof)	                         -PM-*/                                                                                
	no_sys,      /* 45 = symlink	      -FS-*/                                                                        
	do_set,        /* 46 = setgid	      -FS-*/                                                                                
	no_sys,        /* 47 = getgid	                         -PM-*/                                                                                
	no_sys,        /* 48 = (signal)                          -PM-*/                                                                              
	no_sys,     /* 49 = readlink       -FS-*/                                                                              
	do_lstat,      /* 50 = lstat	      -FS-*/                                                                                
	no_sys,        /* 51 = (acct)	                         -PM-*/                                                                                
	no_sys,        /* 52 = (phys)	                         -PM-*/                                                                                
	no_sys,        /* 53 = (lock)	                         -PM-*/                                                                                
	do_ioctl,      /* 54 = ioctl	      -FS-*/                                                                                
	do_fcntl,      /* 55 = fcntl	      -FS-*/                                                                                
	no_sys,        /* 56 = (mpx)	                         -PM-*/                                                                                
	no_sys,        /* 57 = unused	      -FS-*/                                                                                
	no_sys,        /* 58 = unused	      -FS-*/                                                                                
	no_sys,        /* 59 = execve	                         -PM-*/                                                                                
	no_sys,      /* 60 = umask	      -FS-*/                                                                                
	do_chroot,     /* 61 = chroot	      -FS-*/                                                                                
	do_setsid,     /* 62 = setsid	      -FS-*/                                                                                
	no_sys,        /* 63 = getpgrp	                         -PM-*/                                                                        
	no_sys,        /* 64 = KSIG: signals orig. in the kernel -PM-*/                
	no_sys,        /* 65 = UNPAUSE	      -FS- LLAMA DESDE    PM */                                                                        
	no_sys,        /* 66 = unused                            -PM-*/                                                                              
	no_sys,     /* 67 = REVIVE	      -FS-*/                                                                                
	no_sys,        /* 68 = TASK_REPLY	                     -PM-*/                                                                        
	no_sys,        /* 69 = unused                            -PM-*/                                                                                
	no_sys,        /* 70 = unused                            -PM-*/                                                                                
	no_sys,        /* 71 = sigaction                         -PM-*/                                                                                        
	no_sys,        /* 72 = sigsuspend                        -PM-*/                                                                        
	no_sys,        /* 73 = sigpending                        -PM-*/                                                                        
	no_sys,        /* 74 = sigprocmask                       -PM-*/                                                                      
	no_sys,        /* 75 = sigreturn                         -PM-*/                                                                          
	no_sys,        /* 76 = reboot         -FS- LLAMA DESDE    PM */                                                                   
	do_svrctl,     /* 77 = svrctl         -FS-*/                                                                                
	no_sys,        /* 78 = unused                            -PM-*/                                                                                
	do_getsysinfo, /* 79 = getsysinfo     -FS-*/                                                                        
	no_sys,        /* 80 = unused                            -PM-*/                                                                                
	do_devctl,     /* 81 = devctl         -FS-*/                                                                                
	do_fstatfs,    /* 82 = fstatfs        -FS-*/                                                                              
	no_sys,        /* 83 = memalloc                          -PM-*/                                                                            
	no_sys,        /* 84 = memfree                           -PM-*/                                                                              
	no_sys,        /* 85 = select         -FS- FALTA (timers.c sys_setalarm ???)*/                                                                                
	do_fchdir,     /* 86 = fchdir         -FS-*/                                                                                
	do_fsync,      /* 87 = fsync          -FS-*/                                                                                  
	no_sys,        /* 88 = getpriority                       -PM-*/                                                                      
	no_sys,        /* 89 = setpriority                       -PM-*/                                                                      
	no_sys,        /* 90 = gettimeofday                      -PM-*/                                                                    
	no_sys,        /* 91 = seteuid                           -PM-*/                                                                              
	no_sys,        /* 92 = setegid                           -PM-*/                                                                              
	do_truncate,   /* 93 = truncate       -FS-*/                                                                            
	do_ftruncate,  /* 94 = truncate       -FS-*/                                                                            
};

