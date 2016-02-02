/**
 * File: cozmoContext.h
 *
 * Author: Lee Crippen
 * Created: 1/29/2016
 *
 * Description:
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#ifndef __Cozmo_Basestation_CozmoContext_H__
#define __Cozmo_Basestation_CozmoContext_H__

#include "util/helpers/noncopyable.h"
#include <memory>


// ---------- BEGIN FORWARD DECLARATIONS ----------
namespace Anki {
namespace Util {
  class RandomGenerator;
  
  namespace Data {
    class DataPlatform;
  }
}
}

namespace Anki {
namespace Comms {
  class AdvertisementService;
}
}

namespace Anki {
namespace Cozmo {
  
class IExternalInterface;
class RobotManager;
class CozmoEngine;
  
namespace RobotInterface {
  class MessageHandler;
}
  
namespace Audio {
  class AudioServer;
}
  
} // namespace Cozmo
} // namespace Anki

// ---------- END FORWARD DECLARATIONS ----------



// Here begins the actual namespace and interface for CozmoContext
namespace Anki {
namespace Cozmo {
  
class CozmoContext : private Util::noncopyable
{
public:
  CozmoContext(Util::Data::DataPlatform* dataPlatform, IExternalInterface* externalInterface);
  CozmoContext();
  
  virtual ~CozmoContext();
  
  IExternalInterface*                   GetExternalInterface() const { return _externalInterface; }
  Util::Data::DataPlatform*             GetDataPlatform() const { return _dataPlatform; }
  
  Util::RandomGenerator*                GetRandom() const { return _random.get(); }
  Comms::AdvertisementService*          GetRobotAdvertisementService() const { return _robotAdvertisementService.get(); }
  RobotManager*                         GetRobotManager() const { return _robotMgr.get(); }
  RobotInterface::MessageHandler*       GetRobotMsgHandler() const { return _robotMsgHandler.get(); }
  Audio::AudioServer*                   GetAudioServer() const { return _audioServer.get(); }
  
private:
  // This is passed in and held onto, but not owned by the context (yet.
  // It really should be, and that refactoring will have to happen soon).
  IExternalInterface*                                     _externalInterface = nullptr;
  Util::Data::DataPlatform*                               _dataPlatform = nullptr;
  
  // Context holds onto these things for everybody:
  std::shared_ptr<Audio::AudioServer>                     _audioServer;
  std::shared_ptr<Util::RandomGenerator>                  _random;
  std::shared_ptr<Comms::AdvertisementService>            _robotAdvertisementService;
  std::shared_ptr<RobotManager>                           _robotMgr;
  std::shared_ptr<RobotInterface::MessageHandler>         _robotMsgHandler;
};
  

} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_CozmoContext_H__
