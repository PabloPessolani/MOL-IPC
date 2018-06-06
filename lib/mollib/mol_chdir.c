#include <mollib.h>

int mol_chdir(const char *name)
{
	message m __attribute__((aligned(0x1000)));

	LIBDEBUG("name=%s\n", name);
	mol_loadname(name, &m);
	return (molsyscall(FS_PROC_NR, MOLCHDIR, &m));
}