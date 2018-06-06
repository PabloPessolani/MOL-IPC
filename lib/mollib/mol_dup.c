#include <mollib.h>


int mol_dup(int fd)
{
  return(mol_fcntl(fd, F_DUPFD, 0));
}
