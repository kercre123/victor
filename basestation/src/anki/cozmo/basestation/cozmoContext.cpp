#include "anki/cozmo/basestation/cozmoContext.h"
#include "anki/cozmo/basestation/robotManager.h"
#include "anki/cozmo/basestation/robotInterface/messageHandler.h"
#include "anki/cozmo/basestation/speechRecognition/keyWordRecognizer.h"
#include "anki/cozmo/basestation/audio/audioController.h"
#include "anki/cozmo/basestation/audio/audioServer.h"
//#include "anki/cozmo/game/comms/uiMessageHandler.h"
#include "anki/common/basestation/utils/data/dataPlatform.h"
#include "anki/messaging/basestation/advertisementService.h"
#include "util/random/randomGenerator.h"

namespace Anki {
namespace Cozmo {
  
CozmoContext::CozmoContext(const Util::Data::DataPlatform& dataPlatform, IExternalInterface* externalInterface)
  : _externalInterface(externalInterface)
  , _dataPlatform(new Util::Data::DataPlatform(dataPlatform))
  , _random(new Anki::Util::RandomGenerator())
  , _robotAdvertisementService(new Comms::AdvertisementService("RobotAdvertisementService"))
  , _robotMgr(new RobotManager(externalInterface, _dataPlatform.get()))
  , _robotMsgHandler(new RobotInterface::MessageHandler())
  , _keywordRecognizer(new SpeechRecognition::KeyWordRecognizer(externalInterface))
  , _audioServer(new Audio::AudioServer(new Audio::AudioController(_dataPlatform.get())))
{
  
}
 
// Empty destructor needed in cpp for std::unique_ptr to have full class definitions for destruction
CozmoContext::~CozmoContext()
{
}
    
} // namespace Cozmo
} // namespace Anki
