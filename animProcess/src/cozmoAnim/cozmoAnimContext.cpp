
#include "cozmoAnim/cozmoAnimContext.h"

#include "audioEngine/multiplexer/audioMultiplexer.h"
#include "cozmoAnim/audio/cozmoAudioController.h"
#include "cozmoAnim/micDataProcessor.h"
#include "cozmoAnim/robotDataLoader.h"

#include "anki/common/basestation/utils/data/dataPlatform.h"

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

  
CozmoAnimContext::CozmoAnimContext(Util::Data::DataPlatform* dataPlatform)
  : _dataPlatform(dataPlatform)
  , _random(new Anki::Util::RandomGenerator())
  , _dataLoader(new RobotDataLoader(this))
  , _threadIdHolder(new ThreadIDInternal)
{
  if (dataPlatform != nullptr)
  {
    _micDataProcessor.reset(new MicData::MicDataProcessor(_dataPlatform->pathToResource(Util::Data::Scope::Cache, "micdata")));
  }
  InitAudio(_dataPlatform);
}


CozmoAnimContext::CozmoAnimContext() : CozmoAnimContext(nullptr)
{

}

CozmoAnimContext::~CozmoAnimContext()
{

}


Audio::CozmoAudioController* CozmoAnimContext::GetAudioController() const
{
  if (_audioMux.get() != nullptr) {
    return dynamic_cast<Audio::CozmoAudioController*>( _audioMux->GetAudioController() );
  }
  return nullptr;
}

void CozmoAnimContext::SetRandomSeed(uint32_t seed)
{
  _random->SetSeed("CozmoAnimContext", seed);
}
  

void CozmoAnimContext::SetMainThread()
{
  _threadIdHolder->_id = Util::GetCurrentThreadId();
}


bool CozmoAnimContext::IsMainThread() const
{
  return Util::AreCpuThreadIdsEqual( _threadIdHolder->_id, Util::GetCurrentThreadId() );
}


void CozmoAnimContext::InitAudio(Util::Data::DataPlatform* dataPlatform)
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
