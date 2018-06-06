#include <mollib.h>

int mol_readlink(const char *name, char *buffer, mnx_size_t bufsiz)
{
	message m __attribute__((aligned(0x1000)));

	LIBDEBUG("INICIO %s\n", __func__);

	m.m1_i1 = strlen(name) + 1;
	m.m1_i2 = bufsiz;
	m.m1_p1 = (char *) name;
	m.m1_p2 = (char *) buffer;

	LIBDEBUG("FIN %s\n", __func__);

	return (molsyscall(FS_PROC_NR, MOLRDLNK, &m));
}
