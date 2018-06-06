#include "../../lib/syslib/syslib.h"

dvs_usr_t dvs;
int local_nodeid;
 
void  main ( int argc, char *argv[] )
{
	int dcid, child_pid, parent_pid,  parent_nr, parent_ep, child_ep, child_nr, ret;
	message m, *m_ptr;
	int status;

    	if ( argc != 3) {
 	        printf( "Usage: %s <dcid> <parent_nr>\n", argv[0] );
 	        exit(1);
	    }

	dcid = atoi(argv[1]);
	if ( dcid < 0 || dcid >= NR_DCS) {
 	        printf( "Invalid dcid [0-%d]\n", NR_DCS-1 );
 	        exit(1);
	    }

	local_nodeid = mnx_getdvsinfo(&dvs);
printf("local_nodeid=%d\n",local_nodeid);

	parent_nr = atoi(argv[2]);

	parent_pid = getpid();

	printf("Binding process %d to DC%d with parent_nr=%d\n",parent_pid,dcid,parent_nr);
	parent_ep = mnx_bind(dcid, parent_nr);
	if( parent_ep < 0 ) {
		printf("BIND ERROR %d\n",parent_ep);
		exit(parent_ep);
	}
	printf("Process parent_ep=%d\n",parent_ep);

	parent_ep = sys_bindproc(parent_nr, parent_pid, LCL_BIND);
	if(parent_ep < 0) {
		printf("ERROR sys_bindproc parent_ep=%d\n",parent_ep);
		exit(1);
	}
		
	if( (child_pid = fork()) > 0) { /* PARENT */	
		child_ep = sys_fork(child_pid);
		if(child_ep < 0) {
			printf("ERROR sys_fork child_ep=%d\n",child_ep);
			exit(1);
		}
		child_nr = _ENDPOINT_P(child_ep);
		printf("child_nr=%d child_ep=%d\n",child_nr, child_ep);

//	  	m_ptr->m_type  = SYS_FORK;
//		m_ptr->PR_ENDPT = parent_ep;
//		m_ptr->PR_SLOT = child_nr;
//		m_ptr->PR_PID  = child_pid;
//  		printf("SEND " MSG1_FORMAT, MSG1_FIELDS(m_ptr));
//		ret = mnx_sendrec(SYSTEM , (long) &m);
//		if( ret != 0 )
//		    	printf("SEND ret=%d\n",ret);
//  		printf("RECEIVED " MSG1_FORMAT, MSG1_FIELDS(m_ptr));
//		printf("child endpoint=%d\n",m_ptr->PR_ENDPT);
//
//		printf("Sending SYSTIMES request to SYSTEM child_nr=%d child_pid=%d\n",child_nr, child_pid);
//	  	m_ptr->m_type  = SYS_TIMES;
//		ret = mnx_sendrec(SYSTEM , (long) &m);
//		if( ret != 0 )
//		    	printf("SEND ret=%d\n",ret);
//  		printf("RECEIVED " MSG4_FORMAT, MSG4_FIELDS(m_ptr));
		
		wait(&status);
	}else{				/* CHILD */
		sleep(30);
	}
	
 }



