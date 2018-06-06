#ifndef CONFIG_H
#define CONFIG_H

/* This file defines the kernel configuration. It allows to set sizes of some
 * kernel buffers and to enable or disable debugging code, timing features, 
 * and individual kernel calls.
 *
 * Changes:
 *   Jul 11, 2005	Created.  (Jorrit N. Herder)
 */

/* In embedded and sensor applications, not all the kernel calls may be
 * needed. In this section you can specify which kernel calls are needed
 * and which are not. The code for unneeded kernel calls is not included in
 * the system binary, making it smaller. If you are not sure, it is best
 * to keep all kernel calls enabled.
 */
#define USE_FORK	   1	/* fork a new process */
#define USE_NEWMAP     	   0	/* set a new memory map */
#define USE_EXEC       	   0	/* update process after execute */
#define USE_EXIT	   1	/* clean up after process exit */
#define USE_TRACE      	   0	/* process information and tracing */
#define USE_GETKSIG    	   0	/* retrieve pending kernel signals */
#define USE_ENDKSIG    	   0	/* finish pending kernel signals */
#define USE_KILL       	   0 	/* send a signal to a process */
#define USE_SIGSEND    	   0	/* send POSIX-style signal */
#define USE_SIGRETURN  	   0	/* sys_sigreturn(proc_nr, ctxt_ptr, flags) */
#define USE_ABORT      	   0	/* shut down MINIX */
#define USE_GETINFO    	   1 	/* retrieve a copy of kernel data */
#define USE_TIMES 	   		1	/* get process and system time info */
#define USE_SETALARM	   1	/* schedule a synchronous alarm */
#define USE_DEVIO      	   0	/* read or write a single I/O port */
#define USE_VDEVIO     	   0	/* process vector with I/O requests */
#define USE_SDEVIO     	   0	/* perform I/O request on a buffer */
#define USE_IRQCTL     	   0	/* set an interrupt policy */
#define USE_SEGCTL     	   0	/* set up a remote segment */
#define USE_PRIVCTL    	   1	/* system privileges control */
#define USE_NICE 	   	1	/* change scheduling priority */
#define USE_UMAP       	   0	/* map virtual to physical address */
#define USE_VIRCOPY   	   1	/* copy using virtual addressing */ 
#define USE_VIRVCOPY  	   1	/* vector with virtual copy requests */
#define USE_PHYSCOPY  	   1 	/* copy using physical addressing */
#define USE_PHYSVCOPY  	   0	/* vector with physical copy requests */
#define USE_MEMSET  	   1	/* write char to a given memory area */
#define USE_KILLED  	   0	/* a process has been killed without knowing  */
#define USE_BINDPROC  	   1	/* a system process need to be binded with PM and SYSTASK  */
#define USE_REXEC	   	   1	/* a remote process execute in LOCAL node */
#define USE_MIGRPROC  	   0	/* a system process has migrated to local node  */
#define USE_SETPNAME  	   1	/* change the name of a exec() process  */
#define USE_UNBIND   	   1	/* unbind a process */

/* Length of program names stored in the process table. This is only used
 * for the debugging dumps that can be generated with the IS server. The PM
 * server keeps its own copy of the program name.  
 */
#define P_NAME_LEN	   8

/* Kernel diagnostics are written to a circular buffer. After each message, 
 * a system server is notified and a copy of the buffer can be retrieved to 
 * display the message. The buffers size can safely be reduced.  
 */
#define KMESS_BUF_SIZE   256   	

/* Buffer to gather randomness. This is used to generate a random stream by 
 * the MEMORY driver when reading from /dev/random. 
 */
#define RANDOM_ELEMENTS   32

/* This section contains defines for valuable system resources that are used
 * by device drivers. The number of elements of the vectors is determined by 
 * the maximum needed by any given driver. The number of interrupt hooks may
 * be incremented on systems with many device drivers. 
 */
#define NR_IRQ_HOOKS	  16		/* number of interrupt hooks */
#define VDEVIO_BUF_SIZE   64		/* max elements per VDEVIO request */
#define VCOPY_VEC_SIZE    16		/* max elements per VCOPY request */

/* How many bytes for the kernel stack. Space allocated in mpx.s. */
#define K_STACK_BYTES   1024	

/* This section allows to enable kernel debugging and timing functionality.
 * For normal operation all options should be disabled.
 */
#define DEBUG_SCHED_CHECK  0	/* sanity check of scheduling queues */
#define DEBUG_TIME_LOCKS   0	/* measure time spent in locks */

#define SLOTS_RETRIES 		5	/* The minimun number of don_loops to get free slots */
#define SLOTS_PAUSE 		1	/* Seconds to wait between don_loops  */

/*
* Message types for the protocol
*/

#define SYS_REQ_SLOTS		1	/* A member is requesting free slots 	*/
#define SYS_DON_SLOTS		2	/* A member is donating free slots		*/
#define SYS_PUT_STATUS		3	/* The first_init_mbr member multicast slot state message 	*/
#define SYS_MERGE_STATUS	4	/* An old first members after a network partition broadcast its PST */
/* #define SYS_NEW_STATUS		6	 The new first member after a network merge broadcast its PST */
#define SYS_GET_PROCS		10
#define SYS_SYSCALL			20

/*
* Member States for the local Finite State Machine (FSM) 
*/

#define		BIT_REQUESTING		8		/* the member hasn´t got any slot => owned_slots=0 		*/
#define		BIT_INITIALIZED		12		/* the member PST is synchronized with the other members */
#define		BIT_RUNNING			13		/* the member is running and it can request or donate slots */
#define		BIT_MERGING			14		/* a network merge has occurred, do not send requests until complete merge*/

#define		MASK_REQUESTING		(1 <<	BIT_REQUESTING)
#define		MASK_INITIALIZED	(1 <<	BIT_INITIALIZED)
#define		MASK_RUNNING		(1 <<	BIT_RUNNING)
#define		MASK_MERGING		(1 <<	BIT_MERGING)

#define	   	STS_NEW				0x0000
#define		STS_WAIT_STATUS		0x0001
#define		STS_WAIT_INIT		0x0002
#define		STS_DISCONNECTED	0x0004

#define		STS_RUNNING			(MASK_RUNNING | MASK_INITIALIZED)
#define		STS_REQ_SLOTS		(MASK_REQUESTING | MASK_INITIALIZED)
#define		STS_MERGE_STATUS	(0x0020 | MASK_RUNNING |  MASK_INITIALIZED |  MASK_MERGING)

#define		STS_LEAVE 		STS_DISCONNECTED	


#define		m_donated_slot		m1_i1
#define		m_need_slots 		m1_i1
#define		m_free_slots	 	m1_i2
#define		m_owned_slots		m1_i3

#define		m_dest			m2_i1
#define		m_bm_init		m2_l1
#define		m_bm_waitsts	m2_l2

#define MAX_VSSETS      10
#define MAX_MEMBERS     NR_NODES

#define MAX_MESSLEN     ((NR_PROCS+NR_TASKS)*sizeof(slot_t))+1024
#define CEILING(x,y)	(x/y + (x%y!= 0))

#define	LOCAL_SLOTS			(SYSTEM - dvs.d_nr_nodes) 

#define SYS_DELAY		2

#define SLOT_TIMEOUT_SEC	5
#define SLOT_TIMEOUT_MSEC	0

#define NO_PRIMARY_MBR  LOCALNODE
#define NULL_NODE  LOCALNODE

#endif /* CONFIG_H */

