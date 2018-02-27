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
class BaseBehaviorWrapper;
class UserIntentComponent;

} // namespace Cozmo

// Template specializations mapping enums from the _fwd.h file to the class forward declarations above
template<>
void GetComponentIDForType<Cozmo::BCComponentID, Cozmo::AIComponent>(Cozmo::BCComponentID& enumToSet){enumToSet = Cozmo::BCComponentID::AIComponent;}
template<>
void GetComponentIDForType<Cozmo::BCComponentID, Cozmo::AsyncMessageGateComponent>(Cozmo::BCComponentID& enumToSet){enumToSet = Cozmo::BCComponentID::AsyncMessageComponent;}
template<>
void GetComponentIDForType<Cozmo::BCComponentID, Cozmo::Audio::BehaviorAudioComponent>(Cozmo::BCComponentID& enumToSet){enumToSet =  Cozmo::BCComponentID::BehaviorAudioComponent;}
template<>
void GetComponentIDForType<Cozmo::BCComponentID, Cozmo::BehaviorContainer>(Cozmo::BCComponentID& enumToSet){enumToSet =  Cozmo::BCComponentID::BehaviorContainer;}
template<>
void GetComponentIDForType<Cozmo::BCComponentID, Cozmo::BehaviorEventAnimResponseDirector>(Cozmo::BCComponentID& enumToSet){enumToSet =  Cozmo::BCComponentID::BehaviorEventAnimResponseDirector;}
template<>
void GetComponentIDForType<Cozmo::BCComponentID, Cozmo::BehaviorEventComponent>(Cozmo::BCComponentID& enumToSet){enumToSet =  Cozmo::BCComponentID::BehaviorEventComponent;}
template<>
void GetComponentIDForType<Cozmo::BCComponentID, Cozmo::BehaviorExternalInterface>(Cozmo::BCComponentID& enumToSet){enumToSet =  Cozmo::BCComponentID::BehaviorExternalInterface;}
template<>
void GetComponentIDForType<Cozmo::BCComponentID, Cozmo::BehaviorHelperComponent>(Cozmo::BCComponentID& enumToSet){enumToSet =  Cozmo::BCComponentID::BehaviorHelperComponent;}
template<>
void GetComponentIDForType<Cozmo::BCComponentID, Cozmo::BehaviorSystemManager>(Cozmo::BCComponentID& enumToSet){enumToSet =  Cozmo::BCComponentID::BehaviorSystemManager;}
template<>
void GetComponentIDForType<Cozmo::BCComponentID, Cozmo::BehaviorTimerManager>(Cozmo::BCComponentID& enumToSet){enumToSet =  Cozmo::BCComponentID::BehaviorTimerManager;}
template<>
void GetComponentIDForType<Cozmo::BCComponentID, Cozmo::BlockWorld>(Cozmo::BCComponentID& enumToSet){enumToSet =  Cozmo::BCComponentID::BlockWorld;}
template<>
void GetComponentIDForType<Cozmo::BCComponentID, Cozmo::DelegationComponent>(Cozmo::BCComponentID& enumToSet){enumToSet =  Cozmo::BCComponentID::DelegationComponent;}
template<>
void GetComponentIDForType<Cozmo::BCComponentID, Cozmo::DevBehaviorComponentMessageHandler>(Cozmo::BCComponentID& enumToSet){enumToSet =  Cozmo::BCComponentID::DevBehaviorComponentMessageHandler;}
template<>
void GetComponentIDForType<Cozmo::BCComponentID, Cozmo::FaceWorld>(Cozmo::BCComponentID& enumToSet){enumToSet =  Cozmo::BCComponentID::FaceWorld;}
template<>
void GetComponentIDForType<Cozmo::BCComponentID, Cozmo::BEIRobotInfo>(Cozmo::BCComponentID& enumToSet){enumToSet =  Cozmo::BCComponentID::RobotInfo;}
template<>
void GetComponentIDForType<Cozmo::BCComponentID, Cozmo::BaseBehaviorWrapper>(Cozmo::BCComponentID& enumToSet){enumToSet =  Cozmo::BCComponentID::BaseBehaviorWrapper;}
template<>
void GetComponentIDForType<Cozmo::BCComponentID, Cozmo::UserIntentComponent>(Cozmo::BCComponentID& enumToSet){enumToSet = Cozmo::BCComponentID::UserIntentComponent;}

} // namespace Anki
