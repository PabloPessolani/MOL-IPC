/*-------------------------------------------*/
/* Integer type definitions for FatFs module */
/*-------------------------------------------*/

#ifndef _FF_INTEGER
#define _FF_INTEGER

#ifdef _WIN32	/* FatFs development platform */

#include <windows.h>
#include <tchar.h>
typedef unsigned __int64 FF_QWORD;


#else			/* Embedded platform */

/* These types MUST be 16-bit or 32-bit */
typedef int				FF_INT;
typedef unsigned int	FF_UINT;

/* This type MUST be 8-bit */
typedef unsigned char	FF_BYTE;

/* These types MUST be 16-bit */
typedef short			FF_SHORT;
typedef unsigned short	FF_WORD;
typedef unsigned short	FF_WCHAR;

/* These types MUST be 32-bit */
typedef long			FF_LONG;
typedef unsigned long	FF_DWORD;

/* This type MUST be 64-bit (Remove this for C89 compatibility) */
typedef unsigned long long FF_QWORD;

#endif

#endif
