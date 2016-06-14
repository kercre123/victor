/**
 * File: behaviorDistractedInterface.h
 *
 * Author:  Andrew Stein
 * Created: 2016-06-16
 *
 * Description:  Base class for distraction behaviors
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#ifndef __Cozmo_Basestation_Behaviors_IBehaviorDistracted_H__
#define __Cozmo_Basestation_Behaviors_IBehaviorDistracted_H__

#include "anki/common/basestation/math/pose.h"
#include "anki/common/shared/radians.h"

#include "anki/cozmo/basestation/behaviors/behaviorInterface.h"

#include <map>

namespace Anki {
namespace Cozmo {
  
class IBehaviorDistracted : public IBehavior
{
protected:
  // Note: no public constructor, not directly instantiable, including by BehaviorFactory
  IBehaviorDistracted(Robot& robot, const Json::Value& config);
  
  // Default configuration parameters which can be overriden by JSON config
  struct ConfigParams {
    std::string reactionAnimGroup          = "interactWithFaces_initial";
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
 
}; // class IBehaviorDistracted

  
} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_Behaviors_IBehaviorDistracted_H__
