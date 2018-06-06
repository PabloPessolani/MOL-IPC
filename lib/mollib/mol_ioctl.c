#include <mollib.h>

int mol_ioctl(int fd, int request, void *data)
{
	message m __attribute__((aligned(0x1000)));

	LIBDEBUG("fd=%d request=%X data=%X\n", fd, request, data);
	m.TTY_LINE = fd;
	m.TTY_REQUEST = request;
	m.ADDRESS = (char *) data;
  return(molsyscall(FS_PROC_NR, MOLIOCTL, &m));
}
