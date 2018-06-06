CC=gcc
BINS=fatFS
		
all: $(BINS)

msg_compilando = Compilando...
msg_borrando = Borrando...
		
INCLUDE = ..
OBJS = /home/MoL_Module/mol-ipc/stub_syscall.o
# LIBS = 	/home/MoL_Module/mol-ipc/lib/syslib/libsyslib.so \
# 		/home/MoL_Module/mol-ipc/lib/timers/libtimers.so \
# 		/home/MoL_Module/mol-ipc/lib/mollib/libmollib.so 
LIBS = 	/home/MoL_Module/mol-ipc/lib/timers/libtimers.so \
 		/home/MoL_Module/mol-ipc/lib/mollib/libmollib.so 
 		
CFLAGS = -MMD -MP -ldl -lm -lrt -lnsl -pthread -rdynamic	

# ***** Tests INDIVIDUALES Funciones Ordenadas (NO Loops) ********
# 

fatFS: diskio.c ff.c fatFS.c
	@echo " "
	@echo "====================== FAT FS Tests ======================" 
	@echo "=========================================================="	
	@echo $(msg_compilando) $@
	@$(CC) $(FLAGS) -o $@ $? $(OBJS) $(LIBS) -I$(INCLUDE)
	
clean:
	clear
	@echo " "
	@echo "==================== Limpieza FAT FS Tests ====================" 
	@echo "==============================================================="	
	@$(foreach bin, $(BINS), echo $(msg_borrando) $(bin);)
	@rm -f $(BINS)