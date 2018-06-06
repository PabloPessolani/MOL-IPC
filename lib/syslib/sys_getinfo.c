#include "syslib.h"

/*===========================================================================*
 *                                sys_getinfo				     *
* int request; 			 system info requested *
* void *ptr;				pointer where to store it *
* int len;				max length of value to get *
* void *ptr2;			second pointer *
* int len2;				length or process nr * 
 *===========================================================================*/
int sys_getinfo(int request, void *ptr, int len, void *ptr2, int len2)
{
	message m __attribute__((aligned(0x1000)));

    m.I_REQUEST = request;
    m.I_ENDPT = SELF;			/* always store values at caller */
    m.I_VAL_PTR = ptr;
    m.I_VAL_LEN = len;
    m.I_VAL_PTR2 = ptr2;
    m.I_VAL_LEN2_E = len2;

    return(_taskcall(SYSTASK(local_nodeid), SYS_GETINFO, &m));
}

