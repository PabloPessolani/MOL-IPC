#include "m3ftp.h"

int local_nodeid;
int ftpd_lpid;
int ftpd_ep;
dvs_usr_t dvs, *dvs_ptr;
dc_usr_t  dcu, *dc_ptr;
proc_usr_t m3ftpd, *m3ftpd_ptr;	
FILE *fp;
message *m_ptr;
char *path_ptr;
char *data_ptr;

#define WAIT4BIND_MS 	1000
#define SEND_RECV_MS 		10000

double dwalltime()
{
	double sec;
	struct timeval tv;

	gettimeofday(&tv,NULL);
	sec = tv.tv_sec + tv.tv_usec/1000000.0;
	return sec;
}

/*===========================================================================*
 *				ftpd_reply					     *
 *===========================================================================*/
void ftpd_reply(int rcode)
{
	int ret;

	SVRDEBUG("ftpd_reply=%d\n", rcode);

	m_ptr->m_type = rcode;
	ret = mnx_send_T(m_ptr->m_source, m_ptr, SEND_RECV_MS);
	if (rcode == EMOLTIMEDOUT) {
		SVRDEBUG("M3FTPD: ftpd_reply rcode=%d\n", rcode);
		return;
	}
	if( rcode < 0) ERROR_PRINT(rcode);
	return;
}
/*===========================================================================*
 *				ftpd_init					     *
 *===========================================================================*/
void ftpd_init(void)
{
  	int rcode;

	ftpd_lpid = getpid();
	SVRDEBUG("ftpd_lpid=%d\n", ftpd_lpid);

	do { 
		rcode = mnx_wait4bind_T(WAIT4BIND_MS);
		SVRDEBUG("M3FTPD: mnx_wait4bind_T  rcode=%d\n", rcode);
		if (rcode == EMOLTIMEDOUT) {
			SVRDEBUG("M3FTPD: mnx_wait4bind_T TIMEOUT\n");
			continue ;
		}else if( rcode < 0) 
			ERROR_EXIT(EXIT_FAILURE);
	} while	(rcode < OK);
	
	SVRDEBUG("Get the DVS info from SYSTASK\n");
    rcode = sys_getkinfo(&dvs);
	if(rcode) ERROR_EXIT(rcode);
	dvs_ptr = &dvs;
	SVRDEBUG(DVS_USR_FORMAT,DVS_USR_FIELDS(dvs_ptr));

	SVRDEBUG("Get the VM info from SYSTASK\n");
	rcode = sys_getmachine(&dcu);
	if(rcode) ERROR_EXIT(rcode);
	dc_ptr = &dcu;
	SVRDEBUG(DC_USR_FORMAT,DC_USR_FIELDS(dc_ptr));

	ftpd_ep = mnx_getep(ftpd_lpid);

	SVRDEBUG("Get M3FTPD endpoint=%d info from SYSTASK\n", ftpd_ep);
	rcode = sys_getproc(&m3ftpd, ftpd_ep);
	if(rcode) ERROR_EXIT(rcode);
	m3ftpd_ptr = &m3ftpd;
	SVRDEBUG(PROC_USR_FORMAT,PROC_USR_FIELDS(m3ftpd_ptr));
	
	/*---------------- Allocate memory for message  ---------------*/
	posix_memalign( (void **) &m_ptr, getpagesize(), sizeof(message) );
	if (m_ptr== NULL) {
   		ERROR_EXIT(-errno);
	}
	SVRDEBUG("M3FTPD m_ptr=%p\n",m_ptr);

	/*---------------- Allocate memory for filename  ---------------*/
	posix_memalign( (void **) &path_ptr, getpagesize(), MNX_PATH_MAX );
	if (path_ptr== NULL) {
   		ERROR_EXIT(-errno);
	}
	SVRDEBUG("M3FTPD path_ptr=%p\n",path_ptr);

	/*---------------- Allocate memory for data  ---------------*/
	posix_memalign( (void **) &data_ptr, getpagesize(), MAXCOPYBUF );
	if (data_ptr== NULL) {
   		ERROR_EXIT(-errno);
	}
	SVRDEBUG("M3FTPD data_ptr=%p\n",data_ptr);	

	if(rcode) ERROR_EXIT(rcode);
	
}

