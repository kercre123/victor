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

#include "engine/aiComponent/behaviorComponent/behaviors/freeplay/oneShots/behaviorDance.h"

#include "engine/activeObject.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/blockWorld/blockWorld.h"
#include "engine/components/cubeLightComponent.h"
#include "engine/components/publicStateBroadcaster.h"

#include "util/helpers/boundedWhile.h"

namespace Anki {
namespace Cozmo {

namespace {

const std::vector<CubeAnimationTrigger> kDanceCubeAnims = {
  CubeAnimationTrigger::Dance_01,
  CubeAnimationTrigger::Dance_02,
  CubeAnimationTrigger::Dance_03,
  CubeAnimationTrigger::Dance_04,
  CubeAnimationTrigger::Dance_05,
  CubeAnimationTrigger::Dance_06
};
}
  

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorDance::BehaviorDance(const Json::Value& config)
: BehaviorPlayAnimSequence(config)
{
  
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result BehaviorDance::OnBehaviorActivated(BehaviorExternalInterface& behaviorExternalInterface)
{
  BlockWorldFilter filter;
  filter.AddAllowedFamily(ObjectFamily::LightCube);
  filter.SetFilterFcn(nullptr);

  // Get all connected light cubes
  std::vector<const ActiveObject*> connectedObjects;
  behaviorExternalInterface.GetBlockWorld().FindConnectedActiveMatchingObjects(filter, connectedObjects);
  
  s32 durationModifier_ms = 0;
  // For each of the connected light cubes
  for(const ActiveObject* object : connectedObjects)
  {
    // Start playing a random dance related light animation
    const ObjectID& objectID = object->GetID();
    
    CubeLightComponent::AnimCompletedCallback animCompleteCallback = [this, objectID, &behaviorExternalInterface](){
      CubeAnimComplete(behaviorExternalInterface, objectID);
    };
    
    _lastAnimTrigger[objectID] = CubeAnimationTrigger::Count;
    
    const CubeAnimationTrigger trigger = GetRandomAnimTrigger(behaviorExternalInterface,
                                                              CubeAnimationTrigger::Count);
    
    behaviorExternalInterface.GetCubeLightComponent().PlayLightAnim(objectID,
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
  //robot.GetPublicStateBroadcaster().UpdateBroadcastBehaviorStage(BehaviorStageTag::Dance, 0);
  DEV_ASSERT(false, "BehaviorDance.Init.MusicTurnedOff");
  
  // Init base class to play our animation sequence
  return BehaviorPlayAnimSequence::OnBehaviorActivated(behaviorExternalInterface);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDance::OnBehaviorDeactivated(BehaviorExternalInterface& behaviorExternalInterface)
{
  if(behaviorExternalInterface.HasPublicStateBroadcaster()){
    // Stop dancing audio and go back to previous
    auto& publicStateBroadcaster = behaviorExternalInterface.GetRobotPublicStateBroadcaster();
    publicStateBroadcaster.UpdateBroadcastBehaviorStage(BehaviorStageTag::Count, 0);
  }

  BlockWorldFilter filter;
  filter.AddAllowedFamily(ObjectFamily::LightCube);
  filter.SetFilterFcn(nullptr);
  
  // Get all connected light cubes
  std::vector<const ActiveObject*> connectedObjects;
  behaviorExternalInterface.GetBlockWorld().FindConnectedActiveMatchingObjects(filter, connectedObjects);
  
  // For each of the connected light cubes
  for(const ActiveObject* object : connectedObjects)
  {
    // Layer the fadeOff light animation on top of the last played animation
    // and stop all animations once the fadeOff completes
    const ObjectID& objectID = object->GetID();
    behaviorExternalInterface.GetCubeLightComponent().PlayLightAnim(
      objectID,
      CubeAnimationTrigger::DanceFadeOff,
      [&behaviorExternalInterface](){
        behaviorExternalInterface.GetCubeLightComponent().StopAllAnims();
      }
    );
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDance::CubeAnimComplete(BehaviorExternalInterface& behaviorExternalInterface, const ObjectID& objectID)
{
  // When the light anims are stopped due to the behavior stopping, this callback
  // is called so don't do anything
  if(!IsActivated())
  {
    return;
  }
  
  // When the animation completes it will call this function again to play a
  // different random dance animation
  CubeLightComponent::AnimCompletedCallback animCompleteCallback = [this, objectID, &behaviorExternalInterface](){
    CubeAnimComplete(behaviorExternalInterface, objectID);
  };

  const CubeAnimationTrigger trigger = GetRandomAnimTrigger(behaviorExternalInterface, _lastAnimTrigger[objectID]);

  // Stop the previous animation and play the new random one
  behaviorExternalInterface.GetCubeLightComponent().PlayLightAnim(objectID,
                                                                  trigger,
                                                                  animCompleteCallback);
  
  _lastAnimTrigger[objectID] = trigger;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CubeAnimationTrigger BehaviorDance::GetRandomAnimTrigger(BehaviorExternalInterface& behaviorExternalInterface,
                                                         const CubeAnimationTrigger& prevTrigger) const
{
  std::set<CubeAnimationTrigger> triggersInUse;
  for(const auto& trigger : _lastAnimTrigger)
  {
    triggersInUse.insert(trigger.second);
  }
  
  // Need to have less than kNumDanceTriggers in use otherwise the while loop
  // will never end
  const size_t kNumDanceAnims = kDanceCubeAnims.size();
  DEV_ASSERT(triggersInUse.size() < kNumDanceAnims,
             "BehaviorDance.GetRandomAnimTrigger.NoPossibleTriggers");
  
  // Pick a random dancing animation trigger that is not currently in use
  CubeAnimationTrigger trigger = prevTrigger;
  const u32 kBoundedWhileLoops = 100;
  BOUNDED_WHILE(kBoundedWhileLoops, triggersInUse.count(trigger) > 0)
  {
    const int rand = behaviorExternalInterface.GetRNG().RandInt(static_cast<int>(kNumDanceAnims));
    trigger = kDanceCubeAnims[rand];
  }
  
  return trigger;
}

}
}
