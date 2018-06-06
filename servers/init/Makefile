all: init
# Which compiler
CC = gcc

# Where are include files kept
INCLUDE = ..

# Options for development
# CFLAGS = -v -O3 -Wall -ansi

OBJS = init.o 

LIBS = ../../lib/syslib/libsyslib.so ../../lib/mollib/libmollib.so ../../stub_syscall.o

init: $(OBJS)    
	$(CC)  $(CFLAGS) -MMD -MP -ldl -pthread -o init $(OBJS)  $(LIBS) -I$(INCLUDE)
	
.c.o:
	$(CC) $(CFLAGS) -c -o $@ $< -I$(INCLUDE)

clean:
	rm init  *.o
