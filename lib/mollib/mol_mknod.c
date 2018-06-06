#include <mollib.h>

int mol_mknod(const char* name, mnx_mode_t mode, mnx_dev_t dev)
{
	message m __attribute__((aligned(0x1000)));

	LIBDEBUG("name=%s mode=%o mnx_dev_t=%X\n",name,  mode,  dev);

	m.m1_i1 = strlen(name) + 1;
	m.m1_i2 = mode;
	m.m1_i3 = dev;
	m.m1_p1 = (char *) name;
	m.m1_p2 = (char *) ((int) 0);		/* obsolete size field */

	return (molsyscall(FS_PROC_NR, MOLMKNOD, &m));
}

