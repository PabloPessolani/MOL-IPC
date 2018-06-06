all: libmollib.so 

# Which compiler
CC = gcc

# Where are include files kept
INCLUDE = .
LIBS = /home/MoL_Module/mol-ipc/stub_syscall.o
 

# Options for development
CFLAGS =  -g -c -fPIC -Wall -pthread 

OBJS =  getsysinfo.o molsyscall.o mol_child_exit.o mol_exit.o mol_alarm.o \
		mol_bindproc.o  mol_fork.o mol_getpid.o mol_wait.o mol_waitpid.o \
        mol_getpprocnr.o mol_getprocnr.o mol_getnprocnr.o mol_pm_findproc.o \
		gettimeofday.o mol_stime.o mol_time.o mol_times.o mol_rexec.o mol_setpname.o mol_wait4fork.o\
		mol_loadname.o mol_open.o mol_access.o mol_creat.o mol_close.o mol_read.o mol_write.o \
		mol_sync.o mol_fsync.o mol_stat.o mol_fstat.o mol_lstat.o mol_fcntl.o mol_opendir.o \
		mol_readdir.o mol_umask.o mol_closedir.o mol_mkdir.o mol_rmdir.o mol_chdir.o mol_fchdir.o \
		mol_rename.o mol_link.o mol_symlink.o mol_readlink.o mol_unlink.o mol_lseek.o mol_truncate.o \
		mol_ftruncate.o mol_mount.o mol_umount.o mol_dup.o mol_dup2.o mol_mknod.o mol_utime.o mol_chroot.o \
		mol_chown.o mol_chmod.o mol_ioctl.o mol_unbind.o
 
libmollib.so: $(OBJS) 
	$(CC) -g -shared -pthread -o libmollib.so $(OBJS)  $(LIBS)

.c.o:
	$(CC) $(CFLAGS) -c -o $@ $< -I$(INCLUDE)


clean:
	rm libmollib.so *.o

.PHONY: all clean 
.SECONDARY:

