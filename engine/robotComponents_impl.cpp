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
#include "util/entityComponent/componentTypeEnumMap.h"

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
class StimulationFaceDisplay;
class InventoryComponent;
class ProgressionUnlockComponent;
class BlockTapFilterComponent;
class RobotToEngineImplMessaging;
class RobotIdleTimeoutComponent;
class MicComponent;
class BatteryComponent;
class FullRobotPose;
class DataAccessorComponent;

} // namespace Cozmo

// Template specializations mapping enums from the _fwd.h file to the class forward declarations above
LINK_COMPONENT_TYPE_TO_ENUM(ContextWrapper,                RobotComponentID, CozmoContextWrapper)
LINK_COMPONENT_TYPE_TO_ENUM(BlockWorld,                    RobotComponentID, BlockWorld)
LINK_COMPONENT_TYPE_TO_ENUM(FaceWorld,                     RobotComponentID, FaceWorld)
LINK_COMPONENT_TYPE_TO_ENUM(PetWorld,                      RobotComponentID, PetWorld)
LINK_COMPONENT_TYPE_TO_ENUM(PublicStateBroadcaster,        RobotComponentID, PublicStateBroadcaster)
LINK_COMPONENT_TYPE_TO_ENUM(Audio::EngineRobotAudioClient, RobotComponentID, EngineAudioClient)
LINK_COMPONENT_TYPE_TO_ENUM(PathComponent,                 RobotComponentID, PathPlanning)
LINK_COMPONENT_TYPE_TO_ENUM(DrivingAnimationHandler,       RobotComponentID, DrivingAnimationHandler)
LINK_COMPONENT_TYPE_TO_ENUM(ActionList,                    RobotComponentID, ActionList)
LINK_COMPONENT_TYPE_TO_ENUM(MovementComponent,             RobotComponentID, Movement)
LINK_COMPONENT_TYPE_TO_ENUM(VisionComponent,               RobotComponentID, Vision)
LINK_COMPONENT_TYPE_TO_ENUM(VisionScheduleMediator,        RobotComponentID, VisionScheduleMediator)
LINK_COMPONENT_TYPE_TO_ENUM(MapComponent,                  RobotComponentID, Map)
LINK_COMPONENT_TYPE_TO_ENUM(NVStorageComponent,            RobotComponentID, NVStorage)
LINK_COMPONENT_TYPE_TO_ENUM(AIComponent,                   RobotComponentID, AIComponent)
LINK_COMPONENT_TYPE_TO_ENUM(ObjectPoseConfirmer,           RobotComponentID, ObjectPoseConfirmer)
LINK_COMPONENT_TYPE_TO_ENUM(CubeLightComponent,            RobotComponentID, CubeLights)
LINK_COMPONENT_TYPE_TO_ENUM(BodyLightComponent,            RobotComponentID, BodyLights)
LINK_COMPONENT_TYPE_TO_ENUM(CubeAccelComponent,            RobotComponentID, CubeAccel)
LINK_COMPONENT_TYPE_TO_ENUM(CubeCommsComponent,            RobotComponentID, CubeComms)
LINK_COMPONENT_TYPE_TO_ENUM(RobotGyroDriftDetector,        RobotComponentID, GyroDriftDetector)
LINK_COMPONENT_TYPE_TO_ENUM(DockingComponent,              RobotComponentID, Docking)
LINK_COMPONENT_TYPE_TO_ENUM(CarryingComponent,             RobotComponentID, Carrying)
LINK_COMPONENT_TYPE_TO_ENUM(CliffSensorComponent,          RobotComponentID, CliffSensor)
LINK_COMPONENT_TYPE_TO_ENUM(ProxSensorComponent,           RobotComponentID, ProxSensor)
LINK_COMPONENT_TYPE_TO_ENUM(TouchSensorComponent,          RobotComponentID, TouchSensor)
LINK_COMPONENT_TYPE_TO_ENUM(AnimationComponent,            RobotComponentID, Animation)
LINK_COMPONENT_TYPE_TO_ENUM(RobotStateHistory,             RobotComponentID, StateHistory)
LINK_COMPONENT_TYPE_TO_ENUM(MoodManager,                   RobotComponentID, MoodManager)
LINK_COMPONENT_TYPE_TO_ENUM(StimulationFaceDisplay,        RobotComponentID, StimulationFaceDisplay)
LINK_COMPONENT_TYPE_TO_ENUM(InventoryComponent,            RobotComponentID, Inventory)
LINK_COMPONENT_TYPE_TO_ENUM(ProgressionUnlockComponent,    RobotComponentID, ProgressionUnlock)
LINK_COMPONENT_TYPE_TO_ENUM(BlockTapFilterComponent,       RobotComponentID, BlockTapFilter)
LINK_COMPONENT_TYPE_TO_ENUM(RobotToEngineImplMessaging,    RobotComponentID, RobotToEngineImplMessaging)
LINK_COMPONENT_TYPE_TO_ENUM(RobotIdleTimeoutComponent,     RobotComponentID, RobotIdleTimeout)
LINK_COMPONENT_TYPE_TO_ENUM(MicComponent,                  RobotComponentID, MicComponent)
LINK_COMPONENT_TYPE_TO_ENUM(BatteryComponent,              RobotComponentID, Battery)
LINK_COMPONENT_TYPE_TO_ENUM(FullRobotPose,                 RobotComponentID, FullRobotPose)
LINK_COMPONENT_TYPE_TO_ENUM(DataAccessorComponent,          RobotComponentID, DataAccessor)

