#include "syslib.h"

/*===========================================================================*
 *                                sys_dc_setbuf				     *
 *===========================================================================*/
PUBLIC int sys_dc_setbuf(base, size, high)
phys_bytes base;
phys_bytes size;
phys_bytes high;
{
    message m;
    int result;

    m.m4_l1= base;
    m.m4_l2= size;
    m.m4_l3= high;

    result = _taskcall(SYSTASK, SYS_DC_SETBUF, &m);
    return(result);
}

