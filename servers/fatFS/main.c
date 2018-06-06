/* This file contains the main program of the File System.  It consists of
 * a loop that gets messages requesting work, carries out the work, and sends
 * replies.
 *
 * The entry points into this file are:
 *   main:	main program of the File System
 *   reply:	send a reply to a process after the requested work is done
 *
 */

// #define SVRDBG    1

#define _TABLE
#include "fs.h"

#define WAIT4BIND_MS 1000

#define nil ((void*)0)

void fs_init(const char *cfg_file);
void get_work(void);



void print_usage(char* errmsg, ...) {
  if(errmsg) {
	SVRDEBUG("ERROR: %s\n", errmsg);  
    }
	SVRDEBUG( "Usage: fatFS cfgFilename \n");
}


/*===========================================================================*
 *				main					     *
 *===========================================================================*/
 int main ( int argc, char *argv[] )
 {
/* This is the main program of the file system.  The main loop consists of
 * three major activities: getting new work, processing the work, and sending
 * the reply.  This loop never terminates as long as the file system runs.
 */
	int rcode;
	
	if (argc != 2) {
		SVRDEBUG( "Usage: %s cfgFilename \n", argv[0]);
		ERROR_EXIT(EMOLINVAL);
	}
  
	fs_init(argv[1]);

	SVRDEBUG("FS(%d) main loop that gets work\n", dcu.dc_dcid);

	while (TRUE) {
		get_work();			/* sets who and call_nr */

		fp = &fproc[who_p];	/* pointer to proc table struct */
	// SVRDEBUG(FS_PROC_FORMAT, FS_PROC_FIELDS(fp));
	// SVRDEBUG("Proceso who_p %d\n",who_p);
		super_user = (fp->fp_effuid == SU_UID ? TRUE : FALSE);   /* su? */
	// SVRDEBUG("super_user (0=super_user) %d\n", super_user);
		/* Check for special control messages first. */
		if (call_nr == PROC_EVENT) 
		{
			/* Assume FS got signal. Synchronize, but don't exit. */
	// SVRDEBUG("call_nr %d\n",call_nr);
	// SVRDEBUG("PROC_EVENT %d\n",PROC_EVENT);
			// do_sync();
		} 
		else if (call_nr == SYN_ALARM) 
		{
				/* Alarm timer expired. Used only for select(). Check it. */
	//        	fs_expire_timers(m_in.NOTIFY_TIMESTAMP);
		} 
		else if ((call_nr & NOTIFY_MESSAGE))
		{
				/* Device notifies us of an event. */
	//        	dev_status(&m_in);
		} 
		else 
		{
			/* Call the internal function that does the work. */
			if (call_nr < 0 || call_nr >= NCALLS) 
			{ 
				rcode = EMOLNOSYS;
				//fprintf(stderr,"ERROR: warning illegal %d system call by %d\n", call_nr, who_e);
				ERROR_PRINT(stderr);
	//		} else if (fp->fp_pid == PID_FREE) {
	//			rcode = ENOSYS;
	//			printf("FS, bad process, who = %d, call_nr = %d, endpt1 = %d\n",
	//				 who_e, call_nr, m_in.endpt1);
			} 
			else 
			{
				rcode = (*call_vec[call_nr])();
			}
			// SVRDEBUG("rcode (RESPUESTA syscall) %d\n", rcode);
			/* Copy the results back to the user and send reply. */
			if (rcode != SUSPEND) { reply(who_e, rcode); }
			// if (rdahed_inode != NIL_INODE) {
			// 	read_ahead(); /* do block read ahead */
			// }
		}
	}
  return(OK);				/* shouldn't come here */
}

