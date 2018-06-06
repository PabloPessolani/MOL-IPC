/****************************************************************/
/*		MINIX OVER LINUX IPC PRIMITIVES FOR PROXIES	*/
/****************************************************************/

#include "mol.h"
#include "mol-proto.h"
#include "mol-glo.h"
#include "mol-macros.h"

asmlinkage long mol_bind(int dcid, int pid, int proc, int nodeid);


/*--------------------------------------------------------------*/
/*			mol_put2lcl				*/
/* RECEIVER proxy makes an operation to a local process requested*/
/* by a remote process						*/
/*--------------------------------------------------------------*/
asmlinkage long mol_put2lcl(proxy_hdr_t *usr_hdr_ptr, proxy_payload_t *usr_pay_ptr)
{
    return EMOLNOSYS;
}
