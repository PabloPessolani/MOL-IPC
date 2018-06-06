#include <mollib.h>


int mol_chown(const char *name, _mnx_Uid_t owner, _mnx_Gid_t grp)
{
	message m __attribute__((aligned(0x1000)));

	LIBDEBUG("INICIO %s\n", __func__);
	m.m1_i1 = strlen(name) + 1;
	m.m1_i2 = owner;
	m.m1_i3 = grp;
	m.m1_p1 = (char *) name;
	LIBDEBUG("FIN %s\n", __func__);

	return (molsyscall(FS_PROC_NR, MOLCHOWN, &m));
}