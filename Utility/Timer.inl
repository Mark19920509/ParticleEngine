
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
    
	double nTicks = double(endTime.QuadPart - startTime.QuadPart);
    double time = 1000.0 *(nTicks / frequency.QuadPart);
    if (timerDescription != NULL)
        DEBUG_OUT(timerDescription << ":\t" << time << "\n");
	return float(time);
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
    
	double nTicks = double(endTime.QuadPart - startTime.QuadPart);
    double time = (nTicks / frequency.QuadPart);
	return float(time);

}


float Timer::GetTimer(size_t index) const
{
  // assert(m_StartTimerList.size() > 0);

  // Calculate frame duration 
    LARGE_INTEGER frequency, endTime;
    QueryPerformanceCounter(&endTime);
	QueryPerformanceFrequency(&frequency);
    
    LARGE_INTEGER startTime = m_StartTimerList[index];
    
	double nTicks = double(endTime.QuadPart - startTime.QuadPart);
    double time = (nTicks / frequency.QuadPart);
	return float(time);
}

