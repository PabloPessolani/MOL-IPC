#include <mollib.h>

/*===========================================================================*
 *				mol_bindproc()			     		*
 * bind a SYSTEM PROCESS previously binded to the kernel to the PM and SYSTASK	*
 * pid = MINIX pid or (-1)							*
 * lpid = Linux PID								*
 * p_endpoint = process endpoint							*
 * OPER = BIND TYPE
 * on return: the MINIX assigned PID, or ERROR (<0)				*
 *===========================================================================*/
int mol_bindproc(int p_endpoint, int pid, int lpid, int oper)
{
	int rcode;
	message m __attribute__((aligned(0x1000)));
	message *m_ptr;

LIBDEBUG("p_endpoint=%d pid=%d lpid=%d oper=%d\n",
	p_endpoint, pid, lpid, oper);

	/* Bind INIT into PM and SYSTASK */
	m_ptr = &m;
	m_ptr->M7_PID   = pid;	/* MINIX PID */
	m_ptr->M7_LPID  = lpid;	/* LINUX PID */
	m_ptr->M7_SLOT  = p_endpoint;
	m_ptr->M7_OPER  = oper;
	
	rcode = molsyscall(PM_PROC_NR, MOLBINDPROC, m_ptr);	
	if(rcode < 0) return(rcode);
	return(m_ptr->m_type);

}
