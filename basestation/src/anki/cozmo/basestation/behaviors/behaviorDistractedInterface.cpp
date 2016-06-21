/**
 * File: behaviorDistractedInterface.cpp
 *
 * Author:  Andrew Stein
 * Created: 2016-06-16
 *
 * Description:  Base class for distraction behaviors

 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#include "anki/cozmo/basestation/behaviors/behaviorDistractedInterface.h"
#include "anki/cozmo/basestation/robot.h"


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
  
IBehaviorDistracted::IBehaviorDistracted(Robot& robot, const Json::Value& config)
: IBehavior(robot, config)
{
  LoadConfig(config);
}

void IBehaviorDistracted::LoadConfig(const Json::Value& config)
{
  using namespace JsonTools;
  
  if(GetValueOptional(config, kReactionAnimGroupKey, _params.reactionAnimGroup)) {
    PRINT_NAMED_DEBUG("IBehaviorDistracted.LoadConfig.SetReactionAnimGroup",
                      "%s", _params.reactionAnimGroup.c_str());
  }
  
  if(GetAngleOptional(config, kMaxTurnAngleKey, _params.maxTurnAngle_rad, true)) {
    PRINT_NAMED_DEBUG("IBehaviorDistracted.LoadConfig.SetMaxTurnAngle",
                      "%.1fdeg", _params.maxTurnAngle_rad.getDegrees());
  }
  
  if(GetValueOptional(config, kCoolDownDurationKey , _params.coolDownDuration_ms)) {
    PRINT_NAMED_DEBUG("IBehaviorDistracted.LoadConfig.SetCoolDownDuration",
                      "%ums", _params.coolDownDuration_ms);
  }
  
  if(GetValueOptional(config, kSamePoseDistKey , _params.samePoseDistThreshold_mm)) {
    PRINT_NAMED_DEBUG("IBehaviorDistracted.LoadConfig.SetPoseDistThresh",
                      "%.1f", _params.samePoseDistThreshold_mm);
  }
  
  if(GetAngleOptional(config, kSamePoseAngleKey, _params.samePoseAngleThreshold_rad, true)) {
    PRINT_NAMED_DEBUG("IBehaviorDistracted.LoadConfig.SetPoseAngleThresh",
                      "%.1fdeg", _params.samePoseAngleThreshold_rad.getDegrees());
  }
  
  if(GetAngleOptional(config, kPanToleranceKey, _params.panTolerance_rad, true)) {
    PRINT_NAMED_DEBUG("IBehaviorDistracted.LoadConfig.SetPanTolerance",
                      "%.1fdeg", _params.panTolerance_rad.getDegrees());
  }
  
  if(GetAngleOptional(config, kTiltToleranceKey, _params.tiltTolerance_rad, true)) {
    PRINT_NAMED_DEBUG("IBehaviorDistracted.LoadConfig.SetTiltTolerance",
                      "%.1fdeg", _params.tiltTolerance_rad.getDegrees());
  }

  if(GetValueOptional(config, kScoreIncreaseKey, _params.scoreIncreaseWhileReacting)) {
    PRINT_NAMED_DEBUG("IBehaviorDistracted.LoadConfig.SetScoreIncreaseWhileReacting",
                      "%.3f", _params.scoreIncreaseWhileReacting);
  }
  
  if(GetValueOptional(config, kNumImagesToWaitForKey, _params.numImagesToWaitFor)) {
    PRINT_NAMED_DEBUG("BehaviorDistractedByObject.LoadConfig.SetNumImagesToWaitFor",
                      "%d", _params.numImagesToWaitFor);
  }
} // LoadConfig()
  

bool IBehaviorDistracted::GetReactionData(s32 idToFind, ReactionData* &data)
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
  
void IBehaviorDistracted::AddReactionData(s32 idToAdd, ReactionData &&data)
{
  _reactionData[idToAdd] = std::move(data);
}
  
bool IBehaviorDistracted::RemoveReactionData(s32 idToRemove)
{
  const size_t N = _reactionData.erase(idToRemove);
  const bool idRemoved = (N > 0);
  return idRemoved;
}
  

} // namespace Cozmo
} // namespace Anki

