#include "../../lib/syslib/syslib.h"


dvs_usr_t dvs;
int local_nodeid;
   
void  main ( int argc, char *argv[] )
{
	int dcid, user_pid, server_pid,  server_nr, server_ep, user_ep, user_nr, ret;
	message m, *m_ptr;
	priv_usr_t server_priv, *spriv_ptr;
	priv_usr_t user_priv, *upriv_ptr;
	int status;

    	if ( argc != 4) {
 	        printf( "Usage: %s <dcid> <server_nr> <user_nr> \n", argv[0] );
 	        exit(1);
	    }

	dcid = atoi(argv[1]);
	if ( dcid < 0 || dcid >= NR_DCS) {
 	        printf( "Invalid dcid [0-%d]\n", NR_DCS-1 );
 	        exit(1);
	    }

	server_nr = atoi(argv[2]);
	user_nr = atoi(argv[3]);

	server_pid = getpid();
	local_nodeid = mnx_getdvsinfo(&dvs);
printf("local_nodeid=%d\n",local_nodeid);
	

	printf("Binding process %d to DC%d with server_nr=%d\n",server_pid,dcid,server_nr);
	server_ep = mnx_bind(dcid, server_nr);
	if( server_ep < 0 ) {
		printf("BIND ERROR %d\n",server_ep);
		exit(server_ep);
	}
	printf("Process server_ep=%d\n",server_ep);
	
	spriv_ptr = &server_priv;
	ret = mnx_getpriv(dcid, server_ep, spriv_ptr);
	printf("SERVER getpriv dcid=%d server_ep=%d ret=%d\n",dcid, server_ep, ret);
	printf("OLD SERVER PRIV "PRIV_USR_FORMAT, PRIV_USR_FIELDS(spriv_ptr));

	spriv_ptr->s_level = SYSTEM_PRIV;
	ret = mnx_setpriv(dcid, server_ep, spriv_ptr);
	printf("SERVER setpriv dcid=%d endpoint=%d ret=%d\n",dcid, server_ep, ret);
	printf("NEW SERVER PRIV "PRIV_USR_FORMAT, PRIV_USR_FIELDS(spriv_ptr));

	m_ptr  = &m;
	if( (user_pid = fork()) > 0) { /* PARENT */	
		printf("Sending SYSFORK request to SYSTEM user_nr=%d user_pid=%d\n",user_nr, user_pid);
	  	m_ptr->m_type  = SYS_FORK;
		m_ptr->PR_SLOT = user_nr;
		m_ptr->PR_PID  = user_pid;
   		printf("SEND " MSG5_FORMAT, MSG5_FIELDS(m_ptr));
		ret = mnx_sendrec(SYSTEM , (long) &m);
		if( ret != 0 )
		    	printf("SEND ret=%d\n",ret);
  		printf("RECEIVED " MSG5_FORMAT, MSG5_FIELDS(m_ptr));
		printf("user endpoint=%d\n",m_ptr->PR_ENDPT);
		user_ep = m_ptr->PR_ENDPT;

		printf("Sending SYSPRIVCTL request to SYSTEM user_nr=%d user_pid=%d\n",user_nr, user_pid);
	  	m_ptr->m_type  = SYS_PRIVCTL;
	  	m_ptr->PR_ENDPT= user_ep;
		m_ptr->PR_PRIV = TASK_PRIV;
		ret = mnx_sendrec(SYSTEM, (long) &m);
		if( ret != 0 ) 
		    	printf("SEND ret=%d\n",ret);
  		printf("RECEIVED " MSG1_FORMAT, MSG1_FIELDS(m_ptr));
		
		upriv_ptr = &user_priv;
		ret = mnx_getpriv(dcid, user_ep, upriv_ptr);
		printf("USER getpriv dcid=%d user_ep=%d ret=%d\n",dcid, user_ep, ret);
		printf("NEW USER PRIV "PRIV_USR_FORMAT, PRIV_USR_FIELDS(upriv_ptr));

	wait(&status);
	}else{				/* CHILD */
		sleep(30);
	}
	
 }



