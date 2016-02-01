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
  
namespace SpeechRecognition {
  class KeyWordRecognizer;
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
  CozmoContext() { }
  CozmoContext(const Util::Data::DataPlatform& dataPlatform, IExternalInterface* externalInterface);
  virtual ~CozmoContext();
  
  IExternalInterface*                   GetExternalInterface() const { return _externalInterface; }
  Util::Data::DataPlatform*             GetDataPlatform() const { return _dataPlatform.get(); }
  Util::RandomGenerator*                GetRandom() const { return _random.get(); }
  Comms::AdvertisementService*          GetRobotAdvertisementService() const { return _robotAdvertisementService.get(); }
  RobotManager*                         GetRobotManager() const { return _robotMgr.get(); }
  RobotInterface::MessageHandler*       GetMessageHandler() const { return _robotMsgHandler.get(); }
  SpeechRecognition::KeyWordRecognizer* GetKeywordRecognizer() const { return _keywordRecognizer.get(); }
  Audio::AudioServer*                   GetAudioServer() const { return _audioServer.get(); }
  
private:
  // This is passed in and held onto, but not owned by the context (yet.
  // It really should be, and that refactoring will have to happen soon).
  IExternalInterface*                                     _externalInterface;
  
  // Context holds onto these things for everybody:
  std::unique_ptr<Util::Data::DataPlatform>               _dataPlatform;
  std::unique_ptr<Util::RandomGenerator>                  _random;
  std::unique_ptr<Comms::AdvertisementService>            _robotAdvertisementService;
  std::unique_ptr<RobotManager>                           _robotMgr;
  std::unique_ptr<RobotInterface::MessageHandler>         _robotMsgHandler;
  std::unique_ptr<SpeechRecognition::KeyWordRecognizer>   _keywordRecognizer;
  std::unique_ptr<Audio::AudioServer>                     _audioServer;
};
  

} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_CozmoContext_H__
