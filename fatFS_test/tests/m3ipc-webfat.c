
#include "lwip/opt.h"
#include "lwip/arch.h"
#include "lwip/api.h"

 
#if LWIP_NETCONN

#define SVRDBG		1
#include "../../../../m3ipc.h"
#include "ff.h"

#ifndef HTTPD_DEBUG
#define HTTPD_DEBUG         LWIP_DBG_OFF
#endif

#define BUFSIZE 4096
#define VERSION 23


struct {
  char *ext;
  char *filetype;
} webfat_ext [] = {
  {"gif", "image/gif" },
  {"jpg", "image/jpg" },
  {"jpeg","image/jpeg"},
  {"png", "image/png" },
  {"ico", "image/ico" },
  {"zip", "image/zip" },
  {"gz",  "image/gz"  },
  {"tar", "image/tar" },
  {"htm", "text/html" },
  {"html","text/html" },
  {0,0} };
  
const static char http_forbidden[] = "HTTP/1.1 403 Forbidden\nContent-Length: 185\nConnection: close\nContent-Type: text/html\n\n<html><head>\n<title>403 Forbidden</title>\n</head><body>\n<h1>Forbidden</h1>\nThe requested URL, file type or operation is not allowed on this simple static file webserver.\n</body></html>\n"; 
const static char http_not_found[] = "HTTP/1.1 404 Not Found\nContent-Length: 136\nConnection: close\nContent-Type: text/html\n\n<html><head>\n<title>404 Not Found</title>\n</head><body>\n<h1>Not Found</h1>\nThe requested URL was not found on this server.\n</body></html>\n";
int httpd_lpid;
int httpd_ep;
FILINFO fat_fstat;
char *out_buf;
dvs_usr_t dvs, *dvs_ptr;
VM_usr_t  vmu, *dc_ptr;
proc_usr_t http, *http_ptr;
proc_usr_t rdisk, *rdisk_ptr;
FATFS FatFs;		/* FatFs work area needed for each volume */
message *m_ptr;

