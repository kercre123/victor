//
//  dispatchQueue.h
//  DrivePlugin
//
//  Created by Brian Chapados on 7/29/14.
//  Copyright (c) 2014 Anki, Inc. All rights reserved.
//

#ifndef __DispatchQueue__DispatchQueue_H__
#define __DispatchQueue__DispatchQueue_H__

#include "util/dispatchQueue/iTaskHandle.h"
#include "util/threading/threadPriority.h"
#include <functional>
#include <chrono>

namespace Anki {
namespace Util {
namespace Dispatch {

class Queue;
using Block = std::function<void()>;

// Create a queue that executes block / tasks on a serial queue
Queue* Create(const char* name = nullptr, ThreadPriority threadPriority = ThreadPriority::Default);

// Create a queue that executes blocks immediately
Queue* CreateImmediate();

// Stop queue from execution
// BE CAREFUL this is no pause. there is no start. this is terminal queue state.
void Stop(Queue* queue);

// Destroy a queue
void Release(Queue *&queue);

// Push a block onto the end of a queue and return.
void Async(Queue *queue, const Block &block, const char* name = nullptr);

// Push a block onto the end of a queue and block until it finishes executing.
void Sync(Queue *queue, const Block &block, const char* name = nullptr);

// Push a block on the end of a queue to be executed after the specified delay.
void After(Queue *queue, std::chrono::milliseconds delayMSec, const Block &block, const char* name = nullptr);

// Push a block onto the queue that will execute repeatedly with the specified period
// Returns a smart handle that will remove the callback from the queue on destruction;
// if you do not save this handle, your callback will be immediately removed and never run
Anki::Util::TaskHandle ScheduleCallback(Queue *queue, std::chrono::milliseconds repeatPeriodMSec, const Block &block, const char* name = nullptr);

} // namespace Dispatch
} // namespace Util
} // namespace Anki


#endif /* defined(__DispatchQueue__DispatchQueue_H__) */
