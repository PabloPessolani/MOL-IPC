/*-----------------------------------------------------------------------*/
/* Low level disk I/O module skeleton for FatFs     (C)ChaN, 2016        */
/*-----------------------------------------------------------------------*/
/* If a working storage control module is available, it should be        */
/* attached to the FatFs via a glue function rather than modifying it.   */
/* This is an example of glue functions to attach various exsisting      */
/* storage control modules to the FatFs module with a defined API.       */
/*-----------------------------------------------------------------------*/

// #define SVRDBG		1
#include "fs.h"

#include "ffconf.h"
// #include "diskio.h"		/* FatFs lower layer API */

/* Definitions of physical drive number for each drive */
#define DEV_RDISK 	3	/* Use REPLICATED DISK task with M3IPC */
#define DEV_FILE	1	/* Example: Map FileDisk to physical drive 0 */

// #define DEV_MMC		1	/* Example: Map MMC/SD card to physical drive 1 */
// #define DEV_USB		2	/* Example: Map USB MSD to physical drive 2 */
// #define DEV_MEM		3	/* Example: Map Ramdisk to physical drive 0 */

#define  SECTOR_SIZE	_MAX_SS
#define  MAXPATHNAME	8192
#define  MAXDRIVES	    15


extern message *m_ptr;
extern proc_usr_t *http_ptr;
extern fdisk_sts = STA_NOINIT;
extern rdisk_sts = STA_NOINIT;

int img_fd;

int FILE_IMAGE_disk_status(void)
{
	SVRDEBUG("fdisk_sts:%d\n", fdisk_sts);

	return fdisk_sts;
}

int RDISK_disk_status(void)
{
	SVRDEBUG("rdisk_sts:%d\n", rdisk_sts);

	return rdisk_sts;
}

int FILE_IMAGE_disk_initialize(FF_BYTE pdrv)
{

	SVRDEBUG("pdrv:%d \n", pdrv);

	if ( pdrv < 0 || pdrv > MAXDRIVES) ERROR_EXIT(STA_NOINIT);
	// sprintf (img_name, "images/floppy3FAT_%d.img", pdrv);
	SVRDEBUG("img_name:%s \n", img_name);

	img_fd = open(img_name, O_RDWR);
	// SVRDEBUG("img_fd:%d \n", img_fd );	
	// // img_fd = fopen(img_name, "r+w");
	if ( img_fd < 0) ERROR_EXIT(STA_NOINIT);
	// rdisk_sts = STA_READY;
	fdisk_sts &= ~STA_NOINIT;					/* Initialization succeeded */
	return fdisk_sts;
}

int RDISK_disk_initialize(FF_BYTE pdrv)
{
	int status;
	SVRDEBUG("pdrv:%d \n", pdrv);

	if ( pdrv < 0 || pdrv > MAXDRIVES) ERROR_EXIT(STA_NOINIT);

	if ((status=dev_open(root_dev, FS_PROC_NR, R_BIT|W_BIT)) != OK)
		SVRDEBUG("Cannot open root device %d\n", status);
	
	SVRDEBUG("DEV: root_dev %d \n", root_dev);
	
	if ( status < 0) ERROR_EXIT(STA_NOINIT);
	

	rdisk_sts &= ~STA_NOINIT;					/* Initialization succeeded */
	return rdisk_sts;
}

int FILE_IMAGE_disk_read(FF_BYTE* buff,  FF_DWORD sector, FF_UINT count)
{
	int ret;

	// SVRDEBUG("sector:%ld count=%u \n", sector, count );

	lseek(img_fd, sector * SECTOR_SIZE, SEEK_SET);
	ret = read(img_fd, buff,  count * SECTOR_SIZE);

	// SVRDEBUG("bytes read:%d \n", ret);
	// SVRDEBUG("buffer:%s \n", buff);
	if ( ret < 0 )
	 	return RES_PARERR;
	return RES_OK;
}

int RDISK_disk_read(FF_BYTE* buff,  FF_DWORD sector, FF_UINT count)
{
	int ret;

	// SVRDEBUG("sector:%ld count=%u \n", sector, count );

	// lseek(img_fd, sector * SECTOR_SIZE, SEEK_SET);
	// ret = read(img_fd, buff,  count * SECTOR_SIZE);
	// SVRDEBUG("DEV: root_dev %d \n", root_dev);
	// SVRDEBUG("sector * SECTOR_SIZE %d \n", sector * SECTOR_SIZE);
	// SVRDEBUG("count * SECTOR_SIZE %d \n", count * SECTOR_SIZE);	

	ret = dev_io(DEV_READ, root_dev, FS_PROC_NR, buff, sector * SECTOR_SIZE, count * SECTOR_SIZE, O_RDWR);//Ver lo de las flags

	SVRDEBUG("bytes read:%d \n", ret);
	// SVRDEBUG("buffer:%s \n", buff);
	if ( ret < 0 )
	 	return RES_PARERR;
	return RES_OK;
}


