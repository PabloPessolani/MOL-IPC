#include "syslib.h"
/* INPUT;									*/
/* VCP_VEC_SIZE;		number of requests		*/
/* VCP_VEC_ADDR;		request vector address 	*/
/* OUTPUT;								*/
/* VCP_VEC_SIZE;		number of requests		*/
/* VCP_VEC_ADDR;		request vector address 	*/
/* VCP_NR_OK;			number of successfull copies	*/


int sys_virvcopy(int vect_size, void *vect_addr)
{
/* Transfer a block of data.  The source and destination can each either be a
 * process number or SELF (to indicate own process number). Virtual addresses 
 * are offsets within LOCAL_SEG (text, stack, data), REMOTE_SEG, or BIOS_SEG. 
 */
	int rcode;
	message copy_mess __attribute__((aligned(0x1000)));

	LIBDEBUG("SYS_VIRVCOPY request to SYSTEM vect_size=%d vect_addr=%X\n",
		vect_size, vect_addr);
		
	if (vect_size == 0L) return(OK);
  
	copy_mess.VCP_VEC_SIZE = vect_size;
	copy_mess.VCP_VEC_ADDR = (long) vect_addr;
	rcode = _taskcall(SYSTASK(local_nodeid), SYS_VIRVCOPY, &copy_mess);
	if(rcode != OK) ERROR_RETURN(rcode);
	return(copy_mess.VCP_NR_OK);
}
