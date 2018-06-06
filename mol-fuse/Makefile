CC=gcc
BINS= hello fusexmp test_hello_molfuse molfs_fuse
		
all: $(BINS)

msg_compilando = Compilando...
msg_borrando = Borrando...

MOLFUSE_ROOT = /usr/local

INCLUDE = ..	
LIBS = $(MOLFUSE_ROOT)/lib/libfuse.a

#LIBS = ../../lib/syslib/libsyslib.so ../../lib/timers/libtimers.so
OBJS = ../stub_syscall.o

# CFLAGS = -g -v -O3 -Wall -MMD -MP -ldl -lm -lrt -lnsl -pthread -rdynamic -D_FILE_OFFSET_BITS=64	
CFLAGS = -MMD -MP -ldl -lm -lrt -lnsl -pthread -rdynamic -D_FILE_OFFSET_BITS=64	

# ***** Tests INDIVIDUALES viejos (NO Loops) ********
hello: hello.c
	clear
	@echo " "
	@echo "============================ Tests FUSE =============================="
	@echo "======================================================================"
	@echo $(msg_compilando) $@
	@$(CC) $(CFLAGS) -o $@ $? $(OBJS) $(LIBS) -I$(INCLUDE)	

fusexmp: fusexmp.c
	@echo $(msg_compilando) $@	
	@$(CC) $(CFLAGS) -o $@ $? $(OBJS) $(LIBS) -I$(INCLUDE)

test_hello_molfuse: test_hello_molfuse.c
	@echo $(msg_compilando) $@	
	@$(CC) $(CFLAGS) -o $@ $? $(OBJS) $(LIBS) -I$(INCLUDE)	

molfs_fuse: molfs_fuse.c
	@echo $(msg_compilando) $@	
	@$(CC) $(CFLAGS) -o $@ $? $(OBJS) $(LIBS) -I$(INCLUDE)		

clean:
	clear
	@echo " "
	@echo "============================ Tests FUSE (Limpieza) ==================="
	@echo "======================================================================"	
	@echo $(msg_borrando) 
	@echo $(BINS)
	@rm -f $(BINS)
