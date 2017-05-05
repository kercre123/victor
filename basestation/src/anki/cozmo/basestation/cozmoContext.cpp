
#include "anki/cozmo/basestation/cozmoContext.h"

#include "anki/common/basestation/utils/data/dataPlatform.h"
#include "anki/cozmo/basestation/audio/cozmoAudioController.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "anki/cozmo/basestation/robotDataLoader.h"
#include "anki/cozmo/basestation/robotManager.h"
#include "anki/cozmo/basestation/util/transferQueue/dasTransferTask.h"
#include "anki/cozmo/basestation/util/transferQueue/gameLogTransferTask.h"
#include "anki/cozmo/basestation/util/transferQueue/transferQueueMgr.h"
#include "anki/cozmo/basestation/utils/cozmoFeatureGate.h"
#include "anki/cozmo/basestation/viz/vizManager.h"
#include "anki/cozmo/basestation/voiceCommands/voiceCommandComponent.h"
#include "audioEngine/multiplexer/audioMultiplexer.h"
#include "util/cpuProfiler/cpuThreadId.h"
#include "util/environment/locale.h"
#include "util/fileUtils/fileUtils.h"
#include "util/random/randomGenerator.h"


namespace Anki {
namespace Cozmo {

class ThreadIDInternal : private Util::noncopyable
{
public:
  Util::CpuThreadId _id = Util::kCpuThreadIdInvalid;
};

  
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
  , _threadIdHolder(new ThreadIDInternal)
{

  // Only set up the audio server if we have a real dataPlatform
  if (nullptr != dataPlatform)
  {
    _audioServer.reset(new AudioEngine::Multiplexer::AudioMultiplexer(new Audio::CozmoAudioController(this)));
  }
  #if USE_DAS
  _dasTransferTask->Init(_transferQueueMgr.get());
  #endif
  _gameLogTransferTask->Init(_transferQueueMgr.get());
  
  // This needs to happen after the audio server is set up
  _voiceCommandComponent.reset(new VoiceCommand::VoiceCommandComponent(*this));
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

void CozmoContext::SetRandomSeed(uint32_t seed)
{
  _random->SetSeed("CozmoContext", seed);
}

void CozmoContext::SetMainThread()
{
  _threadIdHolder->_id = Util::GetCurrentThreadId();
}

bool CozmoContext::IsMainThread() const
{
  return Util::AreCpuThreadIdsEqual( _threadIdHolder->_id, Util::GetCurrentThreadId() );
}

  
} // namespace Cozmo
} // namespace Anki
