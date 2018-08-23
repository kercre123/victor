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
namespace Vector {

// Forward declarations
namespace Audio{
class EngineRobotAudioClient;
}

class AppCubeConnectionSubscriber;
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
class BackpackLightComponent;
class CubeAccelComponent;
class CubeBatteryComponent;
class CubeCommsComponent;
class CubeConnectionCoordinator;
class CubeInteractionTracker;
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
class BlockTapFilterComponent;
class RobotToEngineImplMessaging;
class RobotIdleTimeoutComponent;
class MicComponent;
class BatteryComponent;
class FullRobotPose;
class DataAccessorComponent;
class BeatDetectorComponent;
class HabitatDetectorComponent;
class TextToSpeechCoordinator;
class SDKComponent;
class PhotographyManager;
class RobotStatsTracker;
class SettingsCommManager;
class SettingsManager;
class VariableSnapshotComponent;
class PowerStateManager;
class JdocsManager;
class RobotExternalRequestComponent;
class AccountSettingsManager;
class UserEntitlementsManager;

} // namespace Vector

// Template specializations mapping enums from the _fwd.h file to the class forward declarations above
LINK_COMPONENT_TYPE_TO_ENUM(AppCubeConnectionSubscriber,   RobotComponentID, AppCubeConnectionSubscriber)
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
LINK_COMPONENT_TYPE_TO_ENUM(BackpackLightComponent,        RobotComponentID, BackpackLights)
LINK_COMPONENT_TYPE_TO_ENUM(CubeAccelComponent,            RobotComponentID, CubeAccel)
LINK_COMPONENT_TYPE_TO_ENUM(CubeBatteryComponent,          RobotComponentID, CubeBattery)
LINK_COMPONENT_TYPE_TO_ENUM(CubeCommsComponent,            RobotComponentID, CubeComms)
LINK_COMPONENT_TYPE_TO_ENUM(CubeConnectionCoordinator,     RobotComponentID, CubeConnectionCoordinator)
LINK_COMPONENT_TYPE_TO_ENUM(CubeInteractionTracker,        RobotComponentID, CubeInteractionTracker)
LINK_COMPONENT_TYPE_TO_ENUM(RobotGyroDriftDetector,        RobotComponentID, GyroDriftDetector)
LINK_COMPONENT_TYPE_TO_ENUM(HabitatDetectorComponent,      RobotComponentID, HabitatDetector)
LINK_COMPONENT_TYPE_TO_ENUM(DockingComponent,              RobotComponentID, Docking)
LINK_COMPONENT_TYPE_TO_ENUM(CarryingComponent,             RobotComponentID, Carrying)
LINK_COMPONENT_TYPE_TO_ENUM(CliffSensorComponent,          RobotComponentID, CliffSensor)
LINK_COMPONENT_TYPE_TO_ENUM(ProxSensorComponent,           RobotComponentID, ProxSensor)
LINK_COMPONENT_TYPE_TO_ENUM(TouchSensorComponent,          RobotComponentID, TouchSensor)
LINK_COMPONENT_TYPE_TO_ENUM(AnimationComponent,            RobotComponentID, Animation)
LINK_COMPONENT_TYPE_TO_ENUM(RobotStateHistory,             RobotComponentID, StateHistory)
LINK_COMPONENT_TYPE_TO_ENUM(MoodManager,                   RobotComponentID, MoodManager)
LINK_COMPONENT_TYPE_TO_ENUM(StimulationFaceDisplay,        RobotComponentID, StimulationFaceDisplay)
LINK_COMPONENT_TYPE_TO_ENUM(BlockTapFilterComponent,       RobotComponentID, BlockTapFilter)
LINK_COMPONENT_TYPE_TO_ENUM(RobotToEngineImplMessaging,    RobotComponentID, RobotToEngineImplMessaging)
LINK_COMPONENT_TYPE_TO_ENUM(RobotIdleTimeoutComponent,     RobotComponentID, RobotIdleTimeout)
LINK_COMPONENT_TYPE_TO_ENUM(MicComponent,                  RobotComponentID, MicComponent)
LINK_COMPONENT_TYPE_TO_ENUM(BatteryComponent,              RobotComponentID, Battery)
LINK_COMPONENT_TYPE_TO_ENUM(FullRobotPose,                 RobotComponentID, FullRobotPose)
LINK_COMPONENT_TYPE_TO_ENUM(DataAccessorComponent,         RobotComponentID, DataAccessor)
LINK_COMPONENT_TYPE_TO_ENUM(BeatDetectorComponent,         RobotComponentID, BeatDetector)
LINK_COMPONENT_TYPE_TO_ENUM(TextToSpeechCoordinator,       RobotComponentID, TextToSpeechCoordinator)
LINK_COMPONENT_TYPE_TO_ENUM(SDKComponent,                  RobotComponentID, SDK)
LINK_COMPONENT_TYPE_TO_ENUM(PhotographyManager,            RobotComponentID, PhotographyManager)
LINK_COMPONENT_TYPE_TO_ENUM(SettingsCommManager,           RobotComponentID, SettingsCommManager)
LINK_COMPONENT_TYPE_TO_ENUM(SettingsManager,               RobotComponentID, SettingsManager)
LINK_COMPONENT_TYPE_TO_ENUM(RobotStatsTracker,             RobotComponentID, RobotStatsTracker)
LINK_COMPONENT_TYPE_TO_ENUM(VariableSnapshotComponent,     RobotComponentID, VariableSnapshotComponent)
LINK_COMPONENT_TYPE_TO_ENUM(PowerStateManager,             RobotComponentID, PowerStateManager)
LINK_COMPONENT_TYPE_TO_ENUM(JdocsManager,                  RobotComponentID, JdocsManager)
LINK_COMPONENT_TYPE_TO_ENUM(RobotExternalRequestComponent, RobotComponentID, RobotExternalRequestComponent)
LINK_COMPONENT_TYPE_TO_ENUM(AccountSettingsManager,        RobotComponentID, AccountSettingsManager)
LINK_COMPONENT_TYPE_TO_ENUM(UserEntitlementsManager,       RobotComponentID, UserEntitlementsManager)

