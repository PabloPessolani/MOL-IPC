#include <asm/ptrace.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <linux/kernel.h>
#include <linux/semaphore.h>        

#include "stub_syscall.h"
#include "./kernel/minix/sys_config.h"
#include "./kernel/minix/config.h"
#include "./kernel/minix/ipc.h"
#include "./kernel/minix/proc.h"
#include "./kernel/minix/kipc.h"

void main(void)
 {
    	int ret, i;
	struct dc_user *(vmu_tab[])
	struct dc_user *vmu_ptr;
  	
	printf("Get DC table\nn");
	
	vmu_ptr = (struct dc_user *([])) malloc ( NR_DCS*sizeof(struct dc_user) ); 
	if( vmu_ptr == NULL) {
		printf("ERROR malloc  %d\n", errno);
		exit(1);
	}
	vmu_ptr = vmu_tab;
	for(i = 0; i < NR_DCS; i++, vmu_ptr++) {
		printf("dcid=%d, flags=%X, nr_procs=%d, nr_tasks=%d nr_sysprocs=%d dc_name=%s\n",
		vmu_ptr->dc_dcid,
		vmu_ptr->dc_flags,
		vmu_ptr->dc_nr_procs,
		vmu_ptr->dc_nr_tasks,
		vmu_ptr->dc_nr_sysprocs,
		vmu_ptr->dc_name);
	}

	free(vmtab_tab);
	exit(0);		
 }

