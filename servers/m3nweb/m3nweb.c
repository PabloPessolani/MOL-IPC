/* 
		HTTP WEB SERVER using MOL-FS
		but LINUX SOCKETS
 */

 
#define _TABLE
//#define SVRDBG		1
#include "m3nweb.h"

struct {
  char *ext;
  char *filetype;
} extensions [] = {
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
  {"txt","text/txt" },
  {0,0} };
  
const static char http_forbidden[] = "HTTP/1.1 403 Forbidden\nContent-Length: 185\nConnection: close\nContent-Type: text/html\n\n<html><head>\n<title>403 Forbidden</title>\n</head><body>\n<h1>Forbidden</h1>\nThe requested URL, file type or operation is not allowed on this simple static file webserver.\n</body></html>\n"; 
const static char http_not_found[] = "HTTP/1.1 404 Not Found\nContent-Length: 136\nConnection: close\nContent-Type: text/html\n\n<html><head>\n<title>404 Not Found</title>\n</head><body>\n<h1>Not Found</h1>\nThe requested URL was not found on this server.\n</body></html>\n";
  
void nweb_init(char *cfg_file);
int nweb_server(int socket_fd);
int search_web_config(config_t *cfg);
int read_config(char *file_conf);

void usage(char* errmsg, ...) {
	if(errmsg) {
		printf("ERROR: %s\n", errmsg);
	} 
	fprintf(stderr, "Usage: m3nweb <config_file>\n");
}

double dwalltime()
{
	double sec;
	struct timeval tv;

	gettimeofday(&tv,NULL);
	sec = tv.tv_sec + tv.tv_usec/1000000.0;
	return sec;
}


/*===========================================================================*
 *				nweb_init					     *
 *===========================================================================*/
 void nweb_init(char *cfg_file)
 {
 	int rcode;
    config_t *cfg;

 	nweb_lpid = getpid();
	SVRDEBUG("nweb_lpid=%d cfg_file=%s\n", nweb_lpid, cfg_file);
	
#define WAIT4BIND_MS	1000
	do { 
		rcode = mnx_wait4bind_T(WAIT4BIND_MS);
		SVRDEBUG("WEBSRV: mnx_wait4bind_T  rcode=%d\n", rcode);
		if (rcode == EMOLTIMEDOUT) {
			SVRDEBUG("WEBSRV: mnx_wait4bind_T TIMEOUT\n");
			continue ;
		}else if( rcode < 0) 
			ERROR_EXIT(EXIT_FAILURE);
	} while	(rcode < OK);
	
	SVRDEBUG("Get the DVS info from SYSTASK\n");
	rcode = sys_getkinfo(&dvs);
	if(rcode) ERROR_EXIT(rcode);
	dvs_ptr = &dvs;
	SVRDEBUG(DVS_USR_FORMAT,DVS_USR_FIELDS(dvs_ptr));
	
	SVRDEBUG("Get the DC info from SYSTASK\n");
	rcode = sys_getmachine(&dcu);
	if(rcode) ERROR_EXIT(rcode);
	dc_ptr = &dcu;
	SVRDEBUG(DC_USR_FORMAT,DC_USR_FIELDS(dc_ptr));

	SVRDEBUG("Get WEB_PROC_NR info from SYSTASK\n");
	rcode = sys_getproc(&proc_web, WEB_PROC_NR);
	if(rcode) ERROR_EXIT(rcode);
	nweb_ptr = &proc_web;
	SVRDEBUG(PROC_USR_FORMAT,PROC_USR_FIELDS(nweb_ptr));
	if( TEST_BIT(nweb_ptr->p_rts_flags, BIT_SLOT_FREE)) {
		fprintf(stderr,"WEBSRV task not started\n");
		fflush(stderr);		
		ERROR_EXIT(EMOLNOTBIND);
	}
	
  	/*---------------- Allocate memory for struct mnx_stat  ---------------*/
	posix_memalign( (void **) &fstat_ptr, getpagesize(), sizeof(struct mnx_stat) );
	if (fstat_ptr== NULL) {
   		SVRDEBUG("posix_memalign fstat_ptr\n");
   		ERROR_EXIT(-errno);
	}

  	/*---------------- Allocate memory for output buffer  ---------------*/
	posix_memalign( (void **) &out_buf, getpagesize(), WEBMAXBUF );
	if (out_buf== NULL) {
   		SVRDEBUG("posix_memalign out_buf\n");
		free(fstat_ptr);
   		ERROR_EXIT(-errno);
	}

  	/*---------------- Allocate memory for output buffer  ---------------*/
	posix_memalign( (void **) &in_buf, getpagesize(), WEBMAXBUF );
	if (in_buf== NULL) {
   		SVRDEBUG("posix_memalign in_buf\n");
		free(fstat_ptr);
		free(out_buf);
   		ERROR_EXIT(-errno);
	}
	
	cfg= nil;
	SVRDEBUG("cfg_file=%s\n", cfg_file);
	cfg = config_read(cfg_file, CFG_ESCAPED, cfg);
	
	SVRDEBUG("before search_web_config\n");	
	rcode = search_web_config(cfg);
	if(rcode) ERROR_EXIT(rcode);
		
	if((listenfd = socket(AF_INET, SOCK_STREAM,0)) <0)
		ERROR_EXIT(-errno);

	nweb_addr.sin_family = AF_INET;
	nweb_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	nweb_addr.sin_port = htons(nweb_port);
	rcode = bind(listenfd, (struct sockaddr *)&nweb_addr,sizeof(nweb_addr));
	if (rcode <0)
		ERROR_EXIT(-errno);
	rcode = listen(listenfd,64);
	if ( rcode <0)
		ERROR_EXIT(-errno);
}

