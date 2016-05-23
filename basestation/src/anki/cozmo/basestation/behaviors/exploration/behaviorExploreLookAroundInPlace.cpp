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
#include "anki/cozmo/basestation/actions/animActions.h"
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
, _s4HeadMovesRolled(0)
, _s4HeadMovesLeft(0)
{
  SetDefaultName("BehaviorExploreLookAroundInPlace");

  SubscribeToTags({{
    EngineToGameTag::RobotCompletedAction,
    EngineToGameTag::RobotPutDown
  }});
  
  // parse all parameters now
  LoadConfig(config[kConfigParamsKey]);

  if( !_configParams.behavior_ShouldResetTurnDirection ) {
    // we won't be resetting this, so set it once now
    DecideTurnDirection();
  }

  // We init the body direction to zero because behaviors are init before robot pose is set to anything real
  _initialBodyDirection = 0.f;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorExploreLookAroundInPlace::~BehaviorExploreLookAroundInPlace()
{  
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorExploreLookAroundInPlace::IsRunnableInternal(const Robot& robot) const
{
  // don't run if I'm holding a block (can't really look around through the cube)
  if( robot.IsCarryingObject() ) {
    return false;
  }
  
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
void BehaviorExploreLookAroundInPlace::LoadConfig(const Json::Value& config)
{
  using namespace JsonTools;
  const std::string& debugName = "BehaviorExploreLookAroundInPlace.LoadConfig";

  _configParams.behavior_DistanceFromRecentLocationMin_mm = ParseFloat(config, "behavior_DistanceFromRecentLocationMin_mm", debugName);
  _configParams.behavior_RecentLocationsMax = ParseUint8(config, "behavior_RecentLocationsMax", debugName);
  _configParams.behavior_ShouldResetTurnDirection = ParseBool(config, "behavior_ShouldResetTurnDirection", debugName);
  _configParams.behavior_AngleOfFocus_deg = ParseFloat(config, "behavior_AngleOfFocus_deg", debugName);
  // turn speed
  _configParams.sx_BodyTurnSpeed_degPerSec = ParseFloat(config, "sx_BodyTurnSpeed_degPerSec", debugName);
  _configParams.sxt_HeadTurnSpeed_degPerSec = ParseFloat(config, "sxt_HeadTurnSpeed_degPerSec", debugName);
  _configParams.sxh_HeadTurnSpeed_degPerSec = ParseFloat(config, "sxh_HeadTurnSpeed_degPerSec", debugName);
  // chance that the main turn will be counter clockwise (vs ccw)
  _configParams.s0_MainTurnCWChance = ParseFloat(config, "s0_MainTurnCWChance", debugName);
  // [min,max] range for random turn angles for step 1
  _configParams.s1_BodyAngleRangeMin_deg = ParseFloat(config, "s1_BodyAngleRangeMin_deg", debugName);
  _configParams.s1_BodyAngleRangeMax_deg = ParseFloat(config, "s1_BodyAngleRangeMax_deg", debugName);
  _configParams.s1_HeadAngleRangeMin_deg = ParseFloat(config, "s1_HeadAngleRangeMin_deg", debugName);
  _configParams.s1_HeadAngleRangeMax_deg = ParseFloat(config, "s1_HeadAngleRangeMax_deg", debugName);
  // [min,max] range for pause for step 2
  _configParams.s2_WaitMin_sec = ParseFloat(config, "s2_WaitMin_sec", debugName);
  _configParams.s2_WaitMax_sec = ParseFloat(config, "s2_WaitMax_sec", debugName);
  _configParams.s2_WaitAnimGroupName = ParseString(config, "s2_WaitAnimGroupName", debugName);
  // [min,max] range for random angle turns for step 3
  _configParams.s3_BodyAngleRangeMin_deg = ParseFloat(config, "s3_BodyAngleRangeMin_deg", debugName);
  _configParams.s3_BodyAngleRangeMax_deg = ParseFloat(config, "s3_BodyAngleRangeMax_deg", debugName);
  _configParams.s3_HeadAngleRangeMin_deg = ParseFloat(config, "s3_HeadAngleRangeMin_deg", debugName);
  _configParams.s3_HeadAngleRangeMax_deg = ParseFloat(config, "s3_HeadAngleRangeMax_deg", debugName);
  // [min,max] range for head move for step 4
  _configParams.s4_BodyAngleRelativeRangeMin_deg = ParseFloat(config, "s4_BodyAngleRelativeRangeMin_deg", debugName);
  _configParams.s4_BodyAngleRelativeRangeMax_deg = ParseFloat(config, "s4_BodyAngleRelativeRangeMax_deg", debugName);
  _configParams.s4_HeadAngleRangeMin_deg = ParseFloat(config, "s4_HeadAngleRangeMin_deg", debugName);
  _configParams.s4_HeadAngleRangeMax_deg = ParseFloat(config, "s4_HeadAngleRangeMax_deg", debugName);
  _configParams.s4_HeadAngleChangesMin = ParseUint8(config, "s4_HeadAngleChangesMin", debugName);
  _configParams.s4_HeadAngleChangesMax = ParseUint8(config, "s4_HeadAngleChangesMax", debugName);
  _configParams.s4_WaitBetweenChangesMin_sec = ParseFloat(config, "s4_WaitBetweenChangesMin_sec", debugName);
  _configParams.s4_WaitBetweenChangesMax_sec = ParseFloat(config, "s4_WaitBetweenChangesMax_sec", debugName);
  _configParams.s4_WaitAnimGroupName = ParseString(config, "s4_WaitAnimGroupName", debugName);
  // [min,max] range for head move  for step 5
  _configParams.s5_BodyAngleRelativeRangeMin_deg = ParseFloat(config, "s5_BodyAngleRelativeRangeMin_deg", debugName);
  _configParams.s5_BodyAngleRelativeRangeMax_deg = ParseFloat(config, "s5_BodyAngleRelativeRangeMax_deg", debugName);
  _configParams.s5_HeadAngleRangeMin_deg = ParseFloat(config, "s5_HeadAngleRangeMin_deg", debugName);
  _configParams.s5_HeadAngleRangeMax_deg = ParseFloat(config, "s5_HeadAngleRangeMax_deg", debugName);
  // [min,max] range for random angle turns for step 6
  _configParams.s6_BodyAngleRangeMin_deg = ParseFloat(config, "s6_BodyAngleRangeMin_deg", debugName);
  _configParams.s6_BodyAngleRangeMax_deg = ParseFloat(config, "s6_BodyAngleRangeMax_deg", debugName);
  _configParams.s6_HeadAngleRangeMin_deg = ParseFloat(config, "s6_HeadAngleRangeMin_deg", debugName);
  _configParams.s6_HeadAngleRangeMax_deg = ParseFloat(config, "s6_HeadAngleRangeMax_deg", debugName);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result BehaviorExploreLookAroundInPlace::InitInternal(Robot& robot)
{
  // grab run values
  _behaviorBodyFacingDone_rad = 0;
  
  if( _configParams.behavior_ShouldResetTurnDirection ) {
    DecideTurnDirection();
  }
  
  // start first iteration
  TransitionToS1_OppositeTurn(robot);

  return Result::RESULT_OK;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorExploreLookAroundInPlace::StopInternal(Robot& robot)
{
  
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorExploreLookAroundInPlace::AlwaysHandle(const EngineToGameEvent& event, const Robot& robot)
{
  switch (event.GetData().GetTag())
  {
    case EngineToGameTag::RobotPutDown:
    {
      _initialBodyDirection = robot.GetPose().GetRotationAngle<'Z'>();
      break;
    }
    default:
    {
      break;
    }
  }
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
  SetStateName("TransitionToS1_OppositeTurn");

  // cache iteration values
  _iterationStartingBodyFacing_rad = robot.GetPose().GetRotationAngle<'Z'>();
  
  // If we have the parameter set for an angle to focus on
  if (!Util::IsNearZero(_configParams.behavior_AngleOfFocus_deg))
  {
    const float angleDiff_deg = (_iterationStartingBodyFacing_rad - _initialBodyDirection).getDegrees() * GetTurnSign(_mainTurnDirection);
    
    if (angleDiff_deg >= _configParams.behavior_AngleOfFocus_deg * 0.5f)
    {
      // We've hit (or gone past) the edge of the cone, so turn the other direction
      _mainTurnDirection = GetOpposite(_mainTurnDirection);
      
      // When we have a cone we're staying inside of, we never finish a location, so reset our counter
      _behaviorBodyFacingDone_rad = 0.f;
    }
  }
  
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
  SetStateName("TransitionToS2_Pause");

  IAction* pauseAction = nullptr;
  const std::string& animGroupName = _configParams.s2_WaitAnimGroupName;
  if ( !animGroupName.empty() )
  {
    pauseAction = CreatePlayAnimationAction(robot, animGroupName);
  }
  else
  {
    // create wait action
    const double waitTime_sec = GetRNG().RandDblInRange(_configParams.s2_WaitMin_sec, _configParams.s2_WaitMax_sec);
    pauseAction = new WaitAction( robot, waitTime_sec );
  }
  
  // request action with transition to proper state
  ASSERT_NAMED( nullptr!=pauseAction, "BehaviorExploreLookAroundInPlace::TransitionToS2_Pause.NullAction");
  StartActing( pauseAction, &BehaviorExploreLookAroundInPlace::TransitionToS3_MainTurn );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorExploreLookAroundInPlace::TransitionToS3_MainTurn(Robot& robot)
{
  SetStateName("TransitionToS3_MainTurn");

  // create turn action for this state
  const EClockDirection turnDir = _mainTurnDirection;
  IAction* turnAction = CreateBodyAndHeadTurnAction(robot, turnDir,
        _configParams.s3_BodyAngleRangeMin_deg, _configParams.s3_BodyAngleRangeMax_deg,
        _configParams.s3_HeadAngleRangeMin_deg, _configParams.s3_HeadAngleRangeMax_deg,
        _configParams.sx_BodyTurnSpeed_degPerSec, _configParams.sxt_HeadTurnSpeed_degPerSec);
  
  // calculate head moves now
  const int randMoves = GetRNG().RandIntInRange(_configParams.s4_HeadAngleChangesMin, _configParams.s4_HeadAngleChangesMax);
  _s4HeadMovesRolled = Util::numeric_cast<uint8_t>(randMoves);
  _s4HeadMovesLeft = _s4HeadMovesRolled;
  
  // request action with transition to proper state
  StartActing( turnAction, &BehaviorExploreLookAroundInPlace::TransitionToS4_HeadOnlyUp );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorExploreLookAroundInPlace::TransitionToS4_HeadOnlyUp(Robot& robot)
{
  {
    std::string stateName = "TransitionToS4_HeadOnlyUp (" + std::to_string(_s4HeadMovesLeft) + "/" +
      std::to_string(_s4HeadMovesRolled) + ")";
    SetStateName(stateName);
  }

  // cache the rotation the first time that we run S4
  const bool isFirstMove = (_s4HeadMovesLeft == _s4HeadMovesRolled);
  if ( isFirstMove )  {
    // set current facing for the next state
    _s4_s5StartingBodyFacing_rad = robot.GetPose().GetRotationAngle<'Z'>();
  }

  // count the action we are going to queue as a move
  --_s4HeadMovesLeft;
  const bool isLastMove = (_s4HeadMovesLeft == 0);

  // check which transition method to call after the head move is done, S5 or S4 again?
  using TransitionCallback = void(BehaviorExploreLookAroundInPlace::*)(Robot&);
  TransitionCallback nextCallback = isLastMove ?
    &BehaviorExploreLookAroundInPlace::TransitionToS5_HeadOnlyDown :
    &BehaviorExploreLookAroundInPlace::TransitionToS4_HeadOnlyUp;

  // this is the lambda that will run after the wait action finishes
  auto runAfterPause = [this, &robot, nextCallback](const ExternalInterface::RobotCompletedAction& actionRet)
  {
    // create head move action
    IAction* moveHeadAction = CreateHeadTurnAction(robot,
          _configParams.s4_BodyAngleRelativeRangeMin_deg,
          _configParams.s4_BodyAngleRelativeRangeMax_deg,
          RAD_TO_DEG( _s4_s5StartingBodyFacing_rad.ToFloat() ),
          _configParams.s4_HeadAngleRangeMin_deg,
          _configParams.s4_HeadAngleRangeMax_deg,
          _configParams.sx_BodyTurnSpeed_degPerSec,
          _configParams.sxh_HeadTurnSpeed_degPerSec);
  
    // do head action and transition to next state or same (depending on callback)
    StartActing( moveHeadAction, nextCallback );
  };
  
  IAction* pauseAction = nullptr;
  const std::string& animGroupName = _configParams.s4_WaitAnimGroupName;
  if ( !animGroupName.empty() )
  {
    pauseAction = CreatePlayAnimationAction(robot, animGroupName);
  }
  else
  {
    // create wait action
    const double waitTime_sec = GetRNG().RandDblInRange(_configParams.s4_WaitBetweenChangesMin_sec,
                                                      _configParams.s4_WaitBetweenChangesMax_sec);
    pauseAction = new WaitAction( robot, waitTime_sec );
  }
  
  // request action with transition to proper state
  ASSERT_NAMED( nullptr!=pauseAction, "BehaviorExploreLookAroundInPlace::TransitionToS4_HeadOnlyUp.NullPauseAction");
  StartActing( pauseAction, runAfterPause );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorExploreLookAroundInPlace::TransitionToS5_HeadOnlyDown(Robot& robot)
{
  SetStateName("TransitionToS5_HeadOnlyDown");

  // create head move action for this state
  IAction* moveHeadAction = CreateHeadTurnAction(robot,
        _configParams.s5_BodyAngleRelativeRangeMin_deg,
        _configParams.s5_BodyAngleRelativeRangeMax_deg,
        RAD_TO_DEG( _s4_s5StartingBodyFacing_rad.ToFloat() ),
        _configParams.s5_HeadAngleRangeMin_deg,
        _configParams.s5_HeadAngleRangeMax_deg,
        _configParams.sx_BodyTurnSpeed_degPerSec,
        _configParams.sxh_HeadTurnSpeed_degPerSec);

  // request action with transition to proper state
  StartActing( moveHeadAction, &BehaviorExploreLookAroundInPlace::TransitionToS6_MainTurnFinal );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorExploreLookAroundInPlace::TransitionToS6_MainTurnFinal(Robot& robot)
{
  SetStateName("TransitionToS6_MainTurnFinal");

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
  SetStateName("TransitionToS7_IterationEnd");

  Radians currentZ_rad = robot.GetPose().GetRotationAngle<'Z'>();
  float doneThisIteration_rad = (currentZ_rad - _iterationStartingBodyFacing_rad).ToFloat();
  _behaviorBodyFacingDone_rad += doneThisIteration_rad;

// rsam: I have found the root of this problem. Say you start at Z = 90 turning positive angles as main direction. With
// the proper data tweaks you, you can make the robot end at Z < 90 at the end of step 5. Step 6 then would try
// to move from < 90 to > 90. However the action for step 6 fails (something we will investigate further), and
// instead of retrying, it simply transitions to S7, which checks that Z should be > 90, when it's not
// Until we know the cause of giving up, disable this assert
//  // assert we are not turning more than PI in one iteration (because of Radian rescaling)
//  ASSERT_NAMED( FLT_GT(doneThisIteration_rad,0.0f) == FLT_GT(GetTurnSign(_mainTurnDirection), 0.0f),
//    "BehaviorExploreLookAroundInPlace.TransitionToS7_IterationEnd.BadSign");

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
void BehaviorExploreLookAroundInPlace::DecideTurnDirection()
{
  // decide main turn direction
  const double randomDirection = GetRNG().RandDbl();
  _mainTurnDirection = (randomDirection<=_configParams.s0_MainTurnCWChance) ? EClockDirection::CW : EClockDirection::CCW;
  // _mainTurnDirection = EClockDirection::CW;
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
  turnAction->SetMaxPanSpeed( DEG_TO_RAD(bodyTurnSpeed_degPerSec) );
  turnAction->SetMaxTiltSpeed( DEG_TO_RAD(headTurnSpeed_degPerSec) );

// Code for debugging
//  PRINT_NAMED_WARNING("RSAM", "STATE %s (bh) set BODY %.2f, HEAD %.2f (curB %.2f, curH %.2f)",
//    GetStateName().c_str(),
//    RAD_TO_DEG(bodyTargetAngleAbs_rad.ToFloat()),
//    RAD_TO_DEG(headTargetAngleAbs_rad.ToFloat()),
//    RAD_TO_DEG(robot.GetPose().GetRotationAngle<'Z'>().ToFloat()),
//    robot.GetHeadAngle() );

  return turnAction;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IAction* BehaviorExploreLookAroundInPlace::CreateHeadTurnAction(Robot& robot,
  float bodyRelativeMin_deg, float bodyRelativeMax_deg,
  float bodyReference_deg,
  float headAbsoluteMin_deg, float headAbsoluteMax_deg,
  float bodyTurnSpeed_degPerSec, float headTurnSpeed_degPerSec)
{
  // generate turn sign
  const EClockDirection turnDir = (GetRNG().RandInt(2) == 0) ? EClockDirection::CW : EClockDirection::CCW;

  // [min,max] range for random body angle turn
  const double bodyTargetAngleRelative_deg =
    GetRNG().RandDblInRange(bodyRelativeMin_deg, bodyRelativeMax_deg) * GetTurnSign(turnDir);

  // [min,max] range for random head angle turn
  const double headTargetAngleAbs_deg =
    GetRNG().RandDblInRange(headAbsoluteMin_deg, headAbsoluteMax_deg);
  
  // create proper action for body & head turn
  const Radians bodyTargetAngleAbs_rad( DEG_TO_RAD(bodyReference_deg + bodyTargetAngleRelative_deg) );
  const Radians headTargetAngleAbs_rad( DEG_TO_RAD(headTargetAngleAbs_deg) );
  PanAndTiltAction* turnAction = new PanAndTiltAction(robot, bodyTargetAngleAbs_rad, headTargetAngleAbs_rad, true, true);
  turnAction->SetMaxPanSpeed( DEG_TO_RAD(bodyTurnSpeed_degPerSec) );
  turnAction->SetMaxTiltSpeed( DEG_TO_RAD(headTurnSpeed_degPerSec) );

// code for debugging
//  PRINT_NAMED_WARNING("RSAM", "STATE %s (ho) set BODY %.2f, HEAD %.2f (curB %.2f, curH %.2f)",
//    GetStateName().c_str(),
//    RAD_TO_DEG(bodyTargetAngleAbs_rad.ToFloat()),
//    RAD_TO_DEG(headTargetAngleAbs_rad.ToFloat()),
//    RAD_TO_DEG(robot.GetPose().GetRotationAngle<'Z'>().ToFloat()),
//    robot.GetHeadAngle() );

  return turnAction;
}

} // namespace Cozmo
} // namespace Anki
