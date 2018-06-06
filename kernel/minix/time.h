/*
sys/time.h
*/

#ifndef _MNX__TIME_H
#define _MNX__TIME_H

#include "ansi.h"

/* Open Group Base Specifications Issue 6 (not complete) */
struct mnx_timeval_s
{
	long /*time_t*/ mnx_tv_sec;
	long /*useconds_t*/ mnx_tv_usec;
};
typedef struct mnx_timeval mnx_timeval_t;

int gettimeofday(struct mnx_timeval *_RESTRICT tp, void *_RESTRICT tzp);

/* Compatibility with other Unix systems */
int settimeofday(const struct mnx_timeval *tp, const void *tzp);

#endif /* _MNX__TIME_H */
