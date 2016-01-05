/**
 * File: investorDemoMotionBehaviorChooser.cpp
 *
 * Author: Brad Neuman
 * Created: 2015-11-25
 *
 * Description: The behavior chooser for the investor demo.
 *
 * Copyright: Anki, Inc. 2015
 *
 **/

#include "anki/cozmo/basestation/investorDemoMotionBehaviorChooser.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorFactory.h"
#include "anki/cozmo/basestation/robot.h"

namespace Anki {
namespace Cozmo {

InvestorDemoMotionBehaviorChooser::InvestorDemoMotionBehaviorChooser(Robot& robot, const Json::Value& config)
  : super()
{
  SetupBehaviors(robot, config);

  // enable live idle animation
  robot.SetIdleAnimation(AnimationStreamer::LiveAnimation);
}

void InvestorDemoMotionBehaviorChooser::SetupBehaviors(Robot& robot, const Json::Value& config)
{
  super::AddBehavior( robot.GetBehaviorFactory().CreateBehavior(BehaviorType::NoneBehavior,   robot, config) );
  super::AddBehavior( robot.GetBehaviorFactory().CreateBehavior(BehaviorType::PounceOnMotion, robot, config) );
  super::AddBehavior( robot.GetBehaviorFactory().CreateBehavior(BehaviorType::FollowMotion,   robot, config) );
}

Result InvestorDemoMotionBehaviorChooser::Update(double currentTime_sec)
{
  return super::Update(currentTime_sec);
}

}
}
