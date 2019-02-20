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

#include "util/dispatchQueue/iTaskHandle.h"
#include "util/threading/threadPriority.h"
#include <atomic>
#include <chrono>
#include <memory>
#include <string>
#include <thread>
#include <vector>

namespace Anki
{
namespace Util
{

typedef struct _TaskHolder {

  using HandlePulse = std::weak_ptr<void>;

  bool sync;
  bool repeat;
  std::function<void()> task;
  bool checkPulse;
  HandlePulse pulse;
  std::chrono::time_point<std::chrono::steady_clock> when;
  std::chrono::milliseconds period;
  std::string name;
  int id;


  bool operator < (const _TaskHolder& th) const
  {
    return (when > th.when);
  }
} TaskHolder;

class TaskExecutor {
public:
  explicit TaskExecutor(const char* name = nullptr, ThreadPriority threadPriority=ThreadPriority::Default);
  virtual ~TaskExecutor();
  void StopExecution();
  void Wake(std::function<void()> task, const char* name);
  void WakeSync(std::function<void()> task, const char* name);
  void WakeAfter(std::function<void()> task, std::chrono::time_point<std::chrono::steady_clock> when, const char* name);
  TaskHandle WakeAfterRepeat(std::function<void()> task, std::chrono::milliseconds period, const char* name);
protected:
  TaskExecutor(const TaskExecutor&) = delete;
  TaskExecutor& operator=(const TaskExecutor&) = delete;
  virtual void Wait(std::unique_lock<std::mutex> &lock,
                    std::condition_variable &condition,
                    const std::vector<TaskHolder>* tasks) const;
private:
  void AddTaskHolder(TaskHolder taskHolder);
  void AddTaskHolderToDeferredQueue(TaskHolder taskHolder);
  void RemoveTaskFromDeferredQueue(int taskId);
  void Execute(std::string threadName);
  void ProcessDeferredQueue(std::string threadName);
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
  std::shared_ptr<void> _heartbeat;
  bool _syncTaskDone;
  std::atomic<bool> _executing;
  const std::string _queueName;
  std::atomic_int _idCounter;
  size_t _cachedDeferredSize;

  friend class TaskExecutorHandle;
};

} // namespace Das
} // namespace Anki

#endif	/* __TaskExecutor_H__ */

