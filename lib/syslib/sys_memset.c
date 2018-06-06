#include "syslib.h"

int sys_memset(unsigned long pattern, char *base, int bytes)
{
/* Zero a block of data.  */
	message m __attribute__((aligned(0x1000)));
	message *m_ptr;

	m_ptr = &m;
	
	if (bytes == 0L) return(OK);

  m_ptr->MEM_PTR =  base;
  m_ptr->MEM_COUNT   = bytes;
  m_ptr->MEM_PATTERN = pattern;

  return(_taskcall(SYSTASK(local_nodeid), SYS_MEMSET, m_ptr));
}

