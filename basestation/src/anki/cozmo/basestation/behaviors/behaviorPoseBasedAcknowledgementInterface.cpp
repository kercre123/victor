/**
 * File: behaviorPoseBasedAcknowledgementInterface.cpp
 *
 * Author:  Andrew Stein
 * Created: 2016-06-16
 *
 * Description:  Base class for distraction behaviors

 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#include "anki/cozmo/basestation/behaviors/behaviorPoseBasedAcknowledgementInterface.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/basestation/events/animationTriggerHelpers.h"


namespace Anki {
namespace Cozmo {

static const char * const kReactionAnimGroupKey = "ReactionAnimGroup";
static const char * const kMaxTurnAngleKey      = "MaxTurnAngle_deg";
static const char * const kCoolDownDurationKey  = "CoolDownDuration_ms";
static const char * const kSamePoseDistKey      = "SamePoseDistThresh_mm";
static const char * const kSamePoseAngleKey     = "SamePoseAngleThresh_deg";
static const char * const kPanToleranceKey      = "PanTolerance_deg";
static const char * const kTiltToleranceKey     = "TiltTolerance_deg";
static const char * const kScoreIncreaseKey     = "ScoreIncreaseWhileReacting";
static const char * const kNumImagesToWaitForKey= "NumImagesToWaitFor";
  
IBehaviorPoseBasedAcknowledgement::IBehaviorPoseBasedAcknowledgement(Robot& robot, const Json::Value& config)
: IBehavior(robot, config)
{
  LoadConfig(config);
}

void IBehaviorPoseBasedAcknowledgement::LoadConfig(const Json::Value& config)
{
  using namespace JsonTools;
  
  JsonTools::GetValueOptional(config,kReactionAnimGroupKey,_params.reactionAnimTrigger);
  
  if(GetAngleOptional(config, kMaxTurnAngleKey, _params.maxTurnAngle_rad, true)) {
    PRINT_NAMED_DEBUG("IBehaviorPoseBasedAcknowledgement.LoadConfig.SetMaxTurnAngle",
                      "%.1fdeg", _params.maxTurnAngle_rad.getDegrees());
  }
  
  if(GetValueOptional(config, kCoolDownDurationKey , _params.coolDownDuration_ms)) {
    PRINT_NAMED_DEBUG("IBehaviorPoseBasedAcknowledgement.LoadConfig.SetCoolDownDuration",
                      "%ums", _params.coolDownDuration_ms);
  }
  
  if(GetValueOptional(config, kSamePoseDistKey , _params.samePoseDistThreshold_mm)) {
    PRINT_NAMED_DEBUG("IBehaviorPoseBasedAcknowledgement.LoadConfig.SetPoseDistThresh",
                      "%.1f", _params.samePoseDistThreshold_mm);
  }
  
  if(GetAngleOptional(config, kSamePoseAngleKey, _params.samePoseAngleThreshold_rad, true)) {
    PRINT_NAMED_DEBUG("IBehaviorPoseBasedAcknowledgement.LoadConfig.SetPoseAngleThresh",
                      "%.1fdeg", _params.samePoseAngleThreshold_rad.getDegrees());
  }
  
  if(GetAngleOptional(config, kPanToleranceKey, _params.panTolerance_rad, true)) {
    PRINT_NAMED_DEBUG("IBehaviorPoseBasedAcknowledgement.LoadConfig.SetPanTolerance",
                      "%.1fdeg", _params.panTolerance_rad.getDegrees());
  }
  
  if(GetAngleOptional(config, kTiltToleranceKey, _params.tiltTolerance_rad, true)) {
    PRINT_NAMED_DEBUG("IBehaviorPoseBasedAcknowledgement.LoadConfig.SetTiltTolerance",
                      "%.1fdeg", _params.tiltTolerance_rad.getDegrees());
  }

  if(GetValueOptional(config, kScoreIncreaseKey, _params.scoreIncreaseWhileReacting)) {
    PRINT_NAMED_DEBUG("IBehaviorPoseBasedAcknowledgement.LoadConfig.SetScoreIncreaseWhileReacting",
                      "%.3f", _params.scoreIncreaseWhileReacting);
  }
  
  if(GetValueOptional(config, kNumImagesToWaitForKey, _params.numImagesToWaitFor)) {
    PRINT_NAMED_DEBUG("IBehaviorPoseBasedAcknowledgement.LoadConfig.SetNumImagesToWaitFor",
                      "%d", _params.numImagesToWaitFor);
  }
} // LoadConfig()
  

bool IBehaviorPoseBasedAcknowledgement::GetReactionData(s32 idToFind, ReactionData* &data)
{
  auto iter = _reactionData.find(idToFind);
  if(iter == _reactionData.end())
  {
    data = nullptr;
    return false;
  }
  else
  {
    data = &(iter->second);
    return true;
  }
}
  
void IBehaviorPoseBasedAcknowledgement::AddReactionData(s32 idToAdd, ReactionData &&data)
{
  _reactionData[idToAdd] = std::move(data);
}
  
bool IBehaviorPoseBasedAcknowledgement ::RemoveReactionData(s32 idToRemove)
{
  const size_t N = _reactionData.erase(idToRemove);
  const bool idRemoved = (N > 0);
  return idRemoved;
}
  

} // namespace Cozmo
} // namespace Anki

