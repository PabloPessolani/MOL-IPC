#include <unistd.h>
#include <time.h>
#include <stdio.h>
#include <sys/sysinfo.h>

/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 compilar cc -o testime -lrt testtime.c
 !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
long long HR_ticks_at_start;
long long LR_ticks_by_sec; 

void init_uptime(void)
{
    struct sysinfo info;
    struct timespec res;
	long long ticks;

	HR_ticks_at_start = 0;
	LR_ticks_by_sec = sysconf(_SC_CLK_TCK);
    printf("HR_ticks_by_sec=%ld LR_ticks_by_sec=%ld\n", CLOCKS_PER_SEC, LR_ticks_by_sec);
    sysinfo(&info);
	getuptime(&ticks);
	HR_ticks_at_start = ticks;
	printf("HR_ticks_at_start   %ld\n", HR_ticks_at_start);

}
	
#define clock_t long long
/* RETORNA EN ticks el  numero de timer ticks desde el arranque  */
int getuptime(clock_t *ticks)
{
	struct timespec t;

	clock_gettime(CLOCK_REALTIME, &t);
	*ticks =  (((double)t.tv_nsec)/1.0e9) * LR_ticks_by_sec;
	*ticks +=  ((t.tv_sec * LR_ticks_by_sec) - HR_ticks_at_start);
	if( HR_ticks_at_start != 0)
		printf("sec=%ld, nsecs=%ld ticks=%ld\n", t.tv_sec, t.tv_nsec, *ticks);
	return(0);
}

int main()
{
	long long ticks;
	
	init_uptime();
	
   	getuptime(&ticks);

	sleep(2);

	getuptime(&ticks);
	
 	
 }
