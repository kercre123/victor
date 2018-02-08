
#include "cozmoAnim/animContext.h"

#include "audioEngine/multiplexer/audioMultiplexer.h"
#include "cozmoAnim/audio/cozmoAudioController.h"
#include "cozmoAnim/micDataProcessor.h"
#include "cozmoAnim/robotDataLoader.h"

#include "webServerProcess/src/webService.h"

#include "coretech/common/engine/utils/data/dataPlatform.h"

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

  
AnimContext::AnimContext(Util::Data::DataPlatform* dataPlatform)
  : _dataPlatform(dataPlatform)
  , _locale(new Anki::Util::Locale(Anki::Util::Locale::GetNativeLocale()))  
  , _random(new Anki::Util::RandomGenerator())
  , _dataLoader(new RobotDataLoader(this))
  , _webService(new WebService::WebService())
  , _threadIdHolder(new ThreadIDInternal)
{
  if (dataPlatform != nullptr)
  {
    const std::string& dataWriteLocation = _dataPlatform->pathToResource(Util::Data::Scope::Cache, "micdata");
    const std::string& triggerDataDir = _dataPlatform->pathToResource(Util::Data::Scope::Resources, "assets/hey_cosmo_and_commands");
    _micDataProcessor.reset(new MicData::MicDataProcessor(dataWriteLocation, triggerDataDir));
  }
  InitAudio(_dataPlatform);
}


AnimContext::AnimContext() : AnimContext(nullptr)
{

}

AnimContext::~AnimContext()
{

}


Audio::CozmoAudioController* AnimContext::GetAudioController() const
{
  if (_audioMux.get() != nullptr) {
    return dynamic_cast<Audio::CozmoAudioController*>( _audioMux->GetAudioController() );
  }
  return nullptr;
}

void AnimContext::SetRandomSeed(uint32_t seed)
{
  _random->SetSeed("AnimContext", seed);
}
  

void AnimContext::SetMainThread()
{
  _threadIdHolder->_id = Util::GetCurrentThreadId();
}


bool AnimContext::IsMainThread() const
{
  return Util::AreCpuThreadIdsEqual( _threadIdHolder->_id, Util::GetCurrentThreadId() );
}


void AnimContext::InitAudio(Util::Data::DataPlatform* dataPlatform)
{
  // Only set up the audio server if we have a real dataPlatform
  if (nullptr == dataPlatform) {
    // Create a dummy Audio Multiplexer
    _audioMux.reset(new AudioEngine::Multiplexer::AudioMultiplexer( nullptr ));
    return;
  }
  // Init Audio Base: Audio Engine & Multiplexer
  _audioMux.reset(new AudioEngine::Multiplexer::AudioMultiplexer(new Audio::CozmoAudioController(this)));
  // Audio Mux Input setup is in cozmoAnim.cpp & engineMessages.cpp
}
  
} // namespace Cozmo
} // namespace Anki
