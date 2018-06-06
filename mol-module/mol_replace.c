//c104f3f0

/**
 * @todo: replace exit_unbind with mm_hyper.c:mm_exit_unbind()
 */

//#pragma once

#include "mol-ipc/kernel/minix/config.h"

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <linux/kallsyms.h>
#include <linux/kprobes.h>

#include "mol-ipc/kernel/mol.h"
#define MOL_MODULE 1
#include "mol-ipc/kernel/mol-proto.h"

/* space for global variables are defined here */
#ifdef EXTERN
#undef EXTERN
#define EXTERN
#define MOLHYPER
#endif
#include "mol-ipc/kernel/mol-glo.h"
#include "mol-ipc/kernel/mol-macros.h"

//#include "mm_holes.c"
#include "mm_procfs.c"
#include "mm_utils.c"
#include "mm_acks.c"
#include "mm_sproxy.c"
#include "mm_rproxy.c"
#include "mm_hyper.c"
#include "mm_ipc.c"
#include "mm_migrate.c"
#include "mm_debugfs.c"

MODULE_LICENSE("GPL");

extern int send_sig_info(int, struct siginfo *, struct task_struct *);
extern struct cpuinfo_x86 boot_cpu_data;

static spinlock_t kern_lock = SPIN_LOCK_UNLOCKED;
unsigned long slock_flags;

#define LOCK_KERN spin_lock_irqsave(&kern_lock, slock_flags)
#define UNLOCK_KERN spin_unlock_irqrestore(&kern_lock, slock_flags)

#define MOL_ROUTINES_NR (NR_MOLCALLS+1)   /* how many */

/*
    "\xbf\x00\x00\x00\x00" means 'mov $0,%edi'
    "\xff\xe7" means 'jmp *%edi'
*/
#define PR_JUMP_STRING  "\xbf\x00\x00\x00\x00\xff\xe7"

typedef unsigned char pr_jump[7];
typedef unsigned char pr_save[7];
char* mol_sys_routine_names[MOL_ROUTINES_NR] = {
    "mol_dc_init",
    "mol_mini_send",
    "mol_mini_receive",
    "mol_void3",
    "mol_mini_notify",
    "mol_mini_sendrec",
    "mol_mini_rcvrqst",
    "mol_mini_reply",
    "mol_dc_end",
    "mol_bind",
    "mol_unbind",
    "mol_dc_dump",
    "mol_proc_dump",
    "mol_getpriv",
    "mol_setpriv",
    "mol_vcopy",
    "mol_getdcinfo",
    "mol_getprocinfo",
    "mol_void18",
    "mol_mini_relay",
    "mol_proxies_bind",
    "mol_proxies_unbind",
    "mol_getnodeinfo",
    "mol_put2lcl",
    "mol_get2rmt",
    "mol_add_node",
    "mol_del_node",
    "mol_dvs_init",
    "mol_dvs_end",
    "mol_getep",
    "mol_getdvsinfo",
    "mol_proxy_conn",
    "mol_wait4bind",
    "mol_migrate",   
    "mol_node_up",
    "mol_node_down",
    "mol_getproxyinfo",
	"mol_wakeup",
    "exit_unbind"
};   /* the ones we are gonna replace */

long mol_new_routines[MOL_ROUTINES_NR] = {
    (long)mm_dc_init,
    (long)mm_mini_send,
    (long)mm_mini_receive,
    (long)mm_void3,
    (long)mm_mini_notify,
    (long)mm_mini_sendrec,
    (long)mm_mini_rcvrqst,
    (long)mm_mini_reply,
    (long)mm_dc_end,
    (long)mm_bind,
    (long)mm_unbind,
    (long)mm_dc_dump,
    (long)mm_proc_dump,
    (long)mm_getpriv,
    (long)mm_setpriv,
    (long)mm_vcopy,
    (long)mm_getdcinfo,
    (long)mm_getprocinfo,
    (long)mm_void18,
    (long)mm_mini_relay,
    (long)mm_proxies_bind,
    (long)mm_proxies_unbind,
    (long)mm_getnodeinfo,
    (long)mm_put2lcl,
    (long)mm_get2rmt,
    (long)mm_add_node,
    (long)mm_del_node,
    (long)mm_dvs_init,
    (long)mm_dvs_end,
    (long)mm_getep,
    (long)mm_getdvsinfo,
    (long)mm_proxy_conn,
    (long)mm_wait4bind,
    (long)mm_migrate,
    (long)mm_node_up,
    (long)mm_node_down,
    (long)mm_getproxyinfo,
    (long)mm_wakeup,
    (long)mm_exit_unbind,
};
//long mol_new_routines[MOL_ROUTINES_NR]; /* new routines pointers stotrage */
static pr_jump pr_jump_array[MOL_ROUTINES_NR];  /* jump string storage */
static pr_save pr_save_array[MOL_ROUTINES_NR];    /* an array of pr_save[7] */
unsigned char* mol_old_routines[MOL_ROUTINES_NR];  /* storage of original routine addresses */

static int __init new_mol_replace_init(void) {
    
    int i = 0;
    pr_jump pr_jump_string = PR_JUMP_STRING;
    
    /* some initialization */
    for (i = 0; i < MOL_ROUTINES_NR; i++) {
        mol_old_routines[i] = (unsigned char *) 0x0;
        memcpy(pr_jump_array[i], pr_jump_string, 7);
    }
    
    LOCK_KERN;
    
    /* get addresses of the functions to replace */
    for(i = 0; i < MOL_ROUTINES_NR; i++) {
        *(long *)&pr_jump_array[i][1] = (long)mol_new_routines[i];
        mol_old_routines[i] = (unsigned char *)kallsyms_lookup_name(mol_sys_routine_names[i]);
        printk("Replacing %s (<%p> by <%p>)\n", 
			mol_sys_routine_names[i], mol_old_routines[i],mol_new_routines[i]);
        memcpy(pr_save_array[i], mol_old_routines[i], 7);
        memcpy(mol_old_routines[i], pr_jump_array[i], 7);
    }    
	printk("Address of mm_exit_unbind=<%p>\n", &mm_exit_unbind);
    UNLOCK_KERN;
    return 0;
}

static void __exit new_mol_replace_exit(void)
{
    int i = 0;
    
    LOCK_KERN;
    /* restore the old initial 7 bytes back */
    for(i = 0; i < MOL_ROUTINES_NR; i++) {
        printk("Restoring %s...\n", mol_sys_routine_names[i]);
        memcpy(mol_old_routines[i], pr_save_array[i], 7);
    }
    UNLOCK_KERN;
}

EXPORT_SYMBOL(k_proxies_bind); 
EXPORT_SYMBOL(mm_proxy_conn); 
EXPORT_SYMBOL(mm_put2lcl);
EXPORT_SYMBOL(mm_get2rmt);
EXPORT_SYMBOL(mm_node_up);

EXPORT_SYMBOL(dvs);

module_init(new_mol_replace_init);
module_exit(new_mol_replace_exit);


