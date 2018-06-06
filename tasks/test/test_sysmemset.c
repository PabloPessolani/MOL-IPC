
#include "../../lib/syslib/syslib.h"


dvs_usr_t dvs;
int local_nodeid;

#define	printk	printf
#ifdef MOLDBG
#undef MOLDBG
#endif

#define 	DCID	0
#define 	RQT_NR	1
#define 	SRC_NR	2
#define 	DST_NR	3


double dwalltime()
{
	double sec;
	struct timeval tv;

	gettimeofday(&tv,NULL);
	sec = tv.tv_sec + tv.tv_usec/1000000.0;
	return sec;
}

   
void  main ( int argc, char *argv[] )
{
	int dcid, src_pid, dst_pid, src_ep, dst_ep, src_nr, dst_nr, ret, pat, maxbuf;
	int rqt_pid, rqt_ep, rqt_nr;
	message m, *m_ptr;
	long int  total_bytes=0;
	double t_start, t_stop, t_total,loopbysec, tput;
	char *buffer, *lclbuf;

	  	if (argc != 3) {
    		printf ("usage: %s <pattern> <bufsize> \n", argv[0]);
    		exit(1);
  	}

	pat    = *argv[1];
  	maxbuf = atoi(argv[2]);

	posix_memalign( (void *) &buffer, getpagesize(), maxbuf );
  	if (buffer== NULL) {
    		perror("malloc");
    		exit(1);
  	}
	printf("buffer %p\n",buffer);

	posix_memalign( (void *) &lclbuf, getpagesize(), 80);

	dcid 	= DCID;
	src_nr	= SRC_NR;
	dst_nr 	= DST_NR;
	rqt_nr 	= RQT_NR;

	local_nodeid = mnx_getdvsinfo(&dvs);
printf("local_nodeid=%d\n",local_nodeid);
	
	rqt_pid = getpid();
    	rqt_ep = mnx_bind(dcid, rqt_nr);
	if( rqt_ep < 0 ) 
		printf("BIND ERROR rqt_ep=%d\n",rqt_ep);

   	printf("BIND REQUESTER dcid=%d rqt_pid=%d rqt_nr=%d rqt_ep=%d\n",
		dcid,
		rqt_pid,
		rqt_nr,
		rqt_ep);
	
	ret = sys_memset(pat, buffer, maxbuf);
	if(ret) {
		printf("ret=%d\n", ret);
		exit(1);
	}

	if( maxbuf < 80)
		printf("buffer=%s\n", buffer);
	else {
		memcpy(lclbuf, buffer+maxbuf-70, 70);
		printf("buffer=%s\n", lclbuf);
 	}
	
  
 exit(0);
}



