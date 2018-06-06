#include <mollib.h>

int mol_fstat(int fd, struct mnx_stat *buffer)
{
  	message m __attribute__((aligned(0x1000)));
  
	LIBDEBUG("fd=%d\n", fd);	
	
	m.m1_i1 = fd;
	m.m1_p1 = (char *) buffer;
	return(molsyscall(FS_PROC_NR, MOLFSTAT, &m));
}

