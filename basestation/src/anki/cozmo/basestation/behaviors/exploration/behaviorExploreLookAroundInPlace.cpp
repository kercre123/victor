/**
 * File: behaviorExploreLookAroundInPlace
 *
 * Author: Raul
 * Created: 03/11/16
 *
 * Description: Behavior for looking around the environment for stuff to interact with. This behavior starts
 * as a copy from behaviorLookAround, which we expect to eventually replace. The difference is this new
 * behavior will simply gather information and put it in a place accessible to other behaviors, rather than
 * actually handling the observed information itself.
 *
 * Copyright: Anki, Inc. 2016
 *
 **/
#include "behaviorExploreLookAroundInPlace.h"

#include "anki/cozmo/basestation/actions/basicActions.h"
#include "anki/cozmo/basestation/robot.h"

#include "util/console/consoleInterface.h"
#include "util/logging/logging.h"
#include "util/math/numericCast.h"
#include "util/random/randomGenerator.h"

namespace Anki {
namespace Cozmo {

CONSOLE_VAR(float, kElaip_sx_BodyTurnSpeed_degPerSec, "BehaviorExploreLookAroundInPlace", 90.0f);
CONSOLE_VAR(float, kElaip_sxt_HeadTurnSpeed_degPerSec, "BehaviorExploreLookAroundInPlace", 15.0f); // for turn states
CONSOLE_VAR(float, kElaip_sxh_HeadTurnSpeed_degPerSec, "BehaviorExploreLookAroundInPlace", 60.0f); // for head move states
// chance that the main turn will be counter clockwise (vs ccw)
CONSOLE_VAR(float, kElaip_s0_MainTurnCWChance , "BehaviorExploreLookAroundInPlace", 0.75f);
// [min,max] range for random turn angles for step 1
CONSOLE_VAR(float, kElaip_s1_BodyAngleRangeMin_deg, "BehaviorExploreLookAroundInPlace",  10.0f);
CONSOLE_VAR(float, kElaip_s1_BodyAngleRangeMax_deg, "BehaviorExploreLookAroundInPlace",  30.0f);
CONSOLE_VAR(float, kElaip_s1_HeadAngleRangeMin_deg, "BehaviorExploreLookAroundInPlace", -20.0f);
CONSOLE_VAR(float, kElaip_s1_HeadAngleRangeMax_deg, "BehaviorExploreLookAroundInPlace", -12.0f);
// [min,max] range for pause for step 2
CONSOLE_VAR(float, kElaip_s2_WaitMin_sec, "BehaviorExploreLookAroundInPlace", 0.50f);
CONSOLE_VAR(float, kElaip_s2_WaitMax_sec, "BehaviorExploreLookAroundInPlace", 1.25f);
// [min,max] range for random angle turns for step 3
CONSOLE_VAR(float, kElaip_s3_BodyAngleRangeMin_deg, "BehaviorExploreLookAroundInPlace",  5.0f);
CONSOLE_VAR(float, kElaip_s3_BodyAngleRangeMax_deg, "BehaviorExploreLookAroundInPlace", 25.0f);
CONSOLE_VAR(float, kElaip_s3_HeadAngleRangeMin_deg, "BehaviorExploreLookAroundInPlace", -5.0f);
CONSOLE_VAR(float, kElaip_s3_HeadAngleRangeMax_deg, "BehaviorExploreLookAroundInPlace",  5.0f);
// [min,max] range for head move for step 4
CONSOLE_VAR(float, kElaip_s4_HeadAngleRangeMin_deg, "BehaviorExploreLookAroundInPlace",  5.0f);
CONSOLE_VAR(float, kElaip_s4_HeadAngleRangeMax_deg, "BehaviorExploreLookAroundInPlace", 20.0f);
CONSOLE_VAR(uint8_t, kElaip_s4_HeadAngleChangesMin, "BehaviorExploreLookAroundInPlace",  2);
CONSOLE_VAR(uint8_t, kElaip_s4_HeadAngleChangesMax, "BehaviorExploreLookAroundInPlace",  4);
// [min,max] range for head move  for step 5
CONSOLE_VAR(float, kElaip_s5_HeadAngleRangeMin_deg, "BehaviorExploreLookAroundInPlace", -5.0f);
CONSOLE_VAR(float, kElaip_s5_HeadAngleRangeMax_deg, "BehaviorExploreLookAroundInPlace",  5.0f);
// [min,max] range for random angle turns for step 6
CONSOLE_VAR(float, kElaip_s6_BodyAngleRangeMin_deg, "BehaviorExploreLookAroundInPlace",  30.0f);
CONSOLE_VAR(float, kElaip_s6_BodyAngleRangeMax_deg, "BehaviorExploreLookAroundInPlace",  65.0f);
CONSOLE_VAR(float, kElaip_s6_HeadAngleRangeMin_deg, "BehaviorExploreLookAroundInPlace", -12.0f);
CONSOLE_VAR(float, kElaip_s6_HeadAngleRangeMax_deg, "BehaviorExploreLookAroundInPlace",  -5.0f);


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorExploreLookAroundInPlace::BehaviorExploreLookAroundInPlace(Robot& robot, const Json::Value& config)
: IBehavior(robot, config)
, _startingBodyFacing_rad(0.0f)
, _mainTurnDirection(EClockDirection::CW)
, _s4HeadMovesLeft(0)
{
  SetDefaultName("BehaviorExploreLookAroundInPlace");

  SubscribeToTags({
    EngineToGameTag::RobotCompletedAction
  });
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorExploreLookAroundInPlace::~BehaviorExploreLookAroundInPlace()
{  
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorExploreLookAroundInPlace::IsRunnable(const Robot& robot, double currentTime_sec) const
{
  // TODO ask memory map for info and decide upon it
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result BehaviorExploreLookAroundInPlace::InitInternal(Robot& robot, double currentTime_sec)
{
  // start
  TransitionToS1_OppositeTurn(robot);

  return Result::RESULT_OK;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IBehavior::Status BehaviorExploreLookAroundInPlace::UpdateInternal(Robot& robot, double currentTime_sec)
{
  // while we are acting
  if ( IsActing() )
  {
    return Status::Running;
  }
  
  // done
  Status retval = Status::Complete;
  return retval;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorExploreLookAroundInPlace::TransitionToS1_OppositeTurn(Robot& robot)
{
  // cache init values
  _startingBodyFacing_rad = robot.GetPose().GetRotationAngle<'Z'>();
  
  // NOTE: Currently the behavior runs only 1 iteration (instead of running a full 360 loop). After that,
  // a new main turn direction could be picked. This can cause a ping-pong effect, resulting in the robot taking
  // a much longer time to clear the 360 degrees. Either the behavior should cover 360 degrees, or we need
  // additional information here about the range that we still have to cover. Disable chance for the moment until
  // I decide what to do
//  // decide main turn direction
//  const double randomDirection = GetRNG().RandDbl();
//  _mainTurnDirection = (randomDirection<=kElaip_s0_MainTurnCWChance) ? EClockDirection::CW : EClockDirection::CCW;
  _mainTurnDirection = EClockDirection::CW;
 
  // create turn action for this state
  const EClockDirection turnDir = GetOpposite(_mainTurnDirection);
  IAction* turnAction = CreateBodyAndHeadTurnAction(robot, turnDir,
        kElaip_s1_BodyAngleRangeMin_deg, kElaip_s1_BodyAngleRangeMax_deg,
        kElaip_s1_HeadAngleRangeMin_deg, kElaip_s1_HeadAngleRangeMax_deg,
        kElaip_sx_BodyTurnSpeed_degPerSec, kElaip_sxt_HeadTurnSpeed_degPerSec);

  // request action with transition to proper state
  StartActing( turnAction, &BehaviorExploreLookAroundInPlace::TransitionToS2_Pause );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorExploreLookAroundInPlace::TransitionToS2_Pause(Robot& robot)
{
  // create action
  const double waitTime_sec = GetRNG().RandDblInRange(kElaip_s2_WaitMin_sec, kElaip_s2_WaitMax_sec);
  WaitAction* waitAction = new WaitAction( robot, waitTime_sec );

  // request action with transition to proper state
  StartActing( waitAction, &BehaviorExploreLookAroundInPlace::TransitionToS3_MainTurn );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorExploreLookAroundInPlace::TransitionToS3_MainTurn(Robot& robot)
{
  // create turn action for this state
  const EClockDirection turnDir = _mainTurnDirection;
  IAction* turnAction = CreateBodyAndHeadTurnAction(robot, turnDir,
        kElaip_s3_BodyAngleRangeMin_deg, kElaip_s3_BodyAngleRangeMax_deg,
        kElaip_s3_HeadAngleRangeMin_deg, kElaip_s3_HeadAngleRangeMax_deg,
        kElaip_sx_BodyTurnSpeed_degPerSec, kElaip_sxt_HeadTurnSpeed_degPerSec);
  
  // calculate head moves now
  const int randMoves = GetRNG().RandIntInRange(kElaip_s4_HeadAngleChangesMin, kElaip_s4_HeadAngleChangesMax);
  _s4HeadMovesLeft = Util::numeric_cast<uint8_t>(randMoves);

  // request action with transition to proper state
  StartActing( turnAction, &BehaviorExploreLookAroundInPlace::TransitionToS4_HeadOnlyUp );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorExploreLookAroundInPlace::TransitionToS4_HeadOnlyUp(Robot& robot)
{
  // create head move action for this state
  IAction* moveHeadAction = CreateHeadTurnAction(robot,
        kElaip_s4_HeadAngleRangeMin_deg, kElaip_s4_HeadAngleRangeMax_deg,
        kElaip_sxh_HeadTurnSpeed_degPerSec);

  // count our action as a move
  --_s4HeadMovesLeft;

  // request action with transition to proper state
  if ( _s4HeadMovesLeft == 0 ) {
    StartActing( moveHeadAction, &BehaviorExploreLookAroundInPlace::TransitionToS5_HeadOnlyDown );
  } else {
    // still moves to do, continue in this state
    StartActing( moveHeadAction, &BehaviorExploreLookAroundInPlace::TransitionToS4_HeadOnlyUp );
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorExploreLookAroundInPlace::TransitionToS5_HeadOnlyDown(Robot& robot)
{
  // create head move action for this state
  IAction* moveHeadAction = CreateHeadTurnAction(robot,
        kElaip_s5_HeadAngleRangeMin_deg, kElaip_s5_HeadAngleRangeMax_deg,
        kElaip_sxh_HeadTurnSpeed_degPerSec);

  // request action with transition to proper state
  StartActing( moveHeadAction, &BehaviorExploreLookAroundInPlace::TransitionToS6_MainTurnFinal );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorExploreLookAroundInPlace::TransitionToS6_MainTurnFinal(Robot& robot)
{
  // create turn action for this state
  const EClockDirection turnDir = _mainTurnDirection;
  IAction* turnAction = CreateBodyAndHeadTurnAction(robot, turnDir,
        kElaip_s6_BodyAngleRangeMin_deg, kElaip_s6_BodyAngleRangeMax_deg,
        kElaip_s6_HeadAngleRangeMin_deg, kElaip_s6_HeadAngleRangeMax_deg,
        kElaip_sx_BodyTurnSpeed_degPerSec, kElaip_sxt_HeadTurnSpeed_degPerSec);

  // request action with transition to proper state
  StartActing( turnAction );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IAction* BehaviorExploreLookAroundInPlace::CreateBodyAndHeadTurnAction(Robot& robot, EClockDirection clockDirection,
  float bodyStartRelativeMin_deg, float bodyStartRelativeMax_deg,
  float headAbsoluteMin_deg, float headAbsoluteMax_deg,
  float bodyTurnSpeed_degPerSec, float headTurnSpeed_degPerSec)
{
  // [min,max] range for random body angle turn
  const EClockDirection turnDir = clockDirection;
  const double bodyTargetAngleRelative_deg =
    GetRNG().RandDblInRange(bodyStartRelativeMin_deg, bodyStartRelativeMax_deg) * GetTurnSign(turnDir);
  
  // [min,max] range for random head angle turn
  const double headTargetAngleAbs_deg =
    GetRNG().RandDblInRange(headAbsoluteMin_deg, headAbsoluteMax_deg);

  // create proper action for body & head turn
  const Radians bodyTargetAngleAbs_rad( _startingBodyFacing_rad + DEG_TO_RAD(bodyTargetAngleRelative_deg) );
  const Radians headTargetAngleAbs_rad( DEG_TO_RAD(headTargetAngleAbs_deg) );
  PanAndTiltAction* turnAction = new PanAndTiltAction(robot, bodyTargetAngleAbs_rad, headTargetAngleAbs_rad, true, true);
  turnAction->SetMaxPanSpeed( DEG_TO_RAD(kElaip_sx_BodyTurnSpeed_degPerSec) );
  turnAction->SetMaxTiltSpeed( DEG_TO_RAD(kElaip_sxt_HeadTurnSpeed_degPerSec) );

  return turnAction;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IAction* BehaviorExploreLookAroundInPlace::CreateHeadTurnAction(Robot& robot,
  float headAbsoluteMin_deg, float headAbsoluteMax_deg,
  float headTurnSpeed_degPerSec)
{
  // [min,max] range for random head angle turn
  const double headTargetAngleAbs_deg =
    GetRNG().RandDblInRange(headAbsoluteMin_deg, headAbsoluteMax_deg);

  // create proper action for body & head turn
  const Radians headTargetAngleAbs_rad( DEG_TO_RAD(headTargetAngleAbs_deg) );
  MoveHeadToAngleAction* moveHeadAction = new MoveHeadToAngleAction(robot, headTargetAngleAbs_rad);
  moveHeadAction->SetMaxSpeed( DEG_TO_RAD(headTurnSpeed_degPerSec) );

  return moveHeadAction;
}

} // namespace Cozmo
} // namespace Anki
