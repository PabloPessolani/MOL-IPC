all: fs
# Which compiler
CC = gcc

# Where are include files kept
INCLUDE = ..

# Options for develofsent
# CFLAGS = -v -O3 -Wall -ansi
CFLAGS = -g -v -O0 -Wall -D_FILE_OFFSET_BITS=64

# OBJS = main.o table.o utility.o open.o read.o write.o mount.o path.o super.o link.o inode.o misc.o cache.o dmap.o device.o \
# 	../../stub_syscall.o

#Objetos a compilar
 OBJS = main.o table.o utility.o open.o read.o write.o filedes.o path.o protect.o \
		super.o link.o inode.o misc.o cache.o dmap.o device.o pipe.o stadir.o lock.o \
		time.o mount.o moldevcfg.o \
 	../../stub_syscall.o

LIBS = ../../lib/syslib/libsyslib.so \
		../../lib/timers/libtimers.so \
		../../lib/config/libconfig.so \
		../../lib/mollib/libmollib.so		

fs: $(OBJS)    
	$(CC)  $(CFLAGS) -MMD -MP -ldl -pthread -o fs $(OBJS)  $(LIBS) -I$(INCLUDE)
	
.c.o:
	$(CC) $(CFLAGS) -c -o $@ $< -I$(INCLUDE)

clean:
	rm fs  *.o
