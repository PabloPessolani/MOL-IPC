/***************************************************************************
					test_fstty.c

This program test basic operation of TTY task through FS
	- OPEN
	- IO_CTL SET ATTR
	- IO_CTL GET ATTR
	- WRITE
	- READ
***************************************************************************/

#include "tty.h"
#include <termios.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include "../../kernel/minix/unistd.h"

#define WAIT4BIND_MS 1000

int local_nodeid;
drvs_usr_t drvs, *drvs_ptr;
int fstty_lpid;		
pthread_t main_thread;		
DC_usr_t  vmu, *dc_ptr;
proc_usr_t ttyt_proc, *ttyt_ptr;
proc_usr_t tty_proc, *tty_ptr;
message *m_ptr;

void test_init(void);
void slave_thread(void *arg);
int tty_fd;

/*===========================================================================*
 *				main					     *
 *===========================================================================*/
int  main( int argc, char *argv[] )
{
	int rcode, lpid, i;
		
	static tty_pseudo_t tty_pse __attribute__((aligned(0x1000)));
	tty_pseudo_t *pse_ptr;
	
	static tty_conf_t tty_cfg __attribute__((aligned(0x1000)));
	tty_conf_t *cfg_ptr;
		
	static char line_msg[]="Hello World! I am Minix Over Linux (MoL), nice to meet you\n";
	static char usr_entry[TTY_IN_BYTES];
	pthread_t		write_thread;

		
	test_init();

	// ************************ OPEN TTY **************************
	
	TASKDEBUG("TEST OPEN\n");
	tty_fd = mol_open("/dev/vtty0", O_RDWR);

	TASKDEBUG("TEST OPEN tty_fd=%d\n", tty_fd);
	if(tty_fd < 0) 
		TASKDEBUG("TEST OPEN errno=%d\n", errno);

	// ************************ DEV_IOCTL  TIOCGETPSE TTY **************************

	sleep(1);
	TASKDEBUG("TEST IOCTL TIOCGETPSE \n");
	pse_ptr = &tty_pse;
	rcode = mol_ioctl(tty_fd, TIOCGETPSE, pse_ptr);

	TASKDEBUG("TEST IOCTL TIOCGETPSE rcode=%d\n", rcode);
	if(tty_fd < 0) 
		TASKDEBUG("TEST IOCTL TIOCGETPSE errno=%d\n", errno);
		
	// ************************ DEV_IOCTL  TIOCGETCFG TTY **************************
	
	sleep(1);
	TASKDEBUG("TEST DEV_IOCTL TIOCGETCFG \n");
	cfg_ptr = &tty_cfg;

	rcode = mol_ioctl(tty_fd, TIOCGETCFG, cfg_ptr);
	TASKDEBUG("TEST IOCTL TIOCGETCFG rcode=%d\n", rcode);
	if(tty_fd < 0) 
		TASKDEBUG("TEST IOCTL TIOCGETCFG errno=%d\n", errno);
	
	// ************************ TTY WRITE **************************

	sleep(1);
	TASKDEBUG("TEST TTY WRITE \n");

	/*Create the VTTY SLAVE Thread */
	TASKDEBUG("Starting thread for minor:%d\n", cfg_ptr->minor);
	rcode = pthread_create( &write_thread, NULL, slave_thread, pse_ptr);
	if( rcode)ERROR_EXIT(rcode);
	
	rcode = mol_write(tty_fd, line_msg, strlen(line_msg));
	TASKDEBUG("TEST WRITE rcode=%d\n", rcode);
	if(tty_fd < 0) 
		TASKDEBUG("TEST WRITE errno=%d\n", errno);
	
	// ************************ TTY READ  **************************

	TASKDEBUG("TEST TTY READ \n");

	rcode = mol_read(tty_fd, usr_entry, TTY_IN_BYTES);
	TASKDEBUG("TEST READ rcode=%d\n", rcode);
	if(tty_fd < 0) 
		TASKDEBUG("TEST READ errno=%d\n", errno);

	TASKDEBUG("TEST TTY READ  usr_entry=%s\n", usr_entry);
	sleep(10);
 }
 
 
 
