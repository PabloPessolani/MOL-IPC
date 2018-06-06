#include "syslib.h"
/* src_proc;			source process 			*/
/* src_vir;				source virtual address 		*/
/* dst_proc;			destination process 		*/
/* dst_vir;				destination virtual address 	*/
/* bytes;				how many bytes 			*/


int sys_vircopy(int src_proc, void *src_vir, int dst_proc, void *dst_vir, int bytes)
{
/* Transfer a block of data.  The source and destination can each either be a
 * process number or SELF (to indicate own process number). Virtual addresses 
 * are offsets within LOCAL_SEG (text, stack, data), REMOTE_SEG, or BIOS_SEG. 
 */

  message copy_mess __attribute__((aligned(0x1000)));

  LIBDEBUG("SYS_VIRCOPY request to SYSTEM(%d) src_proc=%d dst_proc=%d bytes=%d\n",
		src_proc, dst_proc, bytes);
		
  if (bytes == 0L) return(OK);
  copy_mess.CP_SRC_ENDPT = src_proc;
  copy_mess.CP_SRC_ADDR = (long) src_vir;
  copy_mess.CP_DST_ENDPT = dst_proc;
  copy_mess.CP_DST_ADDR = (long) dst_vir;
  copy_mess.CP_NR_BYTES = (long) bytes;
  return(_taskcall(SYSTASK(local_nodeid), SYS_VIRCOPY, &copy_mess));
}
