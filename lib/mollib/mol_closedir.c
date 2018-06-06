#include <mollib.h>
#define nil 0

int mol_closedir(DIR *dp)
/* Finish reading a directory. */
{
	int d;

	if (dp == nil) { errno= EMOLBADF; return -1; }

	d= dp->_fd;
	free((void *) dp);
	return mol_close(d);
}

