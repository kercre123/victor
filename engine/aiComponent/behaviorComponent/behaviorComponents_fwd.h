/**
* File: behaviorComponents_fwd.h
*
* Author: Kevin M. Karol
* Created: 1/5/18
*
* Description: Enumeration of components within behaviorComponent.h's entity
*
* Copyright: Anki, Inc. 2018
*
**/

#ifndef __Cozmo_Basestation_BehaviorSystem_Behavior_Components_fwd_H__
#define __Cozmo_Basestation_BehaviorSystem_Behavior_Components_fwd_H__

#include "engine/dependencyManagedComponent.h"

namespace Anki {
namespace Cozmo {

enum class BCComponentID{
  AIComponent,
  AsyncMessageComponent,
  BehaviorAudioComponent,
  BehaviorComponentCloudReceiver,
  BehaviorContainer,
  BehaviorEventAnimResponseDirector,
  BehaviorEventComponent,
  BehaviorExternalInterface,
  BehaviorHelperComponent,
  BehaviorSystemManager,
  BlockWorld,
  DelegationComponent,
  DevBehaviorComponentMessageHandler,
  FaceWorld,
  RobotInfo,
  BaseBehaviorWrapper,
  Count
};


using BCComp =  DependencyManagedComponentWrapper<BCComponentID>;
using BCCompMap = std::map<BCComponentID, BCComp>;
using BCCompIDSet = std::set<BCComponentID>;

} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_BehaviorSystem_BEI_Components_fwd_H__
