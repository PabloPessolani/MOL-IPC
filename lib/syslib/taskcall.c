/* _taskcall() is the same as _syscall() except it returns negative error
 * codes directly and not in errno.  This is a better interface for MM and
 * FS.
 */

#include "syslib.h"

int _taskcall(int who, int syscallnr, message *msgptr)
{
  int status;

  	msgptr->m_type = syscallnr;
	status = mnx_sendrec(who, msgptr);

  if (status != 0) return(status);
  return(msgptr->m_type);
}
