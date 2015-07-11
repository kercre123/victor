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
#if !ANKI_IOS_BUILD
#  ifndef SYSTEM_ROOT_PATH
#    error No SYSTEM_ROOT_PATH defined. You may need to re-run cmake.
#  endif
#else
#  include "anki/common/basestation/platformPathManager_iOS_interface.h"
#endif // #if !ANKI_IOS_BUILD

namespace Anki {

  
  
  PlatformPathManager::PlatformPathManager()
  {
#   if ANKI_IOS_BUILD
    
    const int MAX_PATH_LENGTH = 1024;
    char buffer[MAX_PATH_LENGTH];
    
    if(true == PlatformPathManager_iOS_GetPath(PlatformPathManager_iOS_Scope_Test, buffer, MAX_PATH_LENGTH)) {
      _scopePrefixes[Test] = buffer;
    }

    if(true == PlatformPathManager_iOS_GetPath(PlatformPathManager_iOS_Scope_Config, buffer, MAX_PATH_LENGTH)) {
      _scopePrefixes[Config] = buffer;
    }

    if(true == PlatformPathManager_iOS_GetPath(PlatformPathManager_iOS_Scope_Animation, buffer, MAX_PATH_LENGTH)) {
      _scopePrefixes[Animation] = buffer;
    }

    if(true == PlatformPathManager_iOS_GetPath(PlatformPathManager_iOS_Scope_Sound, buffer, MAX_PATH_LENGTH)) {
      _scopePrefixes[Sound] = buffer;
    }

    if(true == PlatformPathManager_iOS_GetPath(PlatformPathManager_iOS_Scope_Resource, buffer, MAX_PATH_LENGTH)) {
      _scopePrefixes[Resource] = buffer;
    }

#   elif defined(SYSTEM_ROOT_PATH)
    // TODO: figure out paths at runtime on iOS?
    //#if ON_IOS
    // systemRoot_ = <Get_iOS_BundleRoot()> ??
    //#else
    // Initialize based on environment variables, set by CMAKE
    _scopePrefixes[Test]      = std::string(QUOTE(SYSTEM_ROOT_PATH)) + "/";
    _scopePrefixes[Config]    = std::string(QUOTE(SYSTEM_ROOT_PATH)) + "/resources/basestation/config/";
    _scopePrefixes[Animation] = std::string(QUOTE(SYSTEM_ROOT_PATH)) + "/resources/basestation/animations/";
    _scopePrefixes[Sound]     = std::string(QUOTE(SYSTEM_ROOT_PATH)) + "/../products-cozmo-assets/sounds/";
    _scopePrefixes[Resource]  = std::string(QUOTE(SYSTEM_ROOT_PATH)) + "/";
    
    _scopePrefixes[FaceAnimation] = std::string(QUOTE(SYSTEM_ROOT_PATH)) + "/../products-cozmo-assets/faceAnimations/";
    
#   else
#   error PlatformPathManager scopes will not be set.
#   endif
  } // PlatformPathManager() Constructor
  
  std::string PlatformPathManager::PrependPath(const Scope scope,
                                              const std::string& name) const
  {
    auto scopeIter = _scopePrefixes.find(scope);
    if(scopeIter != _scopePrefixes.end()) {
    return _scopePrefixes.at(scope) /*+ this->GetLibraryPath()*/ + name;
    } else {
      static const std::string missing("MISSING_SCOPE_PATH/");
      return missing + name;
    }
  }
  
  void PlatformPathManager::OverridePath(const Scope scope, const std::string& newPath)
  {
    _scopePrefixes[scope] = newPath;
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