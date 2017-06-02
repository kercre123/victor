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

#include "anki/cozmo/basestation/behaviorSystem/activities/activities/activityGatherCubes.h"
#include "anki/cozmo/basestation/behaviorSystem/aiComponent.h"
#include "anki/cozmo/basestation/behaviorSystem/AIWhiteboard.h"
#include "anki/cozmo/basestation/blockWorld/blockWorld.h"
#include "anki/cozmo/basestation/components/cubeLightComponent.h"
#include "anki/cozmo/basestation/cozmoContext.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "anki/cozmo/basestation/needsSystem/needsManager.h"
#include "anki/cozmo/basestation/robot.h"

namespace Anki {
namespace Cozmo {
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ActivityGatherCubes::ActivityGatherCubes(Robot& robot, const Json::Value& config)
: IActivity(robot, config)
, _robot(robot)
{
  
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result ActivityGatherCubes::Update(Robot& robot){
  if (_gatherCubesFinished)
  {
    return Result::RESULT_OK;
  }
  
  // Get all connected cubes
  std::vector<const ActiveObject*> connectedLightCubes;
  GetConnectedLightCubes(connectedLightCubes);
  
  // Check if all cubes (connected and unconnected) are in beacon
  AIWhiteboard& whiteboard = robot.GetAIComponent().GetWhiteboard();
  if(whiteboard.AreAllCubesInBeacons())
  {
    // If all cubes are in beacon, play "Finish Gather Cubes" light animation on all connected cubes
    for(auto const& lightCube: connectedLightCubes)
    {
      PlayFinishGatherCubeLight(lightCube->GetID());
    }
    
    // Send event to start animations and end spark
    if(robot.HasExternalInterface()){
      robot.GetExternalInterface()
      ->BroadcastToGame<ExternalInterface::BehaviorObjectiveAchieved>(BehaviorObjective::GatheredCubes);
    }
    robot.GetContext()->GetNeedsManager()->RegisterNeedsActionCompleted(NeedsActionId::GatherCubes);
    
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
        ObservableObject* locatedCube = robot.GetBlockWorld().GetLocatedObjectByID(lightCube->GetID());
        if (locatedCube != nullptr)
        {
          if (locatedCube->IsPoseStateKnown() && beacon->IsLocWithinBeacon(locatedCube->GetPose()))
          {
            PlayGatherCubeInProgressLight(lightCube->GetID());
          }
          else
          {
            PlayFreeplayLight(lightCube->GetID());
          }
        }
        else
        {
          PlayFreeplayLight(lightCube->GetID());
        }
      }
    }
  }
  return Result::RESULT_OK;
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ActivityGatherCubes::OnSelectedInternal(Robot& robot)
{
  // destroy beacon so that the sparksThinkAboutBeacons behavior in SparksGatherCube can create it
  ClearBeacons();
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ActivityGatherCubes::OnDeselectedInternal(Robot& robot)
{
  // destroy beacon so that hiking can recreate it in freeplay
  ClearBeacons();
  _gatherCubesFinished = false;
}
  

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ActivityGatherCubes::ClearBeacons()
{
  AIWhiteboard& whiteboard = _robot.GetAIComponent().GetWhiteboard();
  whiteboard.ClearAllBeacons();
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ActivityGatherCubes::GetConnectedLightCubes(std::vector<const ActiveObject*>& result)
{
  BlockWorldFilter filter;
  filter.SetAllowedFamilies({ ObjectFamily::LightCube });
  _robot.GetBlockWorld().FindConnectedActiveMatchingObjects(filter, result);
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ActivityGatherCubes::PlayFinishGatherCubeLight(const ObjectID& cubeId)
{
  // If ring anim started,
  if (_isPlayingRingAnim[cubeId])
  {
    // Change cube light state to flashing green from ring green
    _robot.GetCubeLightComponent().StopAndPlayLightAnim(cubeId,
                                                        CubeAnimationTrigger::GatherCubesCubeInBeacon,
                                                        CubeAnimationTrigger::GatherCubesAllCubesInBeacon);
  }
  else
  {
    // Change cube light state to flashing green from freeplay
    _robot.GetCubeLightComponent().PlayLightAnim(cubeId,
                                                 CubeAnimationTrigger::GatherCubesAllCubesInBeacon);
  }
  
  _isPlayingRingAnim[cubeId] = false;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ActivityGatherCubes::PlayGatherCubeInProgressLight(const ObjectID& cubeId)
{
  // If ring anim not started, set lights to green ring animation
  if (!_isPlayingRingAnim[cubeId])
  {
    _robot.GetCubeLightComponent().PlayLightAnim(cubeId,
                                                 CubeAnimationTrigger::GatherCubesCubeInBeacon);
    _isPlayingRingAnim[cubeId] = true;
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ActivityGatherCubes::PlayFreeplayLight(const ObjectID& cubeId)
{
  // If ring anim started, stop animation and play freeplay
  if (_isPlayingRingAnim[cubeId])
  {
    _robot.GetCubeLightComponent().StopLightAnimAndResumePrevious(CubeAnimationTrigger::GatherCubesCubeInBeacon,
                                                                  cubeId);
    _isPlayingRingAnim[cubeId] = false;
  }
}


} // namespace Cozmo
} // namespace Anki
