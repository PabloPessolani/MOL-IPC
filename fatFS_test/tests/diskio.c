/*-----------------------------------------------------------------------*/
/* Low level disk I/O module skeleton for FatFs     (C)ChaN, 2016        */
/*-----------------------------------------------------------------------*/
/* If a working storage control module is available, it should be        */
/* attached to the FatFs via a glue function rather than modifying it.   */
/* This is an example of glue functions to attach various exsisting      */
/* storage control modules to the FatFs module with a defined API.       */
/*-----------------------------------------------------------------------*/


// #include "lwip/opt.h"
// #include "lwip/arch.h"
// #include "lwip/api.h"

#define SVRDBG		1
#include "m3ipc.h"
#include "ffconf.h"
#include "diskio.h"		/* FatFs lower layer API */

/* Definitions of physical drive number for each drive */
#define DEV_RDISK 	4	/* Use RDISK task with M3IPC */
#define DEV_MMC		1	/* Example: Map MMC/SD card to physical drive 1 */
#define DEV_USB		2	/* Example: Map USB MSD to physical drive 2 */
#define DEV_RAM		3	/* Example: Map Ramdisk to physical drive 0 */
#define DEV_FILE	0	/* Example: Map FileDisk to physical drive 0 */

#define  SECTOR_SIZE	_MAX_SS
#define  MAXPATHNAME	8192
#define  MAXDRIVES	    15


extern message *m_ptr;
extern proc_usr_t *http_ptr;
int	fdisk_sts = STA_NOINIT;

const char img_name[MAXPATHNAME];
int img_fd;

int FILE_IMAGE_disk_status(void)
{
	SVRDEBUG("rdisk_sts:%d\n", fdisk_sts);

	return fdisk_sts;
}

int FILE_IMAGE_disk_initialize(FF_BYTE pdrv)
{

	SVRDEBUG("pdrv:%d \n", pdrv);

	// if ( pdrv < 0 || pdrv > MAXDRIVES) ERROR_EXIT(STA_NOINIT);
	sprintf (img_name, "images/floppy3FAT_%d.img", pdrv);
	SVRDEBUG("img_name:%s \n", img_name );

	img_fd = open(img_name, O_RDWR);
	SVRDEBUG("img_fd:%d \n", img_fd );	
	// img_fd = fopen(img_name, "r+w");
	if ( img_fd < 0) ERROR_EXIT(STA_NOINIT);
	// rdisk_sts = STA_READY;
	fdisk_sts &= ~STA_NOINIT;					/* Initialization succeeded */
	return fdisk_sts;
}

int FILE_IMAGE_disk_read(FF_BYTE* buff,  FF_DWORD sector, FF_UINT count)
{
	int ret;

	SVRDEBUG("sector:%ld count=%u \n", sector, count );

	lseek(img_fd, sector * SECTOR_SIZE, SEEK_SET);
	ret = read(img_fd, buff,  count * SECTOR_SIZE);

	//SVRDEBUG("bytes read:%d \n", ret);
	SVRDEBUG("buffer:%s \n", buff);
	if ( ret < 0 )
	 	return RES_PARERR;
	return RES_OK;
}

int FILE_IMAGE_disk_write(FF_BYTE* buff,  FF_DWORD sector, FF_UINT count)
{
	int ret;


	SVRDEBUG("sector:%ld count=%u \n", sector, count );

	lseek(img_fd, sector * SECTOR_SIZE, 0);
	ret = write(img_fd, buff,  count * SECTOR_SIZE);
	SVRDEBUG("bytes written:%d \n", ret);
	SVRDEBUG("buffer:%s \n", buff);
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
	int result;

	SVRDEBUG("pdrv:%d\n", pdrv);

	switch (pdrv) {
	case DEV_RAM :
//		result = RAM_disk_status();
		// translate the reslut code here
		return fdisk_sts;
	case DEV_MMC :
//		result = MMC_disk_status();
		// translate the reslut code here
		return fdisk_sts;
	case DEV_USB :
//		result = USB_disk_status();
		// translate the reslut code here
		return fdisk_sts;
	case DEV_RDISK :
		// result = FILE_IMAGE_disk_status();
		// translate the reslut code here
		return fdisk_sts;
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
	int result;

	SVRDEBUG("pdrv:%d\n", pdrv);

	switch (pdrv) {
	case DEV_RAM :
//		result = RAM_disk_initialize();
		// translate the reslut code here
		return fdisk_sts;
	case DEV_MMC :
//		result = MMC_disk_initialize();
		// translate the reslut code here
		return fdisk_sts;
	case DEV_USB :
//		result = USB_disk_initialize();
		// translate the reslut code here
		return fdisk_sts;
	case DEV_RDISK:
		//stat = FILE_IMAGE_disk_initialize(0);
		// translate the reslut code here
		return fdisk_sts;
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
	int result;

	SVRDEBUG("pdrv:%d sector=%ld count=%ld\n", pdrv, sector, count );

	switch (pdrv) {
	case DEV_RAM :
		// translate the arguments here
//		result = RAM_disk_read(buff, sector, count);
		// translate the reslut code here
		return res;
	case DEV_MMC :
		// translate the arguments here
//		result = MMC_disk_read(buff, sector, count);
		// translate the reslut code here
		return res;
	case DEV_USB :
		// translate the arguments here
//		result = USB_disk_read(buff, sector, count);
		// translate the reslut code here
		return res;
	case DEV_RDISK:
		// translate the arguments here
		//res = RDISK_disk_read(buff, sector, count);
		// translate the reslut code here
		return res;
	case DEV_FILE:
		// Process of the command the USB drive
		res = FILE_IMAGE_disk_read(buff, sector, count);
		SVRDEBUG("FILE_IMAGE_disk_read (RESULT):%d \n", res);		 
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
	int result;

	SVRDEBUG("pdrv:%d sector=%ld count=%ld\n", pdrv, sector, count );

	switch (pdrv) {
	case DEV_RAM :
		// translate the arguments here
//		result = RAM_disk_write(buff, sector, count);
		// translate the reslut code here
		return res;
	case DEV_MMC :
		// translate the arguments here
//		result = MMC_disk_write(buff, sector, count);
		// translate the reslut code here
		return res;
	case DEV_USB :
		// translate the arguments here
//		result = USB_disk_write(buff, sector, count);
		// translate the reslut code here
		return res;
	case DEV_RDISK:
		// translate the arguments here
		//result = RDISK_disk_write(buff, sector, count);
		// translate the reslut code here
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
	int result;

	SVRDEBUG("pdrv:%d\n", pdrv);

	switch (pdrv) {
	case DEV_RAM :
		// Process of the command for the RAM drive
		return res;
	case DEV_MMC :
		// Process of the command for the MMC/SD card
		return res;
	case DEV_USB :
		// Process of the command the USB drive
		return res;
	case DEV_RDISK:
		// Process of the command the USB drive
		return res;
	case DEV_FILE:
		// Process of the command the USB drive
		return res;		
	}

	return RES_PARERR;
}



