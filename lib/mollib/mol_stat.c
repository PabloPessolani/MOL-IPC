#include <mollib.h>

int mol_stat(const char *name, struct mnx_stat *buffer)
{
  	message m __attribute__((aligned(0x1000)));
  
	LIBDEBUG("name=%s\n", name);	
	
	m.m1_i1 = strlen(name) + 1;
	m.m1_p1 = (char *) name;
	m.m1_p2 = (char *) buffer;
	return(molsyscall(FS_PROC_NR, MOLSTAT, &m));
}

