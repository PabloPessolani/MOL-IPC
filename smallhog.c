/* gcc -o hog smallhog.c */
#include <time.h>
#include <limits.h>
#include <signal.h>
#include <sys/time.h>
#define HIST 10

static volatile sig_atomic_t stop;

static void sighandler (int signr)
{
     (void) signr;
     stop = 1;
}
static unsigned long hog (unsigned long niters)
{
     stop = 0;
     while (!stop && --niters);
     return niters;
}
int main (void)
{
     int i;
     struct itimerval it = { .it_interval = { .tv_sec = 0, .tv_usec = 1 },
                             .it_value = { .tv_sec = 0, .tv_usec = 1 } };
     sigset_t set;
     unsigned long v[HIST];
     double tmp = 0.0;
     unsigned long n;
     signal (SIGALRM, &sighandler);
     setitimer (ITIMER_REAL, &it, NULL);

     hog (ULONG_MAX);
     for (i = 0; i < HIST; ++i) v[i] = ULONG_MAX - hog (ULONG_MAX);
     for (i = 0; i < HIST; ++i) tmp += v[i];
     tmp /= HIST;
     n = tmp - (tmp / 3.0);

     sigemptyset (&set);
     sigaddset (&set, SIGALRM);

     for (;;) {
         hog (n);
         sigwait (&set, &i);
     }
     return 0;
}
