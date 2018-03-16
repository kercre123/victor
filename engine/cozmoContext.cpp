
#include "engine/cozmoContext.h"

#include "anki/common/basestation/utils/data/dataPlatform.h"
#include "engine/audio/cozmoAudioController.h"
#include "engine/externalInterface/externalInterface.h"
#include "engine/needsSystem/needsManager.h"
#include "engine/perfMetric.h"
#include "engine/robotDataLoader.h"
#include "engine/robotManager.h"
#include "engine/util/transferQueue/dasTransferTask.h"
#include "engine/util/transferQueue/gameLogTransferTask.h"
#include "engine/util/transferQueue/transferQueueMgr.h"
#include "engine/utils/cozmoExperiments.h"
#include "engine/utils/cozmoFeatureGate.h"
#include "engine/viz/vizManager.h"
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
  , _needsManager(new NeedsManager(this))
  , _cozmoExperiments(new CozmoExperiments(this))
  , _perfMetric(new PerfMetric(this))
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
  
  
void CozmoContext::SetLocale(const std::string& localeString)
{
  using Locale = Anki::Util::Locale;
  using CozmoAudioController = Anki::Cozmo::Audio::CozmoAudioController;

  if (!localeString.empty()) {
    Locale locale = Locale::LocaleFromString(localeString);
    _locale.reset(new Locale(locale));

    // Update audio controller to use new locale preference
    auto * audioController = (_audioServer ? _audioServer->GetAudioController() : NULL);
    auto * cozmoAudioController = dynamic_cast<CozmoAudioController*>(audioController);
    if (nullptr != cozmoAudioController) {
      cozmoAudioController->SetLocale(*_locale);
    }
  }
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
