/**
* File: behaviorComponents_impl.cpp
*
* Author: Kevin M. Karol
* Created: 2/12/18
*
* Description: Template specializations mapping classes to enums
*
* Copyright: Anki, Inc. 2018
*
**/

#include "engine/aiComponent/behaviorComponent/behaviorComponents_fwd.h"
#include "util/entityComponent/componentTypeEnumMap.h"

namespace Anki {
namespace Vector {

// Forward declarations
namespace Audio{
class BehaviorAudioComponent;
}

class AIComponent;
class AsyncMessageGateComponent;
class BehaviorComponentMessageHandler;
class BehaviorContainer;
class BehaviorEventComponent;
class BehaviorExternalInterface;
class BehaviorHelperComponent;
class BehaviorSystemManager;
class BehaviorTimerManager;
class BlockWorld;
class DelegationComponent;
class FaceWorld;
class BEIRobotInfo;
class UserIntentComponent;
class UserDefinedBehaviorTreeComponent;
class ActiveFeatureComponent;
class ActiveBehaviorIterator;
class BehaviorsBootLoader;
class RobotStatsTracker;
class AttentionTransferComponent;
class PowerStateManager;
class MoodManager;
class SleepTracker;

} // namespace Vector

// Template specializations mapping enums from the _fwd.h file to the class forward declarations above
LINK_COMPONENT_TYPE_TO_ENUM(AIComponent,                        BCComponentID, AIComponent)
LINK_COMPONENT_TYPE_TO_ENUM(AsyncMessageGateComponent,          BCComponentID, AsyncMessageComponent)
LINK_COMPONENT_TYPE_TO_ENUM(Audio::BehaviorAudioComponent,      BCComponentID, BehaviorAudioComponent)
LINK_COMPONENT_TYPE_TO_ENUM(BehaviorComponentMessageHandler,    BCComponentID, BehaviorComponentMessageHandler)
LINK_COMPONENT_TYPE_TO_ENUM(BehaviorContainer,                  BCComponentID, BehaviorContainer)
LINK_COMPONENT_TYPE_TO_ENUM(BehaviorEventComponent,             BCComponentID, BehaviorEventComponent)
LINK_COMPONENT_TYPE_TO_ENUM(BehaviorExternalInterface,          BCComponentID, BehaviorExternalInterface)
LINK_COMPONENT_TYPE_TO_ENUM(BehaviorHelperComponent,            BCComponentID, BehaviorHelperComponent)
LINK_COMPONENT_TYPE_TO_ENUM(BehaviorSystemManager,              BCComponentID, BehaviorSystemManager)
LINK_COMPONENT_TYPE_TO_ENUM(BehaviorTimerManager,               BCComponentID, BehaviorTimerManager)
LINK_COMPONENT_TYPE_TO_ENUM(BlockWorld,                         BCComponentID, BlockWorld)
LINK_COMPONENT_TYPE_TO_ENUM(DelegationComponent,                BCComponentID, DelegationComponent)
LINK_COMPONENT_TYPE_TO_ENUM(FaceWorld,                          BCComponentID, FaceWorld)
LINK_COMPONENT_TYPE_TO_ENUM(BEIRobotInfo,                       BCComponentID, RobotInfo)
LINK_COMPONENT_TYPE_TO_ENUM(UserDefinedBehaviorTreeComponent,   BCComponentID, UserDefinedBehaviorTreeComponent)
LINK_COMPONENT_TYPE_TO_ENUM(UserIntentComponent,                BCComponentID, UserIntentComponent)
LINK_COMPONENT_TYPE_TO_ENUM(ActiveFeatureComponent,             BCComponentID, ActiveFeature)
LINK_COMPONENT_TYPE_TO_ENUM(ActiveBehaviorIterator,             BCComponentID, ActiveBehaviorIterator)
LINK_COMPONENT_TYPE_TO_ENUM(BehaviorsBootLoader,                BCComponentID, BehaviorsBootLoader)
LINK_COMPONENT_TYPE_TO_ENUM(RobotStatsTracker,                  BCComponentID, RobotStatsTracker)
LINK_COMPONENT_TYPE_TO_ENUM(AttentionTransferComponent,         BCComponentID, AttentionTransferComponent)
LINK_COMPONENT_TYPE_TO_ENUM(PowerStateManager,                  BCComponentID, PowerStateManager)
LINK_COMPONENT_TYPE_TO_ENUM(MoodManager,                        BCComponentID, MoodManager)
LINK_COMPONENT_TYPE_TO_ENUM(SleepTracker,                       BCComponentID, SleepTracker)

// Translate entity into string
template<>
std::string GetEntityNameForEnumType<Vector::BCComponentID>(){ return "BehaviorComponent"; }

template<>
std::string GetComponentStringForID<Vector::BCComponentID>(Vector::BCComponentID enumID)
{
  #define CASE(id) case Vector::BCComponentID::id: { return #id; }
  switch (enumID) {
    CASE(ActiveBehaviorIterator)
    CASE(ActiveFeature)
    CASE(AIComponent)
    CASE(AsyncMessageComponent)
    CASE(AttentionTransferComponent)
    CASE(BehaviorAudioComponent)
    CASE(BehaviorComponentMessageHandler)
    CASE(BehaviorContainer)
    CASE(BehaviorEventComponent)
    CASE(BehaviorExternalInterface)
    CASE(BehaviorHelperComponent)
    CASE(BehaviorSystemManager)
    CASE(BehaviorTimerManager)
    CASE(BehaviorsBootLoader)
    CASE(BlockWorld)
    CASE(DelegationComponent)
    CASE(FaceWorld)
    CASE(MoodManager)
    CASE(PowerStateManager)
    CASE(RobotInfo)
    CASE(RobotStatsTracker)
    CASE(SleepTracker)
    CASE(UserDefinedBehaviorTreeComponent)
    CASE(UserIntentComponent)
    CASE(Count)
  }
  #undef CASE
}

} // namespace Anki
