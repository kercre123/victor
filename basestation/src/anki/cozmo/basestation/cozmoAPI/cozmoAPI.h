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
#include "anki/common/types.h"
#include "util/helpers/noncopyable.h"
#include "json/json.h"

#include <thread>
#include <atomic>
#include <memory>

namespace Anki {
  
// Forward declarations
namespace Util {
namespace Data {
  class DataPlatform;
}
}
  
namespace Cozmo {

class CozmoEngine;
class GameMessagePort;

class CozmoAPI : private Util::noncopyable
{
public:
  // When the engine should run in a separate thread
  bool StartRun(Util::Data::DataPlatform* dataPlatform, const Json::Value& config);
  
  // When manual control over updating the engine is desired:
  bool Start(Util::Data::DataPlatform* dataPlatform, const Json::Value& config);
  bool Update(const BaseStationTime_t currentTime_nanosec);

  // Send messages to game, receive messages from game
  size_t SendMessages(uint8_t* buffer, size_t bufferSize);
  void ReceiveMessages(const uint8_t* buffer, size_t size);
  void ExecuteBackgroundTransfers();

  // Debug viz communication
  size_t SendVizMessages(uint8_t* buffer, size_t bufferSize);
  
  // Destroys any running thread and game instance
  void Clear();
  
  virtual ~CozmoAPI();
  
private:
  class CozmoInstanceRunner
  {
  public:
    CozmoInstanceRunner(Util::Data::DataPlatform* dataPlatform,
                        const Json::Value& config, bool& initResult);
    
    virtual ~CozmoInstanceRunner();
    void Run();
    void Stop() { _isRunning.store(false); }
    
    // For manually ticking the game
    bool Update(const BaseStationTime_t currentTime_nanosec);
    GameMessagePort* GetGameMessagePort() const { return _gameMessagePort.get(); }
    GameMessagePort* GetVizMessagePort() const { return _vizMessagePort.get(); }
    CozmoEngine* GetEngine() const { return _cozmoInstance.get(); }
    
  private:
    static GameMessagePort* CreateVizMessagePort();
    std::unique_ptr<GameMessagePort> _gameMessagePort;
    std::unique_ptr<GameMessagePort> _vizMessagePort;
    std::unique_ptr<CozmoEngine> _cozmoInstance;
    std::atomic<bool> _isRunning;

  }; // class CozmoInstanceRunner
  
  // Our running instance, if we have one
  std::unique_ptr<CozmoInstanceRunner> _cozmoRunner;
  std::thread _cozmoRunnerThread;
}; // class CozmoAPI
  
} // namespace Cozmo
} // namespace Anki

#endif // __Anki_Cozmo_CozmoAPI_h__
