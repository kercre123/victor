#include "anki/cozmo/basestation/cozmoContext.h"
#include "anki/cozmo/basestation/robotDataLoader.h"
#include "anki/cozmo/basestation/robotManager.h"
#include "anki/cozmo/basestation/audio/audioController.h"
#include "anki/cozmo/basestation/audio/audioServer.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "anki/cozmo/basestation/viz/vizManager.h"
#include "anki/cozmo/basestation/util/transferQueue/dasTransferTask.h"
#include "anki/cozmo/basestation/util/transferQueue/gameLogTransferTask.h"
#include "anki/cozmo/basestation/util/transferQueue/transferQueueMgr.h"
#include "anki/cozmo/basestation/utils/cozmoFeatureGate.h"
#include "anki/cozmo/shared/cozmoConfig_common.h"
//#include "anki/cozmo/game/comms/uiMessageHandler.h"
#include "anki/common/basestation/utils/data/dataPlatform.h"
#include "util/random/randomGenerator.h"
#include "util/environment/locale.h"
#include "util/fileUtils/fileUtils.h"


namespace Anki {
namespace Cozmo {
  
CozmoContext::CozmoContext(Util::Data::DataPlatform* dataPlatform, IExternalInterface* externalInterface)
  : _externalInterface(externalInterface)
  , _dataPlatform(dataPlatform)
  , _featureGate(new CozmoFeatureGate())
  , _random(new Anki::Util::RandomGenerator())
  , _locale(new Util::Locale(Util::Locale::GetNativeLocale()))
  , _dataLoader(new RobotDataLoader(this))
  , _robotMgr(new RobotManager(this))
  , _vizManager(new VizManager())
  , _transferQueueMgr(new Anki::Util::TransferQueueMgr())
  #if USE_DAS
  , _dasTransferTask(new Anki::Util::DasTransferTask())
  #endif
  , _gameLogTransferTask(new Anki::Util::GameLogTransferTask())
{
  // Only set up the audio server if we have a real dataPlatform
  if (nullptr != dataPlatform)
  {
    _audioServer.reset(new Audio::AudioServer(new Audio::AudioController(this)));
  }
  #if USE_DAS
  _dasTransferTask->Init(_transferQueueMgr.get());
  #endif
  _gameLogTransferTask->Init(_transferQueueMgr.get());
}
  

CozmoContext::CozmoContext() : CozmoContext(nullptr, nullptr)
{
  
}

CozmoContext::~CozmoContext()
{
  _robotMgr->RemoveRobots();
}
  
  
bool CozmoContext::IsInSdkMode() const
{
  if (_externalInterface)
  {
    return _externalInterface->IsInSdkMode();
  }
  return false;
}
  
  
void CozmoContext::SetSdkStatus(SdkStatusType statusType, std::string&& statusText) const
{
  if (_externalInterface)
  {
    _externalInterface->SetSdkStatus(statusType, std::move(statusText));
  }
}

  
} // namespace Cozmo
} // namespace Anki