/*===========================================================================*
 *				nweb_server					     *
 *===========================================================================*/
/* this is a child web server process, so we can exit on errors */
int nweb_server(int socket_fd)
{
	u16_t in_len;
	int rcode;
	int j, file_fd;
	long i, ret, len, rbytes;
	char *fstr;
  
	/* Read the data from the port, blocking if nothing yet there. 
	We assume the request (the part we care about) is in one netbuf */
	SVRDEBUG("request socket_fd=%d\n", socket_fd);
	
	rcode = OK;
	while(rcode == OK) {
		ret =read(socket_fd, in_buf, WEBMAXBUF);
		
		in_len=strlen(in_buf);
		/* remove CF and LF characters */
		for(i=0;i<in_len;i++)  
			if(in_buf[i] == '\r' || in_buf[i] == '\n')
				in_buf[i]='*';
		  
		SVRDEBUG("request:%s\n",in_buf);
	  
		if( strncmp(in_buf,"GET ",4) 
		 && strncmp(in_buf,"get ",4) ) {
			ERROR_PRINT(EMOLNOSYS);
			write(socket_fd,http_forbidden, strlen(http_forbidden));
			break;
		}  
		
		 /* null terminate after the second space to ignore extra stuff */
		for(i=4;i<in_len;i++) {
			if(in_buf[i] == ' ') { /* string is "GET URL " +lots of other stuff */
				in_buf[i] = 0;
				break;
			}
		}
		
		/* check for illegal parent directory use .. */
		for(j=0;j<i-1;j++) {  
			if(in_buf[j] == '.' && in_buf[j+1] == '.') {
				ERROR_PRINT(EMOLACCES);
				write(socket_fd,http_forbidden, strlen(http_forbidden));
				break;
			}
		}
		
		/* convert no filename to index file */
		if( !strncmp(&in_buf[0],"GET /\0",6) 
		 || !strncmp(&in_buf[0],"get /\0",6) ) 
			(void)strcpy(in_buf,"GET /index.html");
		
		
		/* work out the file type and check we support it */
		in_len=strlen(in_buf);
		fstr = (char *)0;
		for(i=0;extensions[i].ext != 0;i++) {
			len = strlen(extensions[i].ext);
			if( !strncmp(&in_buf[in_len-len], extensions[i].ext, len)) {
			fstr =extensions[i].filetype;
			break;
			}
		}
		if(fstr == 0) {
			ERROR_PRINT(EMOLACCES);
			write(socket_fd,http_forbidden, strlen(http_forbidden));
			break;
		}
		
		/* open the file for reading */
		SVRDEBUG("filename:%s\n",&in_buf[5]);
		if(( file_fd = mol_open(&in_buf[5],O_RDONLY)) == -1) {  
			ERROR_PRINT(errno);
			write(socket_fd,http_not_found, strlen(http_not_found));
			break;
		}
		
		/* get file size */
		rcode = mol_fstat(file_fd, fstat_ptr);
		if( rcode < 0)  {
			ERROR_PRINT(errno);
			write(socket_fd,http_not_found, strlen(http_not_found));
			break;
		}
		
		/* Send the HTML header */
		(void)sprintf(out_buf,"HTTP/1.1 200 OK\nServer: nweb/%d.0\nContent-Length: %ld\nConnection: close\nContent-Type: %s\n\n", 
			VERSION, fstat_ptr->st_size, fstr); /* Header + a blank line */
		write(socket_fd,out_buf, strlen(out_buf));

		/* send file in WEBMAXBUF block - last block may be smaller */
		while (  (rbytes = mol_read(file_fd, out_buf, WEBMAXBUF)) > 0 ) {
			write(socket_fd,out_buf, rbytes);
		}
		break;
	}
//	sleep(1);	/* allow socket to drain before signalling the socket is closed */
//	ret = close(socket_fd);
//	if(ret < 0)	ERROR_PRINT(errno);
	sleep(1);	/* allow socket to drain before signalling the socket is closed */
	ret = close(socket_fd);
	if(ret < 0)	ERROR_PRINT(errno);

	ret = close(file_fd); 
	if(ret < 0)	ERROR_PRINT(errno);
	return(OK);
	
}

