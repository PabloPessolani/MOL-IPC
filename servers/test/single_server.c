#include "test.h"

/*===========================================================================*
 *				main					     *
 *===========================================================================*/
void  main ( int argc, char *argv[] )
{
	int ret;
	message *m_ptr;

	printf("SINGLE_SERVER\n");

	/*---------------- Allocate memory for message  ---------------*/
	posix_memalign( (void **) &m_ptr, getpagesize(), sizeof(message) );
	if (m_ptr== NULL) {
   		fprintf(stderr, "SERVER posix_memalign\n");
   		exit(1);
	}
	printf("SERVER m_ptr=%p\n",m_ptr);
	
	while(TRUE){
    	ret = mnx_receive(ANY, (long) m_ptr);
		if(ret < 0) {
			fprintf(stderr,"SERVER mnx_receive ret=%d\n", ret);
			exit(1);		
		}		
		printf(MSG1_FORMAT, MSG1_FIELDS(m_ptr));
		
		m_ptr->m1_i1= 1;
		m_ptr->m1_i2= 2;
		m_ptr->m1_i3= 3;
		
   		ret = mnx_send(m_ptr->m_source, (long) m_ptr);
		if(ret < 0) {
			fprintf(stderr,"SERVER mnx_send ret=%d\n", ret);
			exit(1);		
		}	
	}	
 }


	
