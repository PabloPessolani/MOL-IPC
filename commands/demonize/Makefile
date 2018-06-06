all: demonize
# Which compiler
CC = gcc

# Where are include files kept
INCLUDE = ..

# Options for development
# CFLAGS = -v -O3 -Wall -ansi

OBJS = demonize.o 

LIBS = 	/home/MoL_Module/mol-ipc/lib/syslib/libsyslib.so \
		/home/MoL_Module/mol-ipc/lib/mollib/libmollib.so \
		/home/MoL_Module/mol-ipc/stub_syscall.o
		
demonize: $(OBJS)    
	$(CC)  $(CFLAGS) -MMD -MP -ldl -pthread -o demonize $(OBJS)  $(LIBS) -I$(INCLUDE)
	
.c.o:
	$(CC) $(CFLAGS) -c -o $@ $< -I$(INCLUDE)

clean:
	rm demonize  *.o
