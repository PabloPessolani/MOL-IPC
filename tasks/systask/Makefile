all: systask
# Which compiler
CC = gcc

# Where are include files kept
INCLUDE = ..

# Options for development
# CFLAGS = -v -O3 -Wall -ansi

OBJS =  system.o dc_init.o clock.o  slots.o \
		do_unused.o do_times.o do_privctl.o do_nice.o \
		do_getinfo.o do_fork.o do_exit.o do_copy.o do_killed.o  \
		do_setalarm.o do_bindproc.o do_rexec.o do_memset.o \
		do_migrproc.o do_vcopy.o
#./libspread/libspread-core.a 

LIBS = /home/MoL_Module/mol-ipc/lib/timers/libtimers.so \
	/usr/local/lib/libspread.a \
	/home/MoL_Module/mol-ipc/stub_syscall.o \
	/usr/local/lib/libspread-util.a
##	/usr/local/lib/libspread-core.a \

systask: $(OBJS)   
	$(CC)  $(CFLAGS) -MMD -MP -ldl -lm -lrt -lnsl -pthread -rdynamic -o systask $(OBJS) $(LIBS) -I$(INCLUDE)

		
.c.o:
	$(CC) $(CFLAGS) -c -o $@ $< -I$(INCLUDE)

clean:
	rm systask  *.o
