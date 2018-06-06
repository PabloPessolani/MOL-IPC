#include <asm/ptrace.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include "../stub_syscall.h"
#include "../kernel/minix/config.h"
#include "../kernel/minix/ipc.h"
#include "../kernel/minix/kipc.h"
#include "../kernel/minix/callnr.h"
//#include "../kernel/minix/fcntl.h"
#include "limits.h"


// int open(const char *pathname, int flags);
// int open(const char *pathname, int flags, mode_t mode);
// open('path',O_WRONLY|O_CREAT,0640);
// RUTA, flags, MODO

// Available Values for oflag

// Value	Meaning
// O_RDONLY	Open the file so that it is read only.
// O_WRONLY	Open the file so that it is write only.
// O_RDWR	Open the file so that it can be read from and written to.
// O_APPEND	Append new information to the end of the file.
// O_TRUNC	Initially clear all data from the file.
// O_CREAT	If the file does not exist, create it. If the O_CREAT option is used, then you must include the third parameter.
// O_EXCL	Combined with the O_CREAT option, it ensures that the caller must create the file. If the file already exists, the call will fail.
// Available Values for mode

// Value	Meaning
// S_IRUSR	Set read rights for the owner to true.
// S_IWUSR	Set write rights for the owner to true.
// S_IXUSR	Set execution rights for the owner to true.
// S_IRGRP	Set read rights for the group to true.
// S_IWGRP	Set write rights for the group to true.
// S_IXGRP	Set execution rights for the group to true.
// S_IROTH	Set read rights for other users to true.
// S_IWOTH	Set write rights for other users to true.
// S_IXOTH	Set execution rights for other users to true.
// 

/* File access modes for open() and fcntl().  POSIX Table 6-6. */
#define O_RDONLY           0	/* open(name, O_RDONLY) opens read only */
#define O_WRONLY           1	/* open(name, O_WRONLY) opens write only */
#define O_RDWR             2	/* open(name, O_RDWR) opens read/write */

#define O_CREAT        00100	/* creat file if it doesn't exist */
#define O_EXCL         00200	/* exclusive use flag */
#define O_NOCTTY       00400	/* do not assign a controlling terminal */
#define O_TRUNC        01000	/* truncate flag */

void mnx_loadname(char *name, message *msgptr)
{
/* This function is used to load a string into a type m3 message. If the
 * string fits in the message, it is copied there.  If not, a pointer to
 * it is passed.
 */

 register size_t k;

 k = strlen(name) + 1;
 //k = strlen(name);
 msgptr->m3_i1 = k;
 msgptr->m3_p1 = (char *) name;
 //msgptr->m3_p1 = name;
 // printf("msgptr->m3_p1 (name)=%s\n", msgptr->m3_p1);
 // printf("msgptr->m3_i1 (strlen)=%d\n", msgptr->m3_i1);
 // printf("sizeof msgptr->m3_ca1=%d\n", sizeof msgptr->m3_ca1);
 if (k <= sizeof msgptr->m3_ca1) {
 	// printf("msgptr->m3_ca1 (strlen)=%s\n", msgptr->m3_ca1);
 	strcpy(msgptr->m3_ca1, name);
 }
}


