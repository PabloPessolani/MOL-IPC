#include <mollib.h>

int mol_sync()
{
  	message m __attribute__((aligned(0x1000)));
  
	LIBDEBUG("INICIO %s\n", __func__);	

	LIBDEBUG("FIN %s\n", __func__);		

	return(molsyscall(FS_PROC_NR, MOLSYNC, &m));
}

