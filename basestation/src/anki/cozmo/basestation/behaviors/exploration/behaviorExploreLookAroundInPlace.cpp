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

#include "anki/common/basestation/math/point_impl.h"

#include "util/console/consoleInterface.h"
#include "util/logging/logging.h"
#include "util/math/numericCast.h"
#include "util/random/randomGenerator.h"

namespace Anki {
namespace Cozmo {

static const char* kConfigParamsKey = "params";

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorExploreLookAroundInPlace::BehaviorExploreLookAroundInPlace(Robot& robot, const Json::Value& config)
: IBehavior(robot, config)
, _configParams{}
, _iterationStartingBodyFacing_rad(0.0f)
, _behaviorBodyFacingDone_rad(0.0f)
, _mainTurnDirection(EClockDirection::CW)
, _s4HeadMovesLeft(0)
{
  SetDefaultName("BehaviorExploreLookAroundInPlace");

  SubscribeToTags({
    EngineToGameTag::RobotCompletedAction
  });
  
  // parse all parameters now
  LoadConfig(config[kConfigParamsKey]);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorExploreLookAroundInPlace::~BehaviorExploreLookAroundInPlace()
{  
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorExploreLookAroundInPlace::IsRunnableInternal(const Robot& robot) const
{
  // Probably want to run if I don't have any other exploration behavior that wants to, unless I have completely
  // mapped the floor around me 'recently'.
  // Now this is the case for exploration, but some other supergroup that uses the same behavior would have different
  // conditions. Maybe conditions should be datadriven?
  bool nearRecentLocation = false;
  for( const auto& recentLocation : _visitedLocations )
  {
    // if this location is from a different origin simply ignore it
    const bool sharesOrigin = (&recentLocation.FindOrigin()) == (&robot.GetPose().FindOrigin());
    if ( !sharesOrigin ) {
      continue;
    }

    // if close to any recent location, flag
    const float distSQ = (robot.GetPose().GetTranslation() - recentLocation.GetTranslation()).LengthSq();
    const float maxDistSq = _configParams.behavior_DistanceFromRecentLocationMin_mm*_configParams.behavior_DistanceFromRecentLocationMin_mm;
    if ( distSQ < maxDistSq ) {
      nearRecentLocation = true;
      break;
    }
  }

  // TODO consider asking memory map for info and decide upon it instead of just based on recent locations?
  const bool canRun = !nearRecentLocation;
  return canRun;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
namespace {
float ParseFloat(const Json::Value& config, const char* key ) {
  ASSERT_NAMED_EVENT(config[key].isNumeric(), "BehaviorExploreLookAroundInPlace.ParseFloat.NotValidFloat", "%s", key);
  return config[key].asFloat();
}
uint8_t ParseUint8(const Json::Value& config, const char* key ) {
  ASSERT_NAMED_EVENT(config[key].isNumeric(), "BehaviorExploreLookAroundInPlace.ParseUint8.NotValidUint8", "%s", key);
  Json::Int intVal = config[key].asInt();
  return Anki::Util::numeric_cast<uint8_t>(intVal);
}
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorExploreLookAroundInPlace::LoadConfig(const Json::Value& config)
{
  _configParams.behavior_DistanceFromRecentLocationMin_mm = ParseFloat(config, "behavior_DistanceFromRecentLocationMin_mm");
  _configParams.behavior_RecentLocationsMax = ParseUint8(config, "behavior_RecentLocationsMax");
  // turn speed
  _configParams.sx_BodyTurnSpeed_degPerSec = ParseFloat(config, "sx_BodyTurnSpeed_degPerSec");
  _configParams.sxt_HeadTurnSpeed_degPerSec = ParseFloat(config, "sxt_HeadTurnSpeed_degPerSec");
  _configParams.sxh_HeadTurnSpeed_degPerSec = ParseFloat(config, "sxh_HeadTurnSpeed_degPerSec");
  // chance that the main turn will be counter clockwise (vs ccw)
  _configParams.s0_MainTurnCWChance = ParseFloat(config, "s0_MainTurnCWChance");
  // [min,max] range for random turn angles for step 1
  _configParams.s1_BodyAngleRangeMin_deg = ParseFloat(config, "s1_BodyAngleRangeMin_deg");
  _configParams.s1_BodyAngleRangeMax_deg = ParseFloat(config, "s1_BodyAngleRangeMax_deg");
  _configParams.s1_HeadAngleRangeMin_deg = ParseFloat(config, "s1_HeadAngleRangeMin_deg");
  _configParams.s1_HeadAngleRangeMax_deg = ParseFloat(config, "s1_HeadAngleRangeMax_deg");
  // [min,max] range for pause for step 2
  _configParams.s2_WaitMin_sec = ParseFloat(config, "s2_WaitMin_sec");
  _configParams.s2_WaitMax_sec = ParseFloat(config, "s2_WaitMax_sec");
  // [min,max] range for random angle turns for step 3
  _configParams.s3_BodyAngleRangeMin_deg = ParseFloat(config, "s3_BodyAngleRangeMin_deg");
  _configParams.s3_BodyAngleRangeMax_deg = ParseFloat(config, "s3_BodyAngleRangeMax_deg");
  _configParams.s3_HeadAngleRangeMin_deg = ParseFloat(config, "s3_HeadAngleRangeMin_deg");
  _configParams.s3_HeadAngleRangeMax_deg = ParseFloat(config, "s3_HeadAngleRangeMax_deg");
  // [min,max] range for head move for step 4
  _configParams.s4_HeadAngleRangeMin_deg = ParseFloat(config, "s4_HeadAngleRangeMin_deg");
  _configParams.s4_HeadAngleRangeMax_deg = ParseFloat(config, "s4_HeadAngleRangeMax_deg");
  _configParams.s4_HeadAngleChangesMin = ParseUint8(config, "s4_HeadAngleChangesMin");
  _configParams.s4_HeadAngleChangesMax = ParseUint8(config, "s4_HeadAngleChangesMax");
  // [min,max] range for head move  for step 5
  _configParams.s5_HeadAngleRangeMin_deg = ParseFloat(config, "s5_HeadAngleRangeMin_deg");
  _configParams.s5_HeadAngleRangeMax_deg = ParseFloat(config, "s5_HeadAngleRangeMax_deg");
  // [min,max] range for random angle turns for step 6
  _configParams.s6_BodyAngleRangeMin_deg = ParseFloat(config, "s6_BodyAngleRangeMin_deg");
  _configParams.s6_BodyAngleRangeMax_deg = ParseFloat(config, "s6_BodyAngleRangeMax_deg");
  _configParams.s6_HeadAngleRangeMin_deg = ParseFloat(config, "s6_HeadAngleRangeMin_deg");
  _configParams.s6_HeadAngleRangeMax_deg = ParseFloat(config, "s6_HeadAngleRangeMax_deg");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result BehaviorExploreLookAroundInPlace::InitInternal(Robot& robot)
{
  // grab run values
  _behaviorBodyFacingDone_rad = 0;
  
  // decide main turn direction
  const double randomDirection = GetRNG().RandDbl();
  _mainTurnDirection = (randomDirection<=_configParams.s0_MainTurnCWChance) ? EClockDirection::CW : EClockDirection::CCW;
  // _mainTurnDirection = EClockDirection::CW;

  // start first iteration
  TransitionToS1_OppositeTurn(robot);

  return Result::RESULT_OK;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IBehavior::Status BehaviorExploreLookAroundInPlace::UpdateInternal(Robot& robot)
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
  // cache iteration values
  _iterationStartingBodyFacing_rad = robot.GetPose().GetRotationAngle<'Z'>();
  
  // create turn action for this state
  const EClockDirection turnDir = GetOpposite(_mainTurnDirection);
  IAction* turnAction = CreateBodyAndHeadTurnAction(robot, turnDir,
        _configParams.s1_BodyAngleRangeMin_deg, _configParams.s1_BodyAngleRangeMax_deg,
        _configParams.s1_HeadAngleRangeMin_deg, _configParams.s1_HeadAngleRangeMax_deg,
        _configParams.sx_BodyTurnSpeed_degPerSec, _configParams.sxt_HeadTurnSpeed_degPerSec);

  // request action with transition to proper state
  StartActing( turnAction, &BehaviorExploreLookAroundInPlace::TransitionToS2_Pause );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorExploreLookAroundInPlace::TransitionToS2_Pause(Robot& robot)
{
  // create action
  const double waitTime_sec = GetRNG().RandDblInRange(_configParams.s2_WaitMin_sec, _configParams.s2_WaitMax_sec);
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
        _configParams.s3_BodyAngleRangeMin_deg, _configParams.s3_BodyAngleRangeMax_deg,
        _configParams.s3_HeadAngleRangeMin_deg, _configParams.s3_HeadAngleRangeMax_deg,
        _configParams.sx_BodyTurnSpeed_degPerSec, _configParams.sxt_HeadTurnSpeed_degPerSec);
  
  // calculate head moves now
  const int randMoves = GetRNG().RandIntInRange(_configParams.s4_HeadAngleChangesMin, _configParams.s4_HeadAngleChangesMax);
  _s4HeadMovesLeft = Util::numeric_cast<uint8_t>(randMoves);

  // request action with transition to proper state
  StartActing( turnAction, &BehaviorExploreLookAroundInPlace::TransitionToS4_HeadOnlyUp );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorExploreLookAroundInPlace::TransitionToS4_HeadOnlyUp(Robot& robot)
{
  // create head move action for this state
  IAction* moveHeadAction = CreateHeadTurnAction(robot,
        _configParams.s4_HeadAngleRangeMin_deg, _configParams.s4_HeadAngleRangeMax_deg,
        _configParams.sxh_HeadTurnSpeed_degPerSec);

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
        _configParams.s5_HeadAngleRangeMin_deg, _configParams.s5_HeadAngleRangeMax_deg,
        _configParams.sxh_HeadTurnSpeed_degPerSec);

  // request action with transition to proper state
  StartActing( moveHeadAction, &BehaviorExploreLookAroundInPlace::TransitionToS6_MainTurnFinal );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorExploreLookAroundInPlace::TransitionToS6_MainTurnFinal(Robot& robot)
{
  // create turn action for this state
  const EClockDirection turnDir = _mainTurnDirection;
  IAction* turnAction = CreateBodyAndHeadTurnAction(robot, turnDir,
        _configParams.s6_BodyAngleRangeMin_deg, _configParams.s6_BodyAngleRangeMax_deg,
        _configParams.s6_HeadAngleRangeMin_deg, _configParams.s6_HeadAngleRangeMax_deg,
        _configParams.sx_BodyTurnSpeed_degPerSec, _configParams.sxt_HeadTurnSpeed_degPerSec);

  // request action with transition to proper state
  StartActing( turnAction, &BehaviorExploreLookAroundInPlace::TransitionToS7_IterationEnd );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorExploreLookAroundInPlace::TransitionToS7_IterationEnd(Robot& robot)
{
  Radians currentZ_rad = robot.GetPose().GetRotationAngle<'Z'>();
  float doneThisIteration_rad = (currentZ_rad - _iterationStartingBodyFacing_rad).ToFloat();
  _behaviorBodyFacingDone_rad += doneThisIteration_rad;
  
  // assert we are not turning more than PI in one iteration (because of Radian rescaling)
  ASSERT_NAMED( FLT_GT(doneThisIteration_rad,0.0f) == FLT_GT(GetTurnSign(_mainTurnDirection), 0.0f),
    "BehaviorExploreLookAroundInPlace.TransitionToS7_IterationEnd.BadSign");

  // while not completed a whole turn start another iteration
  if ( fabsf(_behaviorBodyFacingDone_rad) < 2*PI )
  {
    TransitionToS1_OppositeTurn(robot);
  }
  else
  {
    // we have finished a location by iterating more than 360 degrees, note down as recent location
    // make room if necessary
    if ( _visitedLocations.size() >= _configParams.behavior_RecentLocationsMax ) {
      assert( !_visitedLocations.empty() ); // otherwise the limit of locations is 0, so the behavior would run forever
      _visitedLocations.pop_front();
    }
  
    // note down this location so that we don't do it again in the same place
    _visitedLocations.emplace_back( robot.GetPose() );
  }
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
  const Radians bodyTargetAngleAbs_rad( _iterationStartingBodyFacing_rad + DEG_TO_RAD(bodyTargetAngleRelative_deg) );
  const Radians headTargetAngleAbs_rad( DEG_TO_RAD(headTargetAngleAbs_deg) );
  PanAndTiltAction* turnAction = new PanAndTiltAction(robot, bodyTargetAngleAbs_rad, headTargetAngleAbs_rad, true, true);
  turnAction->SetMaxPanSpeed( DEG_TO_RAD(_configParams.sx_BodyTurnSpeed_degPerSec) );
  turnAction->SetMaxTiltSpeed( DEG_TO_RAD(_configParams.sxt_HeadTurnSpeed_degPerSec) );

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
