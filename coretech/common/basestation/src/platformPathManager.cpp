/**
 * File: platformPathManager.cpp
 *
 * Author: Andrew Stein
 * Created: 2014-02-12
 *
 * Description: For providing / pre-pending platform-dependent system paths.
 *
 * Copyright: Anki, Inc. 2014
 *
 **/


#include "anki/common/constantsAndMacros.h"
#include "anki/common/basestation/platformPathManager.h"

// TODO: Only use these preprocessors on PC, not iOS
//#if ON_IOS
#ifndef SYSTEM_ROOT_PATH
#error No SYSTEM_ROOT_PATH defined. You may need to re-run cmake.
#endif
//#endif // #if ON_IOS

namespace Anki {

  
  PlatformPathManager::PlatformPathManager()
  {
    // TODO: figure out paths at runtime on iOS?
    //#if ON_IOS
    // systemRoot_ = <Get_iOS_BundleRoot()> ??
    //#else
    // Initialize based on environment variables, set by CMAKE
    scopePrefixes_[Test]     = std::string(QUOTE(SYSTEM_ROOT_PATH)) + "/";
    scopePrefixes_[Config]   = std::string(QUOTE(SYSTEM_ROOT_PATH)) + "/";
    scopePrefixes_[Resource] = std::string(QUOTE(SYSTEM_ROOT_PATH)) + "/";
    
    //#endif
  }
  
  std::string PlatformPathManager::PrependPath(const Scope scope,
                                              const std::string& name) const
  {
    return scopePrefixes_.at(scope) /*+ this->GetLibraryPath()*/ + name;
  }
  
  void PlatformPathManager::OverridePath(const Scope scope, const std::string& newPath)
  {
    scopePrefixes_[scope] = newPath;
  }
  
  /*
  int main(void)
  {
    CommonPathManager* cpm = CommonPathManager::getInstance();
    cpm->PrependPath(CommonPathManager::Test, "stuff/things/...");
    return 0;
  }
   */
  
} // namespace Anki