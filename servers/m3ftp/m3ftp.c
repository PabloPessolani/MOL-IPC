#include "m3ftp.h"

int local_nodeid;
int ftp_lpid;
int ftp_ep, ftpd_ep;
dvs_usr_t dvs, *dvs_ptr;
dc_usr_t  dcu, *dc_ptr;
proc_usr_t m3ftp, *m3ftp_ptr;	
proc_usr_t m3ftpd, *m3ftpd_ptr;	
FILE *fp;
message *m_ptr;
char *path_ptr;
char *data_ptr;

#define WAIT4BIND_MS 	1000
#define SEND_RECV_MS 		10000

/*===========================================================================*
 *				ftp_reply					     *
 *===========================================================================*/
void ftp_reply(int rcode)
{
	int ret;
	
	m_ptr->m_type = rcode;
	ret = mnx_send_T(m_ptr->m_source, m_ptr, SEND_RECV_MS);
	if (rcode == EMOLTIMEDOUT) {
		SVRDEBUG("M3FTP: ftp_reply rcode=%d\n", rcode);
		return;
	}
	if( rcode < 0) ERROR_EXIT(rcode);
	return;
}
/*===========================================================================*
 *				ftp_init					     *
 *===========================================================================*/
void ftp_init(void)
{
  	int rcode;

	ftp_lpid = getpid();
	SVRDEBUG("ftp_lpid=%d\n", ftp_lpid);

	do { 
		rcode = mnx_wait4bind_T(WAIT4BIND_MS);
		SVRDEBUG("M3FTP: mnx_wait4bind_T  rcode=%d\n", rcode);
		if (rcode == EMOLTIMEDOUT) {
			SVRDEBUG("M3FTP: mnx_wait4bind_T TIMEOUT\n");
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

	ftp_ep = mnx_getep(ftp_lpid);
	SVRDEBUG("Get M3FTP endpoint=%d info from SYSTASK\n", ftp_ep);
	rcode = sys_getproc(&m3ftp, ftp_ep);
	if(rcode) ERROR_EXIT(rcode);
	m3ftp_ptr = &m3ftp;
	SVRDEBUG(PROC_USR_FORMAT,PROC_USR_FIELDS(m3ftp_ptr));
	
	SVRDEBUG("Get M3FTPD endpoint=%d info from SYSTASK\n", ftpd_ep);
	rcode = sys_getproc(&m3ftpd, ftpd_ep);
	if(rcode) ERROR_EXIT(rcode);
	m3ftpd_ptr = &m3ftpd;
	SVRDEBUG(PROC_USR_FORMAT,PROC_USR_FIELDS(m3ftpd_ptr))
	if( TEST_BIT(m3ftpd_ptr->p_rts_flags, BIT_SLOT_FREE)) {
		fprintf(stderr, "m3ftpd not started\n");	
	}
	
	/*---------------- Allocate memory for message  ---------------*/
	posix_memalign( (void **) &m_ptr, getpagesize(), sizeof(message) );
	if (m_ptr== NULL) {
   		ERROR_EXIT(-errno);
	}
	SVRDEBUG("M3FTP m_ptr=%p\n",m_ptr);

	/*---------------- Allocate memory for filename  ---------------*/
	posix_memalign( (void **) &path_ptr, getpagesize(), MNX_PATH_MAX );
	if (path_ptr== NULL) {
   		ERROR_EXIT(-errno);
	}
	SVRDEBUG("M3FTP path_ptr=%p\n",path_ptr);

	/*---------------- Allocate memory for data  ---------------*/
	posix_memalign( (void **) &data_ptr, getpagesize(), MAXCOPYBUF );
	if (data_ptr== NULL) {
   		ERROR_EXIT(-errno);
	}
	SVRDEBUG("M3FTP data_ptr=%p\n",data_ptr);	

	if(rcode) ERROR_EXIT(rcode);
	
}

void usage(void) {
	fprintf(stderr, "Usage: m3ftp {-p|-g} <srv_ep> <rmt_fname> <lcl_fname> \n");
	ERROR_EXIT(EMOLINVAL);
}

/*===========================================================================*
 *				main					     *
 * usage:    m3ftp {-p|-g} <srv_ep> <rmt_fname> <lcl_fname> * 
 * p: PUT
 * g: GET
  *===========================================================================*/
int main ( int argc, char *argv[] )
{
	int rcode, ret, rlen , oper, opt;
	char *dir_ptr;
	
	SVRDEBUG("M3FTP argc=%d\n", argc);

	if( argc != 5) usage();
		
	ftpd_ep = atoi(argv[2]);
	SVRDEBUG("M3FTP ftpd_ep=%d\n", ftpd_ep);
	ftp_init();
	dir_ptr = dirname(argv[0]);
	SVRDEBUG("M3FTP old cwd=%s\n", dir_ptr);
	rcode =  chdir(dir_ptr);
	if(rcode)ERROR_EXIT(-errno);
	dir_ptr = getcwd(path_ptr, MNX_PATH_MAX);
	SVRDEBUG("M3FTP new cwd=%s\n", dir_ptr);

	oper = FTP_NONE;
    while ((opt = getopt(argc, argv, "pg")) != -1) {
        switch (opt) {
			case 'p':
				oper = FTP_PUT;
				break;
			case 'g':
				oper = FTP_GET;
				break;
			default: /* '?' */
				usage();
				break;
        }
    }
	SVRDEBUG("oper=%d\n", oper);

	switch(oper){
		case FTP_PUT:
			SVRDEBUG("M3FTP FTP_PUT %s->%s\n", argv[3], argv[4]);
			fp = fopen(argv[4], "r");
			if(fp == NULL) ERROR_EXIT(-errno);
			m_ptr->FTPOPER 	= FTP_PUT;
			m_ptr->FTPPATH 	= argv[3];
			m_ptr->FTPPLEN  = strlen(argv[3]);
			do { 
				rlen = fread(data_ptr, 1, MAXCOPYBUF, fp);
				m_ptr->FTPDATA = data_ptr;
				m_ptr->FTPDLEN = rlen;
				SVRDEBUG("M3FTP: request " MSG1_FORMAT, MSG1_FIELDS(m_ptr));
				rcode = mnx_sendrec_T(ftpd_ep, m_ptr, SEND_RECV_MS);
				if ( rcode < 0) {ERROR_PRINT(rcode); break;}
				SVRDEBUG("M3FTP: reply   " MSG1_FORMAT, MSG1_FIELDS(m_ptr));
				if (m_ptr->m_type != OK) {
					rcode = m_ptr->m_type;
					ERROR_PRINT(rcode); 
					break;
				}
				m_ptr->FTPOPER = FTP_NEXT;
			}while(rlen > 0);
			break;
		case FTP_GET:
			SVRDEBUG("M3FTP FTP_GET %s->%s\n", argv[3], argv[4]);
			fp = fopen(argv[4], "w");
			if(fp == NULL) ERROR_EXIT(-errno);
			m_ptr->FTPOPER = FTP_GET;
			m_ptr->FTPPATH = argv[3];
			m_ptr->FTPPLEN = strlen(argv[3]);
			do {
				m_ptr->FTPDATA = data_ptr;
				m_ptr->FTPDLEN = MAXCOPYBUF;
				SVRDEBUG("M3FTP: request " MSG1_FORMAT, MSG1_FIELDS(m_ptr));
				rcode = mnx_sendrec_T(ftpd_ep, m_ptr, SEND_RECV_MS);
				if ( rcode < 0) {ERROR_PRINT(rcode); break;}
				SVRDEBUG("M3FTP: reply   " MSG1_FORMAT, MSG1_FIELDS(m_ptr));
				if (m_ptr->m_type != OK) {
					rcode = m_ptr->m_type;
					ERROR_PRINT(rcode); 
					break;
				}
				if( m_ptr->FTPDLEN < 0 || m_ptr->FTPDLEN > MAXCOPYBUF)
					ERROR_EXIT(EMOLMSGSIZE);
				if( m_ptr->FTPDLEN == 0) break; // EOF 			
				rcode = fwrite(data_ptr, 1, m_ptr->FTPDLEN, fp);
				if(rcode < 0 ) {ERROR_PRINT(rcode); break;};
				m_ptr->FTPOPER = FTP_NEXT;
			}while(TRUE);
			break;
		default:
			ERROR_EXIT(EMOLINVAL);
	}
	SVRDEBUG("M3FTP: CLOSE\n");
	fclose(fp);
	if ( rcode < 0) ERROR_PRINT(rcode);
}
	
