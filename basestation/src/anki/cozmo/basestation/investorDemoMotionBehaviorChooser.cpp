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

#include "anki/cozmo/basestation/behaviors/behaviorFollowMotion.h"
#include "anki/cozmo/basestation/behaviors/behaviorNone.h"
#include "anki/cozmo/basestation/behaviors/behaviorPounceOnMotion.h"
#include "anki/cozmo/basestation/investorDemoMotionBehaviorChooser.h"
#include "anki/cozmo/basestation/robot.h"

namespace Anki {
namespace Cozmo {

InvestorDemoMotionBehaviorChooser::InvestorDemoMotionBehaviorChooser(Robot& robot, const Json::Value& config)
  : super()
{
  SetupBehaviors(robot, config);

  // robot.GetVisionComponent().EnableMode(VisionMode::DetectingMotion, true);
}

void InvestorDemoMotionBehaviorChooser::SetupBehaviors(Robot& robot, const Json::Value& config)
{
  super::AddBehavior( new BehaviorNone(robot, config) );
  super::AddBehavior( new BehaviorPounceOnMotion(robot, config) );
  super::AddBehavior( new BehaviorFollowMotion(robot, config) );
}


Result InvestorDemoMotionBehaviorChooser::Update(double currentTime_sec)
{
  return super::Update(currentTime_sec);
}

}
}
