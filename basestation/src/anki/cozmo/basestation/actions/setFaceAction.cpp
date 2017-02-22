/**
 * File: setFaceAction.cpp
 *
 * Author: Andrew Stein
 * Date:   9/12/2016
 *
 *
 * Description: Implements an action for setting a procedural or bitmap face on Cozmo.
 *
 *
 * Copyright: Anki, Inc. 2016
 **/


#include "anki/cozmo/basestation/actions/animActions.h"
#include "anki/cozmo/basestation/actions/setFaceAction.h"
#include "anki/cozmo/basestation/keyframe.h"
#include "anki/cozmo/basestation/robot.h"
#include "util/math/math.h"

namespace Anki {
namespace Cozmo {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SetFaceAction::SetFaceAction(Robot& robot, const Vision::Image& faceImage, u32 duration_ms)
: IAction(robot,
          "SetFaceImage",
          RobotActionType::DISPLAY_FACE_IMAGE,
          (u8)AnimTrackFlag::NO_TRACKS)
, _faceImage(faceImage)
, _animation("SetFaceImageAnimation")
, _duration_ms(duration_ms)
{
  
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SetFaceAction::SetFaceAction(Robot& robot, const ProceduralFace& procFace, u32 duration_ms)
: IAction(robot,
          "SetProceduralFace",
          RobotActionType::DISPLAY_PROCEDURAL_FACE,
          (u8)AnimTrackFlag::NO_TRACKS)
, _procFace(procFace)
, _animation("SetProceduralFaceAnimation")
, _duration_ms(duration_ms)
{
  
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ActionResult SetFaceAction::Init()
{
  _animation.SetIsLive(true);
  
  const u32 kMaxDurationToAvoidScreenBurnIn_ms = 30000; // TODO: use value from AnimationStreamer or elsewhere
  if(_duration_ms > kMaxDurationToAvoidScreenBurnIn_ms)
  {
    PRINT_NAMED_WARNING("SetFaceAction.Init.DurationTooLong",
                        "Clamping duration to %.1f seconds to avoid screen burn-in",
                        (f32)kMaxDurationToAvoidScreenBurnIn_ms * 0.001f);
    
    _duration_ms = kMaxDurationToAvoidScreenBurnIn_ms;
  }
  else if(_duration_ms < IKeyFrame::SAMPLE_LENGTH_MS)
  {
    PRINT_NAMED_WARNING("SetFaceAction.Init.DurationTooShort",
                        "Clamping duration to %ums which is the minimum animation resolution",
                        IKeyFrame::SAMPLE_LENGTH_MS);
    
    _duration_ms = IKeyFrame::SAMPLE_LENGTH_MS;
  }
  
  Result addResult = RESULT_FAIL;
  switch(GetType())
  {
    case RobotActionType::DISPLAY_PROCEDURAL_FACE:
    {
      // Always add one keyframe
      addResult = _animation.AddKeyFrameToBack(ProceduralFaceKeyFrame(_procFace));
      
      // Add a second one later to interpolate to, if duration is longer than one keyframe
      if(RESULT_OK == addResult && _duration_ms > IKeyFrame::SAMPLE_LENGTH_MS)
      {
        addResult = _animation.AddKeyFrameToBack(ProceduralFaceKeyFrame(_procFace, _duration_ms-IKeyFrame::SAMPLE_LENGTH_MS));
      }
        
      break;
    }
  
    case RobotActionType::DISPLAY_FACE_IMAGE:
    {
      FaceAnimationManager* faceAnimMgr = FaceAnimationManager::getInstance();
      faceAnimMgr->ClearAnimation(FaceAnimationManager::ProceduralAnimName);
      addResult = faceAnimMgr->AddImage(FaceAnimationManager::ProceduralAnimName, _faceImage, _duration_ms);
      if(RESULT_OK == addResult)
      {
        addResult = _animation.AddKeyFrameToBack(FaceAnimationKeyFrame(FaceAnimationManager::ProceduralAnimName));
      }
      break;
    }
      
    default:
      PRINT_NAMED_ERROR("SetFaceAction.Init.BadActionType",
                        "Unexpected type: %s", EnumToString(GetType()));
      return ActionResult::ABORT;
  }
  
  if(RESULT_OK != addResult)
  {
    PRINT_NAMED_WARNING("SetFaceAction.Init.AddFaceKeyFrameFailed", "Type=%s",
                        EnumToString(GetType()));
    return ActionResult::ABORT;
  }
  
  _playAnimationAction.reset(new PlayAnimationAction(_robot, &_animation));
  
  return ActionResult::SUCCESS;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SetFaceAction::~SetFaceAction()
{
  if(_playAnimationAction != nullptr) {
    _playAnimationAction->PrepForCompletion();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ActionResult SetFaceAction::CheckIfDone()
{
  return _playAnimationAction->Update();
}
  
} // namespace Cozmo
} // namespace Anki
