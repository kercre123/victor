
#include "cozmoAnim/animContext.h"

#include "audioEngine/multiplexer/audioMultiplexer.h"
#include "cozmoAnim/audio/audioPlaybackSystem.h"
#include "cozmoAnim/audio/cozmoAudioController.h"
#include "cozmoAnim/micData/micDataSystem.h"
#include "cozmoAnim/robotDataLoader.h"

#include "webServerProcess/src/webService.h"

#include "coretech/common/engine/utils/data/dataPlatform.h"

#include "util/cpuProfiler/cpuThreadId.h"
#include "util/environment/locale.h"
#include "util/fileUtils/fileUtils.h"
#include "util/random/randomGenerator.h"

#define LOG_CHANNEL "AnimContext"

namespace Anki {
namespace Vector {

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
  , _micDataSystem(new MicData::MicDataSystem(dataPlatform, this))
  , _webService(new WebService::WebService())
  , _audioPlayer(new Audio::AudioPlaybackSystem(this))
  , _threadIdHolder(new ThreadIDInternal)
{
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

void AnimContext::SetLocale(const std::string & locale)
{
  using Locale = Anki::Util::Locale;

  LOG_INFO("AnimContext.SetLocale", "Set locale to %s", locale.c_str());
  _locale = std::make_unique<Locale>(Locale::LocaleFromString(locale));

  if (_micDataSystem != nullptr) {
    _micDataSystem->UpdateLocale(*_locale);
  }
}
} // namespace Vector
} // namespace Anki
