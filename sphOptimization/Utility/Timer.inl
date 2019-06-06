
#include "Utility.h"

void Timer::StartTimerProfile()
{
#if !(defined(RELEASE) || defined(DEBUG))
    LARGE_INTEGER startTime;
    QueryPerformanceCounter(&startTime);
    m_StartTimeProfileList.push_back(startTime);
#endif // PROFILE
}

float Timer::StopTimerProfile(char * timerDescription /* = NULL*/)
{
#if !(defined(RELEASE) || defined(DEBUG))
  // assert(m_StartTimeProfileList.size() > 0);

  // Calculate frame duration 
    LARGE_INTEGER frequency, endTime;
    QueryPerformanceCounter(&endTime);
	QueryPerformanceFrequency(&frequency);
    
    LARGE_INTEGER startTime = m_StartTimeProfileList.back();
    m_StartTimeProfileList.pop_back();
    
	float nTicks = float(endTime.QuadPart - startTime.QuadPart);
    float time = 1000.0f*(nTicks / float(frequency.QuadPart));
    if (timerDescription != NULL)
        DEBUG_OUT(timerDescription << ":\t" << time << "\n");
	return time;
#else // PROFILE
    UNUSED_PARAMETER(timerDescription);
    return 0.0f;
#endif // PROFILE

}

void Timer::StartTimer()
{
    LARGE_INTEGER startTime;
    QueryPerformanceCounter(&startTime);
    m_StartTimerList.push_back(startTime);
}

float Timer::StopTimer()
{
  // assert(m_StartTimerList.size() > 0);

  // Calculate frame duration 
    LARGE_INTEGER frequency, endTime;
    QueryPerformanceCounter(&endTime);
	QueryPerformanceFrequency(&frequency);
    
    LARGE_INTEGER startTime = m_StartTimerList.back();
    m_StartTimerList.pop_back();
    
	float nTicks = float(endTime.QuadPart - startTime.QuadPart);
    float time = 1000.0f*(nTicks / float(frequency.QuadPart));
	return time;

}

