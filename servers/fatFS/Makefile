all: fatFS
# Which compiler
CC = gcc

# Where are include files kept
INCLUDE = ..

# Options for develofsent
CFLAGS = -g -v -O0 -Wall
# CFLAGS = -v -O3 -Wall -ansi
# CFLAGS = -g -v -O0 -Wall -D_FILE_OFFSET_BITS=64 

#-MMD -MP -ldl -lm -lrt -lnsl -pthread -rdynamic

#Objetos a compilar
OBJS = main.o table.o utility.o open.o read.o write.o filedes.o path.o protect.o \
		super.o link.o inode.o misc.o cache.o dmap.o device.o pipe.o stadir.o lock.o \
		time.o mount.o moldevcfg.o ff.o diskio.o \
		/home/MoL_Module/mol-ipc/stub_syscall.o

LIBS = 	/home/MoL_Module/mol-ipc/lib/syslib/libsyslib.so \
		/home/MoL_Module/mol-ipc/lib/timers/libtimers.so \
		/home/MoL_Module/mol-ipc/lib/config/libconfig.so \
		/home/MoL_Module/mol-ipc/lib/mollib/libmollib.so 

fatFS: $(OBJS)    
	$(CC)  $(CFLAGS) -MMD -MP -ldl -pthread -o fatFS $(OBJS)  $(LIBS) -I$(INCLUDE)
	
.c.o:
	$(CC) $(CFLAGS) -c -o $@ $< -I$(INCLUDE)

clean:
	rm fatFS *.o