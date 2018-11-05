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
#ifndef __TaskExecutor_H__
#define	__TaskExecutor_H__

#include <chrono>
#include <thread>
#include <vector>
#include <mutex>
#include <condition_variable>

namespace Anki
{
namespace Das
{

typedef struct _TaskHolder {
  bool sync;
  std::function<void()> task;
  std::chrono::time_point<std::chrono::system_clock> when;

  bool operator < (const _TaskHolder& th) const
  {
    return (when > th.when);
  }
} TaskHolder;

class TaskExecutor {
public:
  TaskExecutor();
  ~TaskExecutor();
  void Wake(std::function<void()> task);
  void WakeSync(std::function<void()> task);
  void WakeAfter(std::function<void()> task, std::chrono::time_point<std::chrono::system_clock> when);
  void StopExecution();
protected:
  TaskExecutor(const TaskExecutor&) = delete;
  TaskExecutor& operator=(const TaskExecutor&) = delete;
  void Wait(std::unique_lock<std::mutex> &lock,
            std::condition_variable &condition,
            const std::vector<TaskHolder>* tasks) const;
private:
  void AddTaskHolder(TaskHolder taskHolder);
  void AddTaskHolderToDeferredQueue(TaskHolder taskHolder);
  void Execute();
  void ProcessDeferredQueue();
  void Run(std::unique_lock<std::mutex> &lock);

private:
  std::thread _taskExecuteThread;
  std::mutex _taskQueueMutex;
  std::condition_variable _taskQueueCondition;
  std::vector<TaskHolder> _taskQueue;
  std::thread _taskDeferredThread;
  std::mutex _taskDeferredQueueMutex;
  std::condition_variable _taskDeferredCondition;
  std::vector<TaskHolder> _deferredTaskQueue;
  std::mutex _addSyncTaskMutex;
  std::mutex _syncTaskCompleteMutex;
  std::condition_variable _syncTaskCondition;
  bool _syncTaskDone;
  bool _executing;
};

} // namespace Das
} // namespace Anki

#endif	/* __TaskExecutor_H__ */
