all: libsyslib.so 

# Which compiler
CC = gcc

# Where are include files kept
INCLUDE = .
 

# Options for development
CFLAGS =  -g -c -fPIC -Wall -pthread 

LIBS = /home/MoL_Module/mol-ipc/stub_syscall.o

OBJS =  taskcall.o sys_times.o sys_setalarm.o sys_fork.o sys_exit.o \
	sys_privctl.o sys_getinfo.o sys_bindproc.o sys_vircopy.o sys_getuptime.o \
	sys_memset.o sys_migrproc.o sys_rexec.o sys_setpname.o sys_virvcopy.o 
libsyslib.so: $(OBJS) 
	$(CC) -g -shared -pthread -o libsyslib.so $(OBJS)  $(LIBS)

.c.o:
	$(CC) $(CFLAGS) -c -o $@ $< -I$(INCLUDE)


clean:
	rm libsyslib.so *.o

.PHONY: all clean 
.SECONDARY:

