/********************** threads.c *************************************/

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
/* COMPILAR  cc -o threads -pthread threads.c  */
#define MAXLOOP 		20
#define MAXTHREADS 	5

int r; 
char letra[10];

static void *threadFunc(void *arg)
{
	int i;
	char car, *ptr;

	car = (char) arg;
	sleep(1);
	for(i =0; i < MAXLOOP; i++)
		{
		putchar(car);
		sleep(car-65);
		fflush(stdout);
		}
	r = (int)car;
	return((void*)&r);
}


int main(int argc, char *argv[])
{
    	pthread_t t[MAXTHREADS];
    	void *res;
    	int i, j, s;
	char car;

	for( j = 0; j < MAXTHREADS; j++)
		{
		letra[j] = 65+j;
		s = pthread_create(&t[j], NULL, threadFunc, letra[j]);
	    	if (s != 0)
      		{
	   		printf("ERROR pthread_create %d \n",j);
			exit(1);
			}
		else
	   		printf("pthread_create %d letra[j]=%c\n",j, letra[j]);
		}

	for( j = 0; j < MAXTHREADS; j++)
		{
		s = pthread_join(t[j], &res);
    		if (s != 0)
      		{
	   		printf("pthread_join %d\n",j);
			exit(1);
			}
		else
	   		printf("pthread_join success %d\n",j);
		}
		
    exit(EXIT_SUCCESS);
}
