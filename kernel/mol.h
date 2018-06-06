/****************************************************************/
/*			MOL.H				 			*/
/****************************************************************/
#include <asm/page.h>
#include <linux/kernel.h>
#include <linux/semaphore.h>        
#include <linux/module.h>	/* THIS IS FOR EXPORT_SYMBOL!!! */
#include <linux/bitops.h>
#include <linux/bitmap.h>
#include <linux/sched.h>                    
#include <linux/list.h>                    
#include <linux/syscalls.h>   
#include <linux/vmalloc.h>                 
#include <linux/timer.h>                 
#include <linux/kfifo.h>                 
#include <linux/wait.h>
#include <linux/proc_fs.h>
#include <linux/signal.h>
#include <linux/log2.h>
#include <linux/kref.h>
#include <linux/debugfs.h>
#include <asm/unistd.h>  
#include <asm/uaccess.h>
#include <asm/spinlock.h>
#include <asm/pgalloc.h>
#include <asm/pgtable.h>
/* this header files wraps some common module-space operations ... 
   here we use mem_map_reserve() macro */ 
#include <linux/wrapper.h> 
/* needed for virt_to_phys() */ 
#include <asm/io.h> // virt_to_phys()
/* needed for remap_page_range */ 
#include <linux/mm.h>

#include "../arch/x86/include/asm/processor.h"

#include "./minix/config.h"
#include "./minix/const.h"
#include "./minix/ipc.h"
#include "./minix/kipc.h"
#include "./minix/timers.h"
#include "./minix/cmd.h"
#include "./minix/timers.h"
#include "./minix/proc.h"
#include "./minix/proxy_sts.h"
#include "./minix/proxy_usr.h"
#include "./minix/proxy.h"
#include "./minix/molerrno.h"
#include "./minix/endpoint.h"
#include "./minix/moldebug.h"
#include "./minix/dvs_usr.h"
#include "./minix/callnr.h"

#include "mol-const.h"
