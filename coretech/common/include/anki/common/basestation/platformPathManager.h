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
  
  // TODO: Move this to coretech/common/singleton.h/cpp
  template<class Derived>
  class SingletonBase
  {
  public:
    static Derived* GetInstance() {
      if(_singletonInstance == nullptr) {
        _singletonInstance = new Derived();
      }
      return _singletonInstance;
    }
    
    // TODO: Add singleton-appropriate destructor
    
  protected:
    SingletonBase() { }
    static Derived* _singletonInstance;
    
  }; // class SingletonBase<Derived>

  template<class Derived>
  Derived* SingletonBase<Derived>::_singletonInstance = nullptr;
  
  // Macro for convenience
#define PREPEND_SCOPED_PATH(__SCOPE__, __NAME__) \
  ::Anki::PlatformPathManager::GetInstance()->PrependPath(::Anki::PlatformPathManager::__SCOPE__, __NAME__)
  
  class PlatformPathManager : public SingletonBase<PlatformPathManager>
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
    
    std::string PrependPath(const Scope scope, const std::string& name) const;
    
    // e.g. for special unit tests or situations
    void OverridePath(const Scope scope, const std::string& newPath);
    
  protected:
    friend SingletonBase<PlatformPathManager>;
    
    PlatformPathManager();
    //virtual const std::string& GetLibraryPath() const = 0;
    
  private:
    // TODO: Could also be a std::array<std::string,NumScopes>
    std::map<Scope, std::string> _scopePrefixes;
    
  }; // class SystemPathManager
  
  /*
  class CommonPathManager : public PlatformPathManager, public SingletonBase<CommonPathManager>
  {
    virtual const std::string& GetLibraryPath() const {
      static const std::string LibraryPath("Common");
      return LibraryPath;
    }
  };
  
  class VisionPathManager : public PlatformPathManager, public SingletonBase<VisionPathManager>
  {
  protected:
    virtual const std::string& GetLibraryPath() const {
      static const std::string LibraryPath("Vision");
      return LibraryPath;
    }
    
    VisionPathManager() {
      OverridePath(Test, "visionTestPath");
    }
  };
  */
  
  /*
#define CREATE_LIBRARY_PATH_MANAGER(__NAME__, __LIB_PATH__, __VA_ARGS__) \
class __NAME__ : public PlatformPathManager, public SingletonBase<__NAME__> \
{ \
protected: \
  virtual const std::string& GetLibraryPath() const { \
    static const std::string LibraryPath(__LIB_PATH__); \
    return LibraryPath; \
  } \
  __NAME__() { \
    FOR_EACH(__VA_ARGS__); \
  } \
};

#define FOR_EACH
#define FOR_EACH_2(__SCOPE__, __PATH__) OverridePath(__SCOPE__, __PATH__);
#define FOR_EACH_4(__SCOPE__, __PATH__,...) OverridePath(__SCOPE__, __PATH__); FOR_EACH_2(__VA_ARGS__)
#define FOR_EACH_6(__SCOPE__, __PATH__,...) OverridePath(__SCOPE__, __PATH__); FOR_EACH_4(__VA_ARGS__)
   */
  
} // namespace Anki

#endif // ANKI_COMMON_PLATFORM_PATH_MANAGER_H_

