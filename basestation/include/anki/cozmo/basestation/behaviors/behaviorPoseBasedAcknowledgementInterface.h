/**
 * File: behaviorPoseBasedAcknowledgementInterface.h
 *
 * Author:  Andrew Stein / Brad Neuman
 * Created: 2016-06-16
 *
 * Description: Base class for behaviors which sotp to acknowledge something at a given pose
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#ifndef __Cozmo_Basestation_Behaviors_IBehaviorPoseBasedAcknowledgement_H__
#define __Cozmo_Basestation_Behaviors_IBehaviorPoseBasedAcknowledgement_H__

#include "anki/common/basestation/math/pose.h"
#include "anki/common/shared/radians.h"

#include "anki/cozmo/basestation/behaviors/behaviorInterface.h"
#include "clad/types/animationTrigger.h"

#include <map>

namespace Anki {
namespace Cozmo {
  
class IBehaviorPoseBasedAcknowledgement : public IBehavior
{
protected:
  // Note: no public constructor, not directly instantiable, including by BehaviorFactory
  IBehaviorPoseBasedAcknowledgement(Robot& robot, const Json::Value& config);
  
  // Default configuration parameters which can be overriden by JSON config
  struct ConfigParams {
    AnimationTrigger reactionAnimTrigger   = AnimationTrigger::InteractWithFacesInitial;
    Radians     maxTurnAngle_rad           = DEG_TO_RAD_F32(45.f);
    Radians     samePoseAngleThreshold_rad = DEG_TO_RAD_F32(45.f);
    Radians     panTolerance_rad           = DEG_TO_RAD_F32(5.f);
    Radians     tiltTolerance_rad          = DEG_TO_RAD_F32(5.f);
    TimeStamp_t coolDownDuration_ms        = 30000; // time since last seen (in same pose) before re-reaction
    float       samePoseDistThreshold_mm   = 80.f;
    float       scoreIncreaseWhileReacting = 0.3f;
    s32         numImagesToWaitFor         = 3; // After turning before playing animation
  } _params;
  
  void LoadConfig(const Json::Value& config);
  
  struct ReactionData
  {
    Pose3d      lastPose;
    TimeStamp_t lastSeenTime_ms;
  };

  bool GetReactionData(s32 idToFind, ReactionData* &data);
  void AddReactionData(s32 idToAdd, ReactionData&& data);
  bool RemoveReactionData(s32 idToRemove);
  
private:
  
  std::map<s32, ReactionData > _reactionData;
 
}; // class IBehaviorPoseBasedAcknowledgement

  
} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_Behaviors_IBehaviorPoseBasedAcknowledgement_H__
