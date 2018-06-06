/************** PROGRAMA DE USUARIO PARA PRUEBA DE ALGORITMO DE COPIA ENTRE PROCESOS ****************/
#include <stdio.h>
#include <string.h>

#define PAGE_SIZE 	16
#define PAGE_SHIFT	4
#define PAGE_MASK		(~(PAGE_SIZE-1))
#define NR_PAG		4

#define MIN(x, y)		(x<y)?x:y

char src_buf[PAGE_SIZE*NR_PAG];
char dst_buf[PAGE_SIZE*NR_PAG];
char rule_buf[PAGE_SIZE*NR_PAG];

int get_user_pages(int addr)
{
	return(addr/PAGE_SIZE);
} 

void  main ( int argc, char *argv[] )
{
	int	i, bytes;
	int 	src_off, dst_off;
	int 	src_npag, dst_npag;	/* number of pages the the copy implies*/
	int	src_addr, dst_addr;
	int	slen, dlen, len;
	int	spag, dpag;		/* page number */

  	if (argc != 4) {
    		printf ("usage: %s <src_addr> <dst_addr> <bytes> \n", argv[0]);
    		exit(1);
  	}

	/* parametros de entrada */
	src_addr = atoi(argv[1]);
	dst_addr = atoi(argv[2]);
	bytes	= atoi(argv[3]);

printf("src_addr=%d dst_addr=%d bytes=%d \n",src_addr, dst_addr, bytes );

if(bytes < 0 || bytes > ((NR_PAG*PAGE_SIZE)-1)) {
	printf("bad bytes\n");
	exit(1);
}

if( src_addr > ((NR_PAG*PAGE_SIZE)-1) || dst_addr > ((NR_PAG*PAGE_SIZE)-1)  ){
	printf("bad addr\n");
	exit(1);
}

if( (src_addr+bytes) > ((NR_PAG*PAGE_SIZE)-1) || (dst_addr+bytes) > ((NR_PAG*PAGE_SIZE)-1)  ){
	printf("out of range\n");
	exit(1);
}


	for(i = 0; i < ((PAGE_SIZE*NR_PAG)-1); i++) {
		src_buf[i] = ((i%10) + '0');
		dst_buf[i] = '.';
		if( i%PAGE_SIZE)
			rule_buf[i] = '-';
		else
			rule_buf[i] = '|';

	}
	src_buf[PAGE_SIZE*NR_PAG-1] = 0;
	dst_buf[PAGE_SIZE*NR_PAG-1] = 0;
	rule_buf[PAGE_SIZE*NR_PAG-1] = 0;

	printf("    [%s]\n",rule_buf);
	printf("SRC [%s]\n",src_buf);
	printf("DST [%s]\n",dst_buf);


	while( bytes > 0) {
		src_off  = src_addr & (~PAGE_MASK);
		src_npag = (src_off+bytes+PAGE_SIZE-1)>>PAGE_SHIFT;
printf("src_off=%d src_npag=%d\n",src_off, src_npag);

		dst_off  = dst_addr & (~PAGE_MASK);
		dst_npag = (dst_off+bytes+PAGE_SIZE-1)>>PAGE_SHIFT;
printf("dst_off=%d dst_npag=%d\n",dst_off, dst_npag);

		spag = get_user_pages(src_addr); 
		dpag = get_user_pages(dst_addr);
printf("bytes=%d spag=%d dpag=%d\n",bytes, spag, dpag);

		slen = PAGE_SIZE-src_off;
		dlen = PAGE_SIZE-dst_off;
printf("dlen=%d slen=%d\n",dlen, slen);

		len = MIN(slen, dlen);
		len = MIN(bytes, len);

		memcpy((void*)&dst_buf[dst_off+(dpag*PAGE_SIZE)],
			(void*)&src_buf[src_off+(spag*PAGE_SIZE)],len);
		src_addr+=len;
		dst_addr+=len;		
		bytes=bytes-len;
	}

	printf("    [%s]\n",rule_buf);
	printf("SRC [%s]\n",src_buf);
	printf("DST [%s]\n",dst_buf);
exit(0);
}
