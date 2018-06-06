all: m3eth 
# Which compiler
CC = gcc

# Where are include files kept
INCLUDE = ..

# Options for develofsent
# CFLAGS = -v -O3 -Wall -ansi

OBJS =  m3eth.o 
OBJ1 =  taptool.o
OBJ2 =  rawtool.o

LIBS =  ../../lib/syslib/libsyslib.so \
		../../lib/timers/libtimers.so \
		../../stub_syscall.o

m3eth: $(OBJS)   
	$(CC)  $(CFLAGS) -MMD -MP -ldl -lm -lrt -lnsl  -pthread -rdynamic -o m3eth $(OBJS) $(LIBS) -I$(INCLUDE)	
	
.c.o:
	$(CC) $(CFLAGS) -c -o $@ $< -I$(INCLUDE) -Wall

clean:
	rm m3eth *.o
