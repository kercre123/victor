//
//  iTaskHandle.h
//  DrivePlugin
//
//  Created by Bryan Austin on 11/12/14.
//  Copyright (c) 2014 Anki, Inc. All rights reserved.
//

#ifndef __DispatchQueue__iTaskHandle_H__
#define __DispatchQueue__iTaskHandle_H__

#include <memory>

namespace Anki {
namespace Util {

// TaskHandleContainer uses a virtual inner class to hide any reference to the actual system
// implementing the task scheduler
// To create a task handle, make a class deriving from ITaskHandle and create a
// TaskHandleContainer with it; the TaskHandleContainer will invalidate the handle on destruction
class TaskHandleContainer {
public:
  class ITaskHandle {
  public:
    virtual void Invalidate() = 0;
    virtual ~ITaskHandle() {}
  };

  TaskHandleContainer(ITaskHandle& handle) : _handle(&handle), _invalid(false) {}
  ~TaskHandleContainer() { Invalidate(); delete _handle; }

  void Invalidate(void) {
    if (_invalid) return;
    _invalid = true;
    _handle->Invalidate();
  }

  inline bool IsValid() const { return !_invalid; }

private:
  ITaskHandle* _handle;
  bool _invalid;
};

using TaskHandle = std::unique_ptr<TaskHandleContainer>;


} // namespace Util
} // namespace Anki

#endif // __DispatchQueue__iTaskHandle_H__
