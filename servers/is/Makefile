all: is
# Which compiler
CC = gcc

# Where are include files kept
INCLUDE = ..

# Options for development
# CFLAGS = -v -O3 -Wall -ansi

OBJS = is.o 

LIBS = ../../lib/syslib/libsyslib.so ../../lib/mollib/libmollib.so ../../stub_syscall.o

is: $(OBJS)    
	$(CC)  $(CFLAGS) -MMD -MP -ldl -pthread -o is $(OBJS)  $(LIBS) -I$(INCLUDE)
	
.c.o:
	$(CC) $(CFLAGS) -c -o $@ $< -I$(INCLUDE)

clean:
	rm is  *.o
