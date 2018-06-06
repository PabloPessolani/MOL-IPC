#ifndef DVS_USR_H
#define DVS_USR_H

struct dvs_usr {
	int	d_nr_dcs;
	int	d_nr_nodes;
	int 	d_nr_procs;
	int 	d_nr_tasks;
	int 	d_nr_sysprocs;

	int 	d_max_copybuf;
	int 	d_max_copylen;

	unsigned long int d_dbglvl;
	int	d_version;
	int	d_subver;
	int d_size_proc;
};
typedef struct dvs_usr dvs_usr_t;

#define DVS_USR_FORMAT "d_nr_dcs=%d d_nr_nodes=%d d_nr_procs=%d d_nr_tasks=%d d_nr_sysprocs=%d \n"
#define DVS_USR_FIELDS(p) p->d_nr_dcs,p->d_nr_nodes, p->d_nr_procs, p->d_nr_tasks, p->d_nr_sysprocs  

#define DVS_MAX_FORMAT "d_max_copybuf=%d d_max_copylen=%d\n"
#define DVS_MAX_FIELDS(p) p->d_max_copybuf,p->d_max_copylen  

#define DVS_VER_FORMAT "d_dbglvl=%lX version=%d.%d sizeof(proc)=%d\n"
#define DVS_VER_FIELDS(p) p->d_dbglvl, p->d_version, p->d_subver, p->d_size_proc  



#endif /* DVS_USR_H */