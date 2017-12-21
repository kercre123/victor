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
#include "engine/components/progressionUnlockComponent.h"
#include "engine/components/publicStateBroadcaster.h"
#include "engine/cozmoContext.h"
#include "engine/externalInterface/externalInterface.h"
#include "engine/moodSystem/moodManager.h"
#include "engine/needsSystem/needsManager.h"
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
void BehaviorExternalInterface::Init(AIComponent*                   aiComponent,
                                     AnimationComponent*            animationComponent,
                                     BehaviorContainer*             behaviorContainer,
                                     BehaviorEventComponent*        behaviorEventComponent,
                                     BlockWorld*                    blockWorld,
                                     BodyLightComponent*            bodyLightComponent,
                                     CubeAccelComponent*            cubeAccelComponent,
                                     CubeLightComponent*            cubeLightComponent,
                                     DelegationComponent*           delegationComponent,
                                     FaceWorld*                     faceWorld,
                                     MapComponent*                  mapComponent,
                                     MicDirectionHistory*           micDirectionHistory,
                                     MoodManager*                   moodManager,
                                     MultiRobotComponent*           multiRobotComponent,
                                     NeedsManager*                  needsManager,
                                     ObjectPoseConfirmer*           objectPoseConfirmer,
                                     PetWorld*                      petWorld,
                                     ProgressionUnlockComponent*    progressionUnlockComponent,
                                     ProxSensorComponent*           proxSensor,
                                     PublicStateBroadcaster*        publicStateBroadcaster,
                                     Audio::EngineRobotAudioClient* robotAudioClient,
                                     BEIRobotInfo*                  robotInfo,
                                     TouchSensorComponent*          touchSensorComponent,
                                     VisionComponent*               visionComponent)
{
  _arrayWrapper = std::make_unique<CompArrayWrapper>(aiComponent,
                                                     animationComponent,
                                                     behaviorContainer,
                                                     behaviorEventComponent,
                                                     blockWorld,
                                                     bodyLightComponent,
                                                     cubeAccelComponent,
                                                     cubeLightComponent,
                                                     delegationComponent,
                                                     faceWorld,
                                                     mapComponent,
                                                     micDirectionHistory,
                                                     moodManager,
                                                     multiRobotComponent,
                                                     needsManager,
                                                     objectPoseConfirmer,
                                                     petWorld,
                                                     progressionUnlockComponent,
                                                     proxSensor,
                                                     publicStateBroadcaster,
                                                     robotAudioClient,
                                                     robotInfo,
                                                     touchSensorComponent,
                                                     visionComponent);
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
                                                              BlockWorld*                    blockWorld,
                                                              BodyLightComponent*            bodyLightComponent,
                                                              CubeAccelComponent*            cubeAccelComponent,
                                                              CubeLightComponent*            cubeLightComponent,
                                                              DelegationComponent*           delegationComponent,
                                                              FaceWorld*                     faceWorld,
                                                              MapComponent*                  mapComponent,
                                                              MicDirectionHistory*           micDirectionHistory,
                                                              MoodManager*                   moodManager,
                                                              MultiRobotComponent*           multiRobotComponent,
                                                              NeedsManager*                  needsManager,
                                                              ObjectPoseConfirmer*           objectPoseConfirmer,
                                                              PetWorld*                      petWorld,
                                                              ProgressionUnlockComponent*    progressionUnlockComponent,
                                                              ProxSensorComponent*           proxSensor,
                                                              PublicStateBroadcaster*        publicStateBroadcaster,
                                                              Audio::EngineRobotAudioClient* robotAudioClient,
                                                              BEIRobotInfo*                  robotInfo,
                                                              TouchSensorComponent*          touchSensorComponent,
                                                              VisionComponent*               visionComponent)
: _array({
    {BEIComponentID::AIComponent,            BEIComponentWrapper(aiComponent)},
    {BEIComponentID::Animation,              BEIComponentWrapper(animationComponent)},
    {BEIComponentID::BehaviorContainer,      BEIComponentWrapper(behaviorContainer)},
    {BEIComponentID::BehaviorEvent,          BEIComponentWrapper(behaviorEventComponent)},
    {BEIComponentID::BlockWorld,             BEIComponentWrapper(blockWorld)},
    {BEIComponentID::BodyLightComponent,     BEIComponentWrapper(bodyLightComponent)},
    {BEIComponentID::CubeAccel,              BEIComponentWrapper(cubeAccelComponent)},
    {BEIComponentID::CubeLight,              BEIComponentWrapper(cubeLightComponent)},
    {BEIComponentID::Delegation,             BEIComponentWrapper(delegationComponent)},
    {BEIComponentID::FaceWorld,              BEIComponentWrapper(faceWorld)},
    {BEIComponentID::Map,                    BEIComponentWrapper(mapComponent)},
    {BEIComponentID::MicDirectionHistory,    BEIComponentWrapper(micDirectionHistory)},
    {BEIComponentID::MoodManager,            BEIComponentWrapper(moodManager)},
    {BEIComponentID::MultiRobotComponent,    BEIComponentWrapper(multiRobotComponent)},
    {BEIComponentID::NeedsManager,           BEIComponentWrapper(needsManager)},
    {BEIComponentID::ObjectPoseConfirmer,    BEIComponentWrapper(objectPoseConfirmer)},
    {BEIComponentID::PetWorld,               BEIComponentWrapper(petWorld)},
    {BEIComponentID::ProgressionUnlock,      BEIComponentWrapper(progressionUnlockComponent)},
    {BEIComponentID::ProxSensor,             BEIComponentWrapper(proxSensor)},
    {BEIComponentID::PublicStateBroadcaster, BEIComponentWrapper(publicStateBroadcaster)},
    {BEIComponentID::RobotAudioClient,       BEIComponentWrapper(robotAudioClient)},
    {BEIComponentID::RobotInfo,              BEIComponentWrapper(robotInfo)},
    {BEIComponentID::TouchSensor,            BEIComponentWrapper(touchSensorComponent)},
    {BEIComponentID::Vision,                 BEIComponentWrapper(visionComponent)}
}){}
  
} // namespace Cozmo
} // namespace Anki
