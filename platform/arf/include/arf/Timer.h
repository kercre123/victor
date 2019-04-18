#pragma once

#include "arf/Types.h"
#include <thread>

namespace ARF
{

struct TimerEvent
{
    MonotonicTime callbackTime;
    MonotonicTime idealTime;
};

// A thread-based timer that calls an interrupt at a regular interval
class Timer
{
public:

    using TimerCallback = std::function<void(const TimerEvent&)>;

    Timer();
    ~Timer();

    void SetPeriod( double period );
    void RegisterCallback( const TimerCallback& cb );

    void Start();
    void Stop();

private:

    enum class Status
    {
        IDLE = 0,
        RUNNING,
        HALTED
    };

    MonotonicTime _period;
    TimerEvent _event;
    std::vector<TimerCallback> _callbacks;

    std::thread _thread;
    Mutex _mutex;
    Status _status = Status::IDLE;
    ConditionVariable _isRunning;

    void ThreadSpin();
};

} // end namespace ARF