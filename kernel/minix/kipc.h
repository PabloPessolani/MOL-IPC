#ifndef IPC_H
#define IPC_H

/* This header file defines constants for MINIX inter-process communication.
 * These definitions are used in the file proc.c.
 */
#include "com.h"

/* Masks and flags for system calls. */
#define SYSCALL_FUNC	0x000F	/* mask for system call function */
#define SYSCALL_FLAGS   0x00F0  /* mask for system call flags */
#define NON_BLOCKING    0x0010  /* do not block if target not ready */

/* System call numbers that are passed when trapping to the kernel. The 
 * numbers are carefully defined so that it can easily be seen (based on 
 * the bits that are on) which checks should be done in sys_call().
 */
#define DCINIT		   	0	
#define SEND		   	1	/* 0001 : blocking send */
#define RECEIVE			2	/* 0010 : blocking receive */
#define MOLVOID3		3
#define NOTIFY		   	4	/* 0100 : nonblocking notify */
#define SENDREC	 		5  	
#define RCVRQST			6
#define REPLY			7
#define DCEND			8	/* End a DC 			*/
#define BIND			9	/* Bind a process to IPC  	*/
#define UNBIND			10	/* UnBind a process to IPC  	*/
#define DCDUMP			11	/* Dump DC's tables		*/
#define PROCDUMP		12	/* Dump a DC process table	*/
#define GETPRIV			13	/* Get process priviledges	*/
#define SETPRIV			14	/* Set process priviledges	*/
#define VCOPY 			15   /* Virtual Copy			*/
#define GETDCINFO		16	/* Get DC information		*/
#define GETPROCINFO		17	/* Get Proc information		*/
#define MOLVOID18		18  	/* used by mol_rmtbind()    = molbind(dcid,proc,nodeid) */
#define RELAY			19	
#define PROXYBIND		20	
#define PROXYUNBIND		21	
#define GETNODEINFO		22	
#define PUT2LCL			23	/* Used by receiver proxy to send all kind of messages to local processes */	
#define GET2RMT			24	/* Used by sender  proxy to send all kind of local messages to remote  processes */	
#define ADDNODE			25	
#define DELNODE			26	
#define DVSINIT		27	
#define DVSEND			28
#define GETEP			29	/* Get process endpoint 	*/
#define GETDVSINFO		30	
#define PROXYCONN		31	
#define WAIT4BIND		32	
#define MIGRATE			33	
#define NODEUP			34	
#define NODEDOWN		35	
#define GETPROXYINFO	36	
#define WAKEUPEP		37	

#define NR_MOLCALLS	 	38   /* Numero de IPCs/DRDCM Calls habilitadas */ 

#define IPCMASK			0x08	/* mask to test IPC calls */

#define MINIX_ECHO	   8	/* 1000 : echo a message */
#define IPC_REQUEST	   5	/* 0101 : blocking request */
#define IPC_REPLY	   6    /* 0110 : nonblocking reply */
#define IPC_NOTIFY	   7    /* 0111 : nonblocking notification */
#define IPC_RECEIVE	   9	/* 1001 : blocking receive */

/* The following bit masks determine what checks that should be done. */
#define CHECK_PTR       0xBB	/* 1011 1011 : validate message buffer */
#define CHECK_DST       0x55	/* 0101 0101 : validate message destination */
#define CHECK_DEADLOCK  0x93	/* 1001 0011 : check for deadlock */

#endif /* IPC_H */
