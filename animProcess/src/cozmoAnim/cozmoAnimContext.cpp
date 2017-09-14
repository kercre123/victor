
#include "cozmoAnim/cozmoAnimContext.h"
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

  // Only set up the audio server if we have a real dataPlatform
  if (nullptr != dataPlatform)
  {
//    _audioServer.reset(new AudioEngine::Multiplexer::AudioMultiplexer(new Audio::CozmoAudioController(this)));
  }
}


CozmoAnimContext::CozmoAnimContext() : CozmoAnimContext(nullptr)
{

}

CozmoAnimContext::~CozmoAnimContext()
{

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

  
} // namespace Cozmo
} // namespace Anki
