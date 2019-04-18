#include "arf/Timer.h"

namespace ARF
{

Timer::Timer()
{
    _thread = std::thread( [this](){ ThreadSpin(); } );
}

Timer::~Timer()
{
    Lock lock( _mutex );
    _status = Status::HALTED;
    _isRunning.notify_all();
    _thread.join();
}

void Timer::SetPeriod( double period )
{
    Lock lock( _mutex );
    _period = float_to_time<MonotonicTime>( period );
}

void Timer::RegisterCallback( const TimerCallback& cb )
{
    Lock lock( _mutex );
    _callbacks.push_back( cb );
}

void Timer::Start()
{
    Lock lock( _mutex );
    _event.idealTime = get_time<MonotonicClock>() + _period;
    _status = Status::RUNNING;
    _isRunning.notify_all();
}

void Timer::Stop()
{
    Lock lock( _mutex );
    _status = Status::IDLE;
    _isRunning.notify_all();
}

void Timer::ThreadSpin()
{
    Lock lock( _mutex );
    while( true )
    {
        if( _status == Status::HALTED ) { break; }
        else if( _status == Status::IDLE )
        {
            _isRunning.wait( lock );
        }
        else if( _status == Status::RUNNING )
        {
            MonotonicClock::time_point tp( _event.idealTime );
            if( _isRunning.wait_until( lock, tp ) == std::cv_status::timeout )
            {
                _event.callbackTime = get_time<MonotonicClock>();
                for( auto& cb : _callbacks )
                {
                    cb( _event );
                }

                _event.idealTime += _period;
            }
        }
    }
}

 } // end namespace ARF