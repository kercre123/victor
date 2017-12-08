/**
 * File: taskExecutor
 *
 * Author: seichert
 * Created: 07/15/14
 *
 * Description: Execute arbitrary tasks on
 * a background thread serially.
 *
 * Based on original work from Michael Sung on May 30, 2014, 10:10 AM
 *
 * Copyright: Anki, Inc. 2014
 *
 **/

#include "util/dispatchQueue/taskExecutor.h"
#include "util/global/globalDefinitions.h"
#include "util/logging/logging.h"
#include "util/threading/threadPriority.h"

#include <condition_variable>
#include <cstring>
#include <pthread.h>

namespace Anki
{
namespace Util
{

class TaskExecutorHandle : public TaskHandleContainer::ITaskHandle
{
public:
  TaskExecutorHandle(int taskId, TaskExecutor* taskExecutor);
  virtual ~TaskExecutorHandle() {}
  virtual void Invalidate() override;
  TaskHolder::HandlePulse GetPulse();
private:
  int _taskId;
  TaskExecutor* _executor;
  std::weak_ptr<void> _queuePulse;
  std::shared_ptr<void> _handleHeartbeat;
};

TaskExecutor::TaskExecutor(const char* name, ThreadPriority threadPriority)
: _heartbeat( (void*)0x12345678, [] (void*) {} )
, _executing(true)
, _queueName(name != nullptr ? name : "")
, _idCounter(0)
, _cachedDeferredSize(0)
{
  std::string baseThreadName = _queueName;
  if (baseThreadName.empty()) {
    baseThreadName = "taskQueue";
  }

  // pthread names restricted to 16 characters, including null terminator
  std::string executeThreadName = baseThreadName.substr(0, 15);
  std::string deferredThreadName = baseThreadName.substr(0, 11) + "_def";

  _taskExecuteThread = std::thread(&TaskExecutor::Execute, this, std::move(executeThreadName));
  _taskDeferredThread = std::thread(&TaskExecutor::ProcessDeferredQueue, this, std::move(deferredThreadName));
  
  if (threadPriority != ThreadPriority::Default)
  {
    SetThreadPriority(_taskExecuteThread, threadPriority);
    SetThreadPriority(_taskDeferredThread, threadPriority);
  }
}

TaskExecutor::~TaskExecutor()
{
  StopExecution();
}

void TaskExecutor::StopExecution()
{
  // Cause Execute and ProcessDeferredQueue to break out of their while loops
  _executing = false;

  // Clear the _taskQueue.  Use a scope so that the mutex is only locked
  // while clearing the queue and notifying the background thread.
  {
    std::lock_guard<std::mutex> lock(_taskQueueMutex);
    _taskQueue.clear();
    _taskQueueCondition.notify_one();
  }

  // Clear the _deferredTaskQueue and also use a scope.
  {
    std::lock_guard<std::mutex> lock(_taskDeferredQueueMutex);
    _deferredTaskQueue.clear();
    _taskDeferredCondition.notify_one();
  }

  // Join the background threads.  We created the threads in the constructor, so they
  // should be cleaned up in our destructor.
  try {
    if (_taskExecuteThread.joinable()) {
      _taskExecuteThread.join();
    }
    if (_taskDeferredThread.joinable()) {
      _taskDeferredThread.join();
    }
  } catch ( ... )
  {
    // Suppress exceptions
  }
}

void TaskExecutor::Wake(std::function<void()> task, const char* name)
{
  WakeAfter(std::move(task), std::chrono::time_point<std::chrono::steady_clock>::min(), name);
}

void TaskExecutor::WakeSync(std::function<void()> task, const char* name)
{
  if (std::this_thread::get_id() == _taskExecuteThread.get_id()) {
    task();
    return;
  }
  std::lock_guard<std::mutex> lock(_addSyncTaskMutex);

  TaskHolder taskHolder;
  taskHolder.sync = true;
  taskHolder.repeat = false;
  taskHolder.checkPulse = false;
  taskHolder.task = std::move(task);
  taskHolder.when = std::chrono::time_point<std::chrono::steady_clock>::min();
  taskHolder.name = name != nullptr ? name : "";
  taskHolder.id = _idCounter++;
  _syncTaskDone = false;

  AddTaskHolder(std::move(taskHolder));

  std::unique_lock<std::mutex> lk(_syncTaskCompleteMutex);
  _syncTaskCondition.wait(lk, [this]{return _syncTaskDone;});

}

void TaskExecutor::WakeAfter(std::function<void()> task, std::chrono::time_point<std::chrono::steady_clock> when, const char* name)
{
  TaskHolder taskHolder;
  taskHolder.sync = false;
  taskHolder.repeat = false;
  taskHolder.checkPulse = false;
  taskHolder.task = std::move(task);
  taskHolder.when = when;
  taskHolder.name = name != nullptr ? name : "";
  taskHolder.id = _idCounter++;

  auto now = std::chrono::steady_clock::now();
  if (now >= when) {
    AddTaskHolder(std::move(taskHolder));
  } else {
    AddTaskHolderToDeferredQueue(std::move(taskHolder));
  }
}

TaskHandle TaskExecutor::WakeAfterRepeat(std::function<void()> task, std::chrono::milliseconds period, const char* name)
{
  TaskHolder taskHolder;
  taskHolder.sync = false;
  taskHolder.repeat = true;
  taskHolder.task = std::move(task);
  taskHolder.period = period;
  taskHolder.name = name != nullptr ? name : "";
  taskHolder.id = _idCounter++;

  auto now = std::chrono::steady_clock::now();
  taskHolder.when = now + period;

  // set up pulse from task holder to the handle so that this task
  // won't execute if the handle is invalidated
  TaskExecutorHandle* handle = new TaskExecutorHandle(taskHolder.id, this);
  taskHolder.pulse = handle->GetPulse();
  taskHolder.checkPulse = true;

  AddTaskHolderToDeferredQueue(std::move(taskHolder));

  return TaskHandle(new TaskHandleContainer(*(handle)));
}

void TaskExecutor::AddTaskHolder(TaskHolder taskHolder)
{
  std::lock_guard<std::mutex> lock(_taskQueueMutex);
  _taskQueue.push_back(std::move(taskHolder));
  _taskQueueCondition.notify_one();
}

void TaskExecutor::AddTaskHolderToDeferredQueue(TaskHolder taskHolder)
{
  std::lock_guard<std::mutex> lock(_taskDeferredQueueMutex);
  _deferredTaskQueue.push_back(std::move(taskHolder));
  // Sort the tasks so that the next one due is at the back of the queue
  std::sort(_deferredTaskQueue.begin(), _deferredTaskQueue.end());
  _taskDeferredCondition.notify_one();
}



void TaskExecutor::Wait(std::unique_lock<std::mutex> &lock,
                        std::condition_variable &condition,
                        const std::vector<TaskHolder>* tasks) const
{
  condition.wait(lock, [this, tasks] {
      return tasks->size() > 0 || !_executing;
  });
}

void TaskExecutor::Execute(std::string threadName)
{
  Anki::Util::SetThreadName(pthread_self(), threadName.c_str());
  while (_executing) {
    std::unique_lock<std::mutex> lock(_taskQueueMutex);
    Wait(lock, _taskQueueCondition, &_taskQueue);
    Run(lock);
  }
}

void TaskExecutor::ProcessDeferredQueue(std::string threadName)
{
  Anki::Util::SetThreadName(pthread_self(), threadName.c_str());
  auto abs_time = std::chrono::time_point<std::chrono::steady_clock>::max();
  while (_executing) {
    std::unique_lock<std::mutex> lock(_taskDeferredQueueMutex);
    _taskDeferredCondition.wait_until(lock, abs_time, [this] {
        return (_deferredTaskQueue.size() > _cachedDeferredSize) || !_executing;
      });

    bool endLoop = false;
    while (_executing && !_deferredTaskQueue.empty() && !endLoop) {
      auto now = std::chrono::steady_clock::now();
      auto& taskHolderRef = _deferredTaskQueue.back();
      if (now >= taskHolderRef.when) {
        AddTaskHolder(taskHolderRef);
        auto taskHolder = std::move(taskHolderRef);
        _deferredTaskQueue.pop_back();
        if (taskHolder.repeat) {
          lock.unlock();
          taskHolder.when = now + taskHolder.period;
          AddTaskHolderToDeferredQueue(std::move(taskHolder));
          lock.lock();
        }
      } else {
        endLoop = true;
        abs_time = std::max(taskHolderRef.when, std::chrono::steady_clock::now());
      }
    }
    if (_deferredTaskQueue.empty()) {
      abs_time = std::chrono::time_point<std::chrono::steady_clock>::max();
    }
    _cachedDeferredSize = _deferredTaskQueue.size();
  }
}

void TaskExecutor::Run(std::unique_lock<std::mutex> &lock)
{
  // copy
  std::vector<TaskHolder> taskQueue = std::move(_taskQueue);
  _taskQueue.clear();
  // we only need the lock when we're reading from _taskQueue
  lock.unlock();
  for (auto const& taskHolder : taskQueue) {
  
    #if ANKI_DEVELOPER_CODE
    const bool printDebug = !taskHolder.name.empty() && !taskHolder.repeat;
    if (printDebug) {
      PRINT_NAMED_DEBUG("TaskExecutor.StartTask", "q:%s, task:%s", _queueName.c_str(), taskHolder.name.c_str());
    }
    #endif

    const bool taskExpired = (taskHolder.checkPulse && taskHolder.pulse.expired());

    // execute the task
    if (!taskExpired) {
      taskHolder.task();
    }

    #if ANKI_DEVELOPER_CODE
    if (printDebug) {
      PRINT_NAMED_DEBUG("TaskExecutor.EndTask", "q:%s, task:%s", _queueName.c_str(), taskHolder.name.c_str());
    }
    #endif
    if (taskHolder.sync) {
      std::lock_guard<std::mutex> lk(_syncTaskCompleteMutex);
      _syncTaskDone = true;
      _syncTaskCondition.notify_one();
    }
  }
}

void TaskExecutor::RemoveTaskFromDeferredQueue(int taskId)
{
  std::lock_guard<std::mutex> lock(_taskDeferredQueueMutex);
  for (auto it = _deferredTaskQueue.begin(); it != _deferredTaskQueue.end(); ++it) {
    if ((*it).id == taskId) {
      _deferredTaskQueue.erase(it);
      _cachedDeferredSize = _deferredTaskQueue.size();
      break;
    }
  }
}


TaskExecutorHandle::TaskExecutorHandle(int taskId, TaskExecutor* taskExecutor)
  : _taskId(taskId)
  , _executor(taskExecutor)
  , _queuePulse(taskExecutor->_heartbeat)
  , _handleHeartbeat((void*)0x12345678, [] (void*) {})
{
}

void TaskExecutorHandle::Invalidate()
{
  _handleHeartbeat = nullptr;
  if (_queuePulse.lock()) {
    _executor->RemoveTaskFromDeferredQueue(_taskId);
  }
}

TaskHolder::HandlePulse TaskExecutorHandle::GetPulse()
{
  return TaskHolder::HandlePulse(_handleHeartbeat);
}

} // namespace Das
} // namespace Anki
