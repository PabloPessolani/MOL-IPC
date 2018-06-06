#include <mollib.h>
/* Tell PM to execute a process on other node				*/
/* M7_ENDPT1  		m7_i1	the remote endpoint		*/
/*M7_BIND_TYPE	m7_i2	bind type 				*/
/* 	LCL_BIND		means bind in the remote node a local proc */	
/* 	RMT_BIND	NOT VALID because any proc need to exec !!! */ 	
/* 	BKUP_BIND	a remote process is a backup of other 	*/	
/* 	REPLICA_BIND	a remote process is a replica of other     */
/* M7_NODEID	   	m7_i3	indicates a NODEID 		*/
/* M7_LEN	 	m7_i4	len in bytes of a copy argv[]	*/
/* M7_ARGV_PTR   	m7_p1	pointer to a copy of argv 	*/
/* return: PID of the remote running process or  error		*/
int mol_rexec(int nodeid, int bind_type, int endpoint, char *arg_v, int arg_len)
{
	message m __attribute__((aligned(0x1000)));
  	int ret;
	int rpid;

	LIBDEBUG("nodeid=%d bind_type=%d endpoint=%d arg_len=%d\n", 
			nodeid, bind_type, endpoint, arg_len);

	if( arg_len > MAXCOPYBUF)
		ERROR_RETURN(EMOL2BIG);
	if( bind_type < 0 || bind_type > MAX_BIND_TYPE || bind_type == RMT_BIND)
		ERROR_RETURN(EMOLINVAL);
	m.M7_NODEID   	= nodeid;
	m.M7_ENDPT1		= endpoint;
	m.M7_BIND_TYPE	= bind_type;
	m.M7_ARGV_PTR 	= arg_v;
	m.M7_LEN 		= arg_len;
	ret = molsyscall(PM_PROC_NR, MOLREXEC, &m);
	
	rpid = m.m_type; /* MINIX PID */
	LIBDEBUG("rpid=%d ret=%d\n", rpid, ret);
	if( rpid < 0 || ret < 0 ){                                          
        /* mnx_errno= rpid; */
	    ERROR_RETURN(-1);
	}
	return(rpid);
}