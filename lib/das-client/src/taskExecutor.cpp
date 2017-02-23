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

#include "taskExecutor.h"

#include <condition_variable>

namespace Anki
{
namespace Das
{

TaskExecutor::TaskExecutor()
: _executing(true)
{
  _taskExecuteThread = std::thread(&TaskExecutor::Execute, this);
  _taskDeferredThread = std::thread(&TaskExecutor::ProcessDeferredQueue, this);
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

void TaskExecutor::Wake(std::function<void()> task)
{
  if (!_executing) {
    return;
  }
  WakeAfter(std::move(task), std::chrono::time_point<std::chrono::system_clock>::min());
}

void TaskExecutor::WakeSync(std::function<void()> task)
{
  if (!_executing) {
    return;
  }
  if (std::this_thread::get_id() == _taskExecuteThread.get_id()) {
    task();
    return;
  }
  std::lock_guard<std::mutex> lock(_addSyncTaskMutex);
  if (!_executing) {
    return;
  }

  TaskHolder taskHolder;
  taskHolder.sync = true;
  taskHolder.task = std::move(task);
  taskHolder.when = std::chrono::time_point<std::chrono::system_clock>::min();
  _syncTaskDone = false;

  AddTaskHolder(std::move(taskHolder));

  std::unique_lock<std::mutex> lk(_syncTaskCompleteMutex);
  _syncTaskCondition.wait(lk, [this]{return _syncTaskDone;});

}

void TaskExecutor::WakeAfter(std::function<void()> task, std::chrono::time_point<std::chrono::system_clock> when)
{
  if (!_executing) {
    return;
  }
  TaskHolder taskHolder;
  taskHolder.sync = false;
  taskHolder.task = std::move(task);
  taskHolder.when = when;

  auto now = std::chrono::system_clock::now();
  if (now >= when) {
    AddTaskHolder(std::move(taskHolder));
  } else {
    AddTaskHolderToDeferredQueue(std::move(taskHolder));
  }
}

void TaskExecutor::AddTaskHolder(TaskHolder taskHolder)
{
  std::lock_guard<std::mutex> lock(_taskQueueMutex);
  if (!_executing) {
    return;
  }
  _taskQueue.push_back(std::move(taskHolder));
  _taskQueueCondition.notify_one();
}

void TaskExecutor::AddTaskHolderToDeferredQueue(TaskHolder taskHolder)
{
  std::lock_guard<std::mutex> lock(_taskDeferredQueueMutex);
  if (!_executing) {
    return;
  }
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

void TaskExecutor::Execute()
{
  while (_executing) {
    std::unique_lock<std::mutex> lock(_taskQueueMutex);
    Wait(lock, _taskQueueCondition, &_taskQueue);
    Run(lock);
  }
}

void TaskExecutor::ProcessDeferredQueue()
{
  auto abs_time = std::chrono::time_point<std::chrono::system_clock>::max();
  size_t queueSize = 0;
  while (_executing) {
    std::unique_lock<std::mutex> lock(_taskDeferredQueueMutex);
    _taskDeferredCondition.wait_until(lock, abs_time, [this, queueSize] {
        return _deferredTaskQueue.size() > queueSize || !_executing;
      });

    bool endLoop = false;
    while (_executing && !_deferredTaskQueue.empty() && !endLoop) {
      auto now = std::chrono::system_clock::now();
      auto& taskHolder = _deferredTaskQueue.back();
      if (now >= taskHolder.when) {
        AddTaskHolder(std::move(taskHolder));
        _deferredTaskQueue.pop_back();
      } else {
        endLoop = true;
        abs_time = std::max(taskHolder.when, std::chrono::system_clock::now());
      }
    }
    if (_deferredTaskQueue.empty()) {
      abs_time = std::chrono::time_point<std::chrono::system_clock>::max();
    }
    queueSize = _deferredTaskQueue.size();
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
    taskHolder.task();
    if (taskHolder.sync) {
      std::lock_guard<std::mutex> lk(_syncTaskCompleteMutex);
      _syncTaskDone = true;
      _syncTaskCondition.notify_one();
    }
  }
}

} // namespace Das
} // namespace Anki
