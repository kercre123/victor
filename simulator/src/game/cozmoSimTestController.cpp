/*
 * File:          cozmoSimTestController.cpp
 * Date:
 * Description:   
 * Author:        
 * Modifications: 
 */

#include "anki/cozmo/simulator/game/cozmoSimTestController.h"
#include "anki/cozmo/shared/cozmoEngineConfig.h"


namespace Anki {
  namespace Cozmo {
    
    namespace {
      
    } // private namespace
  
    
    CozmoSimTestController::CozmoSimTestController()
    : UiGameController(BS_TIME_STEP)
    , _result(0)
    { }
    
    CozmoSimTestController::~CozmoSimTestController()
    { }
    
    
    
    // =========== CozmoSimTestFactory ============
    
    std::shared_ptr<CozmoSimTestController> CozmoSimTestFactory::Create(std::string name)
    {
      CozmoSimTestController * instance = nullptr;
      
      // find name in the registry and call factory method.
      auto it = factoryFunctionRegistry.find(name);
      if(it != factoryFunctionRegistry.end())
      instance = it->second();
      
      // wrap instance in a shared ptr and return
      if(instance != nullptr)
      return std::shared_ptr<CozmoSimTestController>(instance);
      else
      return nullptr;
    }
    
    void CozmoSimTestFactory::RegisterFactoryFunction(std::string name,
                                 std::function<CozmoSimTestController*(void)> classFactoryFunction)
    {
      // register the class factory function
      factoryFunctionRegistry[name] = classFactoryFunction;
    }
    
    
    
  } // namespace Cozmo
} // namespace Anki
