/****************************************************************/
/*		MINIX OVER LINUX IPC MIGRATION SUPPORT 		*/
/* MIGRATION START: On REMOTE nodes, the process descriptor is signaled */
/*                                    On LOCAL old node, only the thread group leader */
/* 					can START the migration and the descriptor is	*/
/*					signaled							*/
/* MIGRATION FAILED: On REMOTE nodes, the process descriptor are reset */
/*                                    On LOCAL old node, only the thread group leader */
/* 					can FAIL the migration					*/
/*					ALL threads are reset					*/
/* MIGRATION END:  EVERY thread migrated needs a mol_migrate call	*/
/****************************************************************/


#include "mol.h"
#include "mol-proto.h"


extern int send_sig_info(int, struct siginfo *, struct task_struct *);
extern struct cpuinfo_x86 boot_cpu_data;

#include "mol-glo.h"
#include "mol-macros.h"

/*--------------------------------------------------------------*/
/*			mol_migrate				*/
/* A process can only migrate when it request a service		*/
/* to its SYSTASK (sendrec operation), therefore its p_rts_flags*/
/* must have only de BIT_RECEIVING and the p_getfrom = SYSTASK  */
/* The caller of mol_migrate would be:				*/
/*	- MIGR_START: old node's SYSTASK 			*/
/*	- MIGR_COMMIT:   new node's SYSTASK 			*/
/*	- MIGR_ROLLBACK: old node's SYSTASK 			*/
/* dcid: get from caller => must be binded in the DC		*/
/* ONLY THE MAIN THREAD aka GROUP LEADER		*/
/* can be invoked in mol_migrate					*/
/* All threads in the process must be in the correct		*/
/* state to be migrated							*/
/*--------------------------------------------------------------*/
asmlinkage long mol_migrate(int oper, int pid, int dcid, int endpoint, int new_nodeid)
{
    return EMOLNOSYS;
}

	