/*===========================================================================*
 *				get_work				     *
 *===========================================================================*/
 void get_work()
 {  
  /* Normally wait for new input.  However, if 'reviving' is
   * nonzero, a suspended process must be awakened.
   */
   int ret;

   	while( TRUE) 
   	{
// SVRDEBUG(" Normal case.  No one to revive. \n");
SVRDEBUG(" LISTENING . \n");
   		if ( (ret=mnx_receive(ANY, &m_in)) != OK) ERROR_EXIT(ret);
// SVRDEBUG(" Peticion: %d\n", ret);
   		m_ptr = &m_in;
SVRDEBUG( MSG1_FORMAT, MSG1_FIELDS(m_ptr));
SVRDEBUG( MSG3_FORMAT, MSG3_FIELDS(m_ptr));

   		who_e = m_in.m_source;
   		who_p = _ENDPOINT_P(who_e);
   		if(who_p < -(dc_ptr->dc_nr_tasks) || who_p >= (dc_ptr->dc_nr_procs))
   			ERROR_EXIT(EMOLBADPROC);

   		call_nr = m_in.m_type;
// SVRDEBUG("m_in.m_type = %d \n", m_in.m_type);
SVRDEBUG("call_nr = %d \n", call_nr);
   		return;
   	}
   }

/*===========================================================================*
 *				fs_init					     *
 *===========================================================================*/
 void fs_init(const char *cfg_file)
 {
    config_t *cfg;
 	int rcode, i;
	// int root_major;
	// dconf_t *root_ptr;
	// char *img_name;		/* name of the ram disk image file*/
	char dev_major[3];		/* number of the dev type for FATFS diskio initialization*/	 

 	fs_lpid = getpid();
	
	do { 
		rcode = mnx_wait4bind_T(WAIT4BIND_MS);
		SVRDEBUG("FS: mnx_wait4bind_T  rcode=%d\n", rcode);
		if (rcode == EMOLTIMEDOUT) {
			SVRDEBUG("FS: mnx_wait4bind_T TIMEOUT\n");
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

	SVRDEBUG("Get FS_PROC_NR info from SYSTASK\n");
	rcode = sys_getproc(&proc_fs, FS_PROC_NR);
	if(rcode) ERROR_EXIT(rcode);
	fs_ptr = &proc_fs;
	SVRDEBUG(PROC_USR_FORMAT,PROC_USR_FIELDS(fs_ptr));
	if( TEST_BIT(fs_ptr->p_rts_flags, BIT_SLOT_FREE)) {
		fprintf(stderr,"FS task not started\n");
		fflush(stderr);		
		ERROR_EXIT(EMOLNOTBIND);
	}
	
	/* alloc dynamic memory for the FS process table */
	SVRDEBUG("Alloc dynamic memory for the FS process table nr_procs=%d\n", dc_ptr->dc_nr_procs);
// 	fproc = malloc((dc_ptr->dc_nr_procs)*sizeof(fproc_t));
	posix_memalign( (void**) &fproc, getpagesize(), (dc_ptr->dc_nr_procs)*sizeof(fproc_t));
 	if(fproc == NULL) ERROR_EXIT(errno);

	/* alloc dynamic memory for a temporal copy of  PM process table */
	SVRDEBUG("Alloc dynamic memory for temporal copy of  PM table nr_procs=%d\n", dc_ptr->dc_nr_procs);
	posix_memalign( (void**) &mproc, getpagesize(), (dc_ptr->dc_nr_procs)*sizeof(mproc_t));
	if(mproc == NULL) ERROR_EXIT(rcode)
	
	/*dump PM proc table */
	rcode = mol_getpmproctab(mproc);
	if(rcode) ERROR_EXIT(rcode);
	
 	/* Initialize FS's process table */
 	for (i = 0; i < dc_ptr->dc_nr_procs; i++) {
		/* descriptors of processes running before 		*/
		/* FS are filled with values get from PM table 	*/
 		fproc_init(i); 
	}
	
	free(mproc);
	
	SVRDEBUG("Alloc dynamic memory for user_path\n");
	posix_memalign( (void**) &user_path, getpagesize(), MNX_PATH_MAX);
 	if(user_path == NULL) ERROR_EXIT(errno);

	SVRDEBUG("Alloc dynamic memory for localbuff\n");
	posix_memalign( (void**) &localbuff, getpagesize(), MAXCOPYLEN);
 	if(localbuff == NULL) ERROR_EXIT(errno);	 

	/* All process table entries have been set. Continue with FS initialization.
	   * Certain relations must hold for the file system to work at all. Some 
	   * extra block_size requirements are checked at super-block-read-in time.
	   */
	// if (OPEN_MAX > 127) panic(__FILE__,"OPEN_MAX > 127", NO_NUM);
	// if (NR_BUFS < 6) panic(__FILE__,"NR_BUFS < 6", NO_NUM);
	// if (V1_INODE_SIZE != 32) panic(__FILE__,"V1 inode size != 32", NO_NUM);
	// if (V2_INODE_SIZE != 64) panic(__FILE__,"V2 inode size != 64", NO_NUM);
	// if (OPEN_MAX > 8 * sizeof(long))
	// 	panic(__FILE__,"Too few bits in fp_cloexec", NO_NUM);

	/* The following initializations are needed to let dev_opcl succeed .*/
	fp = (struct fproc *) NULL;
	who_e = who_p = NONE;

	init_dmap();     /* initialize device table and map boot driver */

	/*Reading the MOLFS config file*/
	cfg_dev_nr = 1;		/* start at 1: I don't know why !!! */
	cfg = nil;
	// SVRDEBUG("Antes de config_read \n"); 
	// SVRDEBUG("cfg_file %s \n", cfg_file);
	// SVRDEBUG("CFG_ESCAPED %d \n", CFG_ESCAPED); 
	cfg = config_read(cfg_file, CFG_ESCAPED, cfg);
	rcode = search_dev_config(cfg);
	if (rcode || cfg_dev_nr==0 ) {
		SVRDEBUG("Configuration error: cfg_dev_nr=%d\n", cfg_dev_nr);        
		ERROR_EXIT(rcode);
	}else{
		SVRDEBUG("cfg_dev_nr=%d\n", cfg_dev_nr);        		
	}
 	/* Verifying only one root device*/
	if (count_root_devs() != 1) {
		SVRDEBUG("\n Configuration error. One root dev is required!\n");        
		ERROR_EXIT(EMOLNXIO);
	}

	/*Get de root device from conf read*/
	root_major = get_root_major();
	
	root_ptr = &MAJOR2TAB(root_major).dmap_cfg;
	SVRDEBUG("DATOS FS SERVER");
	SVRDEBUG(DCONF_FORMAT, DCONF_FIELDS(root_ptr)); 	 	
	SVRDEBUG("root_major %d \n", root_major);
	root_dev = MM2DEV(root_major, root_ptr->minor);
	SVRDEBUG("DEV %d \n", root_dev);

	img_name = root_ptr->filename;
	SVRDEBUG("Configuration FILEIMAGE %s\n", img_name);  

	sprintf(dev_major, "%d", root_ptr->major);
	strcat(dev_major, ":");
	SVRDEBUG("Configuration MAJOR for MOUNT %s\n", dev_major); 
	
	/*MOUNT (INIT) the fatFS filesystem*/
	f_mount(&fsWork, dev_major, 0);		/* Give a work area to the default drive */   
	f_chdrive(dev_major);

}

/*===========================================================================*
 *				reply					     *
 * int whom;			 process to reply to 			     *
 * int result;			result of the call (usually OK or error #)   *
 *===========================================================================*/
 void reply(int whom, int result)
 {
 	int rcode;

SVRDEBUG(" Send a reply to a user process.\n");

 	m_out.reply_type = result;
 	m_ptr = &m_out;
SVRDEBUG( MSG1_FORMAT, MSG1_FIELDS(m_ptr));

 	rcode = mnx_send(whom, &m_out);
 	if (rcode) ERROR_PRINT(rcode);

 }

