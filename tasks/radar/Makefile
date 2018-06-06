all: radar 
# Which compiler
CC = gcc

# Where are include files kept
INCLUDE = ..

# Options for develofsent
# CFLAGS = -v -O3 -Wall -ansi

OBJS =  radar.o

#LIBS = ../../lib/syslib/libsyslib.so ../../lib/timers/libtimers.so
LIBS =  /usr/local/lib/libspread.a \
	../../stub_syscall.o \
	/usr/local/lib/libspread-util.a
	
radar: $(OBJS)   
	$(CC)  $(CFLAGS) -MMD -MP -ldl -lm -lrt -lnsl  -pthread -rdynamic -o radar $(OBJS) $(LIBS) -I$(INCLUDE)	
	
.c.o:
	$(CC) $(CFLAGS) -c -o $@ $< -I$(INCLUDE) -Wall

clean:
	rm radar *.o
