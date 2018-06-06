#include "../../lib/syslib/syslib.h"

dvs_usr_t dvs;
int local_nodeid;  
 
void  main ( int argc, char *argv[] )
{
	int dcid, child_pid, parent_pid,  parent_nr, parent_ep, child_ep, child_nr, ret;
	message m, *m_ptr;
	int status;

    	if ( argc != 4) {
 	        printf( "Usage: %s <dcid> <parent_nr> <child_nr>\n", argv[0] );
 	        exit(1);
	    }

	dcid = atoi(argv[1]);
	if ( dcid < 0 || dcid >= NR_DCS) {
 	        printf( "Invalid dcid [0-%d]\n", NR_DCS-1 );
 	        exit(1);
	    }

	parent_nr = atoi(argv[2]);
	child_nr  = atoi(argv[3]);

	parent_pid = getpid();

	local_nodeid = mnx_getdvsinfo(&dvs);
printf("local_nodeid=%d\n",local_nodeid);
	

	printf("Binding process %d to DC%d with parent_nr=%d\n",parent_pid,dcid,parent_nr);
	parent_ep = mnx_bind(dcid, parent_nr);
	if( parent_ep < 0 ) {
		printf("BIND ERROR %d\n",parent_ep);
		exit(parent_ep);
	}
	printf("Process parent_ep=%d\n",parent_ep);

	m_ptr  = &m;
	if( (child_pid = fork()) > 0) { /* PARENT */	
		printf("Sending SYSFORK request to SYSTEM child_nr=%d child_pid=%d\n",child_nr, child_pid);
	  	m_ptr->m_type  = SYS_FORK;
		m_ptr->PR_SLOT = child_nr;
		m_ptr->PR_PID  = child_pid;
   		printf("SENDREC " MSG5_FORMAT, MSG5_FIELDS(m_ptr));
		ret = mnx_sendrec(SYSTEM , (long) &m);
		if( ret != 0 )
		    	printf("SEND ret=%d\n",ret);
  		printf("RECEIVED " MSG5_FORMAT, MSG5_FIELDS(m_ptr));
		printf("child endpoint=%d\n",m_ptr->PR_ENDPT);
		child_ep = m_ptr->PR_ENDPT;
		sleep(5);

		printf("Sending SYSTIMES request to SYSTEM child_nr=%d child_pid=%d\n",child_nr, child_pid);
	  	m_ptr->m_type  = SYS_TIMES;
		m_ptr->T_ENDPT = child_ep;
  		printf("SENDREC  " MSG4_FORMAT, MSG4_FIELDS(m_ptr));
		ret = mnx_sendrec(SYSTEM , (long) &m);
		if( ret != 0 )
		    	printf("SEND ret=%d\n",ret);
  		printf("RECEIVED " MSG4_FORMAT, MSG4_FIELDS(m_ptr));

		printf("Sending SYSEXIT request to SYSTEM child_ep=%d\n",child_ep);
	  	m_ptr->m_type  = SYS_EXIT;
		m_ptr->PR_ENDPT = child_ep;
   		printf("SEND " MSG1_FORMAT, MSG1_FIELDS(m_ptr));
		ret = mnx_send(SYSTEM, (long) &m);
		if( ret != 0 )
		    	printf("SEND ret=%d\n",ret);
 
	wait(&status);
	}else{				/* CHILD */
		sleep(30);
	}
	
 }



