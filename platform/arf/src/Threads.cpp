#include "arf/Threads.h"
#include <sys/time.h>
#include <sys/resource.h>
#include <signal.h>

// General code
namespace ARF
{

TaskTracker::TaskTracker() {}

int TaskTracker::TotalNumTasks() const
{
    return _pendingTasks + _activeTasks + _doneTasks;
}

void TaskTracker::WaitUntilNumDone(int N)
{
    Lock lock(_mutex);

    int totalTasks = TotalNumTasks();
    if (N > totalTasks)
    {
        std::cerr << "Requested wait for " << N << " tasks, but tracker only has "
                  << totalTasks << " total tasks" << std::endl;
        return;
    }

    while (N > _doneTasks)
    {
        _doneCondition.wait(lock);
    }
}

void TaskTracker::WaitUntilNumActive(int N)
{
    Lock lock(_mutex);

    int totalTasks = TotalNumTasks();
    if (N > totalTasks)
    {
        std::cerr << "Requested wait for " << N << " tasks, but tracker only has "
                  << totalTasks << " total tasks" << std::endl;
        return;
    }

    while (N > _activeTasks + _doneTasks)
    {
        _doneCondition.wait(lock);
    }
}

void TaskTracker::WaitUntilAllDone()
{
    Lock lock(_mutex);

    while (_pendingTasks > 0 || _activeTasks > 0)
    {
        _doneCondition.wait(lock);
    }
}

void TaskTracker::WaitUntilAllActive()
{
    Lock lock(_mutex);

    while (_pendingTasks > 0)
    {
        _doneCondition.wait(lock);
    }
}

void TaskTracker::Cancel()
{
    Lock lock(_mutex);
    _isCancelled = true;
    _doneCondition.notify_all();
}

bool TaskTracker::IsCancelled() const
{
    Lock lock(_mutex);
    return _isCancelled;
}

int TaskTracker::NumPendingTasks() const
{
    Lock lock(_mutex);
    return _pendingTasks;
}

int TaskTracker::NumActiveTasks() const
{
    Lock lock(_mutex);
    return _activeTasks;
}

int TaskTracker::NumDoneTasks() const
{
    Lock lock(_mutex);
    return _doneTasks;
}

void TaskTracker::IndicateTaskReceived(TaskTrackingKey)
{
    Lock lock(_mutex);
    _pendingTasks++;
}

void TaskTracker::IndicateOneTaskStarted(TaskTrackingKey)
{
    Lock lock(_mutex);
    _pendingTasks--;
    _activeTasks++;
    _doneCondition.notify_all();
}

void TaskTracker::IndicateOneTaskCancelled(TaskTrackingKey)
{
    Lock lock(_mutex);
    _pendingTasks--;
    _doneTasks++;
    _doneCondition.notify_all();
}

void TaskTracker::IndicateOneTaskDone(TaskTrackingKey)
{
    Lock lock(_mutex);
    _doneTasks++;
    _activeTasks--;
    _doneCondition.notify_all();
}

Threadpool::TaskQueue::TaskQueue() {}

Threadpool::TaskQueue::~TaskQueue()
{
    Halt();
}

void Threadpool::TaskQueue::Start()
{
    Lock lock(_mutex);
    _isRunning = true;
    _isHalted = false;
    _hasUpdate.notify_all();
}

void Threadpool::TaskQueue::Pause()
{
    Lock lock(_mutex);
    _isRunning = false;
}

void Threadpool::TaskQueue::Halt()
{
    Lock lock(_mutex);
    _isHalted = true;
    _hasUpdate.notify_all();
    for (auto &t : _queue)
    {
        if (t.tracker)
        {
            t.tracker->IndicateOneTaskCancelled(TaskTrackingKey());
        }
    }
}

void Threadpool::TaskQueue::Join()
{
    Lock lock(_mutex);
    // Block until halted
    while (!_isHalted)
    {
        _hasUpdate.wait(lock);
    }
    return;
}

void Threadpool::TaskQueue::Reset()
{
    Lock lock(_mutex);
    for (auto &t : _queue)
    {
        if (t.tracker)
        {
            t.tracker->IndicateOneTaskCancelled(TaskTrackingKey());
        }
    }
    _queue.clear();
    _isRunning = false;
    _isHalted = false;
}

void Threadpool::TaskQueue::Enqueue(const TaskRegistration &t)
{
    Lock lock(_mutex);
    _queue.push_back(t);
    _hasUpdate.notify_one();
}

bool Threadpool::TaskQueue::WaitForTask(TaskRegistration &t)
{
    Lock lock(_mutex);

    // Wait until we are halted, or become unpaused and have uncancelled tasks
    while (true)
    {
        if (_isHalted)
        {
            return false;
        }

        if (_isRunning && !_queue.empty())
        {
            t = _queue.front();
            _queue.pop_front();
            return true;
        }

        _hasUpdate.wait(lock);
    }
}

size_t Threadpool::TaskQueue::Size() const
{
    Lock lock(_mutex);
    return _queue.size();
}

Threadpool *Threadpool::_instance = nullptr;

Threadpool &Threadpool::Inst()
{
    if (!_instance)
    {
        _instance = new Threadpool();
    }
    return *_instance;
}

void Threadpool::Destruct()
{
    if (_instance)
    {
        delete _instance;
    }
}

bool Threadpool::Initialize(unsigned int numStandardThreads,
                            unsigned int numPriorityThreads)
{
    Lock lock(_mutex);

    if (_isInitialized)
    {
        return false;
    }

    _standardQueue.Reset();
    _priorityQueue.Reset();

    // Check arguments
    int numProc = get_num_processors();
    if (static_cast<int>(numPriorityThreads) >= numProc)
    {
        std::cerr << "Have " << numProc << " processors but requested "
                  << numPriorityThreads << " priority threads!" << std::endl;
        return false;
    }

    // Generate set of standard CPUs
    cpu_set_t cpuSets;
    CPU_ZERO(&cpuSets);
    for (int i = numPriorityThreads; i < numProc; ++i)
    {
        CPU_SET(i, &cpuSets);
    }

    try
    {
        // Allocate standard threads
        for (unsigned int i = 0; i < numStandardThreads; ++i)
        {
            _threadPool.emplace_back(std::bind(&Threadpool::ThreadSpin, this, std::ref(_standardQueue)));
            Thread &thread = _threadPool.back();

            pthread_t pid = static_cast<pthread_t>(thread.native_handle());
            int ret = pthread_setaffinity_np(pid, sizeof(cpu_set_t), &cpuSets);
            if (ret != 0)
            {
                throw std::runtime_error("Could not set CPUS!");
            }
            std::cout << "Created thread " << pid << " in standard pool" << std::endl;
        }

        // TODO Boot kernel with -isocpu to isolate priority CPU
        // TODO Clean up this code a bit to minimize duplication
        // Generate priority CPUs and allocate threads
        for (unsigned int i = 0; i < numPriorityThreads; ++i)
        {
            CPU_ZERO(&cpuSets);
            CPU_SET(i, &cpuSets);
            _threadPool.emplace_back(std::bind(&Threadpool::ThreadSpin, this, std::ref(_priorityQueue)));
            Thread &thread = _threadPool.back();

            pthread_t pid = static_cast<pthread_t>(thread.native_handle());
            int ret = pthread_setaffinity_np(pid, sizeof(cpu_set_t), &cpuSets);
            if (ret != 0)
            {
                throw std::runtime_error("Could not set CPUS!");
            }
            ret = set_thread_priority_max(pid);
            if (ret != 0)
            {
                throw std::runtime_error("Could not set priority!");
            }
            std::cout << "Created thread " << pid << " in priority pool" << std::endl;
        }
    }
    catch (std::exception &e)
    {
        std::cerr << e.what() << std::endl;
        _threadPool.clear();
        return false;
    }

    _isInitialized = true;
    return true;
}

void Threadpool::HaltAndDeinitialize()
{
    Lock lock(_mutex);

    // Signal all queues to stop and wait for workers to return
    _standardQueue.Halt();
    _priorityQueue.Halt();
    for (auto &t : _threadPool)
    {
        if (t.joinable())
        {
            t.join();
        }
    }

    // Deallocate workers and clear remaining tasks
    _threadPool.clear();
    _isInitialized = false;
}

void Threadpool::Join()
{
    _standardQueue.Join();
    _priorityQueue.Join();
    Lock lock(_mutex);
    for (auto &t : _threadPool)
    {
        if (t.joinable())
        {
            t.join();
        }
    }
}

bool Threadpool::IsInitialized() const
{
    Lock lock(_mutex);
    return _isInitialized;
}

void Threadpool::StartAll()
{
    _standardQueue.Start();
    _priorityQueue.Start();
}

void Threadpool::Start(TaskPriority p)
{
    switch (p)
    {
    case TaskPriority::Standard:
        _standardQueue.Start();
        break;
    case TaskPriority::High:
        _priorityQueue.Start();
        break;
    default:
        std::cerr << "Unrecognized task priority: " << static_cast<int>(p) << std::endl;
    }
}

void Threadpool::Pause(TaskPriority p)
{
    switch (p)
    {
    case TaskPriority::Standard:
        _standardQueue.Pause();
        break;
    case TaskPriority::High:
        _priorityQueue.Pause();
        break;
    default:
        std::cerr << "Unrecognized task priority: " << static_cast<int>(p) << std::endl;
    }
}

void Threadpool::EnqueueTask(const Task &t, TaskPriority p, TaskTracker::Ptr tracker)
{
    if (tracker && tracker->IsCancelled())
    {
        // TODO Throw a warning about using cancelled tracker?
        return;
    }

    switch (p)
    {
    case TaskPriority::Standard:
        _standardQueue.Enqueue({t, tracker});
        break;
    case TaskPriority::High:
        _priorityQueue.Enqueue({t, tracker});
        break;
    default:
        std::cerr << "Unrecognized task priority: " << static_cast<int>(p) << std::endl;
    }

    if (tracker)
    {
        tracker->IndicateTaskReceived(TaskTrackingKey());
    }
}

size_t Threadpool::NumPendingTasks(TaskPriority p) const
{
    switch (p)
    {
    case TaskPriority::Standard:
        return _standardQueue.Size();
        break;
    case TaskPriority::High:
        return _priorityQueue.Size();
        break;
    default:
        std::cerr << "Unrecognized task priority: " << static_cast<int>(p) << std::endl;
        return 0;
    }
}

void Threadpool::ThreadSpin(TaskQueue &queue)
{
    TaskRegistration reg;
    while (queue.WaitForTask(reg))
    {
        // If there's an assigned TaskTracker, check status
        if (reg.tracker)
        {
            if (reg.tracker->IsCancelled())
            {
                reg.tracker->IndicateOneTaskCancelled(TaskTrackingKey());
            }
            else
            {
                reg.tracker->IndicateOneTaskStarted(TaskTrackingKey());
                reg.task();
                reg.tracker->IndicateOneTaskDone(TaskTrackingKey());
            }
        }
        // Otherwise just run
        else
        {
            reg.task();
        }
    }

    static Mutex printMutex;
    Lock lock(printMutex);
    std::cout << "thread: " << pthread_self() << " exiting" << std::endl;
}

} // namespace ARF
