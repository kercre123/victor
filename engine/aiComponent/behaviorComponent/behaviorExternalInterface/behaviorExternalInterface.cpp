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

#include "util/logging/logging.h"

namespace Anki {
namespace Cozmo {
  
namespace ComponentWrappers{
  
BehaviorExternalInterfaceComponents::BehaviorExternalInterfaceComponents(BEIRobotInfo&& robotInfo,
                                      AIComponent& aiComponent,
                                      const BehaviorContainer& behaviorContainer,
                                      BlockWorld& blockWorld,
                                      FaceWorld& faceWorld,
                                      PetWorld& petWorld,
                                      BehaviorEventComponent& behaviorEventComponent)
: _aiComponent(aiComponent)
, _behaviorContainer(behaviorContainer)
, _blockWorld(blockWorld)
, _faceWorld(faceWorld)
, _petWorld(petWorld)
, _behaviorEventComponent(behaviorEventComponent)
{
  _robotInfo = std::make_unique<BEIRobotInfo>(std::move(robotInfo));
}
  
} // namespace ComponentWrappers

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorExternalInterface::BehaviorExternalInterface()
: _delegationComponent(nullptr)
, _moodManager(nullptr)
, _needsManager(nullptr)
, _progressionUnlockComponent(nullptr)
, _publicStateBroadcaster(nullptr)
, _touchSensorComponent(nullptr)
, _visionComponent(nullptr)
, _mapComponent(nullptr)
, _cubeLightComponent(nullptr)
, _objectPoseConfirmer(nullptr)
, _cubeAccelComponent(nullptr)
{

}
 
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorExternalInterface::~BehaviorExternalInterface()
{

}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorExternalInterface::Init(Robot& robot,
                                     AIComponent& aiComponent,
                                     const BehaviorContainer& behaviorContainer,
                                     BlockWorld& blockWorld,
                                     FaceWorld& faceWorld,
                                     PetWorld& petWorld,
                                     BehaviorEventComponent& behaviorEventComponent)
{

  _beiComponents = std::make_unique<ComponentWrappers::BehaviorExternalInterfaceComponents>(
                                    BEIRobotInfo(robot), aiComponent, behaviorContainer,
                                    blockWorld, faceWorld, petWorld, behaviorEventComponent);
  
  SetOptionalInterfaces(nullptr,
                        &robot.GetMoodManager(),
                        robot.GetContext()->GetNeedsManager(),
                        &robot.GetProgressionUnlockComponent(),
                        &robot.GetPublicStateBroadcaster(),
                        &robot.GetTouchSensorComponent(),
                        &robot.GetVisionComponent(),
                        &robot.GetMapComponent(),
                        &robot.GetCubeLightComponent(),
                        &robot.GetObjectPoseConfirmer(),
                        &robot.GetCubeAccelComponent(),
                        &robot.GetAnimationComponent(),
                        robot.GetAudioClient(),
                        &robot.GetBodyLightComponent());
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorExternalInterface::SetOptionalInterfaces(DelegationComponent*           delegationComponent,
                                                      MoodManager*                   moodManager,
                                                      NeedsManager*                  needsManager,
                                                      ProgressionUnlockComponent*    progressionUnlockComponent,
                                                      PublicStateBroadcaster*        publicStateBroadcaster,
                                                      TouchSensorComponent*          touchSensorComponent,
                                                      VisionComponent*               visionComponent,
                                                      MapComponent*                  mapComponent,
                                                      CubeLightComponent*            cubeLightComponent,
                                                      ObjectPoseConfirmer*           objectPoseConfirmer,
                                                      CubeAccelComponent*            cubeAccelComponent,
                                                      AnimationComponent*            animationComponent,
                                                      Audio::EngineRobotAudioClient* robotAudioClient,
                                                      BodyLightComponent*            bodyLightComponent)
{


  _delegationComponent        = delegationComponent;
  _moodManager                = moodManager;
  _needsManager               = needsManager;
  _progressionUnlockComponent = progressionUnlockComponent;
  _publicStateBroadcaster     = publicStateBroadcaster;
  _touchSensorComponent       = touchSensorComponent;
  _visionComponent            = visionComponent;
  _mapComponent               = mapComponent;
  _cubeLightComponent         = cubeLightComponent;
  _objectPoseConfirmer        = objectPoseConfirmer;
  _cubeAccelComponent         = cubeAccelComponent;
  _animationComponent         = animationComponent;
  _robotAudioClient           = robotAudioClient;
  _bodyLightComponent         = bodyLightComponent;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
OffTreadsState BehaviorExternalInterface::GetOffTreadsState() const
{
  assert(_beiComponents);
  return _beiComponents->_robotInfo->GetOffTreadsState();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Util::RandomGenerator& BehaviorExternalInterface::GetRNG()
{
  assert(_beiComponents);
  return _beiComponents->_robotInfo->GetRNG();
}
  
} // namespace Cozmo
} // namespace Anki
