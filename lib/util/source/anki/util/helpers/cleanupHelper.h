/**
 * File: cleanupHelper.h
 *
 * Author:  Andrew Stein
 * Created: 4/11/2016
 *
 * Description: 
 *
 *   Instantiate this class with a function you want called when it goes out of
 *   scope, to do cleanup for you, e.g. in case of early returns from a function.
 *
 * Copyright: Anki, Inc. 2016
 *
 **/


#ifndef __Util_Helpers_CleanupHelper_H__
#define __Util_Helpers_CleanupHelper_H__

namespace Anki {
namespace Util {

class CleanupHelper
{
  std::function<void()> _cleanupFcn;
public:
  CleanupHelper(std::function<void()>&& fcn) : _cleanupFcn(fcn) { }
  ~CleanupHelper() { _cleanupFcn(); }
};

} // namespace Util
} // namespace Anki

#endif // #ifndef __Util_Helpers_CleanupHelper_H__

