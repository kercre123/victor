/*
 * File:          cozmoSimTestController.h
 * Date:
 * Description:   Any UI/Game to be run as a Webots controller should be derived from this class.
 * Author:
 * Modifications:
 */

#ifndef __COZMO_SIM_TEST_CONTROLLER__H__
#define __COZMO_SIM_TEST_CONTROLLER__H__


#include "anki/cozmo/simulator/game/uiGameController.h"

namespace Anki {
namespace Cozmo {

  
// Registration of test controller derived from CozmoSimTestController
#define REGISTER_COZMO_SIM_TEST_CLASS(CLASS) static CozmoSimTestRegistrar<CLASS> registrar(#CLASS);
  
  
////////// Macros for condition checking and exiting ////////
  
// For local testing, set to 1 so that Webots doesn't exit
#define DO_NOT_QUIT_WEBOTS 0
  
#if (DO_NOT_QUIT_WEBOTS == 1)
#define CST_EXIT()  QuitController(_result);
#else
#define CST_EXIT()  QuitWebots(_result);  
#endif
  
  
#define CST_EXPECT(x, errorStreamOutput) \
if (!(x)) { \
  PRINT_STREAM_WARNING("CST_EXPECT", "(" << #x << "): " << errorStreamOutput << "(" << __FILE__ << "." << __FUNCTION__ << "." << __LINE__ << ")"); \
  _result = -1; \
}
  
#define CST_ASSERT(x, errorStreamOutput) \
if (!(x)) { \
  PRINT_STREAM_WARNING("CST_ASSERT", "(" << #x << "): " << errorStreamOutput << "(" << __FILE__ << "." << __FUNCTION__ << "." << __LINE__ << ")"); \
  _result = -1; \
  CST_EXIT(); \
}
  

  
  
/////////////// CozmoSimTestController /////////////////
  
// Base class from which all cozmo simulation tests should be derived
class CozmoSimTestController : public UiGameController {

public:
  CozmoSimTestController();
  virtual ~CozmoSimTestController();
  
protected:
  
  virtual s32 UpdateInternal() = 0;
  
  int _result;
  
}; // class CozmoSimTestController

  
  
/////////////// CozmoSimTestFactory /////////////////
  
// Factory for creating and registering tests derived from CozmoSimTestController
class CozmoSimTestFactory
{
public:
  
  static CozmoSimTestFactory * getInstance()
  {
    static CozmoSimTestFactory factory;
    return &factory;
  }
  
  std::shared_ptr<CozmoSimTestController> Create(std::string name);
  
  void RegisterFactoryFunction(std::string name,
                               std::function<CozmoSimTestController*(void)> classFactoryFunction);
  
protected:
  std::map<std::string, std::function<CozmoSimTestController*(void)>> factoryFunctionRegistry;
};


template<class T>
class CozmoSimTestRegistrar {
public:
  CozmoSimTestRegistrar(std::string className)
  {
    // register the class factory function
    CozmoSimTestFactory::getInstance()->RegisterFactoryFunction(className,
                                                                [](void) -> CozmoSimTestController * { return new T();});
  }

};
  
  
} // namespace Cozmo
} // namespace Anki

#endif // __COZMO_SIM_TEST_CONTROLLER__H__


