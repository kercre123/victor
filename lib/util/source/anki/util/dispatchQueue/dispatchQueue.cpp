//
//  dispatchQueue.cpp
//  DrivePlugin
//
//  Created by Brian Chapados on 7/29/14.
//  Copyright (c) 2014 Anki, Inc. All rights reserved.
//

#include "util/dispatchQueue/dispatchQueue.h"
#include "util/dispatchQueue/taskExecutor.h"

namespace Anki {
namespace Util {
namespace Dispatch {

class Queue {
public:
  Anki::Util::TaskExecutor taskExecutor;
  Queue(const char* name, ThreadPriority threadPriority=ThreadPriority::Default) : taskExecutor(name, threadPriority) {};
  virtual ~Queue() {};

  virtual void DispatchAsync(const Block &block, const char* name)
  {
    taskExecutor.Wake(block, name);
  }

  virtual void DispatchSync(const Block &block, const char* name)
  {
    taskExecutor.WakeSync(block, name);
  }

  virtual void DispatchAfter(std::chrono::milliseconds durationMSec, const Block &block, const char* name)
  {
    std::chrono::steady_clock::time_point now{std::chrono::steady_clock::now()};
    std::chrono::steady_clock::time_point when{now + durationMSec};

    taskExecutor.WakeAfter(block, when, name);
  }

  virtual Anki::Util::TaskHandle ScheduleCallback(std::chrono::milliseconds repeatPeriodMSec, const Block &block, const char* name)
  {
    return taskExecutor.WakeAfterRepeat(block, repeatPeriodMSec, name);
  }

  virtual void StopExecution()
  {
    taskExecutor.StopExecution();
  }
};

class ImmediateQueue : public Queue {
private:
  bool _running;
public:
  ImmediateQueue() : Queue(nullptr)
  ,  _running(true) {};
  ~ImmediateQueue() {};

  void DispatchAsync(const Block &block, const char* name) override
  {
    if (_running) block();
  }

  void DispatchSync(const Block &block, const char* name) override
  {
    if (_running) block();
  }

  void DispatchAfter(std::chrono::milliseconds durationMSec, const Block &block, const char* name) override
  {
    if (_running) block();
//    std::chrono::time_point<std::chrono::steady_clock> when = std::chrono::time_point<std::chrono::steady_clock>::min();
//    when += durationMSec;
//    taskExecutor.WakeAfter(block, when);
  }

  class InvalidTaskHandle : public Anki::Util::TaskHandleContainer::ITaskHandle {
    void Invalidate() override {}
    virtual ~InvalidTaskHandle() {}
  };

  Anki::Util::TaskHandle ScheduleCallback(std::chrono::milliseconds repeatPeriodMSec, const Block &block, const char* name) override
  {
    block(); // I guess the concept of repetition is kinda weird for the immediate queue...
    return Anki::Util::TaskHandle(new Anki::Util::TaskHandleContainer(*(new InvalidTaskHandle())));
  }

  void StopExecution() override
  {
    _running = false;
  };
};

Queue* Create(const char* name /* = nullptr*/, ThreadPriority threadPriority)
{
  return new Queue(name, threadPriority);
}

Queue* CreateImmediate()
{
  return new ImmediateQueue();
}

// Stop queue from execution
// BE CAREFUL this is no pause. there is no start. this is terminal queue state.
void Stop(Queue* queue)
{
  queue->StopExecution();
}

void Release(Queue*& queue)
{
  delete queue;
  queue = nullptr;
}

void Async(Queue* queue, const Block& block, const char* name)
{
  queue->DispatchAsync(block, name);
}

void Sync(Queue* queue, const Block& block, const char* name)
{
  queue->DispatchSync(block, name);
}

void After(Queue* queue, std::chrono::milliseconds delayMSec, const Block& block, const char* name)
{
  queue->DispatchAfter(delayMSec, block, name);
}

Anki::Util::TaskHandle ScheduleCallback(Queue* queue, std::chrono::milliseconds repeatPeriodMSec, const Block& block, const char* name)
{
  return queue->ScheduleCallback(repeatPeriodMSec, block, name);
}

} // namespace Dispatch
} // namespace DriveEngine
} // namespace Anki