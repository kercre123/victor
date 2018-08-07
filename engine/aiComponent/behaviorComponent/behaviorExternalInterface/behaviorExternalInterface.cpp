/**
* File: behaviorExternalInterface.cpp
*
* Author: Kevin M. Karol
* Created: 08/30/17
*
* Description: Interface that behaviors use to interact with the rest of
* the Cozmo system
*
* Copyright: Anki, Inc. 2017
*
**/


#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"

#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/delegationComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
#include "engine/components/mics/micComponent.h"
#include "engine/components/publicStateBroadcaster.h"
#include "engine/components/variableSnapshot/variableSnapshotComponent.h"
#include "engine/cozmoContext.h"
#include "engine/externalInterface/externalInterface.h"
#include "engine/moodSystem/moodManager.h"
#include "engine/robot.h"


namespace Anki {
namespace Vector {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const BEIComponentWrapper& BehaviorExternalInterface::GetComponentWrapper(BEIComponentID componentID) const
{
  ANKI_VERIFY(_arrayWrapper != nullptr,
              "BehaviorExternalInterface.GetComponentWrapper.NullArray",
              ""); 
  const BEIComponentWrapper& wrapper = _arrayWrapper->_array.GetComponent(componentID);
  return wrapper;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorExternalInterface::~BehaviorExternalInterface()
{

}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorExternalInterface::InitDependent(Robot* robot, const BCCompMap& dependentComps)
{
  auto* aiComponent            = dependentComps.GetComponentPtr<AIComponent>();
  auto* behaviorContainer      = dependentComps.GetComponentPtr<BehaviorContainer>();
  auto* behaviorEventComponent = dependentComps.GetComponentPtr<BehaviorEventComponent>();
  auto* behaviorTimers         = dependentComps.GetComponentPtr<BehaviorTimerManager>();
  auto* blockWorld             = dependentComps.GetComponentPtr<BlockWorld>();
  auto* delegationComponent    = dependentComps.GetComponentPtr<DelegationComponent>();
  auto* faceWorld              = dependentComps.GetComponentPtr<FaceWorld>();
  auto* robotInfo              = dependentComps.GetComponentPtr<BEIRobotInfo>();

  Init(aiComponent,
       robot->GetComponentPtr<AnimationComponent>(),
       robot->GetComponentPtr<BeatDetectorComponent>(),
       behaviorContainer,
       behaviorEventComponent,
       behaviorTimers,
       blockWorld,
       robot->GetComponentPtr<BackpackLightComponent>(),
       robot->GetComponentPtr<CubeAccelComponent>(), 
       robot->GetComponentPtr<CubeCommsComponent>(), 
       robot->GetComponentPtr<CubeConnectionCoordinator>(),
       robot->GetComponentPtr<CubeInteractionTracker>(),
       robot->GetComponentPtr<CubeLightComponent>(),
       robot->GetComponentPtr<CliffSensorComponent>(),
       delegationComponent,
       faceWorld,
       robot->GetComponentPtr<HabitatDetectorComponent>(),
       robot->GetComponentPtr<MapComponent>(),
       robot->GetComponentPtr<MicComponent>(),
       robot->GetComponentPtr<MoodManager>(),
       robot->GetComponentPtr<MovementComponent>(),
       robot->GetComponentPtr<ObjectPoseConfirmer>(),
       robot->GetComponentPtr<PetWorld>(),
       robot->GetComponentPtr<PhotographyManager>(),
       robot->GetComponentPtr<ProxSensorComponent>(),
       robot->GetComponentPtr<PublicStateBroadcaster>(),
       robot->GetComponentPtr<SDKComponent>(),
       robot->GetAudioClient(),
       robotInfo,
       robot->GetComponentPtr<DataAccessorComponent>(),
       robot->GetComponentPtr<TextToSpeechCoordinator>(),
       robot->GetComponentPtr<TouchSensorComponent>(),
       robot->GetComponentPtr<VariableSnapshotComponent>(),
       robot->GetComponentPtr<VisionComponent>(),
       robot->GetComponentPtr<VisionScheduleMediator>(),
       robot->GetComponentPtr<SettingsManager>());
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorExternalInterface::Init(AIComponent*                   aiComponent,
                                     AnimationComponent*            animationComponent,
                                     BeatDetectorComponent*         beatDetectorComponent,
                                     BehaviorContainer*             behaviorContainer,
                                     BehaviorEventComponent*        behaviorEventComponent,
                                     BehaviorTimerManager*          behaviorTimers,
                                     BlockWorld*                    blockWorld,
                                     BackpackLightComponent*        backpackLightComponent,
                                     CubeAccelComponent*            cubeAccelComponent,
                                     CubeCommsComponent*            cubeCommsComponent,
                                     CubeConnectionCoordinator*     cubeConnectionCoordinator,
                                     CubeInteractionTracker*        cubeInteractionTracker,
                                     CubeLightComponent*            cubeLightComponent,
                                     CliffSensorComponent*          cliffSensorComponent,
                                     DelegationComponent*           delegationComponent,
                                     FaceWorld*                     faceWorld,
                                     HabitatDetectorComponent*      habitatDetectorComponent,
                                     MapComponent*                  mapComponent,
                                     MicComponent*                  micComponent,
                                     MoodManager*                   moodManager,
                                     MovementComponent*             movementComponent,
                                     ObjectPoseConfirmer*           objectPoseConfirmer,
                                     PetWorld*                      petWorld,
                                     PhotographyManager*            photographyManager,
                                     ProxSensorComponent*           proxSensor,
                                     PublicStateBroadcaster*        publicStateBroadcaster,
                                     SDKComponent*                  sdkComponent,
                                     Audio::EngineRobotAudioClient* robotAudioClient,
                                     BEIRobotInfo*                  robotInfo,
                                     DataAccessorComponent*         dataAccessor,
                                     TextToSpeechCoordinator*       textToSpeechCoordinator,
                                     TouchSensorComponent*          touchSensorComponent,
                                     VariableSnapshotComponent*     variableSnapshotComponent,
                                     VisionComponent*               visionComponent,
                                     VisionScheduleMediator*        visionScheduleMediator,
                                     SettingsManager*               settingsManager)
{
  _arrayWrapper = std::make_unique<CompArrayWrapper>(aiComponent,
                                                     animationComponent,
                                                     beatDetectorComponent,
                                                     behaviorContainer,
                                                     behaviorEventComponent,
                                                     behaviorTimers,
                                                     blockWorld,
                                                     backpackLightComponent,
                                                     cubeAccelComponent,
                                                     cubeCommsComponent,
                                                     cubeConnectionCoordinator,
                                                     cubeInteractionTracker,
                                                     cubeLightComponent,
                                                     cliffSensorComponent,
                                                     delegationComponent,
                                                     faceWorld,
                                                     habitatDetectorComponent,
                                                     mapComponent,
                                                     micComponent,
                                                     moodManager,
                                                     movementComponent,
                                                     objectPoseConfirmer,
                                                     petWorld,
                                                     photographyManager,
                                                     proxSensor,
                                                     publicStateBroadcaster,
                                                     sdkComponent,
                                                     robotAudioClient,
                                                     robotInfo,
                                                     dataAccessor,
                                                     textToSpeechCoordinator,
                                                     touchSensorComponent,
                                                     variableSnapshotComponent,
                                                     visionComponent,
                                                     visionScheduleMediator,
                                                     settingsManager);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
OffTreadsState BehaviorExternalInterface::GetOffTreadsState() const
{
  return GetComponentWrapper(BEIComponentID::RobotInfo).GetComponent<BEIRobotInfo>().GetOffTreadsState();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Util::RandomGenerator& BehaviorExternalInterface::GetRNG()
{
  return GetComponentWrapper(BEIComponentID::RobotInfo).GetComponent<BEIRobotInfo>().GetRNG();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorExternalInterface::CompArrayWrapper::CompArrayWrapper(AIComponent*                   aiComponent,
                                                              AnimationComponent*            animationComponent,
                                                              BeatDetectorComponent*         beatDetectorComponent,
                                                              BehaviorContainer*             behaviorContainer,
                                                              BehaviorEventComponent*        behaviorEventComponent,
                                                              BehaviorTimerManager*          behaviorTimers,
                                                              BlockWorld*                    blockWorld,
                                                              BackpackLightComponent*        backpackLightComponent,
                                                              CubeAccelComponent*            cubeAccelComponent,
                                                              CubeCommsComponent*            cubeCommsComponent,
                                                              CubeConnectionCoordinator*     cubeConnectionCoordinator,
                                                              CubeInteractionTracker*        cubeInteractionTracker,
                                                              CubeLightComponent*            cubeLightComponent,
                                                              CliffSensorComponent*          cliffSensorComponent,
                                                              DelegationComponent*           delegationComponent,
                                                              FaceWorld*                     faceWorld,
                                                              HabitatDetectorComponent*      habitatDetectorComponent,
                                                              MapComponent*                  mapComponent,
                                                              MicComponent*                  micComponent,
                                                              MoodManager*                   moodManager,
                                                              MovementComponent*             movementComponent,
                                                              ObjectPoseConfirmer*           objectPoseConfirmer,
                                                              PetWorld*                      petWorld,
                                                              PhotographyManager*            photographyManager,
                                                              ProxSensorComponent*           proxSensor,
                                                              PublicStateBroadcaster*        publicStateBroadcaster,
                                                              SDKComponent*                  sdkComponent,
                                                              Audio::EngineRobotAudioClient* robotAudioClient,
                                                              BEIRobotInfo*                  robotInfo,
                                                              DataAccessorComponent*         dataAccessor,
                                                              TextToSpeechCoordinator*       textToSpeechCoordinator,
                                                              TouchSensorComponent*          touchSensorComponent,
                                                              VariableSnapshotComponent*     variableSnapshotComponent,
                                                              VisionComponent*               visionComponent,
                                                              VisionScheduleMediator*        visionScheduleMediator,
                                                              SettingsManager*               settingsManager)
: _array({
    {BEIComponentID::AIComponent,               BEIComponentWrapper(aiComponent)},
    {BEIComponentID::Animation,                 BEIComponentWrapper(animationComponent)},
    {BEIComponentID::BeatDetector,              BEIComponentWrapper(beatDetectorComponent)},
    {BEIComponentID::BehaviorContainer,         BEIComponentWrapper(behaviorContainer)},
    {BEIComponentID::BehaviorEvent,             BEIComponentWrapper(behaviorEventComponent)},
    {BEIComponentID::BehaviorTimerManager,      BEIComponentWrapper(behaviorTimers)},
    {BEIComponentID::BlockWorld,                BEIComponentWrapper(blockWorld)},
    {BEIComponentID::BackpackLightComponent,    BEIComponentWrapper(backpackLightComponent)},
    {BEIComponentID::CubeAccel,                 BEIComponentWrapper(cubeAccelComponent)},
    {BEIComponentID::CubeComms,                 BEIComponentWrapper(cubeCommsComponent)},
    {BEIComponentID::CubeConnectionCoordinator, BEIComponentWrapper(cubeConnectionCoordinator)},
    {BEIComponentID::CubeInteractionTracker,    BEIComponentWrapper(cubeInteractionTracker)},
    {BEIComponentID::CubeLight,                 BEIComponentWrapper(cubeLightComponent)},
    {BEIComponentID::CliffSensor,               BEIComponentWrapper(cliffSensorComponent)},
    {BEIComponentID::DataAccessor,              BEIComponentWrapper(dataAccessor)},
    {BEIComponentID::Delegation,                BEIComponentWrapper(delegationComponent)},
    {BEIComponentID::FaceWorld,                 BEIComponentWrapper(faceWorld)},
    {BEIComponentID::HabitatDetector,           BEIComponentWrapper(habitatDetectorComponent)},
    {BEIComponentID::Map,                       BEIComponentWrapper(mapComponent)},
    {BEIComponentID::MicComponent,              BEIComponentWrapper(micComponent)},
    {BEIComponentID::MoodManager,               BEIComponentWrapper(moodManager)},
    {BEIComponentID::MovementComponent,         BEIComponentWrapper(movementComponent)},
    {BEIComponentID::ObjectPoseConfirmer,       BEIComponentWrapper(objectPoseConfirmer)},
    {BEIComponentID::PetWorld,                  BEIComponentWrapper(petWorld)},
    {BEIComponentID::PhotographyManager,        BEIComponentWrapper(photographyManager)},
    {BEIComponentID::ProxSensor,                BEIComponentWrapper(proxSensor)},
    {BEIComponentID::PublicStateBroadcaster,    BEIComponentWrapper(publicStateBroadcaster)},
    {BEIComponentID::SDK,                       BEIComponentWrapper(sdkComponent)},
    {BEIComponentID::SettingsManager,           BEIComponentWrapper(settingsManager)},
    {BEIComponentID::RobotAudioClient,          BEIComponentWrapper(robotAudioClient)},
    {BEIComponentID::RobotInfo,                 BEIComponentWrapper(robotInfo)},
    {BEIComponentID::TextToSpeechCoordinator,   BEIComponentWrapper(textToSpeechCoordinator)},
    {BEIComponentID::TouchSensor,               BEIComponentWrapper(touchSensorComponent)},
    {BEIComponentID::VariableSnapshotComponent, BEIComponentWrapper(variableSnapshotComponent)},
    {BEIComponentID::Vision,                    BEIComponentWrapper(visionComponent)},
    {BEIComponentID::VisionScheduleMediator,    BEIComponentWrapper(visionScheduleMediator)}
}){}

} // namespace Vector
} // namespace Anki
