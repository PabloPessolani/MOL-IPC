CC=gcc
BINS= cli_open_read cli_open_seek_y_read \
cli_open_write cli_close cli_sync cli_op_dir \
loop_read_fs loop_write_fs \
test_stat test_fstat test_fcntl test_opendir \
test_open_close test_open_read_write_close_sync \
test_open_read_write_close test_open_read_close test_readdir \
test_utime test_chmod test_chown
		
all: $(BINS)

msg_compilando = Compilando...
msg_borrando = Borrando...
		
INCLUDE = ..
OBJS = ../stub_syscall.o
CFLAGS = -MMD -MP -ldl -lm -lrt -lnsl -pthread -rdynamic	

# ***** Tests INDIVIDUALES viejos (NO Loops) ********
cli_open_read: cliente_open_y_read.c 
	clear
	@echo " "
	@echo "==== Tests INDIVIDUALES viejos (NO Loops) ===="
	@echo "=============================================="
	@echo $(msg_compilando) $@
	@$(CC) $(CFLAGS) -o $@ $? $(OBJS) -I$(INCLUDE)	

cli_open_seek_y_read: cliente_open_seek_y_read.c
	@echo $(msg_compilando) $@ 
	@$(CC) $(FLAGS) -o $@ $? $(OBJS) -I$(INCLUDE)	

cli_open_write: cliente_open_y_write.c 
	@echo $(msg_compilando) $@
	@$(CC) $(FLAGS) -o $@ $? $(OBJS) -I$(INCLUDE)

cli_close: client_close.c 
	@echo $(msg_compilando) $@
	@$(CC) $(FLAGS) -o $@ $? $(OBJS) -I$(INCLUDE)	

cli_sync: client_sync.c 
	@echo $(msg_compilando) $@
	@$(CC) $(FLAGS) -o $@ $? $(OBJS) -I$(INCLUDE)	

cli_op_dir: cliente_op_dir.c 
	@echo $(msg_compilando) $@
	@$(CC) $(FLAGS) -o $@ $? $(OBJS) -I$(INCLUDE)		

# ***** Tests LOOPS Funciones Ordenadas ********
loop_read_fs: loop_read_fs.c 
	@echo " "	
	@echo "==== Tests LOOPS Funciones Ordenadas ====" 	
	@echo "========================================="
	@echo $(msg_compilando) $@
	@$(CC) $(FLAGS) -o $@ $? $(OBJS) -I$(INCLUDE)

loop_write_fs: loop_write_fs.c 
	@echo $(msg_compilando) $@
	@$(CC) $(FLAGS) -o $@ $? $(OBJS) -I$(INCLUDE)			

# ***** Tests INDIVIDUALES Funciones Ordenadas (NO Loops) ********
test_stat: test_stat.c
	@echo " "
	@echo "==== Tests INDIVIDUALES Funciones Ordenadas (NO Loops) ====" 
	@echo "==========================================================="
	@echo $(msg_compilando) $@
	@$(CC) $(FLAGS) -o $@ $? $(OBJS) -I$(INCLUDE)

test_fstat: test_fstat.c 
	@echo $(msg_compilando) $@
	@$(CC) $(FLAGS) -o $@ $? $(OBJS) -I$(INCLUDE)

test_fcntl: test_fcntl.c 
	@echo $(msg_compilando) $@
	@$(CC) $(FLAGS) -o $@ $? $(OBJS) -I$(INCLUDE)

test_opendir: test_opendir.c 
	@echo $(msg_compilando) $@
	@$(CC) $(FLAGS) -o $@ $? $(OBJS) -I$(INCLUDE)

test_readdir: test_readdir.c 
	@echo $(msg_compilando) $@
	@$(CC) $(FLAGS) -o $@ $? $(OBJS) -I$(INCLUDE)

test_chmod: test_chmod.c 
	@echo $(msg_compilando) $@
	@$(CC) $(FLAGS) -o $@ $? $(OBJS) -I$(INCLUDE)	

test_chown: test_chown.c 
	@echo $(msg_compilando) $@
	@$(CC) $(FLAGS) -o $@ $? $(OBJS) -I$(INCLUDE)	

test_utime: test_utime.c 
	@echo $(msg_compilando) $@
	@$(CC) $(FLAGS) -o $@ $? $(OBJS) -I$(INCLUDE)	

test_open_close: test_open_close.c 
	@echo $(msg_compilando) $@
	@$(CC) $(FLAGS) -o $@ $? $(OBJS) -I$(INCLUDE)	

test_open_read_close: test_open_read_close.c 
	@echo $(msg_compilando) $@
	@$(CC) $(FLAGS) -o $@ $? $(OBJS) -I$(INCLUDE)	

test_open_read_write_close: test_open_read_write_close.c 
	@echo $(msg_compilando) $@
	@$(CC) $(FLAGS) -o $@ $? $(OBJS) -I$(INCLUDE)	

test_open_read_write_close_sync: test_open_read_write_close_sync.c 
	@echo $(msg_compilando) $@
	@$(CC) $(FLAGS) -o $@ $? $(OBJS) -I$(INCLUDE)	

clean:
	clear
	@echo $(msg_borrando) 
	@echo $(BINS)
	@rm -f $(BINS)