/**
 * File: behaviorReactToFrustration.cpp
 *
 * Author: Brad Neuman
 * Created: 2016-08-09
 *
 * Description: Behavior to react when the robot gets really frustrated (e.g. because he is failing actions)
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#include "anki/cozmo/basestation/behaviors/reactionary/behaviorReactToFrustration.h"

#include "anki/common/basestation/jsonTools.h"
#include "anki/common/basestation/math/pose.h"
#include "anki/common/basestation/utils/timer.h"
#include "anki/cozmo/basestation/actions/animActions.h"
#include "anki/cozmo/basestation/actions/compoundActions.h"
#include "anki/cozmo/basestation/actions/driveToActions.h"
#include "anki/cozmo/basestation/events/animationTriggerHelpers.h"
#include "anki/cozmo/basestation/moodSystem/moodManager.h"
#include "anki/cozmo/basestation/robot.h"
#include "util/math/math.h"

// TODO:(bn) this entire behavior could be generic for any type of emotion.... but that's too much effort

namespace {
static const char* kReactionConfigKey = "reactions";
}

namespace Anki {
namespace Cozmo {

BehaviorReactToFrustration::BehaviorReactToFrustration(Robot& robot, const Json::Value& config)
  : IReactionaryBehavior(robot, config)
{
  SetDefaultName("ReactToFrustration");

  const Json::Value& reactionsJson = config[kReactionConfigKey];
  if( !reactionsJson.isNull() ) {
    for( const auto& reaction : reactionsJson ) {
      _possibleReactions.emplace_back( FrustrationReaction{reaction} );
    }
  }
}

bool BehaviorReactToFrustration::ShouldComputationallySwitch(const Robot& robot)
{  
  bool hasReaction = GetReaction(robot) != _possibleReactions.end();

  return hasReaction;
}

Result BehaviorReactToFrustration::InitInternalReactionary(Robot& robot)
{
  auto reactionIt = GetReaction(robot);
  if( reactionIt != _possibleReactions.end() ) {
    TransitionToReaction(robot, *reactionIt);
    return Result::RESULT_OK;
  }
  else {
    PRINT_NAMED_WARNING("BehaviorReactToFrustration.NoReaction.Bug",
                        "We decided to run the reaction, but there is no valid one. this is a bug");
    return Result::RESULT_FAIL;
  }    
}

void BehaviorReactToFrustration::TransitionToReaction(Robot& robot, FrustrationReaction& reaction)
{

  TriggerAnimationAction* action = new TriggerAnimationAction(robot, reaction.params.anim);

  StartActing(action, [this, &reaction](Robot& robot) {
      AnimationComplete(robot, reaction);
    });    
}

void BehaviorReactToFrustration::AnimationComplete(Robot& robot, FrustrationReaction& reaction)
{  
  // mark cooldown and update emotion. Note that if we get interrupted, this won't happen
  const float currTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  
  if( !reaction.params.finalEmotionEvent.empty() ) {
    robot.GetMoodManager().TriggerEmotionEvent(reaction.params.finalEmotionEvent, currTime_s);
  }

  reaction.lastReactedTime_s = currTime_s;

  // if we want to drive somewhere, do that AFTER the emotion update, so we don't get stuck in a loop if this
  // part gets interrupted
  if( FLT_GT(reaction.params.randomDriveMaxDist_mm, 0.0f) ) {
    // pick a random pose
    // TODO:(bn) use memory map here
    float randomAngleDeg = GetRNG().RandDblInRange(reaction.params.randomDriveMinAngle_deg,
                                                   reaction.params.randomDriveMaxAngle_deg);

    bool randomAngleNegative = GetRNG().RandDbl() < 0.5;
    if( randomAngleNegative ) {
      randomAngleDeg = -randomAngleDeg;
    }

    float randomDist_mm = GetRNG().RandDblInRange(reaction.params.randomDriveMinDist_mm,
                                                  reaction.params.randomDriveMaxDist_mm);

    // pick a pose by starting at the robot pose, then turning by randomAngle, then driving straight by
    // randomDriveMaxDist_mm (note that the real path may be different than this). This makes it look nicer
    // because the robot always turns away first, as if saying "screw this". Note that pose applies
    // translation and then rotation, so this is done as two different transformations
    
    Pose3d randomPoseRot( DEG_TO_RAD(randomAngleDeg), Z_AXIS_3D(),
                          {0.0f, 0.0f, 0.0f},
                          &robot.GetPose() );
    Pose3d randomPoseRotAndTrans( 0.f, Z_AXIS_3D(),
                                  {randomDist_mm, 0.0f, 0.0f},
                                  &randomPoseRot );

    // TODO:(bn) motion profile?
    DriveToPoseAction* action = new DriveToPoseAction(robot, randomPoseRotAndTrans.GetWithRespectToOrigin());
    StartActing(action); // finish behavior when we are done
  }
  BehaviorObjectiveAchieved(BehaviorObjective::ReactedToFrustration);
}

BehaviorReactToFrustration::ReactionContainer::iterator BehaviorReactToFrustration::GetReaction(const Robot& robot)
{
  const float currTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();

  float confidentVal = robot.GetMoodManager().GetEmotionValue(EmotionType::Confident);

  // find the minimum score that is >= confidentVal
  
  float bestScore = 1.0f;
  ReactionContainer::iterator bestIt = _possibleReactions.end();
  
  for( auto it = _possibleReactions.begin(); it != _possibleReactions.end(); ++it ){
    if( confidentVal > it->params.maxConfidentScore ) {
      // this reaction isn't valid (we aren't frustrated enough for it)
      continue;
    }

    if( FLT_GT( it->lastReactedTime_s, 0.0f) && it->lastReactedTime_s + it->params.cooldownTime_s > currTime_s ) {
      // this reaction is on cooldown
      continue;
    }

    if( it->params.maxConfidentScore < bestScore ) {
      // we want to do the "strongest" allowable reaction
      
      // NOTE: this means we would could do the max strength reaction first, then that reaction could be on
      // cooldown, and we'd immediately do a less strong reaction.
      bestScore = it->params.maxConfidentScore;
      bestIt = it;
    }
  }

  return bestIt;
}

BehaviorReactToFrustration::FrustrationParams::FrustrationParams(const Json::Value& config)
{
  using namespace JsonTools;

  if( config.isNull() ) {
    PRINT_NAMED_ERROR("BehaviorReactToFrustration.FrustrationParams.InvalidConfig",
                      "config is null");
  }
  else {
    maxConfidentScore = ParseFloat(config, "maxConfidence", "BehaviorReactToFrustration::FrustrationParams");
    if( maxConfidentScore >= 0.0f ) {
      PRINT_NAMED_WARNING("BehaviorReactToFrustration.FrustrationParams.PositiveConfidence",
                          "you have a reaction in your config with a confidence value of %+f, these should be negative",
                          maxConfidentScore);
    }

    {
      const Json::Value& animJson = config["anim"];
      if( animJson.isString() ) {
        anim = AnimationTriggerFromString(animJson.asCString());
      }
    }
    
    GetValueOptional(config, "randomDriveMinDist_mm", randomDriveMinDist_mm);
    GetValueOptional(config, "randomDriveMaxDist_mm", randomDriveMaxDist_mm);
    GetValueOptional(config, "randomDriveMinAngle_deg", randomDriveMinAngle_deg);
    GetValueOptional(config, "randomDriveMaxAngle_deg", randomDriveMaxAngle_deg);
    GetValueOptional(config, "cooldownTime_s", cooldownTime_s);
    GetValueOptional(config, "finalEmotionEvent", finalEmotionEvent);
  }
}

}
}
