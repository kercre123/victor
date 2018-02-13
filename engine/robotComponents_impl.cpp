/**
* File: robotComponents_impl.cpp
*
* Author: Kevin M. Karol
* Created: 2/12/18
*
* Description: Template specializations mapping classes to enums
*
* Copyright: Anki, Inc. 2018
*
**/

#include "engine/robotComponents_fwd.h"

namespace Anki {
namespace Cozmo {

// Forward declarations
namespace Audio{
class EngineRobotAudioClient;
}

class ContextWrapper;
class BlockWorld;
class FaceWorld;
class PetWorld;
class PublicStateBroadcaster;
class PathComponent;
class DrivingAnimationHandler;
class ActionList;
class MovementComponent;
class VisionComponent;
class VisionScheduleMediator;
class MapComponent;
class NVStorageComponent;
class AIComponent;
class ObjectPoseConfirmer;
class CubeLightComponent;
class BodyLightComponent;
class CubeAccelComponent;
class CubeCommsComponent;
class RobotGyroDriftDetector;
class DockingComponent;
class CarryingComponent;
class CliffSensorComponent;
class ProxSensorComponent;
class TouchSensorComponent;
class AnimationComponent;
class RobotStateHistory;
class MoodManager;
class InventoryComponent;
class ProgressionUnlockComponent;
class BlockTapFilterComponent;
class RobotToEngineImplMessaging;
class RobotIdleTimeoutComponent;
class MicDirectionHistory;

} // namespace Cozmo

// Template specializations mapping enums from the _fwd.h file to the class forward declarations above
template<>
void GetComponentIDForType<Cozmo::RobotComponentID, Cozmo::ContextWrapper>(Cozmo::RobotComponentID& enumToSet){enumToSet =  Cozmo::RobotComponentID::CozmoContext;}
template<>
void GetComponentIDForType<Cozmo::RobotComponentID, Cozmo::BlockWorld>(Cozmo::RobotComponentID& enumToSet){enumToSet =  Cozmo::RobotComponentID::BlockWorld;}
template<>
void GetComponentIDForType<Cozmo::RobotComponentID, Cozmo::FaceWorld>(Cozmo::RobotComponentID& enumToSet){enumToSet =  Cozmo::RobotComponentID::FaceWorld;}
template<>
void GetComponentIDForType<Cozmo::RobotComponentID, Cozmo::PetWorld>(Cozmo::RobotComponentID& enumToSet){enumToSet =  Cozmo::RobotComponentID::PetWorld;}
template<>
void GetComponentIDForType<Cozmo::RobotComponentID, Cozmo::PublicStateBroadcaster>(Cozmo::RobotComponentID& enumToSet){enumToSet =  Cozmo::RobotComponentID::PublicStateBroadcaster;}
template<>
void GetComponentIDForType<Cozmo::RobotComponentID, Cozmo::Audio::EngineRobotAudioClient>(Cozmo::RobotComponentID& enumToSet){enumToSet =  Cozmo::RobotComponentID::EngineAudioClient;}
template<>
void GetComponentIDForType<Cozmo::RobotComponentID, Cozmo::PathComponent>(Cozmo::RobotComponentID& enumToSet){enumToSet =  Cozmo::RobotComponentID::PathPlanning;}
template<>
void GetComponentIDForType<Cozmo::RobotComponentID, Cozmo::DrivingAnimationHandler>(Cozmo::RobotComponentID& enumToSet){enumToSet =  Cozmo::RobotComponentID::DrivingAnimationHandler;}
template<>
void GetComponentIDForType<Cozmo::RobotComponentID, Cozmo::ActionList>(Cozmo::RobotComponentID& enumToSet){enumToSet =  Cozmo::RobotComponentID::ActionList;}
template<>
void GetComponentIDForType<Cozmo::RobotComponentID, Cozmo::MovementComponent>(Cozmo::RobotComponentID& enumToSet){enumToSet =  Cozmo::RobotComponentID::Movement;}
template<>
void GetComponentIDForType<Cozmo::RobotComponentID, Cozmo::VisionComponent>(Cozmo::RobotComponentID& enumToSet){enumToSet =  Cozmo::RobotComponentID::Vision;}
template<>
void GetComponentIDForType<Cozmo::RobotComponentID, Cozmo::VisionScheduleMediator>(Cozmo::RobotComponentID& enumToSet){enumToSet =  Cozmo::RobotComponentID::VisionScheduleMediator;}
template<>
void GetComponentIDForType<Cozmo::RobotComponentID, Cozmo::MapComponent>(Cozmo::RobotComponentID& enumToSet){enumToSet =  Cozmo::RobotComponentID::Map;}
template<>
void GetComponentIDForType<Cozmo::RobotComponentID, Cozmo::NVStorageComponent>(Cozmo::RobotComponentID& enumToSet){enumToSet =  Cozmo::RobotComponentID::NVStorage;}
template<>
void GetComponentIDForType<Cozmo::RobotComponentID, Cozmo::AIComponent>(Cozmo::RobotComponentID& enumToSet){enumToSet =  Cozmo::RobotComponentID::AIComponent;}
template<>
void GetComponentIDForType<Cozmo::RobotComponentID, Cozmo::ObjectPoseConfirmer>(Cozmo::RobotComponentID& enumToSet){enumToSet =  Cozmo::RobotComponentID::ObjectPoseConfirmer;}
template<>
void GetComponentIDForType<Cozmo::RobotComponentID, Cozmo::CubeLightComponent>(Cozmo::RobotComponentID& enumToSet){enumToSet =  Cozmo::RobotComponentID::CubeLights;}
template<>
void GetComponentIDForType<Cozmo::RobotComponentID, Cozmo::BodyLightComponent>(Cozmo::RobotComponentID& enumToSet){enumToSet =  Cozmo::RobotComponentID::BodyLights;}
template<>
void GetComponentIDForType<Cozmo::RobotComponentID, Cozmo::CubeAccelComponent>(Cozmo::RobotComponentID& enumToSet){enumToSet =  Cozmo::RobotComponentID::CubeAccel;}
template<>
void GetComponentIDForType<Cozmo::RobotComponentID, Cozmo::CubeCommsComponent>(Cozmo::RobotComponentID& enumToSet){enumToSet =  Cozmo::RobotComponentID::CubeComms;}
template<>
void GetComponentIDForType<Cozmo::RobotComponentID, Cozmo::RobotGyroDriftDetector>(Cozmo::RobotComponentID& enumToSet){enumToSet =  Cozmo::RobotComponentID::GyroDriftDetector;}
template<>
void GetComponentIDForType<Cozmo::RobotComponentID, Cozmo::DockingComponent>(Cozmo::RobotComponentID& enumToSet){enumToSet =  Cozmo::RobotComponentID::Docking;}
template<>
void GetComponentIDForType<Cozmo::RobotComponentID, Cozmo::CarryingComponent>(Cozmo::RobotComponentID& enumToSet){enumToSet =  Cozmo::RobotComponentID::Carrying;}
template<>
void GetComponentIDForType<Cozmo::RobotComponentID, Cozmo::CliffSensorComponent>(Cozmo::RobotComponentID& enumToSet){enumToSet =  Cozmo::RobotComponentID::CliffSensor;}
template<>
void GetComponentIDForType<Cozmo::RobotComponentID, Cozmo::ProxSensorComponent>(Cozmo::RobotComponentID& enumToSet){enumToSet =  Cozmo::RobotComponentID::ProxSensor;}
template<>
void GetComponentIDForType<Cozmo::RobotComponentID, Cozmo::TouchSensorComponent>(Cozmo::RobotComponentID& enumToSet){enumToSet =  Cozmo::RobotComponentID::TouchSensor;}
template<>
void GetComponentIDForType<Cozmo::RobotComponentID, Cozmo::AnimationComponent>(Cozmo::RobotComponentID& enumToSet){enumToSet =  Cozmo::RobotComponentID::Animation;}
template<>
void GetComponentIDForType<Cozmo::RobotComponentID, Cozmo::RobotStateHistory>(Cozmo::RobotComponentID& enumToSet){enumToSet =  Cozmo::RobotComponentID::StateHistory;}
template<>
void GetComponentIDForType<Cozmo::RobotComponentID, Cozmo::MoodManager>(Cozmo::RobotComponentID& enumToSet){enumToSet =  Cozmo::RobotComponentID::MoodManager;}
template<>
void GetComponentIDForType<Cozmo::RobotComponentID, Cozmo::InventoryComponent>(Cozmo::RobotComponentID& enumToSet){enumToSet =  Cozmo::RobotComponentID::Inventory;}
template<>
void GetComponentIDForType<Cozmo::RobotComponentID, Cozmo::ProgressionUnlockComponent>(Cozmo::RobotComponentID& enumToSet){enumToSet =  Cozmo::RobotComponentID::ProgressionUnlock;}
template<>
void GetComponentIDForType<Cozmo::RobotComponentID, Cozmo::BlockTapFilterComponent>(Cozmo::RobotComponentID& enumToSet){enumToSet =  Cozmo::RobotComponentID::BlockTapFilter;}
template<>
void GetComponentIDForType<Cozmo::RobotComponentID, Cozmo::RobotToEngineImplMessaging>(Cozmo::RobotComponentID& enumToSet){enumToSet =  Cozmo::RobotComponentID::RobotToEngineImplMessaging;}
template<>
void GetComponentIDForType<Cozmo::RobotComponentID, Cozmo::RobotIdleTimeoutComponent>(Cozmo::RobotComponentID& enumToSet){enumToSet =  Cozmo::RobotComponentID::RobotIdleTimeout;}
template<>
void GetComponentIDForType<Cozmo::RobotComponentID, Cozmo::MicDirectionHistory>(Cozmo::RobotComponentID& enumToSet){enumToSet =  Cozmo::RobotComponentID::MicDirectionHistory;}


} // namespace Anki
