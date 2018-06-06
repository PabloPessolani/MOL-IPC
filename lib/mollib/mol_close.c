#include <mollib.h>

int mol_close(int fd)
{
  	message m __attribute__((aligned(0x1000)));
  
	LIBDEBUG("fd=%d\n", fd);
	m.m1_i1 = fd;
	return(molsyscall(FS_PROC_NR, MOLCLOSE, &m));
}