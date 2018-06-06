#include <mollib.h>

int mol_open(const char *name, int flags, ...)
{
	message m __attribute__((aligned(0x1000)));
	va_list argp;

	LIBDEBUG("name=%s flags=%X\n" , name, flags);

	va_start(argp, flags);
	if (flags & O_CREAT) {
		m.m1_i1 = strlen(name) + 1;
		m.m1_i2 = flags;
		m.m1_i3 = va_arg(argp, int);
		m.m1_p1 = (char *) name;
		// LIBDEBUG("m.m1_i1=%d m.m1_i2=%d m.m1_i3=%d\n", m.m1_i1, m.m1_i2, m.m1_i3, m.m1_p1);
	} else {
		mol_loadname(name, &m);
		m.m3_i2 = flags;
		// LIBDEBUG("m.m1_i1=%d m.m1_i2=%d\n", m.m1_i1, m.m1_i2);
	}
	va_end(argp);
	return (molsyscall(FS_PROC_NR, MOLOPEN, &m));
}
