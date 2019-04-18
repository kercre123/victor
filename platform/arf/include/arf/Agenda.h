#pragma once

#include <functional>
#include <atomic>
#include "arf/Types.h"

namespace ARF
{

// Object describing generic work item
// Contains methods for registering pre/post execution callbacks
// to add conditional execution logic
// Note that there is no mutex around the task members, so 
// initializaton/configuration should be done in a single thread
class AgendaTask
{
public:

    // Tasks take no arguments and return nothing
    using TaskFunctor = std::function<void()>;

    // A callback given to the task by the scheduler to request scheduling
    // Schedulers must guarantee that if this function returns, the task's
    // Execute method will eventually be called
    using ReadyCallback = std::function<void()>;

    AgendaTask() = default;
    virtual ~AgendaTask() = default;

    // Checks if both the task and readiness callback are set
    bool IsInitialized() const;

    // Specifies the body of this task
    void SetTask( const TaskFunctor& t );

    // Sets a callback for the task to execute when ready
    // Should only be used by schedulers to enable signaling
    void SetReadyCallback( const ReadyCallback& cb );

    // Entry point for thread
    virtual void Execute() = 0;

    // User software indicates to scheduler that this task is ready to be run
    void IndicateReadiness();

protected:

    // Execute the assigned task body
    void ExecuteBody() const;

private:

    TaskFunctor _task;
    ReadyCallback _readinessCallback;
};


// A task that can only be executed once
class OneShotTask : public AgendaTask
{
public:

    OneShotTask() = default;

    virtual void Execute() override;

    // Return whether the task has been run to completion N times
    bool IsDone() const;

    // Returns whether there are any active instances of this task
    bool IsActive() const;

private:

    enum Status
    {
        PENDING,
        ACTIVE,
        DONE
    };

    mutable Mutex _mutex;
    Status _status = PENDING;
};

// A specialized task that allows at most N threads to run at a time
// but repeated execution
class SerialTask : public AgendaTask
{
public:

    SerialTask() = default;

    virtual void Execute() override;

    bool IsActive() const;

private:

    mutable Mutex _mutex;
    bool _isActive = false;
};

// A task with no restrictions, allowing repeated execution
// with concurrency
class ParallelTask : public AgendaTask
{
public:

    ParallelTask() = default;

    virtual void Execute() override;

    int NumActive() const;

private:

    mutable Mutex _mutex;
    int _numActive = 0;
};

} // end namespace ARF