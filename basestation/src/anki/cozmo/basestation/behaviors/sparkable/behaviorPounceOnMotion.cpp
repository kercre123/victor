/**
 * File: behaviorPounceOnMotion.cpp
 *
 * Author: Brad Neuman
 * Created: 2015-11-19
 *
 * Description: This is a behavior which "pounces". Basically, it looks for motion nearby in the
 *              ground plane, and then drive to drive towards it and "catch" it underneath it's lift
 *
 * Copyright: Anki, Inc. 2015
 *
 **/

#include "anki/cozmo/basestation/behaviors/sparkable/behaviorPounceOnMotion.h"

#include "anki/common/basestation/math/point.h"
#include "anki/common/basestation/math/pose.h"
#include "anki/common/basestation/utils/timer.h"
#include "anki/cozmo/basestation/actions/basicActions.h"
#include "anki/cozmo/basestation/actions/animActions.h"
#include "anki/cozmo/basestation/behaviorManager.h"
#include "anki/cozmo/basestation/events/ankiEvent.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "anki/cozmo/basestation/robot.h"
#include "clad/externalInterface/messageEngineToGame.h"
#include "anki/cozmo/basestation/components/visionComponent.h"
#include "anki/cozmo/basestation/actions/basicActions.h"
#include "anki/cozmo/basestation/behaviorSystem/AIWhiteboard.h"
#include "anki/cozmo/basestation/drivingAnimationHandler.h"

#define SET_STATE(s) SetState_internal(State::s, #s)

