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
#include "limits.h"
 
 
void  main ( int argc, char *argv[] )
{
    int vmid;
    int clt_pid, svr_pid;
    int clt_ep, svr_ep;
    int clt_nr, svr_nr, ret;
    message m;
 
    //Variables para hacer un OPEN
    char path[PATH_MAX];
    int pathLength = 0;    
    int flags = 0;
 
    // Variables para hacer un CLOSE
    char buffer[PATH_MAX];
    int bytesCount = 0;
    int fileDescriptor = 0;
 
 
    if ( argc != 5)
    {
        printf( "Usage: %s <vmid> <clt_nr> <svr_nr> <fileDescriptor> \n", argv[0] );
        exit(1);
    }
 
    vmid = atoi(argv[1]);
    if ( vmid < 0 || vmid >= NR_VMS)
    {
        printf( "Invalid vmid [0-%d]\n", NR_VMS - 1 );
        exit(1);
    }
 
    clt_nr = atoi(argv[2]);
    svr_ep = svr_nr = atoi(argv[3]);
 
    clt_pid = getpid();
    clt_ep =    mnx_bind(vmid, clt_nr);
    if ( clt_ep < 0 )
        printf("BIND ERROR clt_ep=%d\n", clt_ep);
 
    printf("BIND CLIENT vmid=%d clt_pid=%d clt_nr=%d clt_ep=%d\n",
           vmid,
           clt_pid,
           clt_nr,
           clt_ep);
 
    // strcpy(path, argv[4]);
    // if ( path == NULL)
    // {
    //     printf( "Path NULO:%s\n", path);
    //     exit(1);
    // }
 
    fileDescriptor = atoi(argv[4]);
    // if ( fileDescriptor == NULL)
    // {
    //     printf( "fileDescriptor NULO:%s\n", fileDescriptor);
    //     exit(1);
    // }
    
    //strcpy(path, "/home/prueba/texto.txt");
    //pathLength = strlen(path);

 
    // printf("CLIENT pause before SENDREC (OPEN)\n");
    // sleep(2);
 
    // // armado del mensaje OPEN, siguiendo el codigo fuente minix
    // m.m_type = MOLOPEN;
    // m.m1_i1 = pathLength; //Largo del path + 1
    // m.m1_i2 = flags; // modo de apertura (Lectura/Escritura/etc).
    // m.m1_p1 = path; // path del archivo.
 
    // printf("SENDREC msg OPEN: m_type=%d, m1_i1=%d, m1_i2=%d, m1_p1=%s\n",
    //        m.m_type,
    //        m.m1_i1,
    //        m.m1_i2,
    //        m.m1_p1);
 
    // ret = mnx_sendrec(svr_ep, (long) &m);
    // if ( ret != OK )
    //     printf("ERROR SENDREC ret=%d\n", ret);
    // else
    //     printf("SENDREC ret=%d\n", ret);
 
    // printf("REPLY OPEN msg: m_source=%d, m_type=%d, m1_i1=%d, m1_i2=%d, m1_p1=%s\n",
    //        m.m_source,
    //        m.m_type,
    //        m.m1_i1,
    //        m.m1_i2,
    //        m.m1_p1);
 
    // fileDescriptor = m.m1_i1;
   
    /*CLOSE!!!!*********************************************************************/
    printf("CLIENT pause before SENDREC (CLOSE)\n");
    sleep(2);
 
    m.m_type = MOLCLOSE;
    m.m1_i1 = fileDescriptor; //fileDescriptor obtenido
 
    printf("SENDREC msg CLOSE: m_type=%d, m1_i1=%d, m1_i2=%d, m1_p1=%p\n",
           m.m_type,
           m.m1_i1,
           m.m1_i2,
           m.m1_p1);
 
    ret = mnx_sendrec(svr_ep, (long) &m);
    if ( ret != OK )
        printf("ERROR SENDREC ret=%d\n", ret);
    else
        printf("SENDREC ret=%d\n", ret);
 
    printf("REPLY CLOSE msg: m_source=%d, m_type=%d, m1_i1=%d\n",
           m.m_source,
           m.m_type,
           m.m1_i1);    
 
}