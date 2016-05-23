/**
 * File: callbackProvider
 *
 * Author: damjan
 * Created: 10/29/13
 * 
 * Description: Provides callback functionality to any class
 * 
 *
 * Copyright: Anki, Inc. 2013
 *
 **/
#include "util/helpers/callbackProvider.h"

namespace Anki {
namespace Util {

void CallbackProvider::AddCallback(CallbackFunction function, void *userData)
{
  callbacks_.push_back(CallbackWrapper(function,userData));
}

void CallbackProvider::ExecuteCallbacks()
{
  // execute all callbacks
  for (int i = 0; i < callbacks_.size(); ++i)
    if (callbacks_[i].function_)
      callbacks_[i].function_(callbacks_[i].userData_);
  callbacks_.clear();
}

} // end namespace Util
} // end namespace Anki
