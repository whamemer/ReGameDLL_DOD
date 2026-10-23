/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
*
*	This product contains software technology licensed from Id
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc.
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/
#pragma once
#if !defined(PERF_COUNTER_H)
#define PERF_COUNTER_H

#if defined(_WIN32)
#include <windows.h>
#else
#include <sys/time.h>
#endif

// The time in seconds since the first call, from the high resolution counter of the system.
#pragma pack(push, 1)
class CPerformanceCounter
{
public:
	CPerformanceCounter();

	void InitializePerformanceCounter();
	double GetCurTime();

private:
	int m_iLowShift;
	double m_flPerfCounterFreq;
	double m_flCurrentTime;
	double m_flLastCurrentTime;
};
#pragma pack(pop)

inline CPerformanceCounter::CPerformanceCounter()
{
	m_iLowShift = 0;
	m_flPerfCounterFreq = 0.0;
	m_flCurrentTime = 0.0;
	m_flLastCurrentTime = 0.0;

	InitializePerformanceCounter();
}

#if defined(_WIN32)

inline void CPerformanceCounter::InitializePerformanceCounter()
{
	LARGE_INTEGER performanceFreq;

	QueryPerformanceFrequency( &performanceFreq );

	// the counter runs too fast for a 32 bit difference: shift it down until it is a sane number of ticks per second
	unsigned int lowpart = (unsigned int)performanceFreq.LowPart;
	unsigned int highpart = (unsigned int)performanceFreq.HighPart;

	m_iLowShift = 0;

	while ( highpart || ( lowpart > 2000000.0 ) )
	{
		m_iLowShift++;
		lowpart >>= 1;
		lowpart |= ( highpart & 1 ) << 31;
		highpart >>= 1;
	}

	m_flPerfCounterFreq = 1.0 / (double)lowpart;
}

inline double CPerformanceCounter::GetCurTime()
{
	static int sametimecount;
	static unsigned int oldtime;
	static int first = 1;
	LARGE_INTEGER performanceCount;
	unsigned int temp, t2;
	double time;

	QueryPerformanceCounter( &performanceCount );

	temp = ( (unsigned int)performanceCount.LowPart >> m_iLowShift ) |
		   ( (unsigned int)performanceCount.HighPart << ( 32 - m_iLowShift ) );

	if ( first )
	{
		oldtime = temp;
		first = 0;
	}
	else
	{
		// check for turnover or backward time
		if ( ( temp <= oldtime ) && ( ( oldtime - temp ) < 0x10000000 ) )
		{
			oldtime = temp;	 // so we can't go backward
		}
		else
		{
			t2 = temp - oldtime;

			time = (double)t2 * m_flPerfCounterFreq;
			oldtime = temp;

			m_flCurrentTime += time;

			if ( m_flCurrentTime == m_flLastCurrentTime )
			{
				sametimecount++;

				if ( sametimecount > 100000 )
				{
					m_flCurrentTime += 1.0;
					sametimecount = 0;
				}
			}
			else
			{
				sametimecount = 0;
			}

			m_flLastCurrentTime = m_flCurrentTime;
		}
	}

	return m_flCurrentTime;
}

#else

inline void CPerformanceCounter::InitializePerformanceCounter()
{
}

inline double CPerformanceCounter::GetCurTime()
{
	struct timeval tp;
	static int secbase;

	gettimeofday( &tp, NULL );

	if ( !secbase )
	{
		secbase = tp.tv_sec;
		return tp.tv_usec / 1000000.0;
	}

	return ( tp.tv_sec - secbase ) + tp.tv_usec / 1000000.0;
}

#endif

#endif // PERF_COUNTER_H