namespace Anki {
namespace Cozmo {

using namespace ExternalInterface;

namespace {

static const char* kMaxNoMotionBeforeBored_running_Sec    = "maxNoGroundMotionBeforeBored_running_Sec";
static const char* kMaxNoMotionBeforeBored_notRunning_Sec = "maxNoGroundMotionBeforeBored_notRunning_Sec";
static const char* kTimeBeforeRotate_Sec                  = "TimeBeforeRotate_Sec";
static const char* kOddsOfPouncingOnTurn                  = "oddsOfPouncingOnTurn";
static const char* kBoredomMultiplier                     = "boredomMultiplier";
static const char* kSearchAmplitudeDeg                    = "searchAmplitudeDeg";
static float kBoredomMultiplierDefault = 0.8f;

// combination of offset between lift and robot orign and motion built into animation
static constexpr float kDriveForwardUntilDist = 50.f;
// Anything below this basically all looks the same, so just play the animation and possibly miss
static constexpr float kVisionMinDistMM = 65.f;
// How long to wait before re-calling
static constexpr float kWaitForMotionInterval_s = 2.0f;

// how far to randomly turn the body
static constexpr float kRandomPanMin_Deg = 20;
static constexpr float kRandomPanMax_Deg = 45;
  
  
// how long ago to consider a cliff currently in front of us for an initial pounce
static constexpr float kMinCliffInFrontWait_sec = 10.f;
} 
  
// Cozmo's low head angle for watching for fingers
static const Radians tiltRads(MIN_HEAD_ANGLE);

BehaviorPounceOnMotion::BehaviorPounceOnMotion(Robot& robot, const Json::Value& config)
  : IBehavior(robot, config)
  , _cumulativeTurn_rad(0)
  , _observedX(0)
  , _observedY(0)
  , _lastTimeRotate(0)
  , _lastCliffEvent_sec(0)
{
  SetDefaultName("PounceOnMotion");

  SubscribeToTags({{
    EngineToGameTag::RobotObservedMotion,
    EngineToGameTag::CliffEvent,
    EngineToGameTag::RobotOffTreadsStateChanged
  }});

  _maxTimeSinceNoMotion_running_sec = config.get(kMaxNoMotionBeforeBored_running_Sec,
                                                 _maxTimeSinceNoMotion_running_sec).asFloat();
  _maxTimeSinceNoMotion_notRunning_sec = config.get(kMaxNoMotionBeforeBored_notRunning_Sec,
                                                    _maxTimeSinceNoMotion_notRunning_sec).asFloat();
  _boredomMultiplier = config.get(kBoredomMultiplier, kBoredomMultiplierDefault).asFloat();
  _maxTimeBeforeRotate = config.get(kTimeBeforeRotate_Sec, _maxTimeBeforeRotate).asFloat();
  _oddsOfPouncingOnTurn = config.get(kOddsOfPouncingOnTurn, 0.0).asFloat();
  _searchAmplitude_rad = Radians(DEG_TO_RAD(config.get(kSearchAmplitudeDeg, 45).asFloat()));

  SET_STATE(Inactive);
  _lastMotionTime = -1000.f;
}

bool BehaviorPounceOnMotion::IsRunnableInternal(const Robot& robot) const
{
  return true;
}

float BehaviorPounceOnMotion::EvaluateScoreInternal(const Robot& robot) const
{
  // more likely to run if we did happen to see ground motion recently.
  // This isn't likely unless cozmo is looking down in explore mode, but possible
  float multiplier = 1.f;
  if( !IsRunning() )
  {
    double currentTime_sec = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
    if ( _lastMotionTime + _maxTimeSinceNoMotion_notRunning_sec < currentTime_sec )
    {
      multiplier = _boredomMultiplier;
    }
  }
  return IBehavior::EvaluateScoreInternal(robot) * multiplier;
}

Result BehaviorPounceOnMotion::InitInternal(Robot& robot)
{
  double currentTime_sec = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  _lastMotionTime = (float)currentTime_sec;
  
  // Don't override sparks idle animation
  if(!_shouldStreamline){
    robot.GetAnimationStreamer().PushIdleAnimation(AnimationTrigger::PounceFace);
    
    robot.GetDrivingAnimationHandler().PushDrivingAnimations({AnimationTrigger::PounceDriveStart,
      AnimationTrigger::PounceDriveLoop,
      AnimationTrigger::PounceDriveEnd});
  }
  
  TransitionToInitialPounce(robot);
  
  return Result::RESULT_OK;
}
  
  
Result BehaviorPounceOnMotion::ResumeInternal(Robot& robot)
{
  _motionObserved = false;
  return InitInternal(robot);
}

void BehaviorPounceOnMotion::StopInternal(Robot& robot)
{
  Cleanup(robot);
}
  
  
void BehaviorPounceOnMotion::TransitionToInitialPounce(Robot& robot)
{
  SET_STATE(InitialPounce);
  
  // Determine if there is a cliff in front of us so we don't pounce off an edge
  IActionRunner* potentialCliffSafetyTurn = nullptr;
  const double currentTime_sec = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  const bool cliffInFront = currentTime_sec - _lastCliffEvent_sec < kMinCliffInFrontWait_sec;
  
  if(cliffInFront){
    // This initial turn means that if Cozmo hits a cliff during an initial pounce
    // he won't get stuck in an infinite loop
    const Radians bodyPan(DEG_TO_RAD(90));
    const Radians headTilt(0);
    potentialCliffSafetyTurn = new PanAndTiltAction(robot, bodyPan, headTilt, false, false);
  }
  
  PounceOnMotionWithCallback(robot, &BehaviorPounceOnMotion::TransitionToBringingHeadDown, potentialCliffSafetyTurn);
}

void BehaviorPounceOnMotion::TransitionToBringingHeadDown(Robot& robot)
{
  PRINT_NAMED_DEBUG("BehaviorPounceOnMotion.TransitionToBringingHeadDown",
                    "BehaviorPounceOnMotion.TransitionToBringingHeadDown");
  SET_STATE(BringingHeadDown);
  
  CompoundActionSequential* fullAction = new CompoundActionSequential(robot);

  float panDirection = 1.0f;
  {
    // set pan and tilt
    Radians panAngle(DEG_TO_RAD(GetRNG().RandDblInRange(kRandomPanMin_Deg,kRandomPanMax_Deg)));
    // randomize direction
    if( GetRNG().RandDbl() < 0.5 )
    {
      panDirection = -1.0f;
    }
    
    panAngle *= panDirection;
    
    IActionRunner* panAndTilt = new PanAndTiltAction(robot, panAngle, tiltRads, false, true);
    fullAction->AddAction(panAndTilt);
  }
  
  // pan another random amount in the other direction (should get us back close to where we started, but not
  // exactly)
  {
    // get new pan angle
    Radians panAngle(DEG_TO_RAD(GetRNG().RandDblInRange(kRandomPanMin_Deg,kRandomPanMax_Deg)));
    // opposite direction
    panAngle *= -panDirection;

    IActionRunner* panAndTilt = new PanAndTiltAction(robot, panAngle, tiltRads, false, true);
    fullAction->AddAction(panAndTilt);
  }
  
  StartActing(fullAction, &BehaviorPounceOnMotion::TransitionToWaitForMotion);
}
  
void BehaviorPounceOnMotion::TransitionToRotateToWatchingNewArea(Robot& robot)
{
  SET_STATE( RotateToWatchingNewArea );
  _lastTimeRotate = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();

  Radians panAngle(DEG_TO_RAD(GetRNG().RandDblInRange(kRandomPanMin_Deg,kRandomPanMax_Deg)));

  // other direction weighted based on the cumulative turn rads - constantly pulls robot towards center
  const double weightedSearchAmplitude = (_searchAmplitude_rad.ToDouble()/(_searchAmplitude_rad.ToDouble() - _cumulativeTurn_rad.ToDouble())) * 0.5;
  if( GetRNG().RandDbl() < weightedSearchAmplitude)
  {
    panAngle *= -1.f;
  }
  _cumulativeTurn_rad += panAngle;
  
  IActionRunner* panAction = new PanAndTiltAction(robot, panAngle, tiltRads, false, false);
  
  //if we are above the threshold percentage, pounce and pan - otherwise, just pan
  const float shouldPounceOnTurn = GetRNG().RandDblInRange(0, 1);
  if(shouldPounceOnTurn < _oddsOfPouncingOnTurn){
    PounceOnMotionWithCallback(robot, &BehaviorPounceOnMotion::TransitionToWaitForMotion, panAction);
  }else{
    StartActing(panAction, &BehaviorPounceOnMotion::TransitionToWaitForMotion);
  }
}
  
void BehaviorPounceOnMotion::TransitionToWaitForMotion(Robot& robot)
{
  SET_STATE( WaitingForMotion);
  _numValidPouncePoses = 0;
  _backUpDistance = 0.f;
  _motionObserved = false;
  StartActing(new WaitAction(robot, kWaitForMotionInterval_s), &BehaviorPounceOnMotion::TransitionFromWaitForMotion);
  
}
  
void BehaviorPounceOnMotion::TransitionFromWaitForMotion(Robot& robot)
{
  // In the event motion is seen, this callback is triggered immediately
  if(_motionObserved){
    TransitionToTurnToMotion(robot, _observedX, _observedY);
    return;
  }
  
  //Otherwise, check to see if there has been a timeout or go back to waitingForMotion
  
  float currentTime_sec = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  
  // We're done if we haven't seen motion in a long while or since start.
  if ( _lastMotionTime + _maxTimeSinceNoMotion_running_sec < currentTime_sec )
  {
    PRINT_CH_INFO("Behaviors", "BehaviorPounceOnMotion.Timeout", "No motion found, giving up");
    
    //Set the exit state information and then cancel the hang action
    TransitionToGetOutBored(robot);
  }
  else if ( _lastTimeRotate + _maxTimeBeforeRotate < currentTime_sec )
  {
    //Set the exit state information and then cancel the hang action
    TransitionToRotateToWatchingNewArea(robot);
  }else{
    TransitionToWaitForMotion(robot);
  }
}

  
void BehaviorPounceOnMotion::TransitionToTurnToMotion(Robot& robot, int16_t motion_img_x, int16_t motion_img_y)
{
  SET_STATE(TurnToMotion);
  _lastTimeRotate = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();

  const Point2f motionCentroid(motion_img_x, motion_img_y);
  Radians absPanAngle;
  Radians absTiltAngle;
  robot.GetVisionComponent().GetCamera().ComputePanAndTiltAngles(motionCentroid, absPanAngle, absTiltAngle);
  
  auto callback = &BehaviorPounceOnMotion::TransitionToPounce;
  if(_lastPoseDist > kVisionMinDistMM){
    callback = &BehaviorPounceOnMotion::TransitionToCreepForward;
  }
  
  StartActing(new PanAndTiltAction(robot, absPanAngle, 0, true, false),
              callback);
}

float BehaviorPounceOnMotion::GetDriveDistance()
{
  return _lastPoseDist - kDriveForwardUntilDist;
}
  
void BehaviorPounceOnMotion::TransitionToCreepForward(Robot& robot)
{
  SET_STATE(CreepForward);
  // Sneak... Sneak... Sneak...
  _backUpDistance = GetDriveDistance();
  
  DriveStraightAction* driveAction = new DriveStraightAction(robot, _backUpDistance, DEFAULT_PATH_MOTION_PROFILE.dockSpeed_mmps);
  driveAction->SetAccel(DEFAULT_PATH_MOTION_PROFILE.dockAccel_mmps2);
  
  StartActing(driveAction, &BehaviorPounceOnMotion::TransitionToBringingHeadDown);
}

void BehaviorPounceOnMotion::TransitionToPounce(Robot& robot)
{
  SET_STATE(Pouncing);
  
  _prePouncePitch = robot.GetPitchAngle().ToFloat();
  if( _backUpDistance <= 0.f )
  {
    _backUpDistance = GetDriveDistance();
  }
  
  
  PounceOnMotionWithCallback(robot, &BehaviorPounceOnMotion::TransitionToResultAnim);
}

void BehaviorPounceOnMotion::TransitionToResultAnim(Robot& robot)
{
  SET_STATE(PlayingFinalReaction);
  
  // Tuned down for EP3
  const float liftHeightThresh = 35.5f;
  const float bodyAngleThresh = 0.02f;

  float robotBodyAngleDelta = robot.GetPitchAngle().ToFloat() - _prePouncePitch;
    
  // check the lift angle, after some time, transition state
  PRINT_CH_INFO("Behaviors", "BehaviorPounceOnMotion.CheckResult", "lift: %f body: %fdeg (%frad) (%f -> %f)",
                robot.GetLiftHeight(),
                RAD_TO_DEG_F32(robotBodyAngleDelta),
                robotBodyAngleDelta,
                RAD_TO_DEG_F32(_prePouncePitch),
                robot.GetPitchAngle().getDegrees());

  bool caught = robot.GetLiftHeight() > liftHeightThresh || robotBodyAngleDelta > bodyAngleThresh;

  IActionRunner* newAction = nullptr;
  if( caught ) {
    newAction = new TriggerLiftSafeAnimationAction(robot, AnimationTrigger::PounceSuccess);
    PRINT_CH_INFO("Behaviors", "BehaviorPounceOnMotion.CheckResult.Caught", "got it!");
  }
  else {
    // currently equivalent to "isSparked" - don't play failure anim when sparked
    if(!_shouldStreamline){
      newAction = new TriggerLiftSafeAnimationAction(robot, AnimationTrigger::PounceFail );
    }else{
      newAction = new TriggerAnimationAction(robot, AnimationTrigger::Count);
    }
    PRINT_CH_INFO("Behaviors", "BehaviorPounceOnMotion.CheckResult.Miss", "missed...");
  }
  
  auto callback = &BehaviorPounceOnMotion::TransitionToBringingHeadDown;
  if(_backUpDistance > 0.f){
    callback = &BehaviorPounceOnMotion::TransitionToBackUp;
  }
  
  _numValidPouncePoses = 0; // wait until we're seeing motion again

  StartActing(newAction, callback);

  if( caught ) {
    // send this after we start the action, so if the goal tries to cancel us, we will play the react first
    BehaviorObjectiveAchieved(BehaviorObjective::PouncedAndCaught);
  }
}
  
void BehaviorPounceOnMotion::TransitionToBackUp(Robot& robot)
{
  SET_STATE(BackUp);
  // back up some of the way
  
  StartActing(new DriveStraightAction(robot, -_backUpDistance, DEFAULT_PATH_MOTION_PROFILE.reverseSpeed_mmps),
              &BehaviorPounceOnMotion::TransitionToBringingHeadDown);
}
  
void BehaviorPounceOnMotion::TransitionToGetOutBored(Robot& robot)
{
  SET_STATE(GetOutBored);
  StartActing(new TriggerLiftSafeAnimationAction(robot, AnimationTrigger::PounceGetOut));
}

  
void BehaviorPounceOnMotion::AlwaysHandle(const EngineToGameEvent& event, const Robot& robot)
{
  switch (event.GetData().GetTag())
  {
    case MessageEngineToGameTag::RobotObservedMotion: {
      // handled differently based on running/not running
      break;
    }
      
    case MessageEngineToGameTag::CliffEvent:{
      if(event.GetData().Get_CliffEvent().detected){
        _lastCliffEvent_sec = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
      }
      break;
    }
      
    case EngineToGameTag::RobotOffTreadsStateChanged:{
      _lastCliffEvent_sec = 0;
      break;
    }
      
    default: {
      PRINT_NAMED_ERROR("BehaviorPounceOnMotion.AlwaysHandle.InvalidEvent", "");
      break;
    }
  }
  
}


void BehaviorPounceOnMotion::HandleWhileNotRunning(const EngineToGameEvent& event, const Robot& robot)
{
  switch (event.GetData().GetTag())
  {
    case MessageEngineToGameTag::RobotObservedMotion: {
      // be more likely to run with observed motion
      const auto & motionObserved = event.GetData().Get_RobotObservedMotion();
      const bool inGroundPlane = motionObserved.ground_area > _minGroundAreaForPounce;
      if( inGroundPlane )
      {
        const float robotOffsetX = motionObserved.ground_x;
        const float robotOffsetY = motionObserved.ground_y;
        float distSquared = robotOffsetX * robotOffsetX + robotOffsetY * robotOffsetY;
        float maxDistSquared = _maxPounceDist * _maxPounceDist;
        if( distSquared <= maxDistSquared )
        {
          double currentTime_sec = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
          _lastMotionTime = (float)currentTime_sec;
        }
      }
      break;
    }
      
    case MessageEngineToGameTag::CliffEvent:
    case MessageEngineToGameTag::RobotOffTreadsStateChanged:
    {
      // handled in always handle
      break;
    }
      
    default: {
      PRINT_NAMED_ERROR("BehaviorPounceOnMotion.AlwaysHandle.InvalidEvent", "");
      break;
    }
  }
}

void BehaviorPounceOnMotion::HandleWhileRunning(const EngineToGameEvent& event, Robot& robot)
{
  switch (event.GetData().GetTag())
  {
    case MessageEngineToGameTag::RobotObservedMotion: {
      // don't update the pounce location while we are active but go back.
      const auto & motionObserved = event.GetData().Get_RobotObservedMotion();
      const bool inGroundPlane = motionObserved.ground_area > _minGroundAreaForPounce;
      
      double currTime = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
      if( inGroundPlane )
      {
        _lastMotionTime = (float)currTime;
      }
      if( _state == State::WaitingForMotion )
      {
        const float robotOffsetX = motionObserved.ground_x;
        const float robotOffsetY = motionObserved.ground_y;
        
        bool gotPose = false;
        // we haven't started the pounce, so update the pounce location
        if( inGroundPlane )
        {
          float dist = std::sqrt( std::pow( robotOffsetX, 2 ) + std::pow( robotOffsetY, 2) );
          if( dist <= _maxPounceDist )
          {
            gotPose = true;
            _numValidPouncePoses++;
            _lastValidPouncePoseTime = currTime;
            
            PRINT_CH_INFO("Behaviors", "BehaviorPounceOnMotion.GotPose", "got valid pose with dist = %f. Now have %d",
                          dist, _numValidPouncePoses);
            _lastPoseDist = dist;
            
            //Set the exit state information and then cancel the hang action
            _observedX = motionObserved.img_x;
            _observedY = motionObserved.img_y;
            _motionObserved = true;
            StopActing();
          }
          else
          {
            PRINT_CH_INFO("Behaviors", "BehaviorPounceOnMotion.IgnorePose",
                          "got pose, but dist of %f is too large, ignoring",
                          dist);
          }
        }
        else if(_numValidPouncePoses > 0)
        {
          PRINT_NAMED_DEBUG("BehaviorPounceOnMotion.IgnorePose", "got pose, but ground plane area is %f, which is too low",
                            motionObserved.ground_area);
        }
        
        // reset everything if it's been this long since we got a valid pose
        if( ! gotPose && currTime >= _lastValidPouncePoseTime + _maxTimeBetweenPoses ) {
          if( _numValidPouncePoses > 0 ) {
            PRINT_CH_INFO("Behaviors", "BehaviorPounceOnMotion.ResetValid",
                          "resetting num valid poses because it has been %f seconds since the last one",
                          currTime - _lastValidPouncePoseTime);
            _numValidPouncePoses = 0;
          }
        }
      }
      break;
    }
      
    case MessageEngineToGameTag::CliffEvent:
    case MessageEngineToGameTag::RobotOffTreadsStateChanged:
    {
      // handled in always handle
      break;
    }
      
    default: {
      PRINT_NAMED_ERROR("BehaviorPounceOnMotion.AlwaysHandle.InvalidEvent", "");
      break;
    }
  }
}
  
template<typename T>
void BehaviorPounceOnMotion::PounceOnMotionWithCallback(Robot& robot, void(T::*callback)(Robot&),  IActionRunner* intermittentAction)
{
  CompoundActionSequential* compAction = new CompoundActionSequential(robot);
  
  if(intermittentAction != nullptr){
    compAction->AddAction(intermittentAction);
  }
  
  compAction->AddAction(new TriggerLiftSafeAnimationAction(robot, AnimationTrigger::PouncePounce));

  StartActing(compAction, [this, callback](Robot& robot){
    // wait for the lift to relax 
    robot.GetMoveComponent().EnableLiftPower(false);
    SET_STATE(RelaxingLift);
    _relaxedLift = true;
    // We don't get an accurate pitch evaulation if the head is moving during an animation
    // so hold this for a bit longer
    const float relaxTime = 0.15f;
    
    StartActing(new WaitAction(robot, relaxTime), [this, callback](Robot& robot){
      robot.GetMoveComponent().EnableLiftPower(true);
      _relaxedLift = false;
      (this->*callback)(robot);
    });
  });
}
  
void BehaviorPounceOnMotion::Cleanup(Robot& robot)
{
  SET_STATE(Complete);
  if( _relaxedLift ) {
    robot.GetMoveComponent().EnableLiftPower(true);
    _relaxedLift = false;
  }
  
  _numValidPouncePoses = 0;
  _lastValidPouncePoseTime = 0.0f;
  
  // Only pop animations if set within this behavior
  if(!_shouldStreamline){
    robot.GetAnimationStreamer().PopIdleAnimation();
    robot.GetDrivingAnimationHandler().PopDrivingAnimations();
  }
}
  
void BehaviorPounceOnMotion::SetState_internal(State state, const std::string& stateName)
{
  _state = state;
  PRINT_NAMED_DEBUG("BehaviorPounceOnMotion.TransitionTo", "%s", stateName.c_str());
  SetDebugStateName(stateName);
}

}
}
