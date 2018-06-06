#include <mollib.h>

mnx_mode_t mol_umask(mnx_mode_t complmode)
{
	message m __attribute__((aligned(0x1000)));
	LIBDEBUG("INICIO %s\n", __func__);

	m.m1_i1 = complmode;

	LIBDEBUG("FIN %s\n", __func__);

	return ( (mnx_mode_t) molsyscall(FS_PROC_NR, MOLUMASK, &m));
}
