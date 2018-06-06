all: memory 
# Which compiler
CC = gcc

# Where are include files kept
INCLUDE = ..

# Options for develofsent
# CFLAGS = -v -O3 -Wall -ansi

OBJS =  utility.o ../../stub_syscall.o
	
LIBDRIVER = ../libdriver/libdriver.so

#LIBS = ../../lib/syslib/libsyslib.so ../../lib/timers/libtimers.so
LIBS =
memory:  memory.c $(OBJS) $(LIBDRIVER) 
	$(CC)  $(CFLAGS) -MMD -MP -ldl -pthread -o memory memory.c $(OBJS) $(LIBS) $(LIBDRIVER) -I$(INCLUDE)

readfs: readfs.c  $(OBJS) $(LIBDRIVER) 
	$(CC)  $(CFLAGS) -MMD -MP -ldl -pthread -o readfs readfs.c $(OBJS) $(LIBS) -I$(INCLUDE)
	
#.c.o:
#	$(CC) $(CFLAGS) -c -o $@ $< -I$(INCLUDE) -Wall

clean:
	rm memory readfs *.o
