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
namespace Cozmo {

// Forward declarations
namespace Audio{
class BehaviorAudioComponent;
}

class AIComponent;
class AsyncMessageGateComponent;
class BehaviorContainer;
class BehaviorEventAnimResponseDirector;
class BehaviorEventComponent;
class BehaviorExternalInterface;
class BehaviorHelperComponent;
class BehaviorSystemManager;
class BehaviorTimerManager;
class BlockWorld;
class DelegationComponent;
class DevBehaviorComponentMessageHandler;
class FaceWorld;
class BEIRobotInfo;
class UserIntentComponent;
class ActiveFeatureComponent;
class ActiveBehaviorIterator;
class BehaviorsBootLoader;
class RobotStatsTracker;
class AttentionTransferComponent;
class PowerStateManager;

} // namespace Cozmo

// Template specializations mapping enums from the _fwd.h file to the class forward declarations above
LINK_COMPONENT_TYPE_TO_ENUM(AIComponent,                        BCComponentID, AIComponent)
LINK_COMPONENT_TYPE_TO_ENUM(AsyncMessageGateComponent,          BCComponentID, AsyncMessageComponent)
LINK_COMPONENT_TYPE_TO_ENUM(Audio::BehaviorAudioComponent,      BCComponentID, BehaviorAudioComponent)
LINK_COMPONENT_TYPE_TO_ENUM(BehaviorContainer,                  BCComponentID, BehaviorContainer)
LINK_COMPONENT_TYPE_TO_ENUM(BehaviorEventAnimResponseDirector,  BCComponentID, BehaviorEventAnimResponseDirector)
LINK_COMPONENT_TYPE_TO_ENUM(BehaviorEventComponent,             BCComponentID, BehaviorEventComponent)
LINK_COMPONENT_TYPE_TO_ENUM(BehaviorExternalInterface,          BCComponentID, BehaviorExternalInterface)
LINK_COMPONENT_TYPE_TO_ENUM(BehaviorHelperComponent,            BCComponentID, BehaviorHelperComponent)
LINK_COMPONENT_TYPE_TO_ENUM(BehaviorSystemManager,              BCComponentID, BehaviorSystemManager)
LINK_COMPONENT_TYPE_TO_ENUM(BehaviorTimerManager,               BCComponentID, BehaviorTimerManager)
LINK_COMPONENT_TYPE_TO_ENUM(BlockWorld,                         BCComponentID, BlockWorld)
LINK_COMPONENT_TYPE_TO_ENUM(DelegationComponent,                BCComponentID, DelegationComponent)
LINK_COMPONENT_TYPE_TO_ENUM(DevBehaviorComponentMessageHandler, BCComponentID, DevBehaviorComponentMessageHandler)
LINK_COMPONENT_TYPE_TO_ENUM(FaceWorld,                          BCComponentID, FaceWorld)
LINK_COMPONENT_TYPE_TO_ENUM(BEIRobotInfo,                       BCComponentID, RobotInfo)
LINK_COMPONENT_TYPE_TO_ENUM(UserIntentComponent,                BCComponentID, UserIntentComponent)
LINK_COMPONENT_TYPE_TO_ENUM(ActiveFeatureComponent,             BCComponentID, ActiveFeature)
LINK_COMPONENT_TYPE_TO_ENUM(ActiveBehaviorIterator,             BCComponentID, ActiveBehaviorIterator)
LINK_COMPONENT_TYPE_TO_ENUM(BehaviorsBootLoader,                BCComponentID, BehaviorsBootLoader)
LINK_COMPONENT_TYPE_TO_ENUM(RobotStatsTracker,                  BCComponentID, RobotStatsTracker)
LINK_COMPONENT_TYPE_TO_ENUM(AttentionTransferComponent,         BCComponentID, AttentionTransferComponent)
LINK_COMPONENT_TYPE_TO_ENUM(PowerStateManager,                  BCComponentID, PowerStateManager)

// Translate entity into string
template<>
std::string GetEntityNameForEnumType<Cozmo::BCComponentID>(){ return "BehaviorComponent"; }

template<>
std::string GetComponentStringForID<Cozmo::BCComponentID>(Cozmo::BCComponentID enumID)
{
  switch(enumID){
    case Cozmo::BCComponentID::AIComponent:                        { return "AIComponent";}
    case Cozmo::BCComponentID::AsyncMessageComponent:              { return "AsyncMessageComponent";}
    case Cozmo::BCComponentID::BehaviorAudioComponent:             { return "BehaviorAudioComponent";}
    case Cozmo::BCComponentID::BehaviorContainer:                  { return "BehaviorContainer";}
    case Cozmo::BCComponentID::BehaviorEventAnimResponseDirector:  { return "BehaviorEventAnimResponseDirector";}
    case Cozmo::BCComponentID::BehaviorEventComponent:             { return "BehaviorEventComponent";}
    case Cozmo::BCComponentID::BehaviorExternalInterface:          { return "BehaviorExternalInterface";}
    case Cozmo::BCComponentID::BehaviorHelperComponent:            { return "BehaviorHelperComponent";}
    case Cozmo::BCComponentID::BehaviorSystemManager:              { return "BehaviorSystemManager";}
    case Cozmo::BCComponentID::BlockWorld:                         { return "BlockWorld";}
    case Cozmo::BCComponentID::DelegationComponent:                { return "DelegationComponent";}
    case Cozmo::BCComponentID::DevBehaviorComponentMessageHandler: { return "DevBehaviorComponentMessageHandler";}
    case Cozmo::BCComponentID::FaceWorld:                          { return "FaceWorld";}
    case Cozmo::BCComponentID::RobotInfo:                          { return "RobotInfo";}
    case Cozmo::BCComponentID::UserIntentComponent:                { return "UserIntentComponent";}
    case Cozmo::BCComponentID::BehaviorTimerManager:               { return "BehaviorTimerManager";}
    case Cozmo::BCComponentID::ActiveFeature:                      { return "ActiveFeature";}
    case Cozmo::BCComponentID::ActiveBehaviorIterator:             { return "ActiveBehaviorIterator";}
    case Cozmo::BCComponentID::BehaviorsBootLoader:                { return "BehaviorsBootLoader";}
    case Cozmo::BCComponentID::RobotStatsTracker:                  { return "RobotStatsTracker";}
    case Cozmo::BCComponentID::AttentionTransferComponent:         { return "AttentionTransferComponent";}
    case Cozmo::BCComponentID::PowerStateManager:                  { return "PowerStateManager";}
    case Cozmo::BCComponentID::Count:                              { return "Count";}
  }
}

} // namespace Anki
