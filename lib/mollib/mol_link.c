#include <mollib.h>

int mol_link(const char *name, const char *name2)
{
	message m __attribute__((aligned(0x1000)));
	LIBDEBUG("INICIO %s\n", __func__);

	m.m1_i1 = strlen(name) + 1;
	m.m1_i2 = strlen(name2) + 1;
	m.m1_p1 = (char *) name;
	m.m1_p2 = (char *) name2;

	LIBDEBUG("FIN %s\n", __func__);

	return (molsyscall(FS_PROC_NR, MOLLINK, &m));
}
