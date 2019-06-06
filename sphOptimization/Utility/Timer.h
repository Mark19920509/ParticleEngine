#ifndef TIMER
#define TIMER

#include <windows.h>
#include <vector>

class Timer
{

public:
    inline void StartTimerProfile();
    inline float StopTimerProfile(char * timerDescription = NULL);
    inline void StartTimer();
    inline float StopTimer();
    void Initialize();
    void Release();
    static Timer* GetInstance();


private:
    static Timer *m_Timer;
    std::vector<LARGE_INTEGER>  m_StartTimeProfileList;
    std::vector<LARGE_INTEGER>  m_StartTimerList;

};

#include "Timer.inl"

#endif // TIMER