all:  m3copy
# Which compiler
CC = gcc

# Where are include files kept
INCLUDE = ..

# Options for develofsent
# CFLAGS = -v -O3 -Wall -ansi

CFLAGS = -MMD -MP -ldl -lm -lrt -lnsl -pthread -rdynamic
#LZ4FLAGS = $(CFLAGS)  
#LZ4FLAGS += -std=c99 -Wall -Wextra -Wundef -Wshadow -Wcast-align -Wstrict-prototypes

OBJC = m3copy.o 
LIBS = 	/home/MoL_Module/mol-ipc/stub_syscall.o \
		/home/MoL_Module/mol-ipc/lib/timers/libtimers.so \
		/home/MoL_Module/mol-ipc/lib/config/libconfig.so \
		/home/MoL_Module/mol-ipc/lib/syslib/libsyslib.so \
		/home/MoL_Module/mol-ipc/lib/mollib/libmollib.so
	
m3copy: $(OBJC) 
	$(CC)  $(CFLAGS) -o m3copy $(OBJC) $(LIBS) -I$(INCLUDE)	
	
.c.o:
	$(CC) $(CFLAGS) -c -o $@ $< $(LIBS) -I$(INCLUDE) -Wall

clean:
	rm m3copy *.o
