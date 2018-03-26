/**
 * File: animContext.h
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

#ifndef __Anki_Cozmo_AnimContext_H__
#define __Anki_Cozmo_AnimContext_H__

#include "util/helpers/noncopyable.h"
#include <memory>


// ---------- BEGIN FORWARD DECLARATIONS ----------
namespace Anki {
namespace Util {
  class Locale;
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
  
namespace MicData {
  class MicDataProcessor;
}
class RobotDataLoader;
class ThreadIDInternal;
  
namespace Audio {
class CozmoAudioController;
}
namespace RobotInterface {
class MessageHandler;
}
namespace WebService {
class WebService;
}

} // namespace Cozmo
} // namespace Anki

// ---------- END FORWARD DECLARATIONS ----------



// Here begins the actual namespace and interface for AnimContext
namespace Anki {
namespace Cozmo {
  
class AnimContext : private Util::noncopyable
{
  using AudioMultiplexer = AudioEngine::Multiplexer::AudioMultiplexer;
  
public:
  AnimContext(Util::Data::DataPlatform* dataPlatform);
  AnimContext();
  virtual ~AnimContext();
  
  Util::Data::DataPlatform*             GetDataPlatform() const { return _dataPlatform; }
  Util::Locale *                        GetLocale() const { return _locale.get(); }
  Util::RandomGenerator*                GetRandom() const { return _random.get(); }
  RobotDataLoader*                      GetDataLoader() const { return _dataLoader.get(); }
  Audio::CozmoAudioController*          GetAudioController() const; // Can return nullptr
  AudioMultiplexer*                     GetAudioMultiplexer() const { return _audioMux.get(); }
  MicData::MicDataProcessor*            GetMicDataProcessor() const { return _micDataProcessor.get(); }
  WebService::WebService*               GetWebService() const { return _webService.get(); }

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
  std::unique_ptr<Util::Locale>                  _locale;
  std::unique_ptr<AudioMultiplexer>              _audioMux;
  std::unique_ptr<Util::RandomGenerator>         _random;
  std::unique_ptr<RobotDataLoader>               _dataLoader;
  std::unique_ptr<MicData::MicDataProcessor>     _micDataProcessor;
  std::unique_ptr<WebService::WebService>        _webService;

  // for holding the thread id (and avoiding needed to include the .h here)
  std::unique_ptr<ThreadIDInternal> _threadIdHolder;
  
  void InitAudio(Util::Data::DataPlatform* dataPlatform);
};
  

} // namespace Cozmo
} // namespace Anki

#endif // __Anki_Cozmo_AnimContext_H__
