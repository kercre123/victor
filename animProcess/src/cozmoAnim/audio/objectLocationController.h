/**
 * File: objectLocationController.h
 *
 * Author: Jordan Rivas
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#ifndef __Anki_Cozmo_ObjectLocationController_H__
#define __Anki_Cozmo_ObjectLocationController_H__

//#include "audioEngine/audioEngineController.h"
//#include <memory>

namespace Anki {
//namespace AudioEngine {
//class SoundbankLoader;
//}
namespace Cozmo {
struct RobotState;

namespace Audio {

class CozmoAudioController;
struct AddRemoveWorldObject;
struct UpdateWorldObjectPosition;
  
class ObjectLocationController
{

public:

  ObjectLocationController( CozmoAudioController& audioController);
  
  void ObjectLocationControllerInit();

  void HandleMessage(const Anki::Cozmo::Audio::AddRemoveWorldObject& msg);
  void HandleMessage(const Anki::Cozmo::Audio::UpdateWorldObjectPosition& msg);
  
  
  // This doesn't belong here but's convient
  void ProcessRobotState(const RobotState& stateMsg);


private:
  CozmoAudioController& _audioController;
  
 
};

}
}
}

#endif /* __Anki_Cozmo_ObjectLocationController_H__ */
