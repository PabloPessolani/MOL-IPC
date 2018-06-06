
#include "test.h"

   
void  main ( int argc, char *argv[] )
{
	int dcid, pid, endpoint, p_nr, ret;
	priv_usr_t upriv, *p_ptr;


    	if ( argc != 3) {
 	        printf( "Usage: %s <dcid> <p_nr> \n", argv[0] );
 	        exit(1);
	    }

	dcid = atoi(argv[1]);
	endpoint = atoi(argv[2]);

	ret = mnx_getpriv(dcid, endpoint, &upriv );
	printf("getpriv dcid=%d endpoint=%d ret=%d\n",dcid, endpoint, ret);
	p_ptr = &upriv;
	printf("OLD PRIV "PRIV_USR_FORMAT, PRIV_USR_FIELDS(p_ptr));


	p_ptr->s_call_mask += 1;
	ret = mnx_setpriv(dcid, endpoint, p_ptr);
	printf("setpriv dcid=%d endpoint=%d ret=%d\n",dcid, endpoint, ret);
	p_ptr->s_call_mask = 0;

	ret = mnx_getpriv(dcid, endpoint, &upriv );
	printf("getpriv dcid=%d endpoint=%d ret=%d\n",dcid, endpoint, ret);
	p_ptr = &upriv;
	printf("NEW PRIV "PRIV_USR_FORMAT, PRIV_USR_FIELDS(p_ptr));



 }



