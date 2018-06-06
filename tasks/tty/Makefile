all: tty test_tty test_fstty
# Which compiler
CC = gcc

# Where are include files kept
INCLUDE = ..

# Options for develofsent
# CFLAGS = -v -O3 -Wall -ansi

CFLAGS = -MMD -MP -ldl -lm -lrt -lnsl -pthread -rdynamic
#LZ4FLAGS = $(CFLAGS)  
#LZ4FLAGS += -std=c99 -Wall -Wextra -Wundef -Wshadow -Wcast-align -Wstrict-prototypes

OBJS = tty.o ttydevcfg.o
LIBS = 	/home/MoL_Module/mol-ipc/stub_syscall.o \
		/home/MoL_Module/mol-ipc/lib/timers/libtimers.so \
		/home/MoL_Module/mol-ipc/lib/config/libconfig.so \
		/home/MoL_Module/mol-ipc/lib/syslib/libsyslib.so \
		/home/MoL_Module/mol-ipc/lib/mollib/libmollib.so
	
tty: $(OBJS) 
	$(CC)  $(CFLAGS) -o tty $(OBJS) $(LIBS) -I$(INCLUDE)	

test_tty: test_tty.c    
	$(CC)  $(CFLAGS) -MMD -MP -ldl -pthread -o test_tty  test_tty.c $(LIBS) -I$(INCLUDE)

test_fstty: test_fstty.c    
	$(CC)  $(CFLAGS) -MMD -MP -ldl -pthread -o test_fstty  test_fstty.c $(LIBS) -I$(INCLUDE)
	
.c.o:
	$(CC) $(CFLAGS) -c -o $@ $< $(LIBS) -I$(INCLUDE) -Wall

clean:
	rm tty *.o
