CC=gcc
BINS= loop_mol_ipc1 loop_mol_ipc2 loop_mol_ipc3 

all: $(BINS)

loop_mol_ipc1: loop_mol_ipc1.c ../../stub_syscall.c
	$(CC) -o loop_mol_ipc1 loop_mol_ipc1.c ../../stub_syscall.c -I../..

loop_mol_ipc2: loop_mol_ipc2.c ../../stub_syscall.c
	$(CC) -o loop_mol_ipc2 loop_mol_ipc2.c ../../stub_syscall.c -I../..

loop_mol_ipc3: loop_mol_ipc3.c ../../stub_syscall.c
	$(CC) -o loop_mol_ipc3 loop_mol_ipc3.c ../../stub_syscall.c -I../..

	
clean:
	rm -f $(BINS)
