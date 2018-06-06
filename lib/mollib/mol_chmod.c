#include <mollib.h>


int mol_chmod(const char *name, mnx_mode_t mode)
{
	message m __attribute__((aligned(0x1000)));

	LIBDEBUG("INICIO %s\n", __func__);
	m.m3_i2 = mode;
	mol_loadname(name, &m);
	LIBDEBUG("FIN %s\n", __func__);

	return (molsyscall(FS_PROC_NR, MOLCHMOD, &m));
}