// Translate entity into string
template<>
std::string GetEntityNameForEnumType<Vector::RobotComponentID>(){ return "RobotComponents"; }

template<>
std::string GetComponentStringForID<Vector::RobotComponentID>(Vector::RobotComponentID enumID)
{
  switch(enumID){
    case Vector::RobotComponentID::AccountSettingsManager:       { return "AccountSettingsManager";}
    case Vector::RobotComponentID::AIComponent:                  { return "AIComponent";}
    case Vector::RobotComponentID::ActionList:                   { return "ActionList";}
    case Vector::RobotComponentID::Animation:                    { return "Animation";}
    case Vector::RobotComponentID::AppCubeConnectionSubscriber:  { return "AppCubeConnectionSubscriber";}
    case Vector::RobotComponentID::Battery:                      { return "Battery";}
    case Vector::RobotComponentID::BeatDetector:                 { return "BeatDetector";}
    case Vector::RobotComponentID::BlockTapFilter:               { return "BlockTapFilter";}
    case Vector::RobotComponentID::BlockWorld:                   { return "BlockWorld";}
    case Vector::RobotComponentID::BackpackLights:               { return "BackpackLights";}
    case Vector::RobotComponentID::Carrying:                     { return "Carrying";}
    case Vector::RobotComponentID::CliffSensor:                  { return "CliffSensor";}
    case Vector::RobotComponentID::CozmoContextWrapper:          { return "CozmoContextWrapper";}
    case Vector::RobotComponentID::CubeAccel:                    { return "CubeAccel";}
    case Vector::RobotComponentID::CubeBattery:                  { return "CubeBattery";}
    case Vector::RobotComponentID::CubeComms:                    { return "CubeComms";}
    case Vector::RobotComponentID::CubeConnectionCoordinator:    { return "CubeConnectionCoordinator";}
    case Vector::RobotComponentID::CubeInteractionTracker:       { return "CubeInteractionTracker";}
    case Vector::RobotComponentID::CubeLights:                   { return "CubeLights";}
    case Vector::RobotComponentID::DataAccessor:                 { return "DataAccessor";}
    case Vector::RobotComponentID::Docking:                      { return "Docking";}
    case Vector::RobotComponentID::DrivingAnimationHandler:      { return "DrivingAnimationHandler";}
    case Vector::RobotComponentID::EngineAudioClient:            { return "EngineAudioClient";}
    case Vector::RobotComponentID::FaceWorld:                    { return "FaceWorld";}
    case Vector::RobotComponentID::GyroDriftDetector:            { return "GyroDriftDetector";}
    case Vector::RobotComponentID::HabitatDetector:              { return "HabitatDetector";}
    case Vector::RobotComponentID::JdocsManager:                 { return "JdocsManager";}
    case Vector::RobotComponentID::Map:                          { return "Map";}
    case Vector::RobotComponentID::MicComponent:                 { return "MicComponent"; }
    case Vector::RobotComponentID::MoodManager:                  { return "MoodManager";}
    case Vector::RobotComponentID::StimulationFaceDisplay:       { return "StimulationFaceDisplay";}
    case Vector::RobotComponentID::Movement:                     { return "Movement";}
    case Vector::RobotComponentID::NVStorage:                    { return "NVStorage";}
    case Vector::RobotComponentID::ObjectPoseConfirmer:          { return "ObjectPoseConfirmer";}
    case Vector::RobotComponentID::PathPlanning:                 { return "PathPlanning";}
    case Vector::RobotComponentID::PetWorld:                     { return "PetWorld";}
    case Vector::RobotComponentID::PhotographyManager:           { return "PhotographyManager";}
    case Vector::RobotComponentID::ProxSensor:                   { return "ProxSensor";}
    case Vector::RobotComponentID::PublicStateBroadcaster:       { return "PublicStateBroadcaster";}
    case Vector::RobotComponentID::SDK:                          { return "SDK";}
    case Vector::RobotComponentID::SettingsCommManager:          { return "SettingsCommManager";}
    case Vector::RobotComponentID::SettingsManager:              { return "SettingsManager";}
    case Vector::RobotComponentID::FullRobotPose:                { return "FullRobotPose";}
    case Vector::RobotComponentID::RobotIdleTimeout:             { return "RobotIdleTimeout";}
    case Vector::RobotComponentID::RobotStatsTracker:            { return "RobotStatsTracker";}      
    case Vector::RobotComponentID::RobotToEngineImplMessaging:   { return "RobotToEngineImplMessaging";}
    case Vector::RobotComponentID::StateHistory:                 { return "StateHistory";}
    case Vector::RobotComponentID::TextToSpeechCoordinator:      { return "TextToSpeechCoordinator";}
    case Vector::RobotComponentID::TouchSensor:                  { return "TouchSensor";}
    case Vector::RobotComponentID::UserEntitlementsManager:      { return "UserEntitlementsManager";}
    case Vector::RobotComponentID::VariableSnapshotComponent:    { return "VariableSnapshotComponent"; }
    case Vector::RobotComponentID::Vision:                       { return "Vision";}
    case Vector::RobotComponentID::VisionScheduleMediator:       { return "VisionScheduleMediator";}
    case Vector::RobotComponentID::PowerStateManager:            { return "PowerStateManager";}
    case Vector::RobotComponentID::RobotExternalRequestComponent:{ return "RobotExternalRequestComponent";}
    case Vector::RobotComponentID::Count:                        { return "Count";}
  }
}

// Translate enum values into strings

} // namespace Anki