void be_a_daemon(void)
{
	int i;
	
	SVRDEBUG("\n");

	if(getppid()==1) return; /* already a daemon */
	i=fork();
	if (i<0) exit(1); /* fork error */
	if (i>0) exit(0); /* parent exits */
	
	SVRDEBUG("pid=%d\n", getpid());

	/* child (daemon) continues */
	setsid(); /* obtain a new process group */
//	for (i=getdtablesize();i>=0;--i) close(i); /* close all descriptors */
//	i=open("/dev/null",O_RDWR); dup(i); dup(i); /* handle standart I/O */
	umask(027); /* set newly created file permissions */
//	chdir(RUNNING_DIR); /* change running directory */
//	lfp=open(LOCK_FILE,O_RDWR|O_CREAT,0640);
//	if (lfp<0) exit(1); /* can not open */
//	if (lockf(lfp,F_TLOCK,0)<0) exit(0); /* can not lock */
	/* first instance continues */
//	sprintf(str,"%d\n",getpid());
//	write(lfp,str,strlen(str)); /* record pid to lockfile */
	signal(SIGCHLD,SIG_IGN); /* ignore child */
	signal(SIGTSTP,SIG_IGN); /* ignore tty signals */
	signal(SIGTTOU,SIG_IGN);
	signal(SIGTTIN,SIG_IGN);
	signal(SIGHUP,SIG_IGN); /* catch hangup signal */
	signal(SIGTERM,SIG_IGN); /* catch kill signal */
}	



/*===========================================================================*
 *				   main 				     *
 *===========================================================================*/
int main (int argc, char *argv[] )
{
	int length, socketfd, hit , pid, rcode, sts;
	double t_start, t_stop, t_total;
	long total_bytes;
	FILE *fp;
	
	if ( argc != 2) {
		usage( "No arguments", optarg );
		exit(1);
	}
	
	be_a_daemon();
	
	nweb_init(argv[1]);
	
	
	mnx_unbind(dc_ptr->dc_dcid,nweb_ptr->p_endpoint);

	for(hit=1; ;hit++) {
		length = sizeof(cli_addr);
		socketfd = accept(listenfd, (struct sockaddr *)&cli_addr, &length);
		if (socketfd < 0) ERROR_EXIT(-errno);
		if((pid = fork()) < 0) {
			ERROR_EXIT(-errno)
		}
		else {
			if(pid == 0) { 	/* child */
				(void)close(listenfd);
				rcode = mnx_bind(dc_ptr->dc_dcid,nweb_ptr->p_endpoint);
				if ( rcode < 0) ERROR_EXIT(rcode);
				SVRDEBUG("child=%d hit=%d p_endpoint=%d\n", getpid(),  hit, nweb_ptr->p_endpoint );
				t_start = dwalltime();
				nweb_server(socketfd); 
				t_stop = dwalltime();
				t_total = (t_stop-t_start);
				fp = fopen("m3nweb.txt","a+");
				fprintf(fp, "t_start=%.2f t_stop=%.2f t_total=%.2f\n",t_start, t_stop, t_total);
				fprintf(fp, "total_bytes = %ld\n", total_bytes);
				fprintf(fp, "Throuhput = %f [bytes/s]\n", (double)(total_bytes)/t_total);
				fclose(fp);
				rcode = mnx_unbind(dc_ptr->dc_dcid,nweb_ptr->p_endpoint);
				if ( rcode < 0) ERROR_EXIT(rcode);
				exit(1);
			} else { 	/* parent */
				(void)close(socketfd);
				SVRDEBUG("waiting child=%d\n",pid);
				waitpid(pid ,&sts, 0);
			}
		}
	}

	return(OK);				
}