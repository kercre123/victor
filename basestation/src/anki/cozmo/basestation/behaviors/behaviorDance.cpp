/**
 * File: behaviorDance.cpp
 *
 * Author: Al Chaussee
 * Created: 05/11/17
 *
 * Description: Behavior to have Cozmo dance
 *              Plays dancing animation, triggers music from device, and
 *              plays cube light animations
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#include "anki/cozmo/basestation/behaviors/behaviorDance.h"

#include "anki/cozmo/basestation/activeObject.h"
#include "anki/cozmo/basestation/audio/robotAudioClient.h"
#include "anki/cozmo/basestation/blockWorld/blockWorld.h"
#include "anki/cozmo/basestation/components/cubeLightComponent.h"
#include "anki/cozmo/basestation/components/publicStateBroadcaster.h"
#include "anki/cozmo/basestation/robot.h"

namespace Anki {
namespace Cozmo {

namespace {
constexpr ReactionTriggerHelpers::FullReactionArray kAffectTriggersDance = {
  {ReactionTrigger::CliffDetected,                false},
  {ReactionTrigger::CubeMoved,                    true},
  {ReactionTrigger::DoubleTapDetected,            true},
  {ReactionTrigger::FacePositionUpdated,          true},
  {ReactionTrigger::FistBump,                     true},
  {ReactionTrigger::Frustration,                  false},
  {ReactionTrigger::Hiccup,                       false},
  {ReactionTrigger::MotorCalibration,             false},
  {ReactionTrigger::NoPreDockPoses,               false},
  {ReactionTrigger::ObjectPositionUpdated,        true},
  {ReactionTrigger::PlacedOnCharger,              false},
  {ReactionTrigger::PetInitialDetection,          true},
  {ReactionTrigger::PyramidInitialDetection,      true},
  {ReactionTrigger::RobotPickedUp,                false},
  {ReactionTrigger::RobotPlacedOnSlope,           false},
  {ReactionTrigger::ReturnedToTreads,             false},
  {ReactionTrigger::RobotOnBack,                  false},
  {ReactionTrigger::RobotOnFace,                  false},
  {ReactionTrigger::RobotOnSide,                  false},
  {ReactionTrigger::RobotShaken,                  false},
  {ReactionTrigger::Sparked,                      false},
  {ReactionTrigger::StackOfCubesInitialDetection, true},
  {ReactionTrigger::UnexpectedMovement,           true},
  {ReactionTrigger::VC,                           false}
};

static_assert(ReactionTriggerHelpers::IsSequentialArray(kAffectTriggersDance),
              "Reaction triggers duplicate or non-sequential");
}
    
BehaviorDance::BehaviorDance(Robot& robot, const Json::Value& config)
: BehaviorPlayAnimSequence(robot, config)
{
  
}

Result BehaviorDance::InitInternal(Robot& robot)
{
  SmartDisableReactionsWithLock(GetIDStr(), kAffectTriggersDance);

  BlockWorldFilter filter;
  filter.AddAllowedFamily(ObjectFamily::LightCube);
  filter.SetFilterFcn(nullptr);

  // Get all connected light cubes
  std::vector<const ActiveObject*> connectedObjects;
  robot.GetBlockWorld().FindConnectedActiveMatchingObjects(filter, connectedObjects);
  
  s32 durationModifier_ms = 0;
  // For each of the connected light cubes
  for(const ActiveObject* object : connectedObjects)
  {
    // Start playing a random dance related light animation
    const ObjectID& objectID = object->GetID();
    
    CubeLightComponent::AnimCompletedCallback animCompleteCallback = [this, objectID, &robot](){
      CubeAnimComplete(robot, objectID);
    };
    
    const CubeAnimationTrigger trigger = GetRandomAnimTrigger(robot,
                                                              CubeAnimationTrigger::Count);
    
    robot.GetCubeLightComponent().PlayLightAnim(objectID,
                                                trigger,
                                                animCompleteCallback,
                                                false,
                                                {},
                                                durationModifier_ms);
    
    // The dancing song is 183Bpm so each beat is ~328ms so offset each
    // light animation by a beat
    // This will result in a single cube playing a different animation each beat
    // TODO: Move BPM to behavior config file
    const u32 msPerBeat = 328;
    durationModifier_ms -= msPerBeat;
    
    // Store the animation trigger so we can stop it later
    _lastAnimTrigger[objectID] = trigger;
  }
  
  // Start dancing audio
  robot.GetPublicStateBroadcaster().UpdateBroadcastBehaviorStage(BehaviorStageTag::Dance, 0);
  
  // Init base class to play our animation sequence
  return BehaviorPlayAnimSequence::InitInternal(robot);
}

void BehaviorDance::StopInternal(Robot& robot)
{
  // Stop dancing audio and go back to previous
  robot.GetPublicStateBroadcaster().UpdateBroadcastBehaviorStage(BehaviorStageTag::Count, 0);

  BlockWorldFilter filter;
  filter.AddAllowedFamily(ObjectFamily::LightCube);
  filter.SetFilterFcn(nullptr);
  
  // Get all connected light cubes
  std::vector<const ActiveObject*> connectedObjects;
  robot.GetBlockWorld().FindConnectedActiveMatchingObjects(filter, connectedObjects);
  
  // For each of the connected light cubes
  for(const ActiveObject* object : connectedObjects)
  {
    // Layer the fadeOff light animation on top of the last played animation
    // and stop all animations once the fadeOff completes
    const ObjectID& objectID = object->GetID();
    
    robot.GetCubeLightComponent().PlayLightAnim(objectID,
                                                CubeAnimationTrigger::DanceFadeOff,
                                                [&robot](){
                                                  robot.GetCubeLightComponent().StopAllAnims();
                                                });
  }
}

void BehaviorDance::CubeAnimComplete(Robot& robot, const ObjectID& objectID)
{
  // When the light anims are stopped due to the behavior stopping, this callback
  // is called so don't do anything
  if(!IsRunning())
  {
    return;
  }
  
  // When the animation completes it will call this function again to play a
  // different random dance animation
  CubeLightComponent::AnimCompletedCallback animCompleteCallback = [this, objectID, &robot](){
    CubeAnimComplete(robot, objectID);
  };

  const CubeAnimationTrigger trigger = GetRandomAnimTrigger(robot, _lastAnimTrigger[objectID]);

  // Stop the previous animation and play the new random one
  robot.GetCubeLightComponent().PlayLightAnim(objectID,
                                              trigger,
                                              animCompleteCallback);
  
  _lastAnimTrigger[objectID] = trigger;
}

CubeAnimationTrigger BehaviorDance::GetRandomAnimTrigger(Robot& robot,
                                                         const CubeAnimationTrigger& prevTrigger) const
{
  // Pick a random dancing animation trigger that is not the prevTrigger
  CubeAnimationTrigger trigger = prevTrigger;
  while(trigger == prevTrigger)
  {
    const int rand = robot.GetRNG().RandInt(6);
    switch(rand)
    {
      case 0:
        trigger = CubeAnimationTrigger::Dance_01;
        break;
      case 1:
        trigger = CubeAnimationTrigger::Dance_02;
        break;
      case 2:
        trigger = CubeAnimationTrigger::Dance_03;
        break;
      case 3:
        trigger = CubeAnimationTrigger::Dance_04;
        break;
      case 4:
        trigger = CubeAnimationTrigger::Dance_05;
        break;
      case 5:
        trigger = CubeAnimationTrigger::Dance_06;
        break;
      default:
        DEV_ASSERT(false, "BehaviorDance.GetRandomAnimTrigger.UnexpectedValue");
        trigger = CubeAnimationTrigger::Dance_01;
    }
  }
  
  return trigger;
}

}
}
