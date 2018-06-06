#include <mollib.h>

int mol_mount(char *name, char *special, int rwflag)
{
	message m __attribute__((aligned(0x1000)));

	LIBDEBUG("INICIO %s\n", __func__);

	m.m1_i1 = strlen(special) + 1;
	m.m1_i2 = strlen(name) + 1;
	m.m1_i3 = rwflag;
	m.m1_p1 = special;
	m.m1_p2 = name;

	LIBDEBUG("FIN %s\n", __func__);

	return molsyscall(FS_PROC_NR, MOLMOUNT, &m);
}


