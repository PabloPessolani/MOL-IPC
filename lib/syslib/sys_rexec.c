#include "syslib.h"

/*===========================================================================*
 *				sys_rexec				     *
 *===========================================================================*/
int sys_rexec( int rmt_nodeid, int rmt_ep, int bind_type, 
			   int thrower_ep, int arg_len, char **argv_ptr)
{
	message m __attribute__((aligned(0x1000)));
	message *m_ptr;
	int rcode;

	LIBDEBUG("SYS_REXEC	rmt_nodeid=%d rmt_ep=%d bind_type=%d thrower_ep=%d arg_len=%d \n"
		,rmt_nodeid, rmt_ep, bind_type, thrower_ep, arg_len);
		
	if( arg_len > MAXCOPYBUF)
		ERROR_RETURN(EMOL2BIG);
	if( bind_type < 0 || bind_type > MAX_BIND_TYPE || bind_type == RMT_BIND)
		ERROR_RETURN(EMOLINVAL);
	
	m_ptr = &m;
	m_ptr->M7_NODEID = rmt_nodeid;
	m_ptr->M7_ENDPT1 = rmt_ep;
	m_ptr->M7_BIND_TYPE = bind_type;
	m_ptr->M7_LEN    = arg_len;
	m_ptr->M7_ARGV_PTR = (char *) argv_ptr;	
	m_ptr->M7_THROWER= (char *) thrower_ep;

	/* Send REXEC to remote SYSTASK */
	LIBDEBUG(MSG7_FORMAT, MSG7_FIELDS(m_ptr));	
	rcode = _taskcall(SYSTASK(rmt_nodeid), SYS_REXEC, &m);
	if(rcode < 0) ERROR_RETURN(rcode);

	/* Send REXEC to local SYSTASK */
	/*!!!!!!!!!!!!!!! ATENCION: Podria no ser necesario informar a la SYSTASK LOCAL */
	/* Se supone que por SPREAD, la SYSTASK local se entera de que se creo el proceso remoto */
	LIBDEBUG(MSG7_FORMAT, MSG7_FIELDS(m_ptr));	
	rcode = _taskcall(SYSTASK(local_nodeid), SYS_REXEC, &m);
	if(rcode < 0) ERROR_RETURN(rcode);

	LIBDEBUG("child_ep=%d\n",m_ptr->M7_ENDPT1);
	return(m_ptr->M7_ENDPT1);
}