int FILE_IMAGE_disk_write(const FF_BYTE* buff,  FF_DWORD sector, FF_UINT count)
{
	int ret;


	// SVRDEBUG("sector:%ld count=%u \n", sector, count );

	lseek(img_fd, sector * SECTOR_SIZE, 0);
	ret = write(img_fd, buff,  count * SECTOR_SIZE);
	
	SVRDEBUG("bytes written:%d \n", ret);
	// SVRDEBUG("buffer:%s \n", buff);
	if ( ret < 0 )
	 	return RES_PARERR;
	return RES_OK;
}

int RDISK_disk_write(FF_BYTE* buff,  FF_DWORD sector, FF_UINT count)
{
	int ret;

	// SVRDEBUG("sector:%ld count=%u \n", sector, count );

	// lseek(img_fd, sector * SECTOR_SIZE, SEEK_SET);
	// ret = read(img_fd, buff,  count * SECTOR_SIZE);
	
	// SVRDEBUG("DEV: root_dev %d \n", root_dev);
	// SVRDEBUG("sector * SECTOR_SIZE %d \n", sector * SECTOR_SIZE);
	// SVRDEBUG("count * SECTOR_SIZE %d \n", count * SECTOR_SIZE);

	ret = dev_io(DEV_WRITE, root_dev, FS_PROC_NR, buff, sector * SECTOR_SIZE, count * SECTOR_SIZE, O_RDWR);//Ver lo de las flags

	SVRDEBUG("bytes written:%d \n", ret);
	// SVRDEBUG("buffer:%s \n", buff);
	if ( ret < 0 )
	 	return RES_PARERR;
	return RES_OK;
}

/*-----------------------------------------------------------------------*/
/* Get Drive Status                                                      */
/*-----------------------------------------------------------------------*/


DSTATUS disk_status (
    FF_BYTE pdrv		/* Physical drive nmuber to identify the drive */
)
{
	// int result;

	// SVRDEBUG("pdrv:%d\n", pdrv);

	switch (pdrv) {
	case DEV_RDISK :
		rdisk_sts = RDISK_disk_status();		 
		return rdisk_sts;
	case DEV_FILE:
		// Process of the command the USB drive
		fdisk_sts = FILE_IMAGE_disk_status();		 
		return fdisk_sts;			
	}
	return STA_NOINIT;
}

/*-----------------------------------------------------------------------*/
/* Inidialize a Drive                                                    */
/*-----------------------------------------------------------------------*/

DSTATUS disk_initialize (
    FF_BYTE pdrv				/* Physical drive nmuber to identify the drive */
)
{
	// int result;

	// SVRDEBUG("pdrv:%d\n", pdrv);

	switch (pdrv) {
	case DEV_RDISK:
		rdisk_sts = RDISK_disk_initialize(pdrv);		 
		return rdisk_sts;
	case DEV_FILE:
		// Process of the command the USB drive
		fdisk_sts = FILE_IMAGE_disk_initialize(pdrv);		 
		return fdisk_sts;			
	}
	return STA_NOINIT;
}

/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/
DRESULT disk_read (
    FF_BYTE pdrv,		/* Physical drive nmuber to identify the drive */
    FF_BYTE *buff,		/* Data buffer to store read data */
    FF_DWORD sector,	/* Start sector in LBA */
    FF_UINT count		/* Number of sectors to read */
)
{
	DRESULT res = RES_PARERR;

	SVRDEBUG("pdrv:%d sector=%lu count=%lu\n", pdrv, sector, count );

	switch (pdrv) {
	case DEV_RDISK:
		// translate the arguments here
		res = RDISK_disk_read(buff, sector, count);
		return res;
	case DEV_FILE:
		// Process of the command the USB drive
		res = FILE_IMAGE_disk_read(buff, sector, count);
		return res;			
	}

	return RES_PARERR;
}


/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/

DRESULT disk_write (
    FF_BYTE pdrv,			/* Physical drive nmuber to identify the drive */
    const FF_BYTE *buff,	/* Data to be written */
    FF_DWORD sector,		/* Start sector in LBA */
    FF_UINT count			/* Number of sectors to write */
)
{
	DRESULT res = RES_PARERR;

	SVRDEBUG("pdrv:%d sector=%lu count=%lu\n", pdrv, sector, count );

	switch (pdrv) {
	case DEV_RDISK:
		// translate the arguments here
		res = RDISK_disk_write(buff, sector, count);
		return res;
	case DEV_FILE:
		// Process of the command the USB drive
		res = FILE_IMAGE_disk_write(buff, sector, count);		 
		return res;		

	}

	return RES_PARERR;
}



/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */
/*-----------------------------------------------------------------------*/

DRESULT disk_ioctl (
    FF_BYTE pdrv,		/* Physical drive nmuber (0..) */
    FF_BYTE cmd,		/* Control code */
    void *buff		/* Buffer to send/receive control data */
)
{
	DRESULT res = RES_PARERR;
	// int result;

	// SVRDEBUG("pdrv:%d\n", pdrv);

	switch (pdrv) {
	case DEV_RDISK:
		// Process of the command the USB drive
		return res;
	case DEV_FILE:
		// Process of the command the USB drive
		return res;		
	}

	return RES_PARERR;
}



