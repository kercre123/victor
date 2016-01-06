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

#include "json/json.h"
#include "util/helpers/noncopyable.h"

#include <thread>
#include <atomic>

namespace Anki {
  
  // Forward declarations
  namespace Util {
  namespace Data {
    class DataPlatform;
  }
  }
  
namespace Cozmo {

class CozmoGame;

class CozmoAPI : private Util::noncopyable
{
public:
  // When the engine should run in a separate thread
  bool StartRun(Util::Data::DataPlatform* dataPlatform, const Json::Value& config);
  
  // When manual control over updating the engine is desired:
  bool Start(Util::Data::DataPlatform* dataPlatform, const Json::Value& config);
  bool Update(const double currentTime_sec);
  
  // Destroys any running thread and game instance
  void Clear();
  
  virtual ~CozmoAPI();
  
private:
  class CozmoInstanceRunner
  {
  public:
    CozmoInstanceRunner(Util::Data::DataPlatform* dataPlatform,
                        const Json::Value& config, bool& initResult);
    ~CozmoInstanceRunner();
    void Run();
    void Stop() { _isRunning.store(false); }
    
    // For manually ticking the game
    bool Update(const double currentTime_sec);
    
  private:
    CozmoGame* _cozmoInstance;
    std::atomic<bool> _isRunning;
    
  }; // class CozmoInstanceRunner
  
  // Our running instance, if we have one
  CozmoInstanceRunner* _cozmoRunner = nullptr;
  std::thread _cozmoRunnerThread;
}; // class CozmoAPI
  
} // namespace Cozmo
} // namespace Anki

#endif // __Anki_Cozmo_CozmoAPI_h__
