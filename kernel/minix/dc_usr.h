#ifndef DC_USR_H
#define DC_USR_H

#define MAXDCNAME	16

struct dc_usr {
	int		dc_dcid;
	int		dc_flags;					/* changeable field */
	int 	dc_nr_procs;
	int 	dc_nr_tasks;
	int 	dc_nr_sysprocs;
	int 	dc_nr_nodes;
	unsigned long int dc_nodes; 	/* changeable field */
	int		dc_warn2proc;			/* which process to inform when a process exit  */
	int		dc_warnmsg;			/* with this message type						*/
	char	dc_name[MAXDCNAME];
#ifdef MOL_USERSPACE
	cpu_set_t dc_cpumask;
#else
	cpumask_t dc_cpumask;
#endif 
};
typedef struct dc_usr dc_usr_t;

#define DC_USR_FORMAT "dc_dcid=%d dc_nr_procs=%d dc_nr_tasks=%d dc_nr_sysprocs=%d dc_nr_nodes=%d flags=%X dc_nodes=%lX dc_name=%s\n"
#define DC_USR_FIELDS(p) p->dc_dcid,p->dc_nr_procs, p->dc_nr_tasks, p->dc_nr_sysprocs, p->dc_nr_nodes,p->dc_flags, p->dc_nodes, p->dc_name

//http://lxr.free-electrons.com/source/include/linux/types.h#L9
// #define DECLARE_BITMAP(name,bits)    unsigned long name[BITS_TO_LONGS(bits)]
//http://lxr.free-electrons.com/source/include/linux/cpumask.h
// typedef struct cpumask { DECLARE_BITMAP(bits, NR_CPUS); } cpumask_t; 
#define DC_CPU_FORMAT "dc_dcid=%d dc_cpumask=%X dc_name=%s \n"
#define DC_CPU_FIELDS(p) p->dc_dcid,  (unsigned int) p->dc_cpumask.bits[0], p->dc_name

#define DC_WARN_FORMAT "dc_dcid=%d dc_warn2proc=%d dc_warnmsg=%d\n"
#define DC_WARN_FIELDS(p) p->dc_dcid, p->dc_warn2proc, p->dc_warnmsg 

#endif /* DC_USR_H */