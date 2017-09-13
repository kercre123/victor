//
//  victorAudioController.hpp
//  cozmoEngine2
//
//  Created by Jordan Rivas on 9/7/17.
//
//

#ifndef victorAudioController_hpp
#define victorAudioController_hpp

#include "audioEngine/audioEngineController.h"

namespace Anki {
namespace Cozmo {
// Forward declaration
class CozmoAnimContext;

namespace Audio {

class VictorAudioController : public AudioEngine::AudioEngineController
{

public:

  VictorAudioController( const CozmoAnimContext* context );

  virtual ~VictorAudioController();



  // Register CLAD Game Objects
  void RegisterCladGameObjectsWithAudioController();

};

}

}
}

#endif /* victorAudioController_hpp */
