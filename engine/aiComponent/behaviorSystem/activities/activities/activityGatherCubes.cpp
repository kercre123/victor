/**
* File: ActivityGatherCubes.cpp
*
* Author: Kevin M. Karol
* Created: 04/27/17
*
* Description: Activity for cozmo to gather his cubes together
*
* Copyright: Anki, Inc. 2017
*
**/

#include "engine/aiComponent/behaviorSystem/activities/activities/activityGatherCubes.h"
#include "engine/aiComponent/aiComponent.h"
#include "engine/aiComponent/AIWhiteboard.h"
#include "engine/aiComponent/behaviorSystem/behaviorExternalInterface.h"
#include "engine/blockWorld/blockWorld.h"
#include "engine/components/cubeLightComponent.h"
#include "engine/cozmoContext.h"
#include "engine/externalInterface/externalInterface.h"
#include "engine/needsSystem/needsManager.h"
#include "engine/robot.h"

namespace Anki {
namespace Cozmo {
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ActivityGatherCubes::ActivityGatherCubes(BehaviorExternalInterface& behaviorExternalInterface, const Json::Value& config)
: IActivity(behaviorExternalInterface, config)
{
  
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result ActivityGatherCubes::Update_Legacy(BehaviorExternalInterface& behaviorExternalInterface){
  if (_gatherCubesFinished)
  {
    return Result::RESULT_OK;
  }
  
  // Get all connected cubes
  std::vector<const ActiveObject*> connectedLightCubes;
  GetConnectedLightCubes(behaviorExternalInterface, connectedLightCubes);
  
  // Check if all cubes (connected and unconnected) are in beacon
  AIWhiteboard& whiteboard = behaviorExternalInterface.GetAIComponent().GetWhiteboard();
  if(whiteboard.AreAllCubesInBeacons())
  {
    // If all cubes are in beacon, play "Finish Gather Cubes" light animation on all connected cubes
    for(auto const& lightCube: connectedLightCubes)
    {
      PlayFinishGatherCubeLight(behaviorExternalInterface, lightCube->GetID());
    }
    
    // Send event to start animations and end spark
    auto robotExternalInterface = behaviorExternalInterface.GetRobotExternalInterface().lock();
    if(robotExternalInterface != nullptr){
      robotExternalInterface->BroadcastToGame<
         ExternalInterface::BehaviorObjectiveAchieved>(BehaviorObjective::GatheredCubes);
    }
    auto needsManager = behaviorExternalInterface.GetNeedsManager().lock();
    if(needsManager != nullptr){
      needsManager->RegisterNeedsActionCompleted(NeedsActionId::GatherCubes);
    }
    _gatherCubesFinished = true;
  }
  else // If not all cubes in beacon
  {
    AIBeacon* beacon = whiteboard.GetActiveBeacon();
    if (beacon != nullptr)
    {
      for(auto const& lightCube: connectedLightCubes)
      {
        // If cube is located and is inside the beacon play freeplay lights
        const ObservableObject* locatedCube = behaviorExternalInterface.GetBlockWorld().GetLocatedObjectByID(lightCube->GetID());
        if (locatedCube != nullptr)
        {
          if (locatedCube->IsPoseStateKnown() && beacon->IsLocWithinBeacon(locatedCube->GetPose()))
          {
            PlayGatherCubeInProgressLight(behaviorExternalInterface, lightCube->GetID());
          }
          else
          {
            PlayFreeplayLight(behaviorExternalInterface, lightCube->GetID());
          }
        }
        else
        {
          PlayFreeplayLight(behaviorExternalInterface, lightCube->GetID());
        }
      }
    }
  }
  return Result::RESULT_OK;
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ActivityGatherCubes::OnActivatedActivity(BehaviorExternalInterface& behaviorExternalInterface)
{
  // destroy beacon so that the sparksThinkAboutBeacons behavior in SparksGatherCube can create it
  ClearBeacons(behaviorExternalInterface);
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ActivityGatherCubes::OnDeactivatedActivity(BehaviorExternalInterface& behaviorExternalInterface)
{
  // destroy beacon so that hiking can recreate it in freeplay
  ClearBeacons(behaviorExternalInterface);
  _gatherCubesFinished = false;
}
  

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ActivityGatherCubes::ClearBeacons(BehaviorExternalInterface& behaviorExternalInterface)
{
  AIWhiteboard& whiteboard = behaviorExternalInterface.GetAIComponent().GetWhiteboard();
  whiteboard.ClearAllBeacons();
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ActivityGatherCubes::GetConnectedLightCubes(BehaviorExternalInterface& behaviorExternalInterface,
                                                 std::vector<const ActiveObject*>& result)
{
  BlockWorldFilter filter;
  filter.SetAllowedFamilies({ ObjectFamily::LightCube });
  behaviorExternalInterface.GetBlockWorld().FindConnectedActiveMatchingObjects(filter, result);
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ActivityGatherCubes::PlayFinishGatherCubeLight(BehaviorExternalInterface& behaviorExternalInterface,
                                                    const ObjectID& cubeId)
{
  // DEPRECATED - Grabbing robot to support current cozmo code, but this should
  // be removed
  Robot& robot = behaviorExternalInterface.GetRobot();
  
  // If ring anim started,
  if (_isPlayingRingAnim[cubeId])
  {
    // Change cube light state to flashing green from ring green
    robot.GetCubeLightComponent().StopAndPlayLightAnim(cubeId,
                                                       CubeAnimationTrigger::GatherCubesCubeInBeacon,
                                                       CubeAnimationTrigger::GatherCubesAllCubesInBeacon);
  }
  else
  {
    // Change cube light state to flashing green from freeplay
    robot.GetCubeLightComponent().PlayLightAnim(cubeId,
                                                CubeAnimationTrigger::GatherCubesAllCubesInBeacon);
  }
  
  _isPlayingRingAnim[cubeId] = false;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ActivityGatherCubes::PlayGatherCubeInProgressLight(BehaviorExternalInterface& behaviorExternalInterface,
                                                        const ObjectID& cubeId)
{
  // If ring anim not started, set lights to green ring animation
  if (!_isPlayingRingAnim[cubeId])
  {
    // DEPRECATED - Grabbing robot to support current cozmo code, but this should
    // be removed
    Robot& robot = behaviorExternalInterface.GetRobot();
    robot.GetCubeLightComponent().PlayLightAnim(cubeId,
                                                CubeAnimationTrigger::GatherCubesCubeInBeacon);
    _isPlayingRingAnim[cubeId] = true;
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ActivityGatherCubes::PlayFreeplayLight(BehaviorExternalInterface& behaviorExternalInterface,
                                            const ObjectID& cubeId)
{
  // If ring anim started, stop animation and play freeplay
  if (_isPlayingRingAnim[cubeId])
  {
    // DEPRECATED - Grabbing robot to support current cozmo code, but this should
    // be removed
    Robot& robot = behaviorExternalInterface.GetRobot();
    robot.GetCubeLightComponent().StopLightAnimAndResumePrevious(CubeAnimationTrigger::GatherCubesCubeInBeacon,
                                                                 cubeId);
    _isPlayingRingAnim[cubeId] = false;
  }
}


} // namespace Cozmo
} // namespace Anki
