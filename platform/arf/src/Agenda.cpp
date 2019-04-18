#include "arf/Agenda.h"

namespace ARF
{

// Checks if both the task and readiness callback are set
bool AgendaTask::IsInitialized() const
{
    return _task && _readinessCallback;
}

// Specifies the body of this task
void AgendaTask::SetTask( const TaskFunctor& t )
{
    _task = t;
}

// Sets a callback for the task to execute when ready
// Should only be used by schedulers to enable signaling
void AgendaTask::SetReadyCallback( const ReadyCallback& cb )
{
    _readinessCallback = cb;
}

// User software indicates to scheduler that this task is ready to be run
void AgendaTask::IndicateReadiness()
{
    _readinessCallback();
}

// Execute the assigned task body
void AgendaTask::ExecuteBody() const
{
    _task();
}

void OneShotTask::Execute()
{
    Lock lock( _mutex );
    if( _status != Status::PENDING ) { return; }
    _status = Status::ACTIVE;
    lock.unlock();
    ExecuteBody();
    lock.lock();
    _status = Status::DONE;
};

// Return whether the task has been run to completion N times
bool OneShotTask::IsDone() const
{
    Lock lock( _mutex );
    return _status == Status::DONE;
}

// Returns whether there are any active instances of this task
bool OneShotTask::IsActive() const
{
    Lock lock( _mutex );
    return _status == Status::ACTIVE;
}


void SerialTask::Execute() 
{
    Lock lock( _mutex );
    if( _isActive ) { return; }
    _isActive = true;
    lock.unlock();
    ExecuteBody();
    lock.lock();
    _isActive = false;
}

bool SerialTask::IsActive() const
{
    Lock lock( _mutex );
    return _isActive;
}

void ParallelTask::Execute()
{
    Lock lock( _mutex );
    _numActive++;
    lock.unlock();
    ExecuteBody();
    lock.lock();
    _numActive--;
}

int ParallelTask::NumActive() const
{
    Lock lock( _mutex );
    return _numActive;
}

}; // end namespace ARF