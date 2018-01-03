/**
 * File: iReactToFaceListener.h
 *
 * Author: Kevin M. Karol
 * Created: 12/14/16
 *
 * Description: Interface for receiving notifications from ReactToFace
 *
 * Copyright: Anki, Inc. 2016
 **/

#ifndef __Cozmo_Basestation_BehaviorSystem_BehaviorListenerInterfaces_IReactToFaceListener_H__
#define __Cozmo_Basestation_BehaviorSystem_BehaviorListenerInterfaces_IReactToFaceListener_H__

#include "coretech/vision/engine/faceIdTypes.h"


namespace Anki {
namespace Cozmo {
  
class SmartFaceID;

class IReactToFaceListener{
public:
  virtual void FinishedReactingToFace(BehaviorExternalInterface& behaviorExternalInterface, SmartFaceID faceID) = 0;
  virtual void ClearDesiredTargets() = 0;
};


} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_BehaviorSystem_BehaviorListenerInterfaces_IReactToFaceListener_H__

