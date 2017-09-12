/**
 * File: cozmoAnimContext.h
 *
 * Author: Lee Crippen
 * Created: 1/29/2016
 *
 * Description: Holds references to components and systems that are used often by all different parts of code,
 *              where it is unclear who the appropriate owner of that system would be.
 *              NOT intended to be a container to hold ALL systems and components, which would simply be lazy.
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#ifndef __Cozmo_Basestation_CozmoAnimContext_H__
#define __Cozmo_Basestation_CozmoAnimContext_H__

#include "util/helpers/noncopyable.h"
#include <memory>


// ---------- BEGIN FORWARD DECLARATIONS ----------
namespace Anki {
namespace Util {
  class RandomGenerator;
  namespace Data {
    class DataPlatform;
  }
}


namespace AudioEngine {
namespace Multiplexer {
class AudioMultiplexer;
}
}

namespace Cozmo {
  
class RobotDataLoader;
class ThreadIDInternal;
  
namespace Audio {
class VictorAudioController;
}
namespace RobotInterface {
class MessageHandler;
}
  
} // namespace Cozmo
} // namespace Anki

// ---------- END FORWARD DECLARATIONS ----------



// Here begins the actual namespace and interface for CozmoAnimContext
namespace Anki {
namespace Cozmo {
  
class CozmoAnimContext : private Util::noncopyable
{
  using AudioMultiplexer = AudioEngine::Multiplexer::AudioMultiplexer;
  
public:
  CozmoAnimContext(Util::Data::DataPlatform* dataPlatform);
  CozmoAnimContext();
  virtual ~CozmoAnimContext();
  
  Util::Data::DataPlatform*             GetDataPlatform() const { return _dataPlatform; }

  Util::RandomGenerator*                GetRandom() const { return _random.get(); }
  RobotDataLoader*                      GetDataLoader() const { return _dataLoader.get(); }
  Audio::VictorAudioController*         GetAudioController() const; // Can return nullptr
  AudioMultiplexer*                     GetAudioMultiplexer() const { return _audioMux.get(); }
  
  void SetRandomSeed(uint32_t seed);

  // Tell the context that this is the main thread
  void SetMainThread();

  // Returns true if the current thread is the "main" one. Requires SetMainThread to have been called
  bool IsMainThread() const;
  
private:
  // This is passed in and held onto, but not owned by the context (yet.
  // It really should be, and that refactoring will have to happen soon).
  Util::Data::DataPlatform*                      _dataPlatform = nullptr;
  
  // Context holds onto these things for everybody:
  std::unique_ptr<AudioMultiplexer>              _audioMux;
  std::unique_ptr<Util::RandomGenerator>         _random;
  std::unique_ptr<RobotDataLoader>               _dataLoader;

  // for holding the thread id (and avoiding needed to include the .h here)
  std::unique_ptr<ThreadIDInternal> _threadIdHolder;
};
  

} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_CozmoAnimContext_H__
