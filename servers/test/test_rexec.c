#include "test.h"

/*===========================================================================*
 *				main					     *
 *===========================================================================*/
void  main ( int argc, char *argv[] )
{
	
	int i, pid ;
	
	for( i = 0 ; i < 10 ; i++){
		pid = mol_getpid();
		printf("%s pid=%d\n", argv[0], pid);
		sleep(2);
	}
	mol_exit(0);
	exit(0);
 }


	