/*===========================================================================*
 *				slave_thread					     *
 *===========================================================================*/
void slave_thread(void *arg)
{
	int fds, rcode, bytes;
	tty_pseudo_t *pse_ptr;
	char buf_in[TTY_IN_BYTES];
	static char buf_out[] = "I'am a test_fstty thread\n";

	static struct termios slave_orig_term_settings; // Saved terminal settings
	static struct termios new_term_settings; // Current terminal settings
	
	pse_ptr = (	tty_pseudo_t *) arg;
	
	TASKDEBUG("%s\n", pse_ptr->sname);
	
	fds = open(pse_ptr->sname, O_RDWR);	
	
	// Save the defaults parameters of the slave side of the PTY
	rcode = tcgetattr(fds, &slave_orig_term_settings);
	
	// Set RAW mode on slave side of PTY
	new_term_settings = slave_orig_term_settings;
	cfmakeraw (&new_term_settings);
	tcsetattr (fds, TCSANOW, &new_term_settings);
	// Make the current process a new session leader
	setsid();
	// As the child is a session leader, set the controlling terminal to be the slave side of the PTY
	// (Mandatory for programs like the shell to make them manage correctly their outputs)
	ioctl(fds, TIOCSCTTY, 1);
	while(TRUE) {
		bytes = read(fds, buf_in, TTY_IN_BYTES);
		if( bytes < 0) {
			ERROR_PRINT(errno);
		}
		TASKDEBUG("bytes=%d buf_in=[%s]\n", bytes, buf_in);
		
		bytes = write(fds, buf_out, strlen(buf_out));
		if( bytes < 0) {
			ERROR_PRINT(errno);
		}
		TASKDEBUG("bytes=%d buf_out=[%s]\n", bytes, buf_out);
	}
}

/*===========================================================================*
 *				test_init					     *
 *===========================================================================*/
void test_init(void)
{
	int rcode;

	fstty_lpid = getpid();
	TASKDEBUG("fstty_lpid=%d \n", fstty_lpid);

	do { 
		rcode = mnx_wait4bind_T(WAIT4BIND_MS);
		TASKDEBUG("TTY_FSTEST: mnx_wait4bind_T  rcode=%d\n", rcode);
		if (rcode == EMOLTIMEDOUT) {
			TASKDEBUG("TTY_FSTEST: mnx_wait4bind_T TIMEOUT\n");
			continue ;
		}else if( rcode < 0) 
			ERROR_EXIT(EXIT_FAILURE);
	} while	(rcode < OK);
	
	TASKDEBUG("Get the DVS info from SYSTASK\n");
    rcode = sys_getkinfo(&drvs);
	if(rcode) ERROR_EXIT(rcode);
	drvs_ptr = &drvs;
	TASKDEBUG(DVS_USR_FORMAT,DVS_USR_FIELDS(drvs_ptr));

	TASKDEBUG("Get the VM info from SYSTASK\n");
	rcode = sys_getmachine(&vmu);
	if(rcode) ERROR_EXIT(rcode);
	dc_ptr = &vmu;
	TASKDEBUG(DC_USR_FORMAT,DC_USR_FIELDS(dc_ptr));

	TASKDEBUG("Get TTY info from SYSTASK\n");
	rcode = sys_getproc(&tty_proc, TTY_PROC_NR);
	if(rcode) ERROR_EXIT(rcode);
	tty_ptr = &tty_proc;
	TASKDEBUG(PROC_USR_FORMAT,PROC_USR_FIELDS(tty_ptr));

	TASKDEBUG("Get TTY_FSTEST info from SYSTASK\n");
	rcode = sys_getproc(&ttyt_proc, SELF);
	if(rcode) ERROR_EXIT(rcode);
	ttyt_ptr = &ttyt_proc;
	TASKDEBUG(PROC_USR_FORMAT,PROC_USR_FIELDS(ttyt_ptr));
	
}



	
