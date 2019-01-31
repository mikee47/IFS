/****
 * Simple class for elapse timing
 ****/

#ifndef __ELAPSETIMER_H
#define __ELAPSETIMER_H

#include <sys/time.h>

static inline uint32_t NOW()
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return (tv.tv_sec * 1000000U) + tv.tv_usec;
}

/*
 * Microsecond elapse timer.
 */
class ElapseTimer
{
public:
	ElapseTimer()
	{
		start();
	}

	void start()
	{
		startTicks = NOW();
	}

	uint32_t elapsed()
	{
		return NOW() - startTicks;
	}

private:
	uint32_t startTicks;
};

#endif // __ELAPSETIMER_H
