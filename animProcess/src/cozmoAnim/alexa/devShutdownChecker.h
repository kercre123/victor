/**
 * File: devShutdownChecker.h
 *
 * Author: ross
 * Created: Dec 5 2018
 *
 * Description: Holds weak_ptrs, and when requested, checks if the object is still valid
 *
 * Copyright: Anki, Inc. 2018
 *
 */

#ifndef ANIMPROCESS_COZMO_ALEXA_DEVSHUTDOWNCHECKER_H
#define ANIMPROCESS_COZMO_ALEXA_DEVSHUTDOWNCHECKER_H
#pragma once

#include "util/logging/logging.h"
#include <memory>
#include <string>
#include <functional>

namespace Anki {
namespace Vector {
  
class DevShutdownChecker
{
public:
  template <typename T>
  void AddObject( const std::string& name, const std::shared_ptr<T>& obj )
  {
    std::weak_ptr<T> weakObj = obj;
    ObjectInfo info;
    info.func = [wptr=std::move(weakObj)]() {
      return wptr.use_count();
    };
    info.name = name;
    _objectInfos.push_back( std::move(info) );
  }
  
  // Print a log line for any object that persists at the time this is called. Pass removeIfDestroyed
  // false if it should only be printed once, even if the object persists for a second call to PrintRemaining
  bool PrintRemaining( bool removeIfDestroyed = true )
  {
    bool anyRemaining = false;
    if( _objectInfos.empty() ) {
      return anyRemaining;
    }
    for( ssize_t i=_objectInfos.size()-1; i>=0; --i ) {
      const long numRefs = _objectInfos[i].func();
      if( numRefs == 0 ) {
        // good work developers! this object was destroyed
        if( removeIfDestroyed ) {
          std::swap( _objectInfos[i], _objectInfos.back() );
          _objectInfos.pop_back();
        }
      } else {
        anyRemaining = true;
        PRINT_NAMED_WARNING( "Alexa.DevShutdownChecker.Remaining",
                             "Object %s has %ld refs remaining",
                             _objectInfos[i].name.c_str(), numRefs );
      }
    }
    return anyRemaining;
  }
  
private:
  
  struct ObjectInfo {
    std::function<long(void)> func;
    std::string name;
  };
  
  std::vector<ObjectInfo> _objectInfos;
};


} // namespace Vector
} // namespace Anki

#endif // ANIMPROCESS_COZMO_ALEXA_DEVSHUTDOWNCHECKER_H
