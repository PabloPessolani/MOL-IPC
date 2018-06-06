#include <mollib.h>

int mol_setpname(int proc_ep, char *proc_name )
/* int *proc_ep;			 process number to set  */
/* char *proc_name;		 name of process to set for */
{
	message m __attribute__((aligned(0x1000)));
  int ret;
 
  m.M3_ENDPT =  proc_ep;			/* search by name */
  strncpy(&m.M3_NAME, proc_name,M3_STRING-1);
  if ((ret = molsyscall(PM_PROC_NR, MOLSETPNAME, &m) < 0)) return(ret);
  return(ret);
}

