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

#ifndef BASESTATION_UTILS_CALLBACKPROVIDER_H_
#define BASESTATION_UTILS_CALLBACKPROVIDER_H_

#if defined(LINUX) || defined(ANDROID)
#include <cstddef> // for 'NULL' in gcc
#endif

#include <vector>

namespace Anki {
namespace Util {

// callback definition
typedef void(*CallbackFunction)(void *);

/**
 * wraps callback function and callback data together for easy storage
 *
 * @author   damjan
 */
struct CallbackWrapper {
  CallbackWrapper(CallbackFunction function, void* userData) : function_(function), userData_(userData) {};
  CallbackWrapper() : function_(NULL), userData_(NULL) {};
  CallbackFunction function_;
  void* userData_;
};

/**
 * Provides callback functionality to any class
 * 
 * @author   damjan
 */
class CallbackProvider {
public:
  void AddCallback(CallbackFunction function, void* userData);
  void ExecuteCallbacks();
protected:
  std::vector<CallbackWrapper>callbacks_;
};

} // end namespace Util
} // end namespace Anki

#endif //BASESTATION_UTILS_CALLBACKPROVIDER_H_
