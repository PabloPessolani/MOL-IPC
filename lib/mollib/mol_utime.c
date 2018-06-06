#include <mollib.h>

int mol_utime(const char *name, struct utimbuf *timp)
{
	message m __attribute__((aligned(0x1000)));
	LIBDEBUG("INICIO %s\n", __func__);

	if (timp == NULL) {
		m.m2_i1 = 0;		/* name size 0 means NULL `timp' */
		m.m2_i2 = strlen(name) + 1;	/* actual size here */
	} else {
		m.m2_l1 = timp->actime;
		m.m2_l2 = timp->modtime;
		m.m2_i1 = strlen(name) + 1;
	}
	m.m2_p1 = (char *) name;
	LIBDEBUG("FIN %s\n", __func__);	
	return (molsyscall(FS_PROC_NR, MOLUTIME, &m));
}