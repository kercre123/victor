/**
 * File: buildPyramidPersistentUpdate.cpp
 *
 * Author: Kevin M. Karol
 * Created: 11/14/16
 *
 * Description: Persistent update function for BuildPyramid
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#include "anki/cozmo/basestation/behaviorSystem/behaviorChoosers/AIGoalPersistentUpdates/buildPyramidPersistentUpdate.h"

#include "anki/common/basestation/utils/timer.h"
#include "anki/cozmo/basestation/audio/behaviorAudioClient.h"
#include "anki/cozmo/basestation/behaviorManager.h"
#include "anki/cozmo/basestation/behaviors/sparkable/behaviorBuildPyramidBase.h"
#include "anki/cozmo/basestation/blockWorld/blockConfigurationManager.h"
#include "anki/cozmo/basestation/blockWorld/blockConfigurationPyramid.h"
#include "anki/cozmo/basestation/blockWorld/blockWorld.h"
#include "anki/cozmo/basestation/components/cubeLightComponent.h"
#include "anki/cozmo/basestation/robot.h"
#include "clad/externalInterface/messageEngineToGameTag.h"
#include "util/helpers/templateHelpers.h"

namespace Anki {
namespace Cozmo {

namespace{
using EngineToGameEvent = AnkiEvent<ExternalInterface::MessageEngineToGame>;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BuildPyramidPersistentUpdate::BuildPyramidPersistentUpdate()
: _lastPyramidBaseSeenCount(0)
, _wasRobotCarryingObject(false)
{

}
 
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BuildPyramidPersistentUpdate::Init(Robot& robot)
{
  auto handlerCallback = [&robot](const EngineToGameEvent& event) {
    robot.GetBehaviorManager().GetAudioClient().UpdateBehaviorRound(
              UnlockId::BuildPyramid, Util::EnumToUnderlying(BehaviorBuildPyramidBase::MusicState::SearchingForCube));
    robot.GetCubeLightComponent().StopAllAnims();
  };
  
  if(robot.HasExternalInterface()){
    _eventHalder = robot.GetExternalInterface()->Subscribe(
                        ExternalInterface::MessageEngineToGameTag::RobotDelocalized, handlerCallback);
  }
  
}

  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BuildPyramidPersistentUpdate::GoalEntered(Robot& robot)
{
  _lastPyramidBaseSeenCount = 0;
  _wasRobotCarryingObject = false;
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BuildPyramidPersistentUpdate::Update(Robot& robot)
{
  using MusicState = BehaviorBuildPyramidBase::MusicState;
  
  const auto& pyramidBases = robot.GetBlockWorld().GetBlockConfigurationManager().GetPyramidBaseCache().GetBases();
  const auto& pyramids = robot.GetBlockWorld().GetBlockConfigurationManager().GetPyramidCache().GetPyramids();
  
  const int pyramidBaseSize = static_cast<int>(pyramidBases.size());
  
  // Did we see a new pyramid base or was a pyramid base destroyed?
  if(!pyramidBases.empty()&&
     _lastPyramidBaseSeenCount == 0){
    PRINT_CH_INFO("BuildPyramid", "BehaviorBuildPyramidBase.SparksPersistantMusicLightControls.PyramidBaseCreated",
                  "Transitioning to base formed music and lights");
    if(robot.IsCarryingObject()){
      RequestUpdateMusicLightState(robot, MusicState::TopBlockCarry);
    }else{
      RequestUpdateMusicLightState(robot, MusicState::BaseFormed);
    }
    
  }else if(pyramidBases.empty() &&
           pyramids.empty() &&
           _lastPyramidBaseSeenCount != 0){
    PRINT_CH_INFO("BuildPyramid", "BehaviorBuildPyramidBase.SparksPersistantMusicLightControls.PyramidBaseDestroyed",
                  "Transitioning to searching for cubes music and clearing lights");
    
    if(robot.IsCarryingObject()){
      RequestUpdateMusicLightState(robot, MusicState::InitialCubeCarry);
    }else{
      RequestUpdateMusicLightState(robot, MusicState::SearchingForCube);
    }
  }
  
  // The robot picked up a block - is it going to place it as a top block, or just looking around?
  if(robot.IsCarryingObject() && !_wasRobotCarryingObject){
    if(pyramidBases.empty()){
      PRINT_CH_INFO("BuildPyramid", "BehaviorBuildPyramidBase.SparksPersistantMusicLightControls.RobotPickedUpBlock",
                    "No pyramid bases known, transitioning to initial cube carry music");
      RequestUpdateMusicLightState(robot, MusicState::InitialCubeCarry);
    }else{
      PRINT_CH_INFO("BuildPyramid", "BehaviorBuildPyramidBase.SparksPersistantMusicLightControls.RobotPickedUpBlock",
                    "Pyramid base known, give control of lights to build pyramid behavior");
      
      RequestUpdateMusicLightState(robot, MusicState::TopBlockCarry);
    }
  }
  
  _wasRobotCarryingObject = robot.IsCarryingObject();
  _lastPyramidBaseSeenCount = pyramidBaseSize;
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BuildPyramidPersistentUpdate::RequestUpdateMusicLightState(Robot& robot, BehaviorBuildPyramidBase::MusicState stateToSet)
{
  using MusicState = BehaviorBuildPyramidBase::MusicState;
  
  // prevent regressions during flourish
  if(_currentState == MusicState::TopBlockCarry
     && (stateToSet == MusicState::BaseFormed ||
         stateToSet == MusicState::SearchingForCube)){
    return;
  }
  _currentState = stateToSet;
  

  switch(stateToSet) {
    case MusicState::SearchingForCube:
    {
      robot.GetBehaviorManager().GetAudioClient().UpdateBehaviorRound(
                UnlockId::BuildPyramid, Util::EnumToUnderlying(MusicState::SearchingForCube));
      robot.GetCubeLightComponent().StopAllAnims();
      break;
    }
    case MusicState::InitialCubeCarry:
    {
      robot.GetBehaviorManager().GetAudioClient().UpdateBehaviorRound(
               UnlockId::BuildPyramid, Util::EnumToUnderlying(MusicState::InitialCubeCarry));
      break;
    }
    case MusicState::BaseFormed:
    {
      robot.GetBehaviorManager().GetAudioClient().UpdateBehaviorRound(
               UnlockId::BuildPyramid, Util::EnumToUnderlying(MusicState::BaseFormed));
      const auto& pyramidBases = robot.GetBlockWorld().GetBlockConfigurationManager().GetPyramidBaseCache().GetBases();
      if(pyramidBases.size() > 0){
        const auto& base = pyramidBases[0];
        BehaviorBuildPyramidBase::SetPyramidBaseLightsByID(robot, base->GetBaseBlockID(), base->GetStaticBlockID());
      }
      break;
    }
    case MusicState::TopBlockCarry:
    {
      break;
    }
    case MusicState::PyramidCompleteFlourish:
    {
      break;
    }
  }
}

  
  
} // namespace Cozmo
} // namespace Anki

