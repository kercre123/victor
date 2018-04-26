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
  auto& aiComponent            = dependentComponents.GetValue<AIComponent>();
  auto& behaviorContainer      = dependentComponents.GetValue<BehaviorContainer>();
  auto& behaviorEventComponent = dependentComponents.GetValue<BehaviorEventComponent>();
  auto& behaviorTimers         = dependentComponents.GetValue<BehaviorTimerManager>();
  auto& blockWorld             = dependentComponents.GetValue<BlockWorld>();
  auto& delegationComponent    = dependentComponents.GetValue<DelegationComponent>();
  auto& faceWorld              = dependentComponents.GetValue<FaceWorld>();
  auto& robotInfo              = dependentComponents.GetValue<BEIRobotInfo>();

  Init(&aiComponent,
       &robot->GetAnimationComponent(),
       &behaviorContainer,
       &behaviorEventComponent,
       &behaviorTimers,
       &blockWorld,
       &robot->GetBodyLightComponent(),
       &robot->GetCubeAccelComponent(), 
       &robot->GetCubeLightComponent(),
       &delegationComponent,
       &faceWorld,
       &robot->GetMapComponent(),
       &robot->GetMicComponent(),
       &robot->GetMoodManager(),
       &robot->GetMoveComponent(),
       &robot->GetObjectPoseConfirmer(),
       &robot->GetPetWorld(),
       &robot->GetProgressionUnlockComponent(),
       &robot->GetProxSensorComponent(),
       &robot->GetPublicStateBroadcaster(),
       robot->GetAudioClient(),
       &robotInfo,
       robot->GetComponentPtr<SpriteCacheComponent>(),
       &robot->GetTouchSensorComponent(),
       &robot->GetVisionComponent(),
       &robot->GetVisionScheduleMediator());
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorExternalInterface::Init(AIComponent*                   aiComponent,
                                     AnimationComponent*            animationComponent,
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
                                     ProgressionUnlockComponent*    progressionUnlockComponent,
                                     ProxSensorComponent*           proxSensor,
                                     PublicStateBroadcaster*        publicStateBroadcaster,
                                     Audio::EngineRobotAudioClient* robotAudioClient,
                                     BEIRobotInfo*                  robotInfo,
                                     SpriteCacheComponent*                   spriteCache,
                                     TouchSensorComponent*          touchSensorComponent,
                                     VisionComponent*               visionComponent,
                                     VisionScheduleMediator*        visionScheduleMediator)
{
  _arrayWrapper = std::make_unique<CompArrayWrapper>(aiComponent,
                                                     animationComponent,
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
                                                     progressionUnlockComponent,
                                                     proxSensor,
                                                     publicStateBroadcaster,
                                                     robotAudioClient,
                                                     robotInfo,
                                                     spriteCache,
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
                                                              ProgressionUnlockComponent*    progressionUnlockComponent,
                                                              ProxSensorComponent*           proxSensor,
                                                              PublicStateBroadcaster*        publicStateBroadcaster,
                                                              Audio::EngineRobotAudioClient* robotAudioClient,
                                                              BEIRobotInfo*                  robotInfo,
                                                              SpriteCacheComponent*                   spriteCache,
                                                              TouchSensorComponent*          touchSensorComponent,
                                                              VisionComponent*               visionComponent,
                                                              VisionScheduleMediator*        visionScheduleMediator)
: _array({
    {BEIComponentID::AIComponent,            BEIComponentWrapper(aiComponent)},
    {BEIComponentID::Animation,              BEIComponentWrapper(animationComponent)},
    {BEIComponentID::BehaviorContainer,      BEIComponentWrapper(behaviorContainer)},
    {BEIComponentID::BehaviorEvent,          BEIComponentWrapper(behaviorEventComponent)},
    {BEIComponentID::BehaviorTimerManager,   BEIComponentWrapper(behaviorTimers)},
    {BEIComponentID::BlockWorld,             BEIComponentWrapper(blockWorld)},
    {BEIComponentID::BodyLightComponent,     BEIComponentWrapper(bodyLightComponent)},
    {BEIComponentID::CubeAccel,              BEIComponentWrapper(cubeAccelComponent)},
    {BEIComponentID::CubeLight,              BEIComponentWrapper(cubeLightComponent)},
    {BEIComponentID::Delegation,             BEIComponentWrapper(delegationComponent)},
    {BEIComponentID::FaceWorld,              BEIComponentWrapper(faceWorld)},
    {BEIComponentID::Map,                    BEIComponentWrapper(mapComponent)},
    {BEIComponentID::MicComponent,           BEIComponentWrapper(micComponent)},
    {BEIComponentID::MoodManager,            BEIComponentWrapper(moodManager)},
    {BEIComponentID::MovementComponent,      BEIComponentWrapper(movementComponent)},
    {BEIComponentID::ObjectPoseConfirmer,    BEIComponentWrapper(objectPoseConfirmer)},
    {BEIComponentID::PetWorld,               BEIComponentWrapper(petWorld)},
    {BEIComponentID::ProgressionUnlock,      BEIComponentWrapper(progressionUnlockComponent)},
    {BEIComponentID::ProxSensor,             BEIComponentWrapper(proxSensor)},
    {BEIComponentID::PublicStateBroadcaster, BEIComponentWrapper(publicStateBroadcaster)},
    {BEIComponentID::RobotAudioClient,       BEIComponentWrapper(robotAudioClient)},
    {BEIComponentID::RobotInfo,              BEIComponentWrapper(robotInfo)},
    {BEIComponentID::SpriteCache,            BEIComponentWrapper(spriteCache)},
    {BEIComponentID::TouchSensor,            BEIComponentWrapper(touchSensorComponent)},
    {BEIComponentID::Vision,                 BEIComponentWrapper(visionComponent)},
    {BEIComponentID::VisionScheduleMediator, BEIComponentWrapper(visionScheduleMediator)}
}){}
  
} // namespace Cozmo
} // namespace Anki
