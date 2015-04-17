// File: timer.h
#ifndef TIMER_H_INC
#define TIMER_H_INC

#include <sys/time.h>

typedef unsigned long timer_ticks;

struct timer;

struct timer
{
	struct timeval start, stop;
};

void timer_start(struct timer *self);
void timer_stop(struct timer *self);
double timer_get_elapsed_secs(struct timer *self);

#endif

