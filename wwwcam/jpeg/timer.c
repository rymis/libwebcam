#include <sys/time.h>
#include <stdlib.h>
#include "timer.h"

void timer_start(struct timer *self)
{
	gettimeofday(&self->start, NULL);
}

void timer_stop(struct timer *self)
{
	gettimeofday(&self->stop, NULL);
}

double timer_get_elapsed_secs(struct timer *self)
{
	return (self->stop.tv_sec - self->start.tv_sec) + (self->stop.tv_usec - self->start.tv_usec) * 1000000.0;
}

