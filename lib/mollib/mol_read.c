#include <mollib.h>

ssize_t mol_read(int fd, void *buffer, mnx_size_t nbytes)
{
	int rcode;
  	message m __attribute__((aligned(0x1000)));
  
	LIBDEBUG("fd=%d nbytes=%d\n", fd, nbytes);	
	
	m.m1_i1 = fd;
	m.m1_i2 = nbytes;
	m.m1_p1 = (char *) buffer;	

	rcode= molsyscall(FS_PROC_NR, MOLREAD, &m);
	
	LIBDEBUG("return %d\n", rcode);	
	return(rcode);
}

