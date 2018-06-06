#include <mollib.h>

mnx_off_t mol_lseek(int fd, mnx_off_t offset, int whence)
{
	message m __attribute__((aligned(0x1000)));

	LIBDEBUG("INICIO %s\n", __func__);
	
	m.m2_i1 = fd;
	m.m2_l1 = offset;
	m.m2_i2 = whence;

	LIBDEBUG("FIN %s\n", __func__);	

	if (molsyscall(FS_PROC_NR, MOLLSEEK, &m) < 0) return ( (mnx_off_t) - 1);
	return ( (mnx_off_t) m.m2_l1);
}
