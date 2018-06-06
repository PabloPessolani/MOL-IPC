#ifndef VM_USR_H
#define VM_USR_H

#define MAXVMNAME	16

struct VM_usr {
	int	vm_vmid;
	int	vm_flags;
	int 	vm_nr_procs;
	int 	vm_nr_tasks;
	int 	vm_nr_sysprocs;
	int 	vm_nr_nodes;
	unsigned long int vm_nodes; 		
	char	vm_name[MAXVMNAME];
#ifdef MOL_USERSPACE
	cpu_set_t vm_cpumask;
#else
	cpumask_t vm_cpumask;
#endif 
};
typedef struct VM_usr VM_usr_t;

#define VM_USR_FORMAT "vm_vmid=%d vm_nr_procs=%d vm_nr_tasks=%d vm_nr_sysprocs=%d vm_nr_nodes=%d flags=%X vm_nodes=%X vm_name=%s\n"
#define VM_USR_FIELDS(p) p->vm_vmid,p->vm_nr_procs, p->vm_nr_tasks, p->vm_nr_sysprocs, p->vm_nr_nodes,p->vm_flags, p->vm_nodes, p->vm_name

//http://lxr.free-electrons.com/source/include/linux/types.h#L9
// #define DECLARE_BITMAP(name,bits)    unsigned long name[BITS_TO_LONGS(bits)]
//http://lxr.free-electrons.com/source/include/linux/cpumask.h
// typedef struct cpumask { DECLARE_BITMAP(bits, NR_CPUS); } cpumask_t; 
#define VM_CPU_FORMAT "vm_vmid=%d vm_cpumask=%X vm_name=%s \n"
#define VM_CPU_FIELDS(p) p->vm_vmid,  (unsigned int) p->vm_cpumask.bits[0], p->vm_name

#endif /* VM_USR_H */