/** Serve one HTTP connection accepted in the http thread */
static void m3ipc_webfat_serve(struct netconn *conn)
{
	struct netbuf *inbuf;
	char *in_buf;
	u16_t in_len;
	err_t err;
	FIL fat_file;			/* File object needed for each open file */
	int j;
	long i, ret, len;
	char *fstr;
	FF_UINT rbytes;  
  
	/* Read the data from the port, blocking if nothing yet there. 
	We assume the request (the part we care about) is in one netbuf */
	err = netconn_recv(conn, &inbuf); 
	if(err) {
		ERROR_PRINT(err);
		return;
	}
	
	while(err == ERR_OK) {
		netbuf_data(inbuf, (void**)&in_buf, &in_len);
		
		/* remove CF and LF characters */
		for(i=0;i<in_len;i++)  
		if(in_buf[i] == '\r' || in_buf[i] == '\n')
		  in_buf[i]='*';
		  
		SVRDEBUG("request:%s\n",in_buf);
	  
		if( strncmp(in_buf,"GET ",4) 
		 && strncmp(in_buf,"get ",4) ) {
			ERROR_PRINT(EMOLNOSYS);
			netconn_write(conn, http_forbidden, strlen(http_forbidden), NETCONN_NOCOPY);
			break;
		}  
		
		 /* null terminate after the second space to ignore extra stuff */
		for(i=4;i<in_len;i++) {
			if(in_buf[i] == ' ') { /* string is "GET URL " +lots of other stuff */
				in_buf[i] = 0;
				break;
			}
		}
		
		SVRDEBUG("cooked request:%s\n",in_buf);

		/* check for illegal parent directory use .. */
		for(j=0;j<i-1;j++) {  
			if(in_buf[j] == '.' && in_buf[j+1] == '.') {
			ERROR_PRINT(EMOLACCES);
			netconn_write(conn, http_forbidden, strlen(http_forbidden), NETCONN_NOCOPY);
			break;
			}
		}
		
		/* convert no filename to index file */
		if( !strncmp(&in_buf[0],"GET /\0",6) 
		 || !strncmp(&in_buf[0],"get /\0",6) ) 
			(void)strcpy(in_buf,"GET /index.htm");
		
		
		/* work out the file type and check we support it */
		in_len=strlen(in_buf);
		fstr = (char *)0;
		for(i=0;webfat_ext[i].ext != 0;i++) {
			len = strlen(webfat_ext[i].ext);
			if( !strncmp(&in_buf[in_len-len], webfat_ext[i].ext, len)) {
			fstr =webfat_ext[i].filetype;
			break;
			}
		}
		if(fstr == 0) {
			ERROR_PRINT(EMOLACCES);
			netconn_write(conn, http_forbidden, strlen(http_forbidden), NETCONN_NOCOPY);
			break;
		}
		
		/* Conver slash to backslash and Uppercase */
		for(i = 4; i < in_len; i++){
			in_buf[i] = toupper(in_buf[i]);
			if( in_buf[i] == '\/')
				in_buf[i] = '\\';
		}

		SVRDEBUG("final FAT filename:%s\n",&in_buf[4]);
		/* open the file for reading */
		if(( ret = f_open(&fat_file, &in_buf[5],FA_READ)) != FR_OK) {  
			ERROR_PRINT(ret);
			netconn_write(conn, http_not_found, strlen(http_not_found), NETCONN_NOCOPY);
			break;
		}
		
		/* get file size */
		err = f_stat(&in_buf[5], &fat_fstat);
		if( err != FR_OK)  {
			ERROR_PRINT(err);
			netconn_write(conn, http_not_found, strlen(http_not_found), NETCONN_NOCOPY);		
			break;
		}
		SVRDEBUG("f_stat:%s fsize=%d f_size=%d fname=%s\n",
			&in_buf[5], fat_fstat.fsize, f_size(&fat_file), fat_fstat.fname);
		
		/* Send the HTML header */
		(void)sprintf(out_buf,"HTTP/1.1 200 OK\nServer: nweb/%d.0\nContent-Length: %ld\nConnection: close\nContent-Type: %s\n\n", 
			VERSION, fat_fstat.fsize, fstr); /* Header + a blank line */
		netconn_write(conn, out_buf, strlen(out_buf), NETCONN_NOCOPY);

		/* send file in BUFSIZE block - last block may be smaller */
		while (  (ret = f_read(&fat_file, out_buf, BUFSIZE, &rbytes)) == FR_OK ) {
			if(rbytes == 0){
				SVRDEBUG("EOF\n");
				break;
			} 
			SVRDEBUG("out_buf:%s\n",out_buf);
			netconn_write(conn, out_buf, rbytes, NETCONN_NOCOPY);
		}
	}
	ERROR_PRINT(ret);

	ret = f_close(&fat_file);
	if(ret != FR_OK) ERROR_PRINT(ret);
	
	/* Close the connection (server closes in HTTP) */
	netconn_close(conn);
	
	/* Delete the buffer (netconn_recv gives us ownership,
	 so we have to make sure to deallocate the buffer) */
	netbuf_delete(inbuf);
	return(ret);
}

/** The main function, never returns! */
static void m3ipc_webfat_thread(void *arg)
{
	struct netconn *conn, *newconn;
	err_t err;
	LWIP_UNUSED_ARG(arg);

	SVRDEBUG( "\n");

	/* Create a new TCP connection handle */
	conn = netconn_new(NETCONN_TCP);
	LWIP_ERROR("http_server: invalid conn", (conn != NULL), return;);

	/* Bind to port 80 (HTTP) with default IP address */
	netconn_bind(conn, NULL, 80);

	/* Put the connection into LISTEN state */
	netconn_listen(conn);

	do {
	  err = netconn_accept(conn, &newconn);
	  if (err == ERR_OK) {
		m3ipc_webfat_serve(newconn);
		netconn_delete(newconn);
	  }
	} while(err == ERR_OK);
	ERROR_PRINT(err);
	netconn_close(conn);
	netconn_delete(conn);
	free(out_buf);
	
	// UMOUNT
	f_mount(NULL, "", 0);		

}

