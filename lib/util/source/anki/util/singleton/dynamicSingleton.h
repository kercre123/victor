/**
 * File: Singleton.h
 *
 * Author: raul
 * Created: 05/02/14
 *
 * Description: Base class for Singletons.
 *
 * Copyright: Anki, Inc. 2014
 *
 **/

#ifndef UTIL_DYNAMIC_SINGLETON_H_
#define UTIL_DYNAMIC_SINGLETON_H_

namespace Anki{ namespace Util {

template <class ClassType>
class DynamicSingleton
{
public:
  virtual ~DynamicSingleton() {}

  // dynamically allocate the singleton instance once
  static inline ClassType* getInstance()
  {
    if ( nullptr == _instance )
    {
      _instance = new ClassType();
    }
    
    return _instance;
  }

  static bool hasInstance()
  {
    return _instance != nullptr;
  }
  
  // destroy the instance if it exists
  static inline void removeInstance()
  {
    delete _instance;  // c++ does not require checking that !=0 (delete 0 is ok)
    _instance = nullptr;
  }
  
  static inline bool exists() {
    return (_instance != nullptr);
  }

protected:

  // default constructor
  DynamicSingleton() = default;

private:

  // instance
  static ClassType* _instance;

  // non-copyable
  DynamicSingleton(const DynamicSingleton&) = delete;
  const DynamicSingleton& operator=(const DynamicSingleton&) = delete;
};

template <class ClassType>
ClassType* DynamicSingleton<ClassType>::_instance = nullptr;

#define ANKIUTIL_FRIEND_SINGLETON(CT) friend class Anki::Util::DynamicSingleton<CT>


} // namespace
} // namespace

#endif
