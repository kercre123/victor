/**
 * File: cozmoAPI.h
 *
 * Author: Lee Crippen
 * Created: 08/19/15
 *
 * Description: Point of entry for anything needing to interact with Cozmo.
 *
 * Copyright: Anki, Inc. 2015
 *
 * COZMO_PUBLIC_HEADER
 **/

#ifndef __Anki_Cozmo_CozmoAPI_h__
#define __Anki_Cozmo_CozmoAPI_h__

#include "anki/cozmo/shared/cozmoTypes.h"
#include "anki/cozmo/game/cozmoGame.h"
#include "json/json.h"
#include "util/helpers/noncopyable.h"

#include <thread>

namespace Anki {
namespace Cozmo {
  
// Forward declarations
namespace Data {
  class DataPlatform;
}

class CozmoAPI : private Util::noncopyable
{
public:
  // When the engine should run in a separate thread
  Result StartRun(Data::DataPlatform* dataPlatform, const Json::Value& config);
  
  // When manual control over updating the engine is desired:
  Result Start(Data::DataPlatform* dataPlatform, const Json::Value& config);
  Result Update(const double currentTime_sec);
  
  // Destroys any running thread and game instance
  void Clear();
  
  ~CozmoAPI();
  
private:
  class CozmoInstanceRunner
  {
  public:
    CozmoInstanceRunner(Data::DataPlatform* dataPlatform, const Json::Value& config, Result& initResult);
    
    void Run();
    
    // For manually ticking the game
    Result Update(const double currentTime_sec);
    
  private:
    CozmoGame _cozmoInstance;
    
  }; // class CozmoInstanceRunner
  
  // Our running instance, if we have one
  CozmoInstanceRunner * _cozmoRunner = nullptr;
  std::thread _cozmoRunnerThread;
}; // class CozmoAPI
  
} // namespace Cozmo
} // namespace Anki

#endif // __Anki_Cozmo_CozmoAPI_h__