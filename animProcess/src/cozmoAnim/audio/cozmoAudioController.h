/**
 * File: cozmoAudioController.h
 *
 * Author: Jordan Rivas
 * Created: 09/07/2017
 *
 * Description: Cozmo interface to Audio Engine
 *              - Subclass of AudioEngine::AudioEngineController
 *              - Implement Cozmo specific audio functionality
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#ifndef __Anki_Cozmo_CozmoAudioController_H__
#define __Anki_Cozmo_CozmoAudioController_H__

#include "audioEngine/audioEngineController.h"

namespace Anki {
namespace Cozmo {
// Forward declaration
class CozmoAnimContext;
namespace Audio {
  

class CozmoAudioController : public AudioEngine::AudioEngineController
{

public:

  CozmoAudioController( const CozmoAnimContext* context );

  virtual ~CozmoAudioController();


  // Register CLAD Game Objects
  void RegisterCladGameObjectsWithAudioController();

};

}
}
}

#endif /* __Anki_Cozmo_CozmoAudioController_H__ */
