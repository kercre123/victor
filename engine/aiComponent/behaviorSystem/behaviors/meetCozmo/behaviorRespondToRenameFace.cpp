/**
 * File: behaviorRespondToRenameFace.cpp
 *
 * Author: Andrew Stein
 * Created: 12/13/2016
 *
 * Description: Behavior for responding to a face being renamed
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#include "engine/aiComponent/behaviorSystem/behaviors/meetCozmo/behaviorRespondToRenameFace.h"

#include "engine/actions/basicActions.h"
#include "engine/actions/sayTextAction.h"
#include "engine/events/ankiEvent.h"
#include "engine/events/animationTriggerHelpers.h"
#include "engine/externalInterface/externalInterface.h"
#include "engine/robot.h"

namespace Anki {
namespace Cozmo {
  
namespace JsonKeys {
  static const char * const AnimationTriggerKey = "animationTrigger";
}
  

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorRespondToRenameFace::BehaviorRespondToRenameFace(const Json::Value& config)
: IBehavior(config)
, _name("")
, _faceID(Vision::UnknownFaceID)
{
  SubscribeToTags({GameToEngineTag::UpdateEnrolledFaceByID});
  
  const std::string& animTriggerString = config.get(JsonKeys::AnimationTriggerKey, "MeetCozmoRenameFaceSayName").asString();
  _animTrigger = AnimationTriggerFromString(animTriggerString.c_str());
  
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorRespondToRenameFace::HandleWhileNotRunning(const GameToEngineEvent& event, BehaviorExternalInterface& behaviorExternalInterface)
{
  auto & msg = event.GetData().Get_UpdateEnrolledFaceByID();
  
  _name   = msg.newName;
  _faceID = msg.faceID;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorRespondToRenameFace::IsRunnableInternal(BehaviorExternalInterface& behaviorExternalInterface) const
{
  const bool haveValidName = !_name.empty();
  return haveValidName;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result BehaviorRespondToRenameFace::OnBehaviorActivated(BehaviorExternalInterface& behaviorExternalInterface)
{
  if(_name.empty())
  {
    PRINT_NAMED_ERROR("BehaviorRespondToRenameFace.InitInternal.EmptyName", "");
    return RESULT_FAIL;
  }
  
  PRINT_CH_INFO("Behaviors", "BehaviorRespondToRenameFace.InitInternal",
                "Responding to rename of %s with %s",
                Util::HidePersonallyIdentifiableInfo(_name.c_str()),
                EnumToString(_animTrigger));
  
  // TODO: Try to turn towards a/the face first COZMO-7991
  //  For some reason the following didn't work (action immediately completed) and I ran
  //  out of time figuring out why. I also tried simply turning towards last face pose with
  //  no luck.
  //
  //  const bool kSayName = true;
  //  TurnTowardsFaceAction* turnTowardsFace = new TurnTowardsLastFacePoseAction(robot, _faceID, M_PI_F, kSayName);
  //
  //  // Play the animation trigger whether or not we find the face
  //  turnTowardsFace->SetSayNameAnimationTrigger(_animTrigger);
  //  turnTowardsFace->SetNoNameAnimationTrigger(_animTrigger);
  //  
  //  StartActing(turnTowardsFace);
  
  // DEPRECATED - Grabbing robot to support current cozmo code, but this should
  // be removed
  Robot& robot = behaviorExternalInterface.GetRobot();
  SayTextAction* sayName = new SayTextAction(robot, _name, SayTextIntent::Name_Normal);
  sayName->SetAnimationTrigger(_animTrigger);
  
  StartActing(sayName);
  
  _name.clear();
  _faceID = Vision::UnknownFaceID;
  
  return RESULT_OK;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IBehavior::Status BehaviorRespondToRenameFace::UpdateInternal(BehaviorExternalInterface& behaviorExternalInterface)
{
  if(!IsActing())
  {
    return Status::Complete;
  }
  
  return Status::Running;
}


} // namespace Cozmo
} // namespace Anki
