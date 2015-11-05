//
//  AudioController.h
//  cozmoEngine
//
//  Created by Jordan Rivas on 11/3/15.
//
//

#ifndef AudioController_h
#define AudioController_h

#include <stdio.h>
#include <util/helpers/noncopyable.h>

namespace AudioEngine {
  class AudioEngineController;
}

namespace Anki {
namespace Util {
namespace Data {
  class DataPlatform;
}
}
namespace Cozmo {
  
class AudioController //: public Anki::Util::noncopyable
{
  
public:
  
  inline static AudioController* getInstance();
  static void removeInstance();
  
  bool Initialize(Util::Data::DataPlatform* dataPlatfrom);
  // TODO: Need to add Language Local, pathToZipFile, zipBasePath, assetManager & assetManagerBasePath for RAMS & Android
  
  

protected:
  
  AudioController();
  ~AudioController();
  
  static AudioController* _singletonInstance;
  AudioEngine::AudioEngineController* _audioEngine;
  
  
  
  bool _isInitialized;
  
  
  
};
  
inline AudioController* AudioController::getInstance()
{
  // If we haven't already instantiated the singleton, do so now.
  if( nullptr == _singletonInstance ) {
    _singletonInstance = new AudioController();
  }
  return _singletonInstance;
}
  
} // Cozmo
} // Anki



#endif /* AudioController_h */
