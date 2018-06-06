/****************************************************************/
/*		MINIX OVER LINUX IPC PRIMITIVES FOR PROXIES	*/
/****************************************************************/

#include "mol.h"
#include "mol-proto.h"
#include "mol-glo.h"
#include "mol-macros.h"

asmlinkage long mol_bind(int dcid, int pid, int proc, int nodeid);

/*--------------------------------------------------------------*/
/*			mol_get2rmt				*/
/* proxy gets local (messages, notifies, errors, ups, data,etc  */
/* to send to a remote processes	 			*/
/* usr_hdr_ptr: buffer address in userspace for the header	*/
/* usr_pay_ptr: buffer address in userspace for the payload	*/
/*--------------------------------------------------------------*/
asmlinkage long mol_get2rmt(proxy_hdr_t *usr_hdr_ptr, proxy_payload_t *usr_pay_ptr)
{
    return EMOLNOSYS;
}