void  main ( int argc, char *argv[] )
{
	int dcid;
	int clt_pid, svr_pid;
	int clt_ep, svr_ep;
	int clt_nr, svr_nr, ret;
	message m;

    //Variables para hacer un OPEN
	char path[PATH_MAX];
	int tipoMensaje = 0;
	int flags = 0;

    // Variables para hacer un READ
	char buffer[PATH_MAX];
	int bytesCount = 0;
	int fileDescriptor = 0;

	printf ("Cant Argumentos %d\n", argc);
	if ( argc != 7)
	{
		printf( "Usage: %s <dcid> <clt_nr> <svr_nr> <path> <bytesCount> <tipoMensaje M1=1 o M3=3> \n", argv[0] );
		exit(1);
	}

	dcid = atoi(argv[1]);
	if ( dcid < 0 || dcid >= NR_DCS)
	{
		printf( "Invalid dcid [0-%d]\n", NR_DCS - 1 );
		exit(1);
	}

	clt_nr = atoi(argv[2]);
	svr_ep = svr_nr = atoi(argv[3]);

	clt_pid = getpid();
	clt_ep =    mnx_bind(dcid, clt_nr);
	if ( clt_ep < 0 )
		printf("BIND ERROR clt_ep=%d\n", clt_ep);

	printf("BIND CLIENT dcid=%d clt_pid=%d clt_nr=%d clt_ep=%d\n",
		dcid,
		clt_pid,
		clt_nr,
		clt_ep);

	strcpy(path, argv[4]);
	if ( path == NULL)
	{
		printf( "Path NULO:%s\n", path);
		exit(1);
	}


	bytesCount = atoi(argv[5]);
	if ( bytesCount == 0)
	{
		printf( "bytesCount no debe ser CERO:%d\n", bytesCount);
		exit(1);
	}

	tipoMensaje = atoi(argv[6]);
	if ( tipoMensaje < 0 || tipoMensaje == 0)
	{
		printf( "tipoMensaje NULO:%s\n", tipoMensaje);
		exit(1);
	}


	printf("CLIENT pause before SENDREC (OPEN)\n");
	sleep(2);

    // armado del mensaje OPEN, siguiendo el codigo fuente minix
    // 

	m.m_type = MOLOPEN;
	if ( tipoMensaje == M1) {
		printf("mensaje tipo M1\n");
		m.m1_i1 = strlen(path) + 1;
		m.m1_i2 = O_RDWR; //flags posibles MODOS de APERTURA para OPEN. Aqui deberia ser O_CREAT para tipo M1 porque en posix esta asiv
		//m.m1_i3 = va_arg(argp, _mnx_Mode_t);
		m.m1_p1 = (char *) path;	
	} else {
		printf("mensaje tipo M3\n");
		//mnx_loadname(path, &m);
		m.m3_i1 = strlen(path) + 1;	
		m.m3_p1 = path;
		m.m3_i2 = O_RDONLY; //para lectura tipo M3
		if ((strlen(path) + 1) <= sizeof m.m3_ca1) {
		// 	printf("msgptr m3_ca1 (strlen)=%d\n", sizeof m.m3_ca1);
		 	strcpy(m.m3_ca1, path);
		}		
	}

	// printf("SENDREC msg OPEN: m_type=%d, m1_i1=%d, m1_i2=%d, m1_p1=%s\n",
	// 	m.m_type,
	// 	m.m1_i1,
	// 	m.m1_i2,
	// 	m.m1_p1);

	printf("SENDREC msg OPEN: m_type=%d, m3_i1=%d, m3_i2=%d, m3_p1=%s\n",
		m.m_type,
		m.m3_i1,
		m.m3_i2,
		m.m3_p1);

	// printf("SENDREC msg OPEN: m_type=%d, m1_i1=%d, m1_i2=%d, m1_p1=%s\n",
	// 	m.m_type,
	// 	strlen(path) + 1,
	// 	O_RDONLY,
	// 	path);

		ret = mnx_sendrec(svr_ep, (long) &m);
		if ( ret != OK )
			printf("ERROR SENDREC ret=%d\n", ret);
		else
			printf("SENDREC ret=%d\n", ret);

    // printf("REPLY OPEN msg: m_source=%d, m_type=%d, m1_i1=%d, m1_i2=%d, m1_p1=%s\n",
    //        m.m_source,
    //        m.m_type,
    //        m.m1_i1,
    //        m.m1_i2,
    //        m.m1_p1);

		printf("REPLY OPEN msg: m_source=%d, m_type=%d\n",
			m.m_source,
			m.m_type);
    //        
		fileDescriptor = m.m_type;

		printf("fileDescriptor =%d\n", fileDescriptor);

    // READ!!!!********************************************************************
		printf("CLIENT pause before SENDREC (READ)\n");
		sleep(2);

    // //armado del mensaje READ
		m.m_type = MOLREAD;
    m.m1_i1 = fileDescriptor; //fileDescriptor obtenido
    m.m1_i2 = bytesCount; // cantidad de bytes a leer
    m.m1_p1 = buffer; // path del archivo.

    printf("SENDREC msg READ: m_type=%d, m1_i1=%d, m1_i2=%d, m1_p1=%p\n",
    	m.m_type,
    	m.m1_i1,
    	m.m1_i2,
    	m.m1_p1);

    ret = mnx_sendrec(svr_ep, (long) &m);
    if ( ret != OK )
    	printf("ERROR SENDREC ret=%d\n", ret);
    else
    	printf("SENDREC ret=%d\n", ret);

    printf("REPLY READ msg: m_source=%d, m_type=%d, m1_i1=%d, m1_i2=%d, m1_p1=%p\n",
    	m.m_source,
    	m.m_type,
    	m.m1_i1,
    	m.m1_i2,
    	(char*)m.m1_p1);  

    printf("buffer %s\n", buffer);

}