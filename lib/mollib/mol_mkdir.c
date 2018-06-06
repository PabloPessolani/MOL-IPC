#include <mollib.h>
#define nil 0

int mol_mkdir(const char *name, mnx_mode_t mode)
{
	message m __attribute__((aligned(0x1000)));

	LIBDEBUG("name=%s mode=%o \n", name, mode);
	m.m1_i1 = strlen(name) + 1;
	m.m1_i2 = mode;
	m.m1_p1 = (char *) name;

	return (molsyscall(FS_PROC_NR, MOLMKDIR, &m));
}
