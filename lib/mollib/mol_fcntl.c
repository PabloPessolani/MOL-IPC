#include <mollib.h>

int mol_fcntl(int fd, int cmd, ...)
{
	message m __attribute__((aligned(0x1000)));
	va_list argp;

	LIBDEBUG("fd=%d cmd=%d\n", fd, cmd);

	va_start(argp, cmd);

	/* Set up for the sensible case where there is no variable parameter.  This
	* covers F_GETFD, F_GETFL and invalid commands.
	*/
	m.m1_i3 = 0;
	m.m1_p1 = NIL_PTR;

	/* Adjust for the stupid cases. */
	switch (cmd) {
		case F_DUPFD:
		case F_SETFD:
		case F_SETFL:
			m.m1_i3 = va_arg(argp, int);
			break;
		case F_GETLK:
		case F_SETLK:
		case F_SETLKW:
		case F_FREESP: // WARNING HAY PROBLEMAS AL COMPILAR CON ESTA CONSTANTE en la inclusion de // #include <fcntl.h> en mollib.h
			m.m1_p1 = (char *) va_arg(argp, struct flock *);
			break;
	}

	/* Clean up and make the system call. */
	va_end(argp);
	m.m1_i1 = fd;
	m.m1_i2 = cmd;
	return (molsyscall(FS_PROC_NR, MOLFCNTL, &m));
}
