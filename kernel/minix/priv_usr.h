#ifndef PRIV_USR_H
#define PRIV_USR_H

#define USER_PRIV	0x0000
#define SERVER_PRIV	0x0001
#define TASK_PRIV	0x0002
#define SYSTEM_PRIV	0x0003
#define KERNEL_PRIV	0x0004
#define PROXY_PRIV	0x0005

struct priv_usr {

  sys_id_t s_id;		/* index of this system structure */
  int	s_warn;			/* process to warn when the process exit/fork */
  int	s_level;		/* privilege level		*/

  short s_trap_mask;		/* allowed system call traps */
  sys_map_t s_ipc_from;		/* allowed callers to receive from */
  sys_map_t s_ipc_to;		/* allowed destination processes */
  long s_call_mask;			/* allowed kernel calls */

  moltimer_t s_alarm_timer;	/* synchronous alarm timer */ 

};
typedef struct priv_usr priv_usr_t;

#define PRIV_USR_FORMAT "s_id=%d s_warn=%d s_level=%d trap=%X call=%X\n"
#define PRIV_USR_FIELDS(p) p->s_id, p->s_warn, p->s_level,(unsigned int)p->s_trap_mask,(unsigned int) p->s_call_mask

#endif /* PRIV_USR_H */
