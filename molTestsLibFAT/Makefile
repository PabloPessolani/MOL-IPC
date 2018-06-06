CC=gcc
BINS=01_test_open_close \
02_test_open_read_close \
03_test_open_read_write_close \
04_test_open_read_write_close_sync \
05_test_creat \
06_test_creat_open_read_write_close_sync \
07_test_open_lseek_read_close \
08_test_stat \
08_test_stat_dir \
09_test_fstat \
10_test_fcntl \
11_test_sync \
12_test_opendir \
13_test_readdir \
14_test_mkdir \
15_test_rmdir \
16_test_chdir \
17_test_closedir \
18_test_chmod \
19_test_chroot \
20_test_chown \
21_test_utime \
22_test_rename \
23_test_link \
24_test_unlink \
25_test_symlink \
26_test_readlink \
27_test_access \
28_test_truncate \
29_test_umask \
30_test_mount \
31_test_umount \
32_test_dup \
33_test_dup2 \
34_test_fsync \
35_test_mknod \
36_test_ioctl \
test_fs_inet_01
		
all: $(BINS)

msg_compilando = Compilando...
msg_borrando = Borrando...
		
INCLUDE = ..
OBJS = /home/MoL_Module/mol-ipc/stub_syscall.o
LIBS = 	/home/MoL_Module/mol-ipc/lib/syslib/libsyslib.so \
		/home/MoL_Module/mol-ipc/lib/timers/libtimers.so \
		/home/MoL_Module/mol-ipc/lib/mollib/libmollib.so 
CFLAGS = -MMD -MP -ldl -lm -lrt -lnsl -pthread -rdynamic

# ***** Tests INDIVIDUALES Funciones Ordenadas (NO Loops) ********
# 

01_test_open_close: 01_test_open_close.c 
	@echo " "
	@echo "==== Tests INDIVIDUALES Funciones MOL Ordenadas (NO Loops) ====" 
	@echo "==============================================================="	
	@echo $(msg_compilando) $@
	@$(CC) $(FLAGS) -o $@ $? $(OBJS) $(LIBS) -I$(INCLUDE)

02_test_open_read_close: 02_test_open_read_close.c 
	@echo $(msg_compilando) $@
	@$(CC) $(FLAGS) -o $@ $? $(OBJS) $(LIBS) -I$(INCLUDE)	

03_test_open_read_write_close: 03_test_open_read_write_close.c 
	@echo $(msg_compilando) $@
	@$(CC) $(FLAGS) -o $@ $? $(OBJS) $(LIBS) -I$(INCLUDE)

04_test_open_read_write_close_sync: 04_test_open_read_write_close_sync.c 
	@echo $(msg_compilando) $@
	@$(CC) $(FLAGS) -o $@ $? $(OBJS) $(LIBS) -I$(INCLUDE)

05_test_creat: 05_test_creat.c 
	@echo $(msg_compilando) $@
	@$(CC) $(FLAGS) -o $@ $? $(OBJS) $(LIBS) -I$(INCLUDE)

06_test_creat_open_read_write_close_sync: 06_test_creat_open_read_write_close_sync.c 
	@echo $(msg_compilando) $@
	@$(CC) $(FLAGS) -o $@ $? $(OBJS) $(LIBS) -I$(INCLUDE)

07_test_open_lseek_read_close: 07_test_open_lseek_read_close.c 
	@echo $(msg_compilando) $@
	@$(CC) $(FLAGS) -o $@ $? $(OBJS) $(LIBS) -I$(INCLUDE)

08_test_stat: 08_test_stat.c
	@echo $(msg_compilando) $@
	@$(CC) $(FLAGS) -o $@ $? $(OBJS) $(LIBS) -I$(INCLUDE)

08_test_stat_dir: 08_test_stat_dir.c
	@echo $(msg_compilando) $@
	@$(CC) $(FLAGS) -o $@ $? $(OBJS) $(LIBS) -I$(INCLUDE)

09_test_fstat: 09_test_fstat.c 
	@echo $(msg_compilando) $@
	@$(CC) $(FLAGS) -o $@ $? $(OBJS) $(LIBS) -I$(INCLUDE)

10_test_fcntl: 10_test_fcntl.c 
	@echo $(msg_compilando) $@
	@$(CC) $(FLAGS) -o $@ $? $(OBJS) $(LIBS) -I$(INCLUDE)

11_test_sync: 11_test_sync.c 
	@echo $(msg_compilando) $@
	@$(CC) $(FLAGS) -o $@ $? $(OBJS) $(LIBS) -I$(INCLUDE)

12_test_opendir: 12_test_opendir.c 
	@echo $(msg_compilando) $@
	@$(CC) $(FLAGS) -o $@ $? $(OBJS) $(LIBS) -I$(INCLUDE)

13_test_readdir: 13_test_readdir.c 
	@echo $(msg_compilando) $@
	@$(CC) $(FLAGS) -o $@ $? $(OBJS) $(LIBS) -I$(INCLUDE)

14_test_mkdir: 14_test_mkdir.c 
	@echo $(msg_compilando) $@
	@$(CC) $(FLAGS) -o $@ $? $(OBJS) $(LIBS) -I$(INCLUDE)

