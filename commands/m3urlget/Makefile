all: websrv webclt
# Which compiler
CC = gcc

# Where are include files kept
INCLUDE = ..

# Options for develofsent
# CFLAGS = -v -O3 -Wall -ansi

CFLAGS = -MMD -MP -ldl -lm -lrt -lnsl -pthread -rdynamic
#LZ4FLAGS = $(CFLAGS)  
#LZ4FLAGS += -std=c99 -Wall -Wextra -Wundef -Wshadow -Wcast-align -Wstrict-prototypes

OBJC = webclt.o 
OBJS = websrv.o websrvcfg.o
LIBS = 	/home/MoL_Module/mol-ipc/stub_syscall.o \
		/home/MoL_Module/mol-ipc/lib/timers/libtimers.so \
		/home/MoL_Module/mol-ipc/lib/config/libconfig.so \
		/home/MoL_Module/mol-ipc/lib/syslib/libsyslib.so \
		/home/MoL_Module/mol-ipc/lib/mollib/libmollib.so
	
websrv: $(OBJS) 
	$(CC)  $(CFLAGS) -o websrv $(OBJS) $(LIBS) -I$(INCLUDE)	

webclt: $(OBJC) 
	$(CC)  $(CFLAGS) -o webclt $(OBJC) $(LIBS) -I$(INCLUDE)	
	
.c.o:
	$(CC) $(CFLAGS) -c -o $@ $< $(LIBS) -I$(INCLUDE) -Wall

clean:
	rm websrv webclt *.o
