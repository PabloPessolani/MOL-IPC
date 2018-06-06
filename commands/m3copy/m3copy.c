#include "m3copy.h"

int local_nodeid;
int m3c_lpid;
int m3c_ep;
dvs_usr_t dvs, *dvs_ptr;
dc_usr_t  vmu, *dc_ptr;
proc_usr_t m3copy, *m3c_ptr;	
proc_usr_t fs, *fs_ptr;	
FILE *fp;
int m3_fd;
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
 *				m3c_init					     *
 *===========================================================================*/
void m3c_init(void)
{
  	int rcode;

	m3c_lpid = getpid();
	CMDDEBUG("m3c_lpid=%d\n", m3c_lpid);

	do { 
		rcode = mnx_wait4bind_T(WAIT4BIND_MS);
		CMDDEBUG("M3COPY: mnx_wait4bind_T  rcode=%d\n", rcode);
		if (rcode == EMOLTIMEDOUT) {
			CMDDEBUG("M3COPY: mnx_wait4bind_T TIMEOUT\n");
			continue ;
		}else if( rcode < 0) 
			ERROR_EXIT(EXIT_FAILURE);
	} while	(rcode < OK);
	
	CMDDEBUG("Get the DVS info from SYSTASK\n");
    rcode = sys_getkinfo(&dvs);
	if(rcode) ERROR_EXIT(rcode);
	dvs_ptr = &dvs;
	CMDDEBUG(DVS_USR_FORMAT,DVS_USR_FIELDS(dvs_ptr));

	CMDDEBUG("Get the DC info from SYSTASK\n");
	rcode = sys_getmachine(&vmu);
	if(rcode) ERROR_EXIT(rcode);
	dc_ptr = &vmu;
	CMDDEBUG(DC_USR_FORMAT,DC_USR_FIELDS(dc_ptr));

	m3c_ep = mnx_getep(m3c_lpid);
	CMDDEBUG("Get M3COPY endpoint=%d info from SYSTASK\n", m3c_ep);
	rcode = sys_getproc(&m3copy, m3c_ep);
	if(rcode) ERROR_EXIT(rcode);
	m3c_ptr = &m3copy;
	CMDDEBUG(PROC_USR_FORMAT,PROC_USR_FIELDS(m3c_ptr));
	
	CMDDEBUG("Get FS endpoint=%d info from SYSTASK\n", FS_PROC_NR);
	rcode = sys_getproc(&fs, FS_PROC_NR);
	if(rcode) ERROR_EXIT(rcode);
	fs_ptr = &fs;
	CMDDEBUG(PROC_USR_FORMAT,PROC_USR_FIELDS(fs_ptr));
	if( TEST_BIT(fs_ptr->p_rts_flags, BIT_SLOT_FREE)) {
		fprintf(stderr, "FS not started\n");	
	}
	
	/*---------------- Allocate memory for message  ---------------*/
	posix_memalign( (void **) &m_ptr, getpagesize(), sizeof(message) );
	if (m_ptr== NULL) {
   		ERROR_EXIT(-errno);
	}
	CMDDEBUG("M3COPY m_ptr=%p\n",m_ptr);

	/*---------------- Allocate memory for filename  ---------------*/
	posix_memalign( (void **) &path_ptr, getpagesize(), MNX_PATH_MAX );
	if (path_ptr== NULL) {
   		ERROR_EXIT(-errno);
	}
	CMDDEBUG("M3COPY path_ptr=%p\n",path_ptr);

	/*---------------- Allocate memory for data  ---------------*/
	posix_memalign( (void **) &data_ptr, getpagesize(), MAXCOPYBUF );
	if (data_ptr== NULL) {
   		ERROR_EXIT(-errno);
	}
	CMDDEBUG("M3COPY data_ptr=%p\n",data_ptr);	

	if(rcode) ERROR_EXIT(rcode);
	
}

void usage(void) {
	fprintf(stderr, "Usage: m3copy {-p|-g} <rmt_fname> <lcl_fname> \n");
	ERROR_EXIT(EMOLINVAL);
}

/*===========================================================================*
 *				main					     *
 * usage:    m3copy {-p|-g} <rmt_fname> <lcl_fname> * 
 * p: PUT
 * g: GET
  *===========================================================================*/
int main ( int argc, char *argv[] )
{
	int rcode, rlen , oper, opt, bytes;
	double t_start, t_stop, t_total, child_ep;

	char *dir_ptr;
	
	CMDDEBUG("M3COPY argc=%d\n", argc);

	if( argc != 4) usage();
		
	m3c_init();
	dir_ptr = dirname(argv[0]);
	CMDDEBUG("M3COPY old cwd=%s\n", dir_ptr);
	rcode =  chdir(dir_ptr);
	if(rcode)ERROR_EXIT(-errno);
	dir_ptr = getcwd(path_ptr, MNX_PATH_MAX);
	CMDDEBUG("M3COPY new cwd=%s\n", dir_ptr);

	oper = M3C_NONE;
    while ((opt = getopt(argc, argv, "pg")) != -1) {
        switch (opt) {
			case 'p':
				oper = M3C_PUT;
				break;
			case 'g':
				oper = M3C_GET;
				break;
			default: /* '?' */
				usage();
				break;
        }
    }
	CMDDEBUG("oper=%d\n", oper);

	t_start = dwalltime();
	switch(oper){
		case M3C_PUT:
			printf("M3COPY M3C_PUT %s->%s\n", argv[3], argv[2]);
			fp = fopen(argv[4], "r");
			if(fp == NULL) ERROR_EXIT(-errno);
			m3_fd = mol_open(argv[3], O_WRONLY);
			if(m3_fd < 0)  ERROR_EXIT(-errno);
			do {
				// read from LINUX 
				rlen = fread(data_ptr, 1, MAXCOPYBUF, fp);
				if(rlen < 0)  ERROR_EXIT(-errno);
				if(rlen == 0) break;
				
				// write to MOL 
				bytes = mol_write(m3_fd, data_ptr, rlen);
				if( bytes < 0)  ERROR_EXIT(-errno);
			}while(TRUE);
			break;
		case M3C_GET:
			printf("M3COPY M3C_GET %s->%s\n", argv[2], argv[3]);
			fp = fopen(argv[3], "w");
			if(fp == NULL) ERROR_EXIT(-errno);
			m3_fd = mol_open(argv[2], O_RDONLY);
			if(m3_fd < 0)  ERROR_EXIT(-errno);
			do {
				// read from MOL 
				rlen = mol_read(m3_fd, data_ptr, MAXCOPYBUF);
				if(rlen < 0)  ERROR_EXIT(-errno);
				if(rlen == 0) break;
				
				// Write to LINUX
				bytes = fwrite(data_ptr, 1, rlen, fp);
				if( bytes < 0)  ERROR_EXIT(-errno);					
			}while(TRUE);
			break;
		default:
			ERROR_EXIT(EMOLINVAL);
	}
	CMDDEBUG("M3COPY: CLOSE\n");
	fclose(fp);
	rcode = mol_close(m3_fd);
	if ( rcode < 0) ERROR_PRINT(rcode);
	t_stop  = dwalltime();
	t_total = (t_stop-t_start);
 	printf("M3COPY %s->%s t_start=%.2f t_stop=%.2f t_total=%.2f\n", 
		argv[2], argv[3], t_start, t_stop, t_total);
	
	return(OK);
}
	
