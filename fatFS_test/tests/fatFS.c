// #include <string.h>
// #include <stdio.h>
// #include <locale.h>
#define SVRDBG	1
#include "m3ipc.h"
#include "diskio.h"
#include "ff.h"		/* Declarations of FatFs API */


#define BUFSIZE 4096
#define VERSION 23


FATFS fsWork;		/* FatFs work area needed for each volume */
FIL Fil;			/* File object needed for each open file */

int httpd_lpid;
int httpd_ep;
char *out_buf;
FILINFO fat_fstat;
dvs_usr_t dvs, *dvs_ptr;
VM_usr_t  vmu, *dc_ptr;
proc_usr_t http, *http_ptr;
proc_usr_t rdisk, *rdisk_ptr;
message *m_ptr;

int main (void)
{
    // FATFS fs[2];         /* Work area (file system object) for logical drives */
    FIL fsrc, fdst;         /* File objects */
    //FF_BYTE buffer[4096];   /* File copy buffer */
    FF_BYTE *buffer;        /* File copy buffer */
    FRESULT fr;             /* FatFs function common result code */
    FF_UINT br, bw;         /* File read/write count */	


	f_mount(&fsWork, "", 0);		/* Give a work area to the default drive */
	fr = f_open(&fsrc, "index.htm", FA_READ);
	// SVRDEBUG("DESPUES OPEN !!!!!!!!!!!!!!!!!!!!!!!!!!!"); //<--------PABLO!!!! ESTE POR EJEMPLO NO ANDA
	// fr = f_open(&fsrc, "mamut1.jpg", FA_READ);
	// fr = f_open(&fsrc, "MAMUT1.JPG", FA_READ);
	SVRDEBUG("result f_open:%d \n", fr);
	SVRDEBUG("FR_OK:%d \n", FR_OK);
	// printf("DESPUES OPEN -- ANTES IF \n");

	if (fr == FR_OK) {	/* Create a file */

		//fr = f_read(&fsrc, buffer, sizeof buffer, &br);  /* Read a chunk of source file */
		printf("ANTES READ \n");
		fr = f_read(&fsrc, buffer, 35, &br);  /* Read a chunk of source file */
		SVRDEBUG("f_read RESULT:%d \n", fr);
		printf("DESPUES READ \n");

		printf("buffer:%s \n", buffer);

		SVRDEBUG("bytes READ:%d \n", br);

	}
	f_close(&fsrc);								/* Close the file */

	for (;;) ;
}
