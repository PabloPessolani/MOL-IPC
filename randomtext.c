/*
	This program creates a random text  output of <nr_regs> registers of 
	<rlen> bytes len. (including \n)
	In a randomized way, it also embeds the single word text  <KEY> into the registers 
	
*/

#include <asm/ptrace.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include <sys/types.h>
#include <sys/wait.h> 

char *buffer;
#define MAXRECLEN 128
#define MAX_ALPHABET (10)
#define MATCH_RATE 10

void  main( int argc, char *argv[])
{
	int rcode;
	FILE *fp;
	int bytes, rlen, c, j, rl, keylen, rate;
	unsigned long nr_regs, i;

	if ( argc != 5 )	{
		fprintf(stderr, "Usage: %s <nr_regs> <rlen> <match_rate> <KEY> \n", argv[0]);
		fprintf(stderr, "\t rlen=0 means variable record lenght\n");
		exit(1);
	}

	/*verifico q el nro de vm sea válido*/
	nr_regs = atol(argv[1]);
	if ( nr_regs < 0 ) {
		fprintf(stderr,"Invalid nr_regs %d\n", nr_regs);
		exit(1);
	}

	rlen = atoi(argv[2]);
	if ( rlen < 0 ) {
		fprintf(stderr,"Invalid rlen %d\n", rlen);
		exit(1);
	}
	
	rate = atoi(argv[3]);
	if ( rate < 0 || rate > RAND_MAX ) {
		fprintf(stderr,"Invalid rate %d\n", rate);
		exit(1);
	}
	
	if( rlen > 0) {
		if( strlen(argv[4]) > (rlen-1))	{
			fprintf(stderr,"Invalid KEY length %d\n", strlen(argv[4]));
			exit(1);
		}
	}
	
	rcode = posix_memalign( (void**) &buffer, getpagesize(), getpagesize());
	if( rcode ) {
		fprintf(stderr,"posix_memalign rcode=%d\n", rcode);
		exit(1);
	}
	
	/* open/create file to create */
//	fp = fopen(argv[1], "w+");
//	if( fp == NULL) {
//		fprintf(stderr,"fopen ERROR for file=%s errno=%d\n", argv[1], errno);
//		exit(1);
//	}
	keylen =  strlen(argv[4]);
	srandom( getpid());
	if( rlen > 0)
		rl = rlen;
//	else
//		printf("variable record length\n");
		
	for ( i = 0; i < nr_regs; i++) {
		if( rlen == 0) {
			rl = (random()/(RAND_MAX/(MAXRECLEN - keylen - 2))) + keylen + 3;
//			printf("rl=%d\n",rl);
		}
		for(j = 0; j < rl-1; j++){
//			c = (random()/(RAND_MAX/MAX_ALPHABET)) + '0';
			buffer[j] = (j%MAX_ALPHABET) + '0';
		}
			
		if( !(random()/(RAND_MAX/rate)))  {
			memcpy( &buffer[(random()/(RAND_MAX/(rl-keylen-2)))],argv[4], keylen);  
		}

		buffer[j] = '\n';
		buffer[j+1] = 0x00;

		fprintf(stdout, "%s",buffer);
		
//		bytes= fwrite( (void *) buffer, 1, rlen, fp);
//		if( bytes < 0) {
//			fprintf(stderr,"fwrite ERROR rcode=%d\n",bytes);
//			exit(1);
//		}
	}

//	rcode = fclose(fp);
//	if (rcode)	{
//		fprintf(stderr,"fclose rcode=%d errno=%d\n",rcode, errno);
//		exit(1);
//	}
	exit(0);

 }
