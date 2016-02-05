#include "anki/cozmo/basestation/cozmoContext.h"
#include "anki/cozmo/basestation/robotManager.h"
#include "anki/cozmo/basestation/robotInterface/messageHandler.h"
#include "anki/cozmo/basestation/audio/audioController.h"
#include "anki/cozmo/basestation/audio/audioServer.h"
#include "anki/cozmo/basestation/viz/vizManager.h"
//#include "anki/cozmo/game/comms/uiMessageHandler.h"
#include "anki/common/basestation/utils/data/dataPlatform.h"
#include "anki/messaging/basestation/advertisementService.h"
#include "util/random/randomGenerator.h"

namespace Anki {
namespace Cozmo {
  
CozmoContext::CozmoContext(Util::Data::DataPlatform* dataPlatform, IExternalInterface* externalInterface)
  : _externalInterface(externalInterface)
  , _dataPlatform(dataPlatform)
  , _random(new Anki::Util::RandomGenerator())
  , _robotAdvertisementService(new Comms::AdvertisementService("RobotAdvertisementService"))
  , _robotMgr(new RobotManager(this))
  , _robotMsgHandler(new RobotInterface::MessageHandler())
  , _vizManager(new VizManager())
{
  // Only set up the audio server if we have a real dataPlatform
  if (nullptr != dataPlatform)
  {
    _audioServer.reset(new Audio::AudioServer(new Audio::AudioController(dataPlatform)));
  }
}
  
CozmoContext::CozmoContext() : CozmoContext(nullptr, nullptr)
{
  
}
  
} // namespace Cozmo
} // namespace Anki
