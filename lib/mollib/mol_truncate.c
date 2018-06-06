#include <mollib.h>

mnx_off_t mol_truncate(const char *_path, mnx_off_t _length)
{
	message m __attribute__((aligned(0x1000)));

	LIBDEBUG("INICIO %s\n", __func__);

	m.m2_p1 = (char *) _path;
	m.m2_i1 = strlen(_path) + 1;
	m.m2_l1 = _length;

	LIBDEBUG("FIN %s\n", __func__);

	return molsyscall(FS_PROC_NR, MOLTRUNCATE, &m);
}
