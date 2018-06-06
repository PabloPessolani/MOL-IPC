all: mnx_ifconfig
# Which compiler
CC = gcc

# Where are include files kept
INCLUDE = ..

# Options for development
# CFLAGS = -v -O3 -Wall -ansi

OBJS = mnx_ifconfig.o 

LIBS = ../../lib/syslib/libsyslib.so ../../lib/mollib/libmollib.so ../../lib/ip/libip.so ../../stub_syscall.o

mnx_ifconfig: $(OBJS)    
	$(CC)  $(CFLAGS) -MMD -MP -ldl -pthread -o mnx_ifconfig $(OBJS)  $(LIBS) -I$(INCLUDE)
	
.c.o:
	$(CC) $(CFLAGS) -c -o $@ $< -I$(INCLUDE)

clean:
	rm mnx_ifconfig  *.o
