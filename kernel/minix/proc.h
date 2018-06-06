#ifndef PROC_H
#define PROC_H

#ifndef _CONFIG_H
#include "config.h"
#endif

/* =================================*/
/* USER SPACE INCLUDE FILES         */
/* =================================*/
#include "types.h"
#include "macros.h"
#include "const.h"
#include "proc_usr.h"
#include "proc_sts.h"
#include "dc_usr.h"
#include "node_usr.h"
#include "priv.h"

/* =================================*/
/* PROCESS DESCRIPTOR	             */
/* =================================*/
struct proc {

  proc_usr_t	 	p_usr;

  char *p_name_ptr;		/* for local processes is pointer to task_ptr->comm */		
  int p_rcode;			/* return code of an IPC operation */

  priv_t  p_priv;		/* privileges structure */

  struct list_head p_list;	/* head of list of procs wishing to send 	*/
  struct list_head p_link;	/* link of the list of procceses whishing to send to other process */

  struct list_head p_mlist;	/* head of list of procs waiting to send to a migrating process 	*/
  struct list_head p_mlink;	/* link of the list of procceses whishing to send to migrating process */

  struct list_head p_ulist;	/* head of list of procs waiting to process unbinding		  */
  struct list_head p_ulink;	/* link of the list of procceses waiting to process unbinding */
  
  message p_message;		/* buffer to store messages  			*/
  message *p_umsg;			/* Pointer to message buffer in user space 	*/

  wait_queue_head_t p_wqhead;	/* LINUX process wait queue head		*/
  int p_pseudosem;			/* pseudo semaphore 				*/	
  cmd_t	   p_rmtcmd;		/* remote command 				*/ 

// struct timer_list p_timer; 	/* IPC timeout TIMER	*/

  #if LOCK_PROC_TYPE == USE_PROC_RWLOCK
  rwlock_t   	p_rwlock;	/* LINUX spinlock to protect this proc	*/	
  #elif LOCK_PROC_TYPE == USE_PROC_MUTEX
   struct mutex p_mutex;	/* LINUX mutex to protect this proc	*/	
  #elif LOCK_PROC_TYPE == USE_PROC_SPINLOCK
	spinlock_t p_spinlock;	/* LINUX spinlock to protect this proc	*/	
  #elif LOCK_PROC_TYPE == USE_PROC_RWSEM
	struct rw_semaphore p_rwsem;	/* LINUX RW Semaphore to protect this proc	*/	
  #endif

  struct task_struct *p_task;	/* pointer to LINUX task structure */

  #if MOLPROFILING != 0
	int	 p_profline[MAX_PROF];
	uint64_t p_profiling[MAX_PROF];
  #endif

};
typedef struct proc proc_t;


/* =================================*/
/* CLUSTER NODE DESCRIPTOR	      */
/* =================================*/
struct cluster_node {

	node_usr_t n_usr;
	struct proc_dir_entry *n_node_dir;		/* nodeX directory under /proc/dvs 	*/
	struct proc_dir_entry *n_info_entry;		/* info file under /proc/dvs/nodeX 	*/
	struct proc_dir_entry *n_stats_entry;		/* stats file under /proc/dvs/nodeX 	*/
#if LOCK_NODE_TYPE == USE_NODE_RWLOCK
	rwlock_t   	n_rwlock;	/* LINUX rwlock to protect this NODE	*/	
#elif LOCK_NODE_TYPE == USE_NODE_MUTEX
	struct mutex n_mutex;	/* LINUX mutex to protect this NODE	*/	
#elif LOCK_NODE_TYPE == USE_NODE_RWSEM
	struct rw_semaphore n_rwsem;	/* LINUX RW semaphore to protect this NODE	*/
#elif LOCK_NODE_TYPE == USE_NODE_RCU
	spinlock_t n_spinlock;	/* LINUX spinlock to protect this NODE	*/	
#endif
};
typedef struct cluster_node cluster_node_t;

/* =================================*/
/* VIRTUAL MACHINE DESCRIPTOR	      */
/* =================================*/
struct dc_struct {

	dc_usr_t dc_usr;

	struct kref dc_kref;

#if LOCK_DC_TYPE == USE_DC_RWLOCK
	rwlock_t  dc_rwlock;
#elif LOCK_DC_TYPE == USE_DC_MUTEX
 	struct mutex dc_mutex;
#elif LOCK_DC_TYPE == USE_DC_RWSEM
 	struct rw_semaphore dc_rwsem;
#elif LOCK_DC_TYPE == USE_DC_RCU
 	spinlock_t dc_spinlock;
#endif

	struct proc_dir_entry *dc_DC_dir;		/* VMx directory under /proc/dvs */
	struct proc_dir_entry *dc_info_entry;	/* /proc/dvs/VMx/info */
	struct proc_dir_entry *dc_procs_entry;	/* /proc/dvs/VMx/procs */
	struct proc_dir_entry *dc_stats_entry;	/* /proc/dvs/VMx/stats */

	struct dentry 	*dc_DC_dbg;			/* /sys/kernel/debug/VMx */
	struct dentry  	*dc_procs_dbg;		/* /sys/kernel/debug/VMx/procs */
	int				dc_ref_dgb;			/* open reference count */

	struct proc *dc_proc;			/* Dynamic memory pointer to VM process table */ 
	struct proc **dc_sid2proc;
};
typedef struct dc_struct dc_desc_t;


	
#endif /* PROC_H */
