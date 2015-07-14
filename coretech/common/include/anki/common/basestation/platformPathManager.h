/**
 * File: platformPathManager.h
 *
 * Author: Andrew Stein
 * Created: 2014-02-12
 *
 * Description: For providing / pre-pending platform-dependent system paths.
 *
 * Copyright: Anki, Inc. 2014
 *
 **/

#ifndef ANKI_COMMON_PLATFORM_PATH_MANAGER_H_
#define ANKI_COMMON_PLATFORM_PATH_MANAGER_H_

#include <map>
#include <string>
//#include <array>

namespace Anki {
  
  // Macro for convenience
#define PREPEND_SCOPED_PATH(__SCOPE__, __NAME__) \
  ::Anki::PlatformPathManager::getInstance()->PrependPath(::Anki::PlatformPathManager::__SCOPE__, __NAME__)
  
  class PlatformPathManager
  {
  public:
    enum Scope {
      Test,
      Config,
      Animation,
      Sound,
      Resource,
      FaceAnimation
      //Temp
    };
    
    // Get a pointer to the singleton instance
    inline static PlatformPathManager* getInstance();
    static void removeInstance();

    std::string PrependPath(const Scope scope, const std::string& name) const;
    
    // e.g. for special unit tests or situations
    void OverridePath(const Scope scope, const std::string& newPath);
    
  protected:
    
    PlatformPathManager();
    
    static PlatformPathManager* _singletonInstance;
    //virtual const std::string& GetLibraryPath() const = 0;
    
  private:
    // TODO: Could also be a std::array<std::string,NumScopes>
    std::map<Scope, std::string> _scopePrefixes;
    
  }; // class PlatformPathManager
  
  
  inline PlatformPathManager* PlatformPathManager::getInstance()
  {
    // If we haven't already instantiated the singleton, do so now.
    if(nullptr == _singletonInstance) {
      _singletonInstance = new PlatformPathManager();
    }
    
    return _singletonInstance;
  }
  
 
  
} // namespace Anki

#endif // ANKI_COMMON_PLATFORM_PATH_MANAGER_H_

