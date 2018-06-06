CC=gcc
BINS = fatfs_fuse
		
all: $(BINS)

msg_compilando = Compilando...
msg_borrando = Borrando...

MOLFUSE_ROOT = /usr/local

INCLUDE = ..	
OBJS = /home/MoL_Module/mol-ipc/stub_syscall.o
LIBS = 	/home/MoL_Module/mol-ipc/lib/syslib/libsyslib.so \
		/home/MoL_Module/mol-ipc/lib/timers/libtimers.so \
		/home/MoL_Module/mol-ipc/lib/mollib/libmollib.so \
		$(MOLFUSE_ROOT)/lib/libfuse.a

# CFLAGS = -g -v -O3 -Wall -MMD -MP -ldl -lm -lrt -lnsl -pthread -rdynamic -D_FILE_OFFSET_BITS=64	
CFLAGS =  -MMD -MP -ldl -lm -lrt -lnsl -pthread -rdynamic -D_FILE_OFFSET_BITS=64

# ***** Tests INDIVIDUALES viejos (NO Loops) ********
fatfs_fuse: fatfs_fuse.c
	clear
	@echo " "
	@echo "============================ Tests FUSE =============================="
	@echo "======================================================================"
	@echo $(msg_compilando) $@
	@$(CC) $(CFLAGS) -o $@ $? $(OBJS) $(LIBS) -I$(INCLUDE)	

clean:
	clear
	@echo " "
	@echo "============================ Tests FUSE (Limpieza) ==================="
	@echo "======================================================================"	
	@$(foreach bin, $(BINS), echo $(msg_borrando) $(bin);)
	@rm -f $(BINS)
