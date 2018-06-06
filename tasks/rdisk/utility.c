
#define _MULTI_THREADED
#define _GNU_SOURCE     
#define  MOL_USERSPACE	1
#include "rdisk.h"


/*===========================================================================*
 *				cvul64				     *
 *===========================================================================*/
/*Functions to manipulate 32 bit disk addresses.*/
 
u64_t cvul64(unsigned long x32)
{
	u64_t x64;
	
	x64._[0] = x32; /*asumiendo cuarteto más significativo en posición 0 del vector*/
	x64._[1] = 0; /*asumo cuarteto menos significativo en posición 1 del vector*/
	TASKDEBUG("f_cvul64: %u:%u %u\n", x64._[0],x64._[1], x64); /*imprime cada componente de 32 bits*/
	return(x64);
}

/*===========================================================================*
 *				cv64ul				     *
*Convert a 64 bit number to an unsigned long if it fits, otherwise return ULONG_MAX.
*===========================================================================*/
unsigned long cv64ul(u64_t i)
{
	unsigned long x64;
	x64 = i._[0] + i._[1]; 
	TASKDEBUG("f_cv64ul: %u\n", x64); 
	return(x64);
}

/*===========================================================================*
 *				div64u				     *
 *===========================================================================*/
unsigned long div64u(u64_t i, unsigned j)
{
	unsigned long x32;
	
	TASKDEBUG("%ul:%ul\n", i._[0], i._[1]); /*imprime cada componente de 32 bits del vector*/
	x32 = i._[0]; /*en la componente uno del vector está el valor de 32 bits y en la cero un 0*/
	TASKDEBUG("x32: %u, j: %u\n", x32,j);
	return(x32/j);
}

