#include "syslib.h"
#include <./i386-linux-gnu/bits/posix1_lim.h>

/*===========================================================================*
 *				sys_bindproc				     *
 * A System process:
 *		- SERVER/TASK: is just registered to the kernel but not to SYSTASK.
  *		- REMOTE USER: it must be registered to the kernel and to SYSTASK     *
 * m_in.M3_LPID: Linux PID 		or node ID				     *
 * m_in.M3_SLOT < dc_ptr->dc_nr_sysprocs: for servers and tasks
 * m_in.M3_OPER  could be
  *	SELF_BIND		0
 * 	LCL_BIND		1
 *	 RMT_BIND		2
 * 	BKUP_BIND		3
 *===========================================================================*/
char* get_process_name_by_pid( int pid, char *name);
 
int sys_rbindproc(int sysproc_ep, int lpid, int oper, int st_nodeid)
{
	message m __attribute__((aligned(0x1000)));
	message *m_ptr;
	int rcode;
	
	LIBDEBUG("SYS_BINDPROC request to SYSTEM(%d) sysproc_ep=%d lpid=%d oper=%X\n",
		st_nodeid, sysproc_ep, lpid, oper);
	m_ptr = &m;
	m_ptr->M3_ENDPT = sysproc_ep;
	m_ptr->M3_LPID  = lpid;
	m_ptr->M3_OPER  = (char *) oper;
	if( st_nodeid == local_nodeid && (oper != RMT_BIND) ){
		char* name = (char*)calloc(_POSIX_PATH_MAX+1,sizeof(char));
		strncpy(m_ptr->m3_ca1,basename(get_process_name_by_pid(lpid, name)), (M3_STRING-1));
		free(name);
	}
	
	rcode = _taskcall(SYSTASK(st_nodeid), SYS_BINDPROC, &m);
	if( rcode < 0) return(rcode);
	LIBDEBUG("endpoint=%d\n",m_ptr->PR_ENDPT);
	return(m_ptr->PR_ENDPT);
}

 char* get_process_name_by_pid( int pid, char *name)
{
	sprintf(name, "/proc/%d/cmdline",pid);
    FILE* f = fopen(name,"r");
    if(f){
        size_t size;
		size = fread(name, sizeof(char), 1024, f);
        if(size>0){
            if('\n'==name[size-1])
                name[size-1]='\0';
        }
        fclose(f);
    }
	LIBDEBUG("name=[%s]\n",name);
    return name;
}
