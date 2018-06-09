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
#include "engine/components/progressionUnlockComponent.h"
#include "engine/components/publicStateBroadcaster.h"
#include "engine/cozmoContext.h"
#include "engine/externalInterface/externalInterface.h"
#include "engine/moodSystem/moodManager.h"
#include "engine/robot.h"


namespace Anki {
namespace Cozmo {

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
void BehaviorExternalInterface::InitDependent(Robot* robot, const BCCompMap& dependentComponents)
{
  auto* aiComponent            = dependentComponents.GetBasePtr<AIComponent>();
  auto* behaviorContainer      = dependentComponents.GetBasePtr<BehaviorContainer>();
  auto* behaviorEventComponent = dependentComponents.GetBasePtr<BehaviorEventComponent>();
  auto* behaviorTimers         = dependentComponents.GetBasePtr<BehaviorTimerManager>();
  auto* blockWorld             = dependentComponents.GetBasePtr<BlockWorld>();
  auto* delegationComponent    = dependentComponents.GetBasePtr<DelegationComponent>();
  auto* faceWorld              = dependentComponents.GetBasePtr<FaceWorld>();
  auto* robotInfo              = dependentComponents.GetBasePtr<BEIRobotInfo>();

  Init(aiComponent,
       robot->GetComponentPtr<AnimationComponent>(),
       robot->GetComponentPtr<BeatDetectorComponent>(),
       behaviorContainer,
       behaviorEventComponent,
       behaviorTimers,
       blockWorld,
       robot->GetComponentPtr<BodyLightComponent>(),
       robot->GetComponentPtr<CubeAccelComponent>(), 
       robot->GetComponentPtr<CubeLightComponent>(),
       delegationComponent,
       faceWorld,
       robot->GetComponentPtr<MapComponent>(),
       robot->GetComponentPtr<MicComponent>(),
       robot->GetComponentPtr<MoodManager>(),
       robot->GetComponentPtr<MovementComponent>(),
       robot->GetComponentPtr<ObjectPoseConfirmer>(),
       robot->GetComponentPtr<PetWorld>(),
       robot->GetComponentPtr<PhotographyManager>(),
       robot->GetComponentPtr<ProgressionUnlockComponent>(),
       robot->GetComponentPtr<ProxSensorComponent>(),
       robot->GetComponentPtr<PublicStateBroadcaster>(),
       robot->GetComponentPtr<SDKComponent>(),
       robot->GetAudioClient(),
       robotInfo,
       robot->GetComponentPtr<DataAccessorComponent>(),
       robot->GetComponentPtr<TextToSpeechCoordinator>(),
       robot->GetComponentPtr<TouchSensorComponent>(),
       robot->GetComponentPtr<VisionComponent>(),
       robot->GetComponentPtr<VisionScheduleMediator>());
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorExternalInterface::Init(AIComponent*                   aiComponent,
                                     AnimationComponent*            animationComponent,
                                     BeatDetectorComponent*         beatDetectorComponent,
                                     BehaviorContainer*             behaviorContainer,
                                     BehaviorEventComponent*        behaviorEventComponent,
                                     BehaviorTimerManager*          behaviorTimers,
                                     BlockWorld*                    blockWorld,
                                     BodyLightComponent*            bodyLightComponent,
                                     CubeAccelComponent*            cubeAccelComponent,
                                     CubeLightComponent*            cubeLightComponent,
                                     DelegationComponent*           delegationComponent,
                                     FaceWorld*                     faceWorld,
                                     MapComponent*                  mapComponent,
                                     MicComponent*                  micComponent,
                                     MoodManager*                   moodManager,
                                     MovementComponent*             movementComponent,
                                     ObjectPoseConfirmer*           objectPoseConfirmer,
                                     PetWorld*                      petWorld,
                                     PhotographyManager*            photographyManager,
                                     ProgressionUnlockComponent*    progressionUnlockComponent,
                                     ProxSensorComponent*           proxSensor,
                                     PublicStateBroadcaster*        publicStateBroadcaster,
                                     SDKComponent*                  sdkComponent,
                                     Audio::EngineRobotAudioClient* robotAudioClient,
                                     BEIRobotInfo*                  robotInfo,
                                     DataAccessorComponent*         dataAccessor,
                                     TextToSpeechCoordinator*       textToSpeechCoordinator,
                                     TouchSensorComponent*          touchSensorComponent,
                                     VisionComponent*               visionComponent,
                                     VisionScheduleMediator*        visionScheduleMediator)
{
  _arrayWrapper = std::make_unique<CompArrayWrapper>(aiComponent,
                                                     animationComponent,
                                                     beatDetectorComponent,
                                                     behaviorContainer,
                                                     behaviorEventComponent,
                                                     behaviorTimers,
                                                     blockWorld,
                                                     bodyLightComponent,
                                                     cubeAccelComponent,
                                                     cubeLightComponent,
                                                     delegationComponent,
                                                     faceWorld,
                                                     mapComponent,
                                                     micComponent,
                                                     moodManager,
                                                     movementComponent,
                                                     objectPoseConfirmer,
                                                     petWorld,
                                                     photographyManager,
                                                     progressionUnlockComponent,
                                                     proxSensor,
                                                     publicStateBroadcaster,
                                                     sdkComponent,
                                                     robotAudioClient,
                                                     robotInfo,
                                                     dataAccessor,
                                                     textToSpeechCoordinator,
                                                     touchSensorComponent,
                                                     visionComponent,
                                                     visionScheduleMediator);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
OffTreadsState BehaviorExternalInterface::GetOffTreadsState() const
{
  return GetComponentWrapper(BEIComponentID::RobotInfo).GetValue<BEIRobotInfo>().GetOffTreadsState();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Util::RandomGenerator& BehaviorExternalInterface::GetRNG()
{
  return GetComponentWrapper(BEIComponentID::RobotInfo).GetValue<BEIRobotInfo>().GetRNG();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorExternalInterface::CompArrayWrapper::CompArrayWrapper(AIComponent*                   aiComponent,
                                                              AnimationComponent*            animationComponent,
                                                              BeatDetectorComponent*         beatDetectorComponent,
                                                              BehaviorContainer*             behaviorContainer,
                                                              BehaviorEventComponent*        behaviorEventComponent,
                                                              BehaviorTimerManager*          behaviorTimers,
                                                              BlockWorld*                    blockWorld,
                                                              BodyLightComponent*            bodyLightComponent,
                                                              CubeAccelComponent*            cubeAccelComponent,
                                                              CubeLightComponent*            cubeLightComponent,
                                                              DelegationComponent*           delegationComponent,
                                                              FaceWorld*                     faceWorld,
                                                              MapComponent*                  mapComponent,
                                                              MicComponent*                  micComponent,
                                                              MoodManager*                   moodManager,
                                                              MovementComponent*             movementComponent,
                                                              ObjectPoseConfirmer*           objectPoseConfirmer,
                                                              PetWorld*                      petWorld,
                                                              PhotographyManager*            photographyManager,
                                                              ProgressionUnlockComponent*    progressionUnlockComponent,
                                                              ProxSensorComponent*           proxSensor,
                                                              PublicStateBroadcaster*        publicStateBroadcaster,
                                                              SDKComponent*                  sdkComponent,
                                                              Audio::EngineRobotAudioClient* robotAudioClient,
                                                              BEIRobotInfo*                  robotInfo,
                                                              DataAccessorComponent*         dataAccessor,
                                                              TextToSpeechCoordinator*       textToSpeechCoordinator,
                                                              TouchSensorComponent*          touchSensorComponent,
                                                              VisionComponent*               visionComponent,
                                                              VisionScheduleMediator*        visionScheduleMediator)
: _array({
    {BEIComponentID::AIComponent,             BEIComponentWrapper(aiComponent)},
    {BEIComponentID::Animation,               BEIComponentWrapper(animationComponent)},
    {BEIComponentID::BeatDetector,            BEIComponentWrapper(beatDetectorComponent)},
    {BEIComponentID::BehaviorContainer,       BEIComponentWrapper(behaviorContainer)},
    {BEIComponentID::BehaviorEvent,           BEIComponentWrapper(behaviorEventComponent)},
    {BEIComponentID::BehaviorTimerManager,    BEIComponentWrapper(behaviorTimers)},
    {BEIComponentID::BlockWorld,              BEIComponentWrapper(blockWorld)},
    {BEIComponentID::BodyLightComponent,      BEIComponentWrapper(bodyLightComponent)},
    {BEIComponentID::CubeAccel,               BEIComponentWrapper(cubeAccelComponent)},
    {BEIComponentID::CubeLight,               BEIComponentWrapper(cubeLightComponent)},
    {BEIComponentID::DataAccessor,            BEIComponentWrapper(dataAccessor)},
    {BEIComponentID::Delegation,              BEIComponentWrapper(delegationComponent)},
    {BEIComponentID::FaceWorld,               BEIComponentWrapper(faceWorld)},
    {BEIComponentID::Map,                     BEIComponentWrapper(mapComponent)},
    {BEIComponentID::MicComponent,            BEIComponentWrapper(micComponent)},
    {BEIComponentID::MoodManager,             BEIComponentWrapper(moodManager)},
    {BEIComponentID::MovementComponent,       BEIComponentWrapper(movementComponent)},
    {BEIComponentID::ObjectPoseConfirmer,     BEIComponentWrapper(objectPoseConfirmer)},
    {BEIComponentID::PetWorld,                BEIComponentWrapper(petWorld)},
    {BEIComponentID::PhotographyManager,      BEIComponentWrapper(photographyManager)},
    {BEIComponentID::ProgressionUnlock,       BEIComponentWrapper(progressionUnlockComponent)},
    {BEIComponentID::ProxSensor,              BEIComponentWrapper(proxSensor)},
    {BEIComponentID::PublicStateBroadcaster,  BEIComponentWrapper(publicStateBroadcaster)},
    {BEIComponentID::SDK,                     BEIComponentWrapper(sdkComponent)},
    {BEIComponentID::RobotAudioClient,        BEIComponentWrapper(robotAudioClient)},
    {BEIComponentID::RobotInfo,               BEIComponentWrapper(robotInfo)},
    {BEIComponentID::TextToSpeechCoordinator, BEIComponentWrapper(textToSpeechCoordinator)},
    {BEIComponentID::TouchSensor,             BEIComponentWrapper(touchSensorComponent)},
    {BEIComponentID::Vision,                  BEIComponentWrapper(visionComponent)},
    {BEIComponentID::VisionScheduleMediator,  BEIComponentWrapper(visionScheduleMediator)}
}){}
  
} // namespace Cozmo
} // namespace Anki
