#pragma once

#include "arf/Types.h"

#include <pthread.h>
#include <sched.h>
#include <unistd.h>

#include <iostream>
#include <thread>
#include <deque>
#include <vector>
#include <unordered_map>

// Classes and methods for managing threads
// TODO Appropriate wrappers to make this cross-platform

#ifdef __APPLE__
typedef struct cpu_set {
  uint32_t    count;
} cpu_set_t;

void CPU_ZERO( cpu_set* );
void CPU_SET( int, cpu_set* );
int pthread_setaffinity_np( pthread_t, size_t, cpu_set* );

#endif

namespace ARF
{

// Functions to be implemented per-platform
// Retrieve the number of cores available
int get_num_processors();

// Set a pthread_t to max scheduling priority
int set_thread_priority_max(pthread_t t);

typedef std::function<void()> Task;

enum class TaskPriority
{
    Standard = 0,
    High,
    Num
};

// Passkey to protect Threadpool-specific accesses to TaskTracker
class TaskTrackingKey 
{
    friend class Threadpool;
};

// Provides methods for sychronization between the Threadpool 
// and the Task requester
class TaskTracker
{
public:

    typedef std::shared_ptr<TaskTracker> Ptr;

    TaskTracker();

    // Blocks until N of the associated tasks are done
    void WaitUntilNumDone( int N );
    // Blocks until N of the associated tasks are started/done
    void WaitUntilNumActive( int N );
    // Blocks until all the associated tasks are done
    void WaitUntilAllDone();
    // Blocks until all the associated tasks are started/done
    void WaitUntilAllActive();
    // Indicates to the Threadpool to pass on any corresponding
    // unexecuted Tasks
    // Does not cancel Tasks already running!
    // Once cancelled, Tasks cannot be uncancelled
    void Cancel();
    // Check to see if the tasks are cancelled or not
    bool IsCancelled() const;

    int NumPendingTasks() const;
    int NumActiveTasks() const;
    int NumDoneTasks() const;

    // Methods for Threadpool
    // ======================
    // Indicates that an associated task was accepted
    void IndicateTaskReceived( TaskTrackingKey );
    // Indicates that a single task has started
    void IndicateOneTaskStarted( TaskTrackingKey );
    // Indicates that a single task has been cancelled
    void IndicateOneTaskCancelled( TaskTrackingKey );
    // Indicates that a single task is done
    void IndicateOneTaskDone( TaskTrackingKey );

private:

    mutable Mutex _mutex;
    ConditionVariable _doneCondition;
    bool _isCancelled = false;
    int _pendingTasks = 0;
    int _activeTasks = 0;
    int _doneTasks = 0;

    // TODO Add more useful data?
    int TotalNumTasks() const;
};


// A threadpool supporting multiple types of Tasks
// Uses pthread methods under the hood to manage scheduling
//
// Thread types
// ============
// Standard : Vanilla threads using the default scheduler
// Priority : Threads for high-priority realtime tasks pinned to their own cores
class Threadpool
{
public:

    // Retrieve the singleton instance
    // If not yet initialized, instantiates the pool
    static Threadpool& Inst();

    // Deallocates the singleton
    static void Destruct();

    // Initializes threads
    // Fails if numPriorityThreads >= num available processors,
    // as this would result in 0 processors for standard threads
    // Note that this does not clear the task queues, so tasks can be
    // added prior to initialization
    // Returns success
    bool Initialize( unsigned int numStandardThreads,
                     unsigned int numPriorityThreads );

    // Destroys all threads immediately and resets all task queues
    void HaltAndDeinitialize();

    // Checks to see if the threadpool is initialized or not
    bool IsInitialized() const;

    // Start all task queues running and signals all threads
    void StartAll();

    // Start a specified task queue and its threads
    void Start( TaskPriority p );

    // Block until all job queues and the threadpool terminate
    // This does not actually signal the pool to stop, so it should
    // be used in conjunction with a signal handler, ie. SIGINT callback
    void Join();

    // Blocks threads for the specified task queue
    // Does not clear the task queue, simply prevents threads from
    // fetching new tasks
    // This state can be reversed using Start() or StartAll()
    void Pause( TaskPriority p );

    // Add a task that will be scheduled using the specified pool and optional
    // TaskTracker
    // TODO: Throw error when pushing to a queue with no threads
    void EnqueueTask( const Task& t, 
                      TaskPriority p = TaskPriority::Standard, 
                      TaskTracker::Ptr tracker = nullptr );

    // Returns the number of tasks not yet being worked on in the specified queue
    size_t NumPendingTasks( TaskPriority p = TaskPriority::Standard ) const;

private:

    static Threadpool* _instance; // Singleton instance

    mutable Mutex _mutex;
    bool _isInitialized = false;

    typedef std::thread Thread;
    std::vector<Thread> _threadPool;
    
    struct TaskRegistration
    {
        Task task;
        TaskTracker::Ptr tracker = nullptr;
    };

    // A synchronized queue with wait/halt mechanics for threads
    class TaskQueue
    {
    public:

        // Create a new queue in the "not running" state
        TaskQueue();

        // Signals all waiting threads to exit
        ~TaskQueue();
        
        // Signals all waiting threads to begin fetching jobs
        void Start();

        // Blocks threads upon their next job fetch until Start() is called
        void Pause();

        // Blocks until the queue is halted
        void Join();

        // Signals all waiting threads to exit their fetch loops
        // Also clears the task queue
        void Halt();

        // Cancels all remaining tasks and then resets the internal state
        // After a call to Reset(), the queue can be restarted with a call to Start()
        void Reset();

        // Adds a task to the back of the queue
        void Enqueue( const TaskRegistration& t );

        // Blocks until the queue is halted, or is running and has tasks
        // Note that the queue starts as not running by default, so threads
        // can immediately be assigned to wait for tasks
        // Returns true if a task is received, or false if halted
        bool WaitForTask( TaskRegistration& t );

        // Returns the number of tasks in this queue
        size_t Size() const;
        
    private:

        mutable Mutex _mutex;
        bool _isRunning = false;
        bool _isHalted = false;
        std::deque<TaskRegistration> _queue;
        ConditionVariable _hasUpdate;
    };

    TaskQueue _standardQueue;
    TaskQueue _priorityQueue;

    // Constructor access restricted to enforce singleton
    Threadpool();

    void ThreadSpin( TaskQueue& queue );
};

}