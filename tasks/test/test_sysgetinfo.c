#include "../../lib/syslib/syslib.h"

#include "../debug.h"

dvs_usr_t dvs;
int local_nodeid;
   
void  main ( int argc, char *argv[] )
{
	int i, dcid, child_pid, proc_pid,  proc_nr, proc_ep, child_ep, child_nr, ret;
	message m, *m_ptr;
	dc_usr_t dcu, *dc_ptr;
	dvs_usr_t dvs, *dvs_ptr;
	proc_usr_t proc_usr, *proc_ptr, *proc_tab;
	priv_usr_t *priv_ptr, *priv_tab;
	slot_t *slots_tab, *slot_ptr;

	int status;

    	if ( argc != 3) {
 	        TASKDEBUG( "Usage: %s <dcid> <proc> \n", argv[0] );
 	        exit(1);
	    }

	dcid = atoi(argv[1]);
	if ( dcid < 0 || dcid >= NR_DCS) {
 	        TASKDEBUG( "Invalid dcid [0-%d]\n", NR_DCS-1 );
 	        exit(1);
	    }

	proc_nr = atoi(argv[2]);

	proc_pid = getpid();

	local_nodeid = mnx_getdvsinfo(&dvs);
TASKDEBUG("local_nodeid=%d\n",local_nodeid);

	TASKDEBUG("Binding process %d to DC%d with proc_nr=%d\n",proc_pid,dcid,proc_nr);
	proc_ep = mnx_bind(dcid, proc_nr);
	if( proc_ep <= (EMOLBADNODEID) ) {
		TASKDEBUG("BIND ERROR %d\n",proc_ep);
		exit(proc_ep);
	}
	TASKDEBUG("Process proc_ep=%d\n",proc_ep);

	m_ptr  = &m;

	/*
	* GET DRVS INFO (GET_KINFO)
	*/
	TASKDEBUG("Sending SYS_GETINFO request %d to SYSTEM\n", GET_KINFO);
	m_ptr->m_type  = SYS_GETINFO;
	m_ptr->I_REQUEST = GET_KINFO;
	dvs_ptr = &dvs;
	m_ptr->I_VAL_PTR = (char*) &dvs;
	m_ptr->I_VAL_LEN = sizeof(dvs_usr_t);
   	TASKDEBUG("SEND " MSG1_FORMAT, MSG1_FIELDS(m_ptr));
	ret = mnx_sendrec(SYSTEM , (long) &m);
	if( ret != 0 )
	    	TASKDEBUG("SEND ret=%d\n",ret);
  	TASKDEBUG("RECEIVED " MSG1_FORMAT, MSG1_FIELDS(m_ptr));
      	TASKDEBUG(DRVS_USR_FORMAT,DRVS_USR_FIELDS(dvs_ptr));

	/*
	* GET DC INFO (GET_MACHINE)
	*/
	TASKDEBUG("Sending SYS_GETINFO request %d to SYSTEM\n", GET_MACHINE);
	m_ptr->m_type  = SYS_GETINFO;
	m_ptr->I_REQUEST = GET_MACHINE;
	dc_ptr = &dcu;
	m_ptr->I_VAL_PTR = (char*) &dcu;
	m_ptr->I_VAL_LEN = sizeof(dc_usr_t);
   	TASKDEBUG("SEND " MSG1_FORMAT, MSG1_FIELDS(m_ptr));
	ret = mnx_sendrec(SYSTEM , (long) &m);
	if( ret != 0 )
	    	TASKDEBUG("SEND ret=%d\n",ret);
  	TASKDEBUG("RECEIVED " MSG1_FORMAT, MSG1_FIELDS(m_ptr));
      	TASKDEBUG(DC_USR_FORMAT,DC_USR_FIELDS(dc_ptr));

	/*
	* GET PROC INFO (GET_PROC)
	*/
	TASKDEBUG("Sending SYS_GETINFO request %d to SYSTEM\n", GET_PROC);
	m_ptr->m_type  = SYS_GETINFO;
	m_ptr->I_REQUEST = GET_PROC;
	proc_ptr = &proc_usr;
	m_ptr->I_VAL_PTR = (char*) &proc_usr;
	m_ptr->I_VAL_LEN = sizeof(proc_usr_t);
	m_ptr->I_VAL_LEN2_E = SELF;
   	TASKDEBUG("SEND " MSG1_FORMAT, MSG1_FIELDS(m_ptr));
	ret = mnx_sendrec(SYSTEM , (long) &m);
	if( ret != 0 )
	    	TASKDEBUG("SEND ret=%d\n",ret);
  	TASKDEBUG("RECEIVED " MSG1_FORMAT, MSG1_FIELDS(m_ptr));
      	TASKDEBUG(PROC_USR_FORMAT,PROC_USR_FIELDS(proc_ptr));

	/*
	* GET SLOTS TAB (GET_SLOTSTAB)
	*/
	TASKDEBUG("Sending SYS_GETINFO request %d to SYSTEM\n", GET_SLOTSTAB);
	m_ptr->m_type  = SYS_GETINFO;
	m_ptr->I_REQUEST = GET_SLOTSTAB;
	slots_tab = malloc((dc_ptr->dc_nr_tasks + dc_ptr->dc_nr_procs)*sizeof(slot_t));
	if( slots_tab == NULL) {
		TASKDEBUG("slots_tab malloc error=%d\n", errno);
		exit(1);
	}
	m_ptr->I_VAL_PTR = (char*) slots_tab;
	m_ptr->I_VAL_LEN = ((dc_ptr->dc_nr_tasks + dc_ptr->dc_nr_procs)*sizeof(slot_t));
   	TASKDEBUG("SEND " MSG1_FORMAT, MSG1_FIELDS(m_ptr));
	ret = mnx_sendrec(SYSTEM , (long) &m);
	if( ret != 0 )
	    	TASKDEBUG("SEND ret=%d\n",ret);
  	TASKDEBUG("RECEIVED " MSG1_FORMAT, MSG1_FIELDS(m_ptr));

	/*
	* GET PROC TAB (GET_PROCTAB)
	*/
	TASKDEBUG("Sending SYS_GETINFO request %d to SYSTEM\n", GET_PROCTAB);
	m_ptr->m_type  = SYS_GETINFO;
	m_ptr->I_REQUEST = GET_PROCTAB;
	proc_tab = malloc((dc_ptr->dc_nr_tasks + dc_ptr->dc_nr_procs)*sizeof(proc_usr_t));
	if( proc_tab == NULL) {
		TASKDEBUG("malloc error=%d\n", errno);
		exit(1);
	}
	m_ptr->I_VAL_PTR = (char*) proc_tab;
	m_ptr->I_VAL_LEN = ((dc_ptr->dc_nr_tasks + dc_ptr->dc_nr_procs)*sizeof(proc_usr_t));
   	TASKDEBUG("SEND " MSG1_FORMAT, MSG1_FIELDS(m_ptr));
	ret = mnx_sendrec(SYSTEM , (long) &m);
	if( ret != 0 )
	    	TASKDEBUG("SEND ret=%d\n",ret);
  	TASKDEBUG("RECEIVED " MSG1_FORMAT, MSG1_FIELDS(m_ptr));


	/*
	* GET PRIV TAB (GET_PRIVTAB)
	*/
	TASKDEBUG("Sending SYS_GETINFO request %d to SYSTEM\n", GET_PRIVTAB);
	m_ptr->m_type  = SYS_GETINFO;
	m_ptr->I_REQUEST = GET_PRIVTAB;
	priv_tab = malloc((dc_ptr->dc_nr_tasks + dc_ptr->dc_nr_procs)*sizeof(priv_usr_t));
	if( priv_tab == NULL) {
		TASKDEBUG("malloc error=%d\n", errno);
		exit(1);
	}
	m_ptr->I_VAL_PTR = (char*) priv_tab;
	m_ptr->I_VAL_LEN = ((dc_ptr->dc_nr_tasks + dc_ptr->dc_nr_procs)*sizeof(priv_usr_t));
   	TASKDEBUG("SEND " MSG1_FORMAT, MSG1_FIELDS(m_ptr));
	ret = mnx_sendrec(SYSTEM , (long) &m);
	if( ret != 0 )
	    	TASKDEBUG("SEND ret=%d\n",ret);
  	TASKDEBUG("RECEIVED " MSG1_FORMAT, MSG1_FIELDS(m_ptr));


	/*
	* PRINT PROC , SLOTS AND PRIV TABLES 
	*/
	for( i= dc_ptr->dc_nr_sysprocs; i < (dc_ptr->dc_nr_tasks + dc_ptr->dc_nr_procs); i++) {
		slot_ptr = &slots_tab[i];
      	TASKDEBUG("[%d]" SLOTS_FORMAT,i, SLOTS_FIELDS(slot_ptr));
	}
	
	for( i=0; i < (dc_ptr->dc_nr_tasks + dc_ptr->dc_nr_procs); i++) {
		proc_ptr = &proc_tab[i];
		if(proc_ptr->p_lpid != -1) {
		      	TASKDEBUG(PROC_USR_FORMAT,PROC_USR_FIELDS(proc_ptr));
				priv_ptr = &priv_tab[i];
		      	TASKDEBUG(PRIV_USR_FORMAT,PRIV_USR_FIELDS(priv_ptr));
		}
	}



 }



