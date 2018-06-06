
#ifndef PRIV_H
#define PRIV_H

#define SYS_PROC	0x10	/* system processes have own priv structure */

/* =================================*/
/* USER SPACE INCLUDE FILES         */
/* =================================*/
#include "priv_usr.h"

struct priv {

  priv_usr_t s_usr;		/* Privileges user fields 		*/

  sys_map_t s_notify_pending; /* bit map with pending notifications */
  irq_id_t 	s_int_pending;	/* pending hardware interrupts */
  ksigset_t s_sig_pending;	/* pending signals */
  update_t  s_updt_pending; /* bit map with pending system process updates  */

};
typedef struct priv priv_t;

#endif /* PRIV_H */
