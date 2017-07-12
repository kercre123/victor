
#include "cozmoAnim/cozmoContext.h"

#include "anki/common/basestation/utils/data/dataPlatform.h"
//#include "anki/cozmo/basestation/audio/cozmoAudioController.h"
#include "cozmoAnim/robotDataLoader.h"
//#include "audioEngine/multiplexer/audioMultiplexer.h"
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

  
CozmoContext::CozmoContext(Util::Data::DataPlatform* dataPlatform)
  : _dataPlatform(dataPlatform)
  , _random(new Anki::Util::RandomGenerator())
  , _dataLoader(new RobotDataLoader(dataPlatform))
  , _threadIdHolder(new ThreadIDInternal)
{

  // Only set up the audio server if we have a real dataPlatform
  if (nullptr != dataPlatform)
  {
//    _audioServer.reset(new AudioEngine::Multiplexer::AudioMultiplexer(new Audio::CozmoAudioController(this)));
  }
}


CozmoContext::CozmoContext() : CozmoContext(nullptr)
{

}

CozmoContext::~CozmoContext()
{

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
