/****************************************************************/
/*			MINIX IPC KERNEL 			*/
/* It uses the same Interrupt Vector than MINIX (0x21)		*/
/* It considers different Virtual Machines (Virtual Minix Envi-	*/
/* ronments or DCE) that let execute multiple Minix Over Linux	*/
/* on the same real machine					*/
/* MINIX IPC Calls: send, receive, sendrec, notify, vcopy 	*/
/* Hypervisor Calls: dc_init, dc_dump, proc_dump, bind, unbind  */
/* setpriv							*/ 
/****************************************************************/

#include "mol.h"

#include "mol-proto.h"
#include "mol-glo.h"
#include "mol-macros.h"

/*2345678901234567890123456789012345678901234567890123456789012345678901234567*/

/*--------------------------------------------------------------*/
/*			mol_mini_send				*/
/*--------------------------------------------------------------*/
asmlinkage long mol_mini_send(int dst_ep, message* m_ptr, long timeout_ms)
{
    return EMOLNOSYS;
}

/*--------------------------------------------------------------*/
/*			mol_mini_receive			*/
/* Receives a message from another MOL process of the same DC	*/
/* Differences with MINIX:					*/
/*	- The receiver copies the message from sender's buffer  */
/*	   to receiver's userspace 				*/
/*	- After a the receiver is unblocked, it must check if 	*/
/*	   it was for doing a copy command (CMD_COPY_IN, CMD_COPY_OUT)	*/
/*--------------------------------------------------------------*/
asmlinkage long mol_mini_receive(int src_ep, message* m_ptr, long timeout_ms)
{
    return EMOLNOSYS;
}

/*--------------------------------------------------------------*/
/*			mol_mini_sendrec			*/
/*--------------------------------------------------------------*/
asmlinkage long mol_mini_sendrec(int srcdst_ep, message* m_ptr, long timeout_ms)
{
    return EMOLNOSYS;
}

/*--------------------------------------------------------------*/
/*			mol_mini_notify				*/
/*--------------------------------------------------------------*/
asmlinkage long mol_mini_notify(int src_ep, int dst_ep)
{
    return EMOLNOSYS;
}

/*--------------------------------------------------------------*/
/*			mol_mini_relay				*/
/*--------------------------------------------------------------*/
asmlinkage long mol_mini_relay(int dst_ep, message* m_ptr)
{
    return EMOLNOSYS;
}

/*----------------------------------------------------------------*/
/*			mol_vcopy				*/
/* This function is used to:			*/
/*  - Copy messages: when src_ep =! NONE and bytes=sizeof(message) */
/*   - Copy data blocks: when src_ep == NONE	*/
/*----------------------------------------------------------------*/

asmlinkage long mol_vcopy(int src_ep, char *src_addr, int dst_ep,char *dst_addr, int bytes)
{
    return EMOLNOSYS;
}

/*--------------------------------------------------------------*/
/*			mol_mini_rcvrqst			*/
/* Receives a message from another MOL process of the same DC	*/
/* Differences with RECEIVE:					*/
/* 	The requester process must do sendrec => p_rts_flags =	*/
/*					(SENDING | RECEIVING)	*/
/*	The request can be ANY process 				*/
/*--------------------------------------------------------------*/
asmlinkage long mol_mini_rcvrqst(message* m_ptr, long timeout_ms)
{
    return EMOLNOSYS;
}

/*--------------------------------------------------------------*/
/*			mol_mini_reply					    */
/*--------------------------------------------------------------*/
asmlinkage long mol_mini_reply(int dst_ep, message* m_ptr, long timeout_ms)
{
    return EMOLNOSYS;
}


