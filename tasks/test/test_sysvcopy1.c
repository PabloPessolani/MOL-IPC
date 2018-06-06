
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
	int dcid, src_pid, dst_pid, src_ep, dst_ep, src_nr, dst_nr, ret, i, maxbuf;
	int rqt_pid, rqt_ep, rqt_nr;
	message m, *m_ptr;
	long int  total_bytes=0;
	double t_start, t_stop, t_total,loopbysec, tput;
	char *buffer;

	  	if (argc != 2) {
    		printf ("usage: %s <bufsize> \n", argv[0]);
    		exit(1);
  	}

  	maxbuf = atoi(argv[1]);

	posix_memalign( (void *) &buffer, getpagesize(), maxbuf );
  	if (buffer== NULL) {
    		perror("malloc");
    		exit(1);
  	}
	printf("buffer %p\n",buffer);

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

	for(i = 0; i < maxbuf-1; i++)
		buffer[i] = ((i%25) + 'a');	
	buffer[maxbuf] = 0;
	
	m_ptr = &m;
	
	if( (src_pid = fork()) != 0 )	{		/* FIRST CHILD SOURCE */
		if( (dst_pid = fork()) != 0 )	{	/* PARENT  REQUESTER */
			printf("REQUESTER pause before RECEIVE\n");

			/* PSEUDO FORK of SOURCE process to BIND to SYSTASK*/
			printf("Sending SYSFORK request to SYSTEM src_nr=%d src_pid=%d\n",src_nr, src_pid);
	  		m_ptr->m_type  = SYS_FORK;
			m_ptr->PR_SLOT = src_nr;
			m_ptr->PR_PID  = src_pid;
   			printf("REQUESTER SEND " MSG5_FORMAT, MSG5_FIELDS(m_ptr));
			ret = mnx_sendrec(SYSTEM , (long) &m);
			if( ret != 0 )
			    	printf("REQUESTER SEND ret=%d\n",ret);
  			printf("REQUESTER RECEIVED " MSG5_FORMAT, MSG5_FIELDS(m_ptr));
			printf("REQUESTER src endpoint=%d\n",m_ptr->PR_ENDPT);
			src_ep = m_ptr->PR_ENDPT;

			/* PSEUDO FORK of DESTINATION process to BIND to SYSTASK*/
			printf("REQUESTER Sending SYSFORK request to SYSTEM dst_nr=%d dst_pid=%d\n",dst_nr, dst_pid);
	  		m_ptr->m_type  = SYS_FORK;
			m_ptr->PR_SLOT = dst_nr;
			m_ptr->PR_PID  = dst_pid;
   			printf("REQUESTER SEND " MSG5_FORMAT, MSG5_FIELDS(m_ptr));
			ret = mnx_sendrec(SYSTEM , (long) &m);
			if( ret != 0 )
			    	printf("REQUESTER SEND ret=%d\n",ret);
  			printf("REQUESTER RECEIVED " MSG5_FORMAT, MSG5_FIELDS(m_ptr));
			printf("REQUESTER dst endpoint=%d\n",m_ptr->PR_ENDPT);
			dst_ep = m_ptr->PR_ENDPT;

			/* WAIT for both children */
			wait(&ret);
			wait(&ret);

		} else {				/* SECOND CHILD: DESTINATION */

			sleep(10);
	
			/* fills the buffer  */
			for(i = 0; i < maxbuf-1; i++)
				buffer[i] = ((i%25) + 'a');	
			buffer[maxbuf] = 0;;	
			if( maxbuf < 80)
				printf("DESTINATION buffer before = %s\n", buffer);

			dst_pid = getpid();
			dst_ep = mnx_getep(dst_pid);
			do {
				src_ep = mnx_getep(src_pid);
				sleep(1);
			} while(src_ep < 0);

			/* RECEIVE to block SENDER  */		
    			ret = mnx_receive(ANY, (long) &m);
   			printf("DESTINATION RECEIVE " MSG5_FORMAT, MSG5_FIELDS(m_ptr));

			/* VCOPY REQUEST to SYSTASK */
			m_ptr->m_type 	    = SYS_VIRCOPY;
			m_ptr->CP_SRC_ADDR  = buffer; 	/* source offset within userspace 	*/
			m_ptr->CP_SRC_ENDPT = src_ep;	/* source process endpoint	*/
			m_ptr->CP_DST_ADDR  = buffer;	/* destination offset within userspace	*/
			m_ptr->CP_DST_ENDPT = dst_ep;	/* destination process endpoint		*/
			m_ptr->CP_NR_BYTES  = maxbuf;	/* number of bytes to copy		*/
   			printf("REQUESTER VCOPY " MSG5_FORMAT, MSG5_FIELDS(m_ptr));

			ret = mnx_sendrec(SYSTEM, &m);
   			printf("REQUESTER SENDREC ret =%d\n", ret);
			printf("REQUESTER VCOPY rcode=%d\n", m_ptr->m_type);

			if( maxbuf < 80)
	     			printf("DESTINATION buffer after = %s\n", buffer);

			/* TWO REPLIEST to SENDER and RECIVER to unblock them */
			mnx_send(src_ep, &m);

		}
	}else{						/* FIRST CHILD: SOURCE		*/
		sleep(10);

		for(i = 0; i < maxbuf-1; i++)
			buffer[i] = ((i%25) + 'A');	
		buffer[maxbuf] = 0;
		if( maxbuf < 80)
			printf("SOURCE buffer before = %s\n", buffer);

		src_pid = getpid();
		src_ep = mnx_getep(src_pid);

    		ret = mnx_sendrec(dst_nr, (long) &m);
		if( maxbuf < 80)
			printf("SOURCE buffer after = %s\n", buffer);

	}
 printf("\n");
  
 exit(0);
}



