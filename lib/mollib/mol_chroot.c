#include <mollib.h>


int mol_chroot(const char *name)
{
	message m __attribute__((aligned(0x1000)));

	LIBDEBUG("INICIO %s\n", __func__);
	mol_loadname(name, &m);
	LIBDEBUG("FIN %s\n", __func__);

	return (molsyscall(FS_PROC_NR, MOLCHROOT, &m));
}