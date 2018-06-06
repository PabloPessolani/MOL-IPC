#include "syslib.h"

/*===========================================================================*
 *                                sys_endksig				     *
 *===========================================================================*/
PUBLIC int sys_rendksig(int endp, int st_nodeid)
{
	message m __attribute__((aligned(0x1000)));
    int result;

    m.SIG_ENDPT = endp;
    result = _taskcall(SYSTASK(local_nodeid), SYS_ENDKSIG, &m);
    return(result);
}

