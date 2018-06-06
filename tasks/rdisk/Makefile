all: rdisk 
# Which compiler
CC = gcc

# Where are include files kept
INCLUDE = ..

# Options for develofsent
# CFLAGS = -v -O3 -Wall -ansi


CFLAGS = -MMD -MP -ldl -lm -lrt -lnsl -pthread -rdynamic
#LZ4FLAGS = $(CFLAGS)  
#LZ4FLAGS += -std=c99 -Wall -Wextra -Wundef -Wshadow -Wcast-align -Wstrict-prototypes

OBJS =  utility.o test_config.o mastercopy.o slavecopy.o replicate.o lz4_data_cd.o rdisk.o

#LIBS = ../../lib/syslib/libsyslib.so ../../lib/timers/libtimers.so
LIBS = 	/home/MoL_Module/mol-ipc/stub_syscall.o \
		/home/MoL_Module/mol-ipc/lib/timers/libtimers.so \
		/home/MoL_Module/mol-ipc/lib/config/libconfig.so \
		/home/MoL_Module/mol-ipc/lib/syslib/libsyslib.so \
		/home/MoL_Module/mol-ipc/lib/mollib/libmollib.so \
		/home/MoL_Module/mol-ipc/tasks/libdriver/libdriver.so \
		/usr/local/lib/libspread.a \
		/usr/local/lib/libspread-util.a 
		
			
LZ4LIBS = $(LIBS) 
LZ4LIBS += 	/home/MoL_Module/lz4-r131/lib/lz4frame.o \
			/home/MoL_Module/lz4-r131/lib/lz4.o \
			/home/MoL_Module/lz4-r131/lib/lz4hc.o \
			/home/MoL_Module/lz4-r131/lib/xxhash.o		
	
rdisk: $(OBJS) 
	$(CC)  $(CFLAGS) -o rdisk $(OBJS) $(LZ4LIBS) -I$(INCLUDE)	
	
readfs: readfs.c  $(OBJS) $(LIBDRIVER) 
	$(CC)  $(CFLAGS) -MMD -MP -ldl -pthread -o readfs readfs.c $(OBJS) $(LIBS) -I$(INCLUDE)
	
.c.o:
	$(CC) $(CFLAGS) -c -o $@ $< $(LZ4LIBS) -I$(INCLUDE) -Wall

clean:
	rm rdisk readfs *.o
	rm ../libdriver/*.o