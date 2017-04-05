/**
 * File: gatherCubesBehaviorChooser.cpp
 *
 * Author: Ivy Ngo
 * Created: 03/27/17
 *
 * Description: Class for managing light and beacon state during the gather cubes spark behavior
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#include "gatherCubesBehaviorChooser.h"

namespace Anki {
namespace Cozmo {

GatherCubesBehaviorChooser::GatherCubesBehaviorChooser(Robot& robot, const Json::Value& config)
: SimpleBehaviorChooser(robot, config){

}

GatherCubesBehaviorChooser::~GatherCubesBehaviorChooser(){

}
  
Result GatherCubesBehaviorChooser::Update(Robot& robot){
  if (_gatherCubesFinished)
  {
    return Result::RESULT_OK;
  }

  // Get all connected cubes
  std::vector<const ActiveObject*> connectedLightCubes;
  GetConnectedLightCubes(connectedLightCubes);
  
  // Check if all cubes (connected and unconnected) are in beacon
  AIWhiteboard& whiteboard = _robot.GetAIComponent().GetWhiteboard();
  if(whiteboard.AreAllCubesInBeacons())
  {
    // If all cubes are in beacon, play "Finish Gather Cubes" light animation on all connected cubes
    for(auto const& lightCube: connectedLightCubes)
    {
      PlayFinishGatherCubeLight(lightCube->GetID());
    }
    
    // Send event to start animations and end spark
    if(_robot.HasExternalInterface()){
      _robot.GetExternalInterface()
        ->BroadcastToGame<ExternalInterface::BehaviorObjectiveAchieved>(BehaviorObjective::GatheredCubes);
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
        ObservableObject* locatedCube = _robot.GetBlockWorld().GetLocatedObjectByID(lightCube->GetID());
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
  
void GatherCubesBehaviorChooser::OnSelected()
{
  // destroy beacon so that the sparksThinkAboutBeacons behavior in SparksGatherCube can create it
  ClearBeacons();
}

void GatherCubesBehaviorChooser::OnDeselected()
{
  // destroy beacon so that hiking can recreate it in freeplay
  ClearBeacons();
  _gatherCubesFinished = false;
}

void GatherCubesBehaviorChooser::ClearBeacons()
{
  AIWhiteboard& whiteboard = _robot.GetAIComponent().GetWhiteboard();
  whiteboard.ClearAllBeacons();
}

void GatherCubesBehaviorChooser::GetConnectedLightCubes(std::vector<const ActiveObject*>& result)
{
  BlockWorldFilter filter;
  filter.SetAllowedFamilies({ ObjectFamily::LightCube });
  _robot.GetBlockWorld().FindConnectedActiveMatchingObjects(filter, result);
}

void GatherCubesBehaviorChooser::PlayFinishGatherCubeLight(const ObjectID& cubeId)
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

void GatherCubesBehaviorChooser::PlayGatherCubeInProgressLight(const ObjectID& cubeId)
{
  // If ring anim not started, set lights to green ring animation
  if (!_isPlayingRingAnim[cubeId])
  {
    _robot.GetCubeLightComponent().PlayLightAnim(cubeId,
                                                 CubeAnimationTrigger::GatherCubesCubeInBeacon);
    _isPlayingRingAnim[cubeId] = true;
  }
}

void GatherCubesBehaviorChooser::PlayFreeplayLight(const ObjectID& cubeId)
{
  // If ring anim started, stop animation and play freeplay
  if (_isPlayingRingAnim[cubeId])
  {
    _robot.GetCubeLightComponent().StopLightAnimAndResumePrevious(CubeAnimationTrigger::GatherCubesCubeInBeacon,
                                                                  cubeId);
    _isPlayingRingAnim[cubeId] = false;
  }
}

}
}