/*===========================================================================*
 *				main					     *
 * REQUEST:
 *	FTPOPER = m_type => FTP_GET, FTP_PUT
 *      FTPPATH = m1p1 => pathname address
 *      FTPPLEN  = m1i1 => lenght of pathname 
 *      FTPDATA = m1p2 => DATA address
 *      FTPDLEN  = m1i2 => lenght of DATA
 *===========================================================================*/
int  main ( int argc, char *argv[] )
{
	int rcode, ret, rlen ;
	double t_start, t_stop, t_total;
	long total_bytes;
	FILE *fp;
	char *dir_ptr;

	SVRDEBUG("M3FTPD\n");
	ftpd_init();
	dir_ptr = dirname(argv[0]);
	SVRDEBUG("M3FTPD old cwd=%s\n", dir_ptr);
	rcode =  chdir(dir_ptr);
	if(rcode)ERROR_EXIT(-errno);
	dir_ptr = getcwd(path_ptr, MNX_PATH_MAX);
	SVRDEBUG("M3FTPD new cwd=%s\n", dir_ptr);
	
	
	while(TRUE){
		// Wait for file request 
		do { 
			rcode = mnx_receive_T(ANY,m_ptr, SEND_RECV_MS);
			SVRDEBUG("M3FTPD: mnx_receive_T  rcode=%d\n", rcode);
			if (rcode == EMOLTIMEDOUT) {
				SVRDEBUG("M3FTPD: mnx_receive_T TIMEOUT\n");
				continue ;
			}else if( rcode < 0) 
				ERROR_EXIT(EXIT_FAILURE);
		} while	(rcode < OK);
		SVRDEBUG("M3FTPD: " MSG1_FORMAT, MSG1_FIELDS(m_ptr));

		if (m_ptr->FTPOPER != FTP_GET && m_ptr->FTPOPER != FTP_PUT){
			fprintf(stderr,"M3FTPD: invalid request %d\n", m_ptr->FTPOPER);
			ftpd_reply(EMOLINVAL);
			continue;
		}

		if (m_ptr->FTPPLEN < 0 || m_ptr->FTPPLEN > (MNX_PATH_MAX-1)){
			fprintf(stderr,"M3FTPD: FTPPLEN=%d\n",  m_ptr->FTPPLEN);
			ftpd_reply(EMOLNAMESIZE);
			continue;			
		}
		
		if (m_ptr->FTPDLEN < 0 || m_ptr->FTPDLEN > (MAXCOPYBUF)){
			fprintf(stderr,"M3FTPD: FTDPLEN=%d\n",  m_ptr->FTPDLEN);
			ftpd_reply(EMOLMSGSIZE);
			continue;			
		}
		rcode = mnx_vcopy(m_ptr->m_source, m_ptr->FTPPATH,
							SELF, path_ptr, m_ptr->FTPPLEN);
		if( rcode < 0) break;
		path_ptr[m_ptr->FTPPLEN] = 0;
		SVRDEBUG("M3FTPD: path >%s<\n", path_ptr);
						
		switch(m_ptr->FTPOPER){
			case FTP_GET:
				SVRDEBUG("M3FTPD: FTP_GET\n");
				fp = fopen(path_ptr, "r");
				if (fp == NULL){
					rcode = (-errno);
					ERROR_PRINT(rcode); 
					break;
				}
				SVRDEBUG("M3FTPD: READ LOOP\n");
				t_start = dwalltime();
				total_bytes = 0;
				while( (rlen = fread(data_ptr, 1, m_ptr->FTPDLEN, fp)) > 0) {
					SVRDEBUG("M3FTPD: FTPDLEN=%d rlen=%d\n", m_ptr->FTPDLEN, rlen);			
					rcode = mnx_vcopy(SELF, data_ptr
								,m_ptr->m_source, m_ptr->FTPDATA
								,rlen);
					if(rcode < 0) {
						ERROR_PRINT(rcode); 
						break;
					}
					m_ptr->FTPDLEN = rlen;
					m_ptr->m_type = OK;					
					rcode = mnx_send_T(m_ptr->m_source, m_ptr, SEND_RECV_MS);
					if(rcode < 0) {
						ERROR_PRINT(rcode); 
						break;
					}
					total_bytes += rlen;
					rcode = mnx_receive_T(m_ptr->m_source,m_ptr, SEND_RECV_MS);
					if(rcode < 0) {
						ERROR_PRINT(rcode); 
						break;
					}
					if( m_ptr->FTPOPER != FTP_NEXT) {
						if( m_ptr->FTPOPER != FTP_CANCEL) {
							rcode = EMOLINVAL;
							ERROR_PRINT(rcode); 
						}
						break;
					}
				}
				SVRDEBUG("M3FTPD: CLOSE \n");
				ret = fclose(fp);		
				if(rcode < 0) {
					ERROR_PRINT(rcode); 
					break;
				}
				rcode = ret;
				m_ptr->FTPDLEN = 0;
				break;
			case FTP_PUT:
				SVRDEBUG("M3FTPD: FTP_PUT\n");
				fp = fopen(path_ptr, "w");					
				if (fp == NULL){
					rcode = -errno;
					ERROR_PRINT(rcode); 
					break;
				}
				SVRDEBUG("M3FTPD: WRITE LOOP\n");
				t_start = dwalltime();
				total_bytes = 0;
				while( m_ptr->FTPDLEN > 0) {
					rcode = mnx_vcopy(m_ptr->m_source, m_ptr->FTPDATA
								,SELF, data_ptr
								,m_ptr->FTPDLEN);
					if(rcode < 0) {
						ERROR_PRINT(rcode); 
						break;
					}
					rlen = fwrite(data_ptr, 1, m_ptr->FTPDLEN, fp);
					SVRDEBUG("M3FTPD: FTPDLEN=%d rlen=%d\n", m_ptr->FTPDLEN, rlen);			
					m_ptr->FTPDLEN = rlen;
					m_ptr->m_type = OK;					
					rcode = mnx_send_T(m_ptr->m_source, m_ptr, SEND_RECV_MS);
					if(rcode < 0) {
						ERROR_PRINT(rcode); 
						break;
					}
					total_bytes += rlen;
					rcode = mnx_receive_T(m_ptr->m_source,m_ptr, SEND_RECV_MS);
					if(rcode < 0) {
						ERROR_PRINT(rcode); 
						break;
					}
					if( m_ptr->FTPOPER != FTP_NEXT) {
						if( m_ptr->FTPOPER != FTP_CANCEL) {
							rcode = EMOLINVAL;
							ERROR_PRINT(rcode); 
						}
						break;
					}
				}
				SVRDEBUG("M3FTPD: CLOSE \n");
				ret = fclose(fp);		
				if(rcode < 0) {
					ERROR_PRINT(rcode); 
					break;
				}
				rcode = ret;
				break;
			default:
				fprintf(stderr,"M3FTPD: invalid request %d\n", m_ptr->FTPOPER);
				ERROR_EXIT(EMOLINVAL);
		}
		ftpd_reply(rcode);
		t_stop = dwalltime();
		/*--------- Report statistics  ---------*/
		t_total = (t_stop-t_start);
		fp = fopen("m3ftpd.txt","a+");
		fprintf(fp, "t_start=%.2f t_stop=%.2f t_total=%.2f\n",t_start, t_stop, t_total);
		fprintf(fp, "total_bytes = %ld\n", total_bytes);
		fprintf(fp, "Throuhput = %f [bytes/s]\n", (double)(total_bytes)/t_total);
		fclose(fp);
	}	
 }


	
