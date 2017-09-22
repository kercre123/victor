/**
 * File: iSubtaskListener.h
 *
 * Author: Kevin M. Karol
 * Created: 12/14/16
 *
 * Description: Interface for receiving notification that a sub-task of the behavior has
 * been completed
 *
 * Copyright: Anki, Inc. 2016
 **/

#ifndef __Cozmo_Basestation_BehaviorSystem_BehaviorListenerInterfaces_ISubtaskListener_H__
#define __Cozmo_Basestation_BehaviorSystem_BehaviorListenerInterfaces_ISubtaskListener_H__


namespace Anki {
namespace Cozmo {

class Robot;
  
class ISubtaskListener{
public:
  virtual void AnimationComplete(BehaviorExternalInterface& behaviorExternalInterface) = 0;
};


} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_BehaviorSystem_BehaviorListenerInterfaces_ISubtaskListener_H__