15_test_rmdir: 15_test_rmdir.c 
	@echo $(msg_compilando) $@
	@$(CC) $(FLAGS) -o $@ $? $(OBJS) $(LIBS) -I$(INCLUDE)	

16_test_chdir: 16_test_chdir.c 
	@echo $(msg_compilando) $@
	@$(CC) $(FLAGS) -o $@ $? $(OBJS) $(LIBS) -I$(INCLUDE)

17_test_closedir: 17_test_closedir.c 
	@echo $(msg_compilando) $@
	@$(CC) $(FLAGS) -o $@ $? $(OBJS) $(LIBS) -I$(INCLUDE)					

18_test_chmod: 18_test_chmod.c 
	@echo $(msg_compilando) $@
	@$(CC) $(FLAGS) -o $@ $? $(OBJS) $(LIBS) -I$(INCLUDE)	

19_test_chroot: 19_test_chroot.c 
	@echo $(msg_compilando) $@
	@$(CC) $(FLAGS) -o $@ $? $(OBJS) $(LIBS) -I$(INCLUDE)	

20_test_chown: 20_test_chown.c 
	@echo $(msg_compilando) $@
	@$(CC) $(FLAGS) -o $@ $? $(OBJS) $(LIBS) -I$(INCLUDE)	

21_test_utime: 21_test_utime.c 
	@echo $(msg_compilando) $@
	@$(CC) $(FLAGS) -o $@ $? $(OBJS) $(LIBS) -I$(INCLUDE)	

22_test_rename: 22_test_rename.c 
	@echo $(msg_compilando) $@
	@$(CC) $(FLAGS) -o $@ $? $(OBJS) $(LIBS) -I$(INCLUDE)					

23_test_link: 23_test_link.c 
	@echo $(msg_compilando) $@
	@$(CC) $(FLAGS) -o $@ $? $(OBJS) $(LIBS) -I$(INCLUDE)		

24_test_unlink: 24_test_unlink.c 
	@echo $(msg_compilando) $@
	@$(CC) $(FLAGS) -o $@ $? $(OBJS) $(LIBS) -I$(INCLUDE)

25_test_symlink: 25_test_symlink.c 
	@echo $(msg_compilando) $@
	@$(CC) $(FLAGS) -o $@ $? $(OBJS) $(LIBS) -I$(INCLUDE)

26_test_readlink: 26_test_readlink.c 
	@echo $(msg_compilando) $@
	@$(CC) $(FLAGS) -o $@ $? $(OBJS) $(LIBS) -I$(INCLUDE)	
	
27_test_access: 27_test_access.c 
	@echo $(msg_compilando) $@
	@$(CC) $(FLAGS) -o $@ $? $(OBJS) $(LIBS) -I$(INCLUDE)							

28_test_truncate: 28_test_truncate.c 
	@echo $(msg_compilando) $@
	@$(CC) $(FLAGS) -o $@ $? $(OBJS) $(LIBS) -I$(INCLUDE)

29_test_umask: 29_test_umask.c 
	@echo $(msg_compilando) $@
	@$(CC) $(FLAGS) -o $@ $? $(OBJS) $(LIBS) -I$(INCLUDE)

30_test_mount: 30_test_mount.c 
	@echo $(msg_compilando) $@
	@$(CC) $(FLAGS) -o $@ $? $(OBJS) $(LIBS) -I$(INCLUDE)

31_test_umount: 31_test_umount.c 
	@echo $(msg_compilando) $@
	@$(CC) $(FLAGS) -o $@ $? $(OBJS) $(LIBS) -I$(INCLUDE)			

32_test_dup: 32_test_dup.c 
	@echo $(msg_compilando) $@
	@$(CC) $(FLAGS) -o $@ $? $(OBJS) $(LIBS) -I$(INCLUDE)			

33_test_dup2: 33_test_dup2.c 
	@echo $(msg_compilando) $@
	@$(CC) $(FLAGS) -o $@ $? $(OBJS) $(LIBS) -I$(INCLUDE)

34_test_fsync: 34_test_fsync.c 
	@echo $(msg_compilando) $@
	@$(CC) $(FLAGS) -o $@ $? $(OBJS) $(LIBS) -I$(INCLUDE)

35_test_mknod: 35_test_mknod.c 
	@echo $(msg_compilando) $@
	@$(CC) $(FLAGS) -o $@ $? $(OBJS) $(LIBS) -I$(INCLUDE)

36_test_ioctl: 36_test_ioctl.c 
	@echo $(msg_compilando) $@
	@$(CC) $(FLAGS) -o $@ $? $(OBJS) $(LIBS) -I$(INCLUDE)
	
test_fs_inet_01: test_fs_inet_01.c 
	@echo $(msg_compilando) $@
	@$(CC) $(FLAGS) -o $@ $? $(OBJS) $(LIBS) -I$(INCLUDE)
	
clean:
	clear
	@echo " "
	@echo "==== Limpieza Funciones MOL Ordenadas (NO Loops) ==============" 
	@echo "==============================================================="	
	@$(foreach bin, $(BINS), echo $(msg_borrando) $(bin);)
	@rm -f $(BINS)