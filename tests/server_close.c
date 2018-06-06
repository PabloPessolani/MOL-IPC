#include <asm/ptrace.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "stub_syscall.h"
#include "./kernel/minix/config.h"
#include "./kernel/minix/ipc.h"
#include "./kernel/minix/kipc.h"
#include "./kernel/minix/callnr.h"
#include "limits.h"
 
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
 
 
void  main ( int argc, char *argv[] )
{
    int dcid, pid, endpoint, p_nr, ret;
    message m;
    int fd;
    int closeResult = 0;
 
 
    char server_path[PATH_MAX];    
 
    if ( argc != 3)
    {
        printf( "Usage: %s <dcid> <p_nr> \n", argv[0] );
        exit(1);
    }
 
    dcid = atoi(argv[1]);
    if ( dcid < 0 || dcid >= NR_DCS)
    {
        printf( "Invalid dcid [0-%d]\n", NR_DCS - 1 );
        exit(1);
    }
 
    p_nr = atoi(argv[2]);
    pid = getpid();
 
    printf("Binding FS Server PID=%d to DC%d with p_nr=%d\n", pid, dcid, p_nr);
    endpoint =  mnx_bind(dcid, p_nr);
    if ( endpoint < 0 )
    {
        printf("BIND ERROR %d\n", endpoint);
        exit(endpoint);
    }
    printf("Process endpoint=%d\n", endpoint);
 
 
    ret = mnx_receive( ANY , (long) &m);
    if ( ret != OK)
    {
        printf("RECEIVE (OPEN) ERROR %d\n", ret);
        mnx_unbind(dcid, endpoint);
        exit(1);
    }
    else
    {
        printf("RECEIVE (OPEN) OK %d\n", ret);
    }
 
    //Me traigo el path desde el cliente
    ret = mnx_zcopy(m.m_source, m.m1_p1, endpoint, server_path, m.m1_i1);
    if ( ret != 0 )
    {
        printf("ZCOPY ret=%d\n", ret);
        printf("server_path %s\n", server_path);
    }
    else
    {
        printf("ZCOPY OK %d\n", ret);
    }
 
 
    printf("ZCOPY RESULT: PATH=%s\n", server_path);
 
    fd = open(server_path, O_WRONLY);
    //fd = open("/etc/shadow", O_RDONLY);
    if (fd > 0)
    {
        printf("file descriptor FD=%d\n", fd);
    }
    else
    {
        printf("OPEN ERROR %d\n", fd);
    }
 
    m.m_type = OK; // OPEN RESULT
    m.m1_i1 = fd; //fd file descriptor
 
    ret = mnx_send(m.m_source, (long) &m);
    if ( ret != OK )
        printf("ERROR SEND ret=%d\n", ret);
    else
        printf("SEND ret=%d\n", ret);
 
    printf("SEND FD msg: server=%d, m_type=%d, fd=%d\n",
       endpoint,
       m.m_type,
       m.m1_i1);
 
    /*TERMINA OPEN, COMIENZA CLOSE*/
 
    ret = mnx_receive( ANY , (long) &m);
    if ( ret != OK)
    {
        printf("RECEIVE (CLOSE) ERROR %d\n", ret);
        mnx_unbind(dcid, endpoint);
        exit(1);
    }
    else
    {
        printf("RECEIVE (CLOSE) OK %d\n", ret);
    }
 
    closeResult = close(m.m1_i1);
    //fd = open("/etc/shadow", O_RDONLY);
    if (closeResult == 0)
    {
        printf("CLOSE OK FD=%d\n", closeResult);
    }
    else
    {
        printf("CLOSE ERROR %d\n", closeResult);
    }
 
    m.m_type = OK; // CLOSE RESULT
    m.m1_i1 = closeResult; //close result
 
    ret = mnx_send(m.m_source, (long) &m);
    if ( ret != OK )
        printf("ERROR SEND ret=%d\n", ret);
    else
        printf("SEND ret=%d\n", ret);
 
    printf("SEND CLOSE_RESULT msg: server=%d, m_type=%d, fd=%d\n",
       endpoint,
       m.m_type,
       m.m1_i1);
 
    exit(1);
 
}