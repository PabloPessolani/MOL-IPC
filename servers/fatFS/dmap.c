/* This file contains the table with device <-> driver mappings. It also
 * contains some routines to dynamically add and/ or remove device drivers
 * or change mappings.  
 */

// #define SVRDBG    1

#include "fs.h"

/* Some devices may or may not be there in the next table. */
#define DT(enable, opcl, io, driver, flags, major, minor, type, img_fl, root, buff, vltl, ipaddr, port, compression) \
  { (enable?(opcl):no_dev), (enable?(io):0), \
    (enable?(driver):0), (flags), \
    (enable?(major):0), (enable?(minor):0), \
    (enable?(type):0), (enable?(img_fl):""), \
    (enable?(root):0), (enable?(buff):0), (enable?(vltl):0),	\
    (enable?(ipaddr):""), (enable?(port):0), (enable?(compression):0)},

#define NC(x) (NR_CTRLRS >= (x))


/* The order of the entries here determines the mapping between major device
 * numbers and tasks.  The first entry (major device 0) is not used.  The
 * next entry is major device 1, etc.  Character and block devices can be
 * intermixed at random.  The ordering determines the device numbers in /dev/.
 * Note that FS knows the device number of /dev/ram/ to load the RAM disk.
 * Also note that the major device numbers used in /dev/ are NOT the same as 
 * the process numbers of the device drivers. 
 */
/*
  Driver enabled     Open/Cls  I/O     Driver #     Flags Device  File
  --------------     --------  ------  -----------  ----- ------  ----       
 */

/*===========================================================================*
 *        do_devctl            *
 *===========================================================================*/
int do_devctl()
{
  int result, proc_nr_e, proc_nr_n;

  switch(m_in.ctl_req) {
  case DEV_MAP:
      /* Check process number of new driver. */
      proc_nr_e= m_in.driver_nr;
      if (isokendpt(proc_nr_e, &proc_nr_n) != OK)
       ERROR_RETURN(EMOLINVAL);

      /* Try to update device mapping. */
      result = map_driver(m_in.dev_nr, proc_nr_e, m_in.dev_style);
      if (result == OK)
      {
  /* If a driver has completed its exec(), it can be announced to be
   * up.
   */
      if(fproc[proc_nr_n].fp_execced) 
        {
          dev_up(m_in.dev_nr);
        } 
      else 
        {
          dmap_tab[m_in.dev_nr].dmap_flags |= DMAP_BABY;
        }
      }
      break;
  case DEV_UNMAP:
      result = map_driver(m_in.dev_nr, NONE, 0);
      break;
  default:
      result = EMOLINVAL;
  }
  return(result);
}

/*===========================================================================*
 *        map_driver             *
 *===========================================================================*/
int map_driver(int major, int proc_nr_e, int style)
// int major;     /* major number of the device */
// int proc_nr_e;     /* process number of the driver */
// int style;     /* style of the device */
{
/* Set a new device driver mapping in the dmap table. Given that correct 
 * arguments are given, this only works if the entry is mutable and the 
 * current driver is not busy.  If the proc_nr is set to NONE, we're supposed
 * to unmap it.
 *
 * Normal error codes are returned so that this function can be used from
 * a system call that tries to dynamically install a new driver.
 */
  dmap_t *dp;
  int proc_nr_n;

  /* Get pointer to device entry in the dmap table. */
  if (major < 0 || major >= NR_DEVICES) ERROR_RETURN(EMOLNODEV);
  dp = &dmap_tab[major];    

  /* Check if we're supposed to unmap it. If so, do it even
   * if busy or unmutable, as unmap is called when driver has
   * exited.
   */
 if(proc_nr_e == NONE) {
  dp->dmap_opcl = no_dev;
  dp->dmap_io = no_dev_io;
  dp->dmap_driver = NONE;
  dp->dmap_flags = DMAP_MUTABLE;  /* When gone, not busy or reserved. */
  return(OK);
  }
  
  /* See if updating the entry is allowed. */
  if (! (dp->dmap_flags & DMAP_MUTABLE))  ERROR_RETURN(EMOLPERM);
  if (dp->dmap_flags & DMAP_BUSY)  ERROR_RETURN(EMOLBUSY);

  /* Check process number of new driver. */
  if (isokendpt(proc_nr_e, &proc_nr_n) != OK)
  ERROR_RETURN(EMOLINVAL);

  /* Try to update the entry. */
  switch (style) {
  case STYLE_DEV: dp->dmap_opcl = r_gen_opcl; break;
  //case STYLE_TTY: dp->dmap_opcl = tty_opcl; break;
  //case STYLE_CLONE: dp->dmap_opcl = clone_opcl; break;
  default:    ERROR_RETURN(EMOLINVAL);
  }
  dp->dmap_io = r_gen_io;
  dp->dmap_driver = proc_nr_e;

  return(OK); 
}


/*===========================================================================*
 *        dmap_unmap_by_endpt          *
 *===========================================================================*/
void dmap_unmap_by_endpt(int proc_nr_e)
{
  int i, r;
  for (i=0; i<NR_DEVICES; i++)
    if(dmap_tab[i].dmap_driver && dmap_tab[i].dmap_driver == proc_nr_e)
      if((r=map_driver(i, NONE, 0)) != OK)
		printf("FS: unmap of p %d / d %d failed: %d\n", proc_nr_e,i,r);

  return;

}

/*===========================================================================*
 *        init_dmap             *
 *===========================================================================*/
void init_dmap()
{
	int i;
	dmap_t *dp;

	SVRDEBUG("\n");

	/* Build table with device <-> driver mappings. */
	for (i=0; i<NR_DEVICES; i++) {
		dp = &dmap_tab[i];    
        dp->dmap_opcl        = no_dev;
        dp->dmap_io          = no_dev_io;
        dp->dmap_driver      = NONE;
        dp->dmap_flags       = DMAP_MUTABLE;
		/*Mol Device Configuration elements*/          
        dp->dmap_cfg.dev_name	  	= NULL;
        dp->dmap_cfg.major   		= (-1);
        dp->dmap_cfg.minor   		= (-1);                    
        dp->dmap_cfg.type    		= (-1);
        dp->dmap_cfg.filename	  	= NULL;
        dp->dmap_cfg.root_dev     	= (-1);
        dp->dmap_cfg.buffer_size 	= (-1);          
        dp->dmap_cfg.volatile_type	= (-1);  
        dp->dmap_cfg.server 		= NULL;
        dp->dmap_cfg.port   		= (-1);                  
        dp->dmap_cfg.compression  	= (-1);
    }

	for (i=0; i<NR_DEVICES; i++) {
		dmap_rev[i] = (-1);
	}
  // SVRDEBUG("Despues de dmap_rev \n");
}

/*===========================================================================*
 *        dmap_driver_match          *
 *===========================================================================*/ 
int dmap_driver_match(int proc, int major)
{
  if (major < 0 || major >= NR_DEVICES) return(0);
  if(dmap_tab[major].dmap_driver != NONE && dmap_tab[major].dmap_driver == proc)
    return 1;
  return 0;
}

/*===========================================================================*
 *        dmap_endpt_up            *
 *===========================================================================*/ 
void dmap_endpt_up(int proc_e)
{
  int i;
  for (i=0; i<NR_DEVICES; i++) {
    if(dmap_tab[i].dmap_driver != NONE
      && dmap_tab[i].dmap_driver == proc_e
      && (dmap_tab[i].dmap_flags & DMAP_BABY)) {
      dmap_tab[i].dmap_flags &= ~DMAP_BABY;
      dev_up(i);
    }
  }
  return;
}


