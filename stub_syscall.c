/* stub_syscall.c */

/*	permite realizar llamadas al sistema
 *	utilizando la int 0x21
 */

#include "stub_syscall.h"

#define STUB_SYSCALL_RET EAX
#define STUB_MMAP_NR __NR_mmap2
#define MMAP_OFFSET(o) ((o) >> UM_KERN_PAGE_SHIFT)

inline long mol_stub_syscall0(long syscall)
{
    long ret;
    
    __asm__ volatile ("int $0x90" : "=a" (ret) : "0" (syscall));

    return ret;
}

inline long mol_stub_syscall1(long syscall, long arg1)
{
    long ret;
    
    __asm__ volatile ("int $0x90" : "=a" (ret) : "0" (syscall), "b" (arg1));
    
    return ret;
}

inline long mol_stub_syscall2(long syscall, long arg1, long arg2)
{
    long ret;

    __asm__ volatile ("int $0x90" : "=a" (ret) : "0" (syscall), "b" (arg1), "c" (arg2));

    return ret;
}

inline long mol_stub_syscall3(long syscall, long arg1, long arg2, long arg3)
{
    long ret;

    __asm__ volatile ("int $0x90" : "=a" (ret) : "0" (syscall), "b" (arg1), "c" (arg2), "d" (arg3));

    return ret;
}


inline long mol_stub_syscall4(long syscall, long arg1, long arg2, long arg3, long arg4)
{
    long ret;

    __asm__ volatile ("int $0x90" : "=a" (ret) : "0" (syscall), "b" (arg1), "c" (arg2), "d" (arg3), "S" (arg4));

    return ret;
}


 inline long mol_stub_syscall5(long syscall, long arg1, long arg2, long arg3, long arg4, long arg5)
 {
    long ret;
 
    __asm__ volatile ("int $0x90" : "=a" (ret) : "0" (syscall), "b" (arg1), "c" (arg2), "d" (arg3), "S" (arg4), "D" (arg5));
	
	return ret;
 }

 