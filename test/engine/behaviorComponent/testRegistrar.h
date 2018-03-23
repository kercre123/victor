/**
 * File: testRegistrar.h
 *
 * Author: ross (sort of pulled out of a brad file from OD)
 * Created: 2018 mar 22
 *
 * Description: Registers unit tests, so that you can test that tests were written
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Test_TestRegistrar_H__
#define __Test_TestRegistrar_H__

namespace Anki {
namespace Cozmo {

template <class T>
struct TestRegistrar
{
public:
  
  explicit TestRegistrar(const std::string& testID)
  {
    // insert if missing (do not punish duplicates)
    RegisteredTests().insert( testID );
  }

  static bool IsRegistered(const std::string& testID)
  {
    const bool ret = RegisteredTests().find( testID ) != RegisteredTests().end();
    return ret;
  }

private:

  // singleton table
  static std::set<std::string>& RegisteredTests()
  {
    static std::set<std::string> testIDs;
    return testIDs;
  }
};
  
#define MAKE_REGISTRAR(T) struct T : public TestRegistrar<T> { \
  T(const std::string& testID) : TestRegistrar(testID) {} \
};


} // namespace
} // namespace

#endif
