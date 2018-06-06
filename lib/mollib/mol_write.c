#include <mollib.h>

ssize_t mol_write(int fd, void *buffer, mnx_size_t nbytes)
{
  	message m __attribute__((aligned(0x1000)));
  
	LIBDEBUG("INICIO %s\n", __func__);	
	
	m.m1_i1 = fd;
	m.m1_i2 = nbytes;
	m.m1_p1 = (char *) buffer;	

	LIBDEBUG("FIN %s\n", __func__);		

	return(molsyscall(FS_PROC_NR, MOLWRITE, &m));
}

