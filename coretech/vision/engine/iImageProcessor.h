/*
 * File: iImageProcessor.h
 *
 * Author: Al Chaussee
 * Created: 4/15/19
 *
 * Description: Generic threaded Processor class with Input and Outputs linked via template Enum values
 *
 * Copyright: Anki, Inc. 2019
 */

#ifndef __Anki_Vision_iImageProcessor_H__
#define __Anki_Vision_iImageProcessor_H__

#include "coretech/vision/engine/iImageProcessor_fwd.h"

#include "util/threading/threadPriority.h"

#include <unistd.h>

namespace Anki {
namespace Vision {

template<typename ModeClass, ModeClass mode, class InputClass>
Processor<ModeClass, mode, InputClass>::Processor(const std::string& name)
: _processorRunning(false)
, _processingDone(false)
, _threadStarted(false)
, _name(name)
{

}

template<typename ModeClass, ModeClass mode, class InputClass>
Processor<ModeClass, mode, InputClass>::~Processor()
{
  Stop();
}

template<typename ModeClass, ModeClass mode, class InputClass>
void Processor<ModeClass, mode, InputClass>::Start()
{
  if(!_processorRunning)
  {
    _processorRunning = true;

    // If not running synchronously then start up processing thread
    if(!_isSynchronous)
    {
      _processingThread = std::thread(&Processor<ModeClass, mode, InputClass>::ProcessorLoop, this);
    }
  }
}

template<typename ModeClass, ModeClass mode, class InputClass>
void Processor<ModeClass, mode, InputClass>::Stop()
{
  if(_processorRunning)
  {
    _processorRunning = false;

    // Notify _inputReadyCondition to wake processing thread so it can stop
    _inputReadyCondition.notify_all();

    // Wake from async input if implemented
    AsyncWake();

    if(_processingThread.joinable())
    {
      _processingThread.join();
    }

    _inputReady = false;
    _threadStarted = false;
  }
}

template<typename ModeClass, ModeClass mode, class InputClass>
void Processor<ModeClass, mode, InputClass>::SetIsSynchronous(bool isSynchronous)
{
  if(_isSynchronous == isSynchronous)
  {
    return;
  }

  const bool wasRunning = _processorRunning;

  Stop();

  _isSynchronous = isSynchronous;

  if(wasRunning)
  {
    Start();
  }
}

template<typename ModeClass, ModeClass mode, class InputClass>
bool Processor<ModeClass, mode, InputClass>::TryGetInput()
{
  const bool gotInput = AsyncAcquireInput();

  // If we got async input then processing is not done
  if(gotInput)
  {
    _processingDone = false;
  }

  return gotInput;
}

template<typename ModeClass, ModeClass mode, class InputClass>
void Processor<ModeClass, mode, InputClass>::ProcessorLoop()
{
  char buf[16] = {0};
  // Only 7 characters for name, other 8 characters of buf
  // reserved for "Proc_<name>_00" plus 1 for null terminator
  std::string name = _name.substr(0, 7);
  sprintf(buf, "Proc_%s_%d", name.c_str(), mode);
  buf[15] = 0;
  Anki::Util::SetThreadName(pthread_self(), buf);

  do
  {
    // Empty lock, will swap in an actual mutex if needed
    std::unique_lock<std::mutex> lock;

    // If the processor has async input then no need to lock
    // _inputReadyMutex, we will get the input on this thread
    const bool usesAsyncInput = HasAsyncInput();
    if(!usesAsyncInput)
    {
      std::unique_lock<std::mutex> swapLock(_inputReadyMutex);
      lock.swap(swapLock);
    }

    // Try to get optional async input if implemented
    TryGetInput();

    if(_input != nullptr)
    {
      _inputReady = false;

      // Process input
      std::shared_ptr<OutputBase> output = Process((InputClass*)_input.get());
      _output = output;
      _processingDone = true;
    }

    // Only block waiting for input if the processor does not have async input
    // and it is not running in synchronous mode
    if(!usesAsyncInput && !_isSynchronous)
    {
      // Thread is considered started once the processor is waiting for input
      _threadStarted = true;

      // Wait for input to be ready or the processor to be stopped
      _inputReadyCondition.wait(lock, [this]{
                                        return (_inputReady || !_processorRunning);
                                      });
    }
    // Loop as long as the processor is running and not in synchronous mode
  } while(_processorRunning && !_isSynchronous);
}

template<typename ModeClass, ModeClass mode, class InputClass>
ProcessorBase::UpdateReturn Processor<ModeClass, mode, InputClass>::Update(std::shared_ptr<InputBase> inputBase)
{
  UpdateReturn ret;

  // Down cast InputBase to specific InputClass as long as it is
  // the input for this processor (linked via templated ModeClass and mode)
  std::shared_ptr<InputClass> input = nullptr;
  if(inputBase != nullptr && inputBase->GetMode() == GetMode())
  {
    input = std::static_pointer_cast<InputClass>(inputBase);
  }

  // If inputBase is null then we still want to return output
  // If inputBase is the correct input for the processor then we should
  // start to process it
  if(inputBase == nullptr || inputBase->GetMode() == GetMode())
  {
    // Attempt to lock input mutex, will only be lockable while
    // processor is not actively processing the last input
    // This means input can potentially be dropped if a processor is updated
    // faster than it can process
    if(_inputReadyMutex.try_lock())
    {
      // If we had previous input and processing is done
      // then set output
      if(_input != nullptr && _processingDone)
      {
        ret.output = _output;
        _input.reset();
        _inputReady = false;
        // _processingDone = true;
      }

      // Get ready to notify thread with new input as long as
      // inputBase was able to be casted to InputClass as input is not null
      // or this is a processor with async input that is running in synchronous mode
      // in which case the notify will do nothing and ProcessorLoop will be called which
      // should get proper input
      const bool isSynchronousWithAsyncInput = (_isSynchronous && HasAsyncInput());
      if(input != nullptr || isSynchronousWithAsyncInput)
      {
        // Set input
        _input = input;
        _processingDone = false;
        _inputReady = true;

        // Unlock mutex and notify thread of new input
        _inputReadyMutex.unlock();
        _inputReadyCondition.notify_all();

        ret.notifiedOfInput = true;

        // If the processor is running in synchronous mode then
        // process input immediately on caller thread
        if(_isSynchronous)
        {
          ProcessorLoop();
        }
      }
      else
      {
        _inputReadyMutex.unlock();
      }
    }
    // Unable to lock input mutex, processor must still
    // be processing previous input. New input will be dropped
    else
    {
      ret.stillProcessingPrevInput = true;
    }
  }

  return ret;
}

template<typename ModeClass, ModeClass mode, class InputClass>
void Processor<ModeClass, mode, InputClass>::WaitUntilDone()
{
  // Wait until processing is done
  while(!_processingDone)
  {
    usleep(10000);
  }

  // Wait until we can lock input mutex which means processing thread is
  // not active
  while(!_inputReadyMutex.try_lock())
  {
    usleep(10000);
  }
  _inputReadyMutex.unlock();
}

template<typename ModeClass, ModeClass mode, class InputClass>
void Processor<ModeClass, mode, InputClass>::WaitForStart()
{
  // Wait until thread is started and waiting for input
  while(!_threadStarted)
  {
    usleep(10000);
  }
}

}
}

#endif
