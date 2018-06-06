all: stub_syscall.o 

# Which compiler
CC = gcc

# Where are include files kept
INCLUDE = .
 
OBJS =  stub_syscall.o   

# Options for development
CFLAGS =  -g -c -fPIC -Wall -pthread 

stub_syscall.o: stub_syscall.c stub_syscall.h 
	$(CC) -g -shared -pthread -o stub_syscall.o  stub_syscall.c

.c.o:
	$(CC) $(CFLAGS) -c -o $@ $< -I$(INCLUDE)

libsyslib.so: $(OBJS) 
	$(CC) -g -shared -pthread -o libsyslib.so $(OBJS)  $(LIBS)
	
clean:
	rm  *.o

.PHONY: all clean 
.SECONDARY:

