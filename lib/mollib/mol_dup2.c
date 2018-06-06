#include <mollib.h>


int mol_dup2(int fd, int fd2)
{
/* The behavior of dup2 is defined by POSIX in 6.2.1.2 as almost, but not
 * quite the same as fcntl.
 */

  if (fd2 < 0 || fd2 > MNX_OPEN_MAX) {
	errno = EMOLBADF;
	return(-1);
  }

  /* Check to see if fildes is valid. */
  if (mol_fcntl(fd, F_GETFL) < 0) {
	/* 'fd' is not valid. */
	return(-1);
  } else {
	/* 'fd' is valid. */
	if (fd == fd2) return(fd2);
	mol_close(fd2);
	return(mol_fcntl(fd, F_DUPFD, fd2));
  }
}