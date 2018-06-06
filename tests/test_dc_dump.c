#include <asm/ptrace.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "stub_syscall.h"
#include "./kernel/minix/config.h"
#include "./kernel/minix/ipc.h"
#include "./kernel/minix/kipc.h"

void main(void)
 {
    	int ret;
  	
	printf("Dump DCs\n"); 
   	ret = mnx_dc_dump();
      if( ret != OK){
		printf("mol_dc_dump error=%d\n",ret); 
		exit(1);
	}
	exit(0);		
 }

