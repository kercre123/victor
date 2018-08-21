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

#include <set>

namespace Anki {

// forward declarations
template<typename EnumType>
class DependencyManagedEntity;

template<typename EnumType>
class IDependencyManagedComponent;

namespace Vector {

// When adding to this enum be sure to also declare a template specialization
// in the _impl.cpp file mapping the enum to the class type it is associated with
enum class BCComponentID{
  AIComponent,
  AsyncMessageComponent,
  BehaviorAudioComponent,
  BehaviorContainer,
  BehaviorEventComponent,
  BehaviorExternalInterface,
  BehaviorHelperComponent,
  BehaviorSystemManager,
  BehaviorTimerManager,
  BlockWorld,
  DelegationComponent,
  BehaviorComponentMessageHandler,
  FaceWorld,
  RobotInfo,
  UserDefinedBehaviorTreeComponent,
  UserIntentComponent,
  ActiveFeature,
  ActiveBehaviorIterator,
  BehaviorsBootLoader,
  RobotStatsTracker,
  AttentionTransferComponent,
  PowerStateManager,
  MoodManager,
  Count
};


using BCComp =  IDependencyManagedComponent<BCComponentID>;
using BCCompMap = DependencyManagedEntity<BCComponentID>;
using BCCompIDSet = std::set<BCComponentID>;

} // namespace Vector
} // namespace Anki

#endif // __Cozmo_Basestation_BehaviorSystem_BEI_Components_fwd_H__
