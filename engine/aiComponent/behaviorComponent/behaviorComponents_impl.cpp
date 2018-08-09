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

// Translate entity into string
template<>
std::string GetEntityNameForEnumType<Vector::BCComponentID>(){ return "BehaviorComponent"; }

template<>
std::string GetComponentStringForID<Vector::BCComponentID>(Vector::BCComponentID enumID)
{
  switch(enumID){
    case Vector::BCComponentID::AIComponent:                        { return "AIComponent";}
    case Vector::BCComponentID::AsyncMessageComponent:              { return "AsyncMessageComponent";}
    case Vector::BCComponentID::BehaviorAudioComponent:             { return "BehaviorAudioComponent";}
    case Vector::BCComponentID::BehaviorComponentMessageHandler:    { return "BehaviorComponentMessageHandler";}
    case Vector::BCComponentID::BehaviorContainer:                  { return "BehaviorContainer";}
    case Vector::BCComponentID::BehaviorEventComponent:             { return "BehaviorEventComponent";}
    case Vector::BCComponentID::BehaviorExternalInterface:          { return "BehaviorExternalInterface";}
    case Vector::BCComponentID::BehaviorHelperComponent:            { return "BehaviorHelperComponent";}
    case Vector::BCComponentID::BehaviorSystemManager:              { return "BehaviorSystemManager";}
    case Vector::BCComponentID::BlockWorld:                         { return "BlockWorld";}
    case Vector::BCComponentID::DelegationComponent:                { return "DelegationComponent";}
    case Vector::BCComponentID::FaceWorld:                          { return "FaceWorld";}
    case Vector::BCComponentID::RobotInfo:                          { return "RobotInfo";}
    case Vector::BCComponentID::UserDefinedBehaviorTreeComponent:   { return "UserDefinedBehaviorTreeComponent";}
    case Vector::BCComponentID::UserIntentComponent:                { return "UserIntentComponent";}
    case Vector::BCComponentID::BehaviorTimerManager:               { return "BehaviorTimerManager";}
    case Vector::BCComponentID::ActiveFeature:                      { return "ActiveFeature";}
    case Vector::BCComponentID::ActiveBehaviorIterator:             { return "ActiveBehaviorIterator";}
    case Vector::BCComponentID::BehaviorsBootLoader:                { return "BehaviorsBootLoader";}
    case Vector::BCComponentID::RobotStatsTracker:                  { return "RobotStatsTracker";}
    case Vector::BCComponentID::AttentionTransferComponent:         { return "AttentionTransferComponent";}
    case Vector::BCComponentID::PowerStateManager:                  { return "PowerStateManager";}
    case Vector::BCComponentID::Count:                              { return "Count";}
  }
}

} // namespace Anki
