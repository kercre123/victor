/*
 * File:          cozmoSimTestController.cpp
 * Date:
 * Description:   
 * Author:        
 * Modifications: 
 */

#include "anki/cozmo/simulator/game/cozmoSimTestController.h"
#include "anki/cozmo/shared/cozmoEngineConfig.h"

// Set to have StartMovie() and StopMovie() do things
// Note: Change _recordingPath to where ever you want the recordings to be saved
// Also may want/need to change starting webots world viewpoint so that you can see everything
// from a good pose
#define RECORD_TEST 0
const static std::string _recordingPath = "/Users/alchaussee/Desktop/webotsMovies/";

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
    
    bool CozmoSimTestController::IsTrueBeforeTimeout(bool cond,
                                                     std::string condAsString,
                                                     double start_time,
                                                     double timeout,
                                                     std::string file,
                                                     std::string func,
                                                     int line)
    {
      if (GetSupervisor()->getTime() - start_time > timeout) {
        if (!cond) {
          PRINT_STREAM_WARNING("CONDITION_WITH_TIMEOUT_ASSERT", "(" << condAsString << ") still false after " << timeout << " seconds. (" << file << "." << func << "." << line << ")");
          _result = 255;
          
          if(RECORD_TEST)
          {
            GetSupervisor()->stopMovie();
            sleep(10);
          }
          
          CST_EXIT();
        }
      }
      
      return cond;
    }
    
    void CozmoSimTestController::StartMovie(std::string name)
    {
      if(RECORD_TEST)
      {
        time_t t;
        time(&t);
        std::stringstream ss;
        ss << _recordingPath << name << " " << std::asctime(std::localtime(&t)) << ".mp4";
        GetSupervisor()->startMovie(ss.str(), 854, 480, 0, 90, 1, false);
      }
    }
    
    void CozmoSimTestController::StopMovie()
    {
      if(RECORD_TEST)
      {
        GetSupervisor()->stopMovie();
        sleep(10);
      }
    }
    
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