// Translate entity into string
template<>
std::string GetEntityNameForEnumType<Cozmo::RobotComponentID>(){ return "RobotComponents"; }

template<>
std::string GetComponentStringForID<Cozmo::RobotComponentID>(Cozmo::RobotComponentID enumID)
{
  switch(enumID){
    case Cozmo::RobotComponentID::AIComponent:                { return "AIComponent";}
    case Cozmo::RobotComponentID::ActionList:                 { return "ActionList";}
    case Cozmo::RobotComponentID::Animation:                  { return "Animation";}
    case Cozmo::RobotComponentID::Battery:                    { return "Battery";}
    case Cozmo::RobotComponentID::BlockTapFilter:             { return "BlockTapFilter";}
    case Cozmo::RobotComponentID::BlockWorld:                 { return "BlockWorld";}
    case Cozmo::RobotComponentID::BodyLights:                 { return "BodyLights";}
    case Cozmo::RobotComponentID::Carrying:                   { return "Carrying";}
    case Cozmo::RobotComponentID::CliffSensor:                { return "CliffSensor";}
    case Cozmo::RobotComponentID::CozmoContextWrapper:        { return "CozmoContextWrapper";}
    case Cozmo::RobotComponentID::CubeAccel:                  { return "CubeAccel";}
    case Cozmo::RobotComponentID::CubeComms:                  { return "CubeComms";}
    case Cozmo::RobotComponentID::CubeLights:                 { return "CubeLights";}
    case Cozmo::RobotComponentID::DataAccessor:               { return "DataAccessor";}
    case Cozmo::RobotComponentID::Docking:                    { return "Docking";}
    case Cozmo::RobotComponentID::DrivingAnimationHandler:    { return "DrivingAnimationHandler";}
    case Cozmo::RobotComponentID::EngineAudioClient:          { return "EngineAudioClient";}
    case Cozmo::RobotComponentID::FaceWorld:                  { return "FaceWorld";}
    case Cozmo::RobotComponentID::GyroDriftDetector:          { return "GyroDriftDetector";}
    case Cozmo::RobotComponentID::Inventory:                  { return "Inventory";}
    case Cozmo::RobotComponentID::Map:                        { return "Map";}
    case Cozmo::RobotComponentID::MicComponent:               { return "MicComponent"; }
    case Cozmo::RobotComponentID::MoodManager:                { return "MoodManager";}
    case Cozmo::RobotComponentID::StimulationFaceDisplay:     { return "StimulationFaceDisplay";}
    case Cozmo::RobotComponentID::Movement:                   { return "Movement";}
    case Cozmo::RobotComponentID::NVStorage:                  { return "NVStorage";}
    case Cozmo::RobotComponentID::ObjectPoseConfirmer:        { return "ObjectPoseConfirmer";}
    case Cozmo::RobotComponentID::PathPlanning:               { return "PathPlanning";}
    case Cozmo::RobotComponentID::PetWorld:                   { return "PetWorld";}
    case Cozmo::RobotComponentID::ProgressionUnlock:          { return "ProgressionUnlock";}
    case Cozmo::RobotComponentID::ProxSensor:                 { return "ProxSensor";}
    case Cozmo::RobotComponentID::PublicStateBroadcaster:     { return "PublicStateBroadcaster";}
    case Cozmo::RobotComponentID::FullRobotPose:              { return "FullRobotPose";}
    case Cozmo::RobotComponentID::RobotIdleTimeout:           { return "RobotIdleTimeout";}
    case Cozmo::RobotComponentID::RobotToEngineImplMessaging: { return "RobotToEngineImplMessaging";}
    case Cozmo::RobotComponentID::StateHistory:               { return "StateHistory";}
    case Cozmo::RobotComponentID::TouchSensor:                { return "TouchSensor";}
    case Cozmo::RobotComponentID::Vision:                     { return "Vision";}
    case Cozmo::RobotComponentID::VisionScheduleMediator:     { return "VisionScheduleMediator";}
    case Cozmo::RobotComponentID::Count:                      { return "Count";}
  }
}

// Translate enum values into strings

} // namespace Anki
