#include "Timer.h"

Timer *Timer::m_Timer = NULL;

void Timer::Initialize()
{
    m_Timer = new Timer();
}

void Timer::Release()
{
    delete m_Timer;
}


Timer* Timer::GetInstance()
{
    if (m_Timer  == NULL)
    {
        m_Timer = new Timer(); // leak
    }
    return m_Timer;
}