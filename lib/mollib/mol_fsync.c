#include <mollib.h>

int mol_fsync(int fd)
{
  	message m __attribute__((aligned(0x1000)));
  
	LIBDEBUG("INICIO %s\n", __func__);	

	m.m1_i1 = fd;

	LIBDEBUG("FIN %s\n", __func__);		

	return(molsyscall(FS_PROC_NR, MOLFSYNC, &m));
}