/*===========================================================================*
 *				m3ipc_wf_init					     *
 *===========================================================================*/
int m3ipc_wf_init(void)
{
  	int rcode;

	httpd_lpid = getpid();
	SVRDEBUG( "httpd_lpid=%d\n", httpd_lpid);

	do { 
		rcode = mnx_wait4bind_T(WAIT4BIND_MS);
		SVRDEBUG("HTTPD: mnx_wait4bind_T  rcode=%d\n", rcode);
		if (rcode == EMOLTIMEDOUT) {
			SVRDEBUG("HTTPD: mnx_wait4bind_T TIMEOUT\n");
			continue ;
		}else if( rcode < 0) 
			return(EXIT_FAILURE);
	} while	(rcode < OK);
	
	httpd_ep = rcode;
	
	SVRDEBUG("Get the DRVS info from SYSTASK\n");
    rcode = sys_getkinfo(&dvs);
	if(rcode) return(rcode);
	dvs_ptr = &dvs;
	SVRDEBUG(DRVS_USR_FORMAT,DRVS_USR_FIELDS(dvs_ptr));

	SVRDEBUG("Get the VM info from SYSTASK\n");
	rcode = sys_getmachine(&vmu);
	if(rcode) return(rcode);
	dc_ptr = &vmu;
	SVRDEBUG(VM_USR_FORMAT,VM_USR_FIELDS(dc_ptr));

	SVRDEBUG("Get HTTPD info from SYSTASK\n");
	rcode = sys_getproc(&http, httpd_ep);
	if(rcode) return(rcode);
	http_ptr = &http;
	SVRDEBUG(PROC_USR_FORMAT,PROC_USR_FIELDS(http_ptr));
	if( TEST_BIT(http_ptr->p_rts_flags, BIT_SLOT_FREE)) {
		fprintf(stderr,"HTTPD server not started\n");
		fflush(stderr);		
		return(EMOLNOTBIND);
	}

	SVRDEBUG("Get RDISK info from SYSTASK\n");
	rcode = sys_getproc(&rdisk, RDISK_PROC_NR);
	if(rcode) return(rcode);
	rdisk_ptr = &rdisk;
	SVRDEBUG(PROC_USR_FORMAT,PROC_USR_FIELDS(rdisk_ptr));
	if( TEST_BIT(rdisk_ptr->p_rts_flags, BIT_SLOT_FREE)) {
		fprintf(stderr,"RDISK task not started\n");
		fflush(stderr);		
		return(EMOLNOTBIND);
	}

  	/*---------------- Allocate memory for message   ---------------*/
	posix_memalign( (void **) &m_ptr, getpagesize(), sizeof(message));
	if (m_ptr== NULL) {
   		SVRDEBUG("posix_memalign m_ptr\n");
   		return(errno);
	}
	
  	/*---------------- Allocate memory for output buffer  ---------------*/
	posix_memalign( (void **) &out_buf, getpagesize(), BUFSIZE );
	if (out_buf== NULL) {
   		SVRDEBUG("posix_memalign out_buf\n");
		free(m_ptr);
   		return(errno);
	}
	
	// mount FAT FS
	f_mount(&FatFs, "", 0);		/* Give a work area to the default drive */

	return(OK);
}



/** Initialize the HTTP server (start its thread) */
void m3ipc_webfat_init(void)
{
	SVRDEBUG("m3ipc_webfat_init\n");

	if( m3ipc_wf_init()) return;

	SVRDEBUG("sys_thread_new m3ipc_webfat\n");
	
	sys_thread_new("m3ipc_webfat", m3ipc_webfat_thread, NULL, 
				DEFAULT_THREAD_STACKSIZE, DEFAULT_THREAD_PRIO);
}

#endif /* LWIP_NETCONN*/
