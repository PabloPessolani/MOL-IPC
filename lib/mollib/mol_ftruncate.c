#include <mollib.h>

mnx_off_t mol_ftruncate(int _fd, mnx_off_t _length)
{
	message m __attribute__((aligned(0x1000)));

	LIBDEBUG("INICIO %s\n", __func__);

	m.m2_l1 = _length;
	m.m2_i1 = _fd;

	LIBDEBUG("FIN %s\n", __func__);

	return molsyscall(FS_PROC_NR, MOLFTRUNCATE, &m);
}
