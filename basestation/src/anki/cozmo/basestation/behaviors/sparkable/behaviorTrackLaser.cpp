/**
 * File: behaviorTrackLaser.cpp
 *
 * Author: Andrew Stein
 * Created: 2017-03-11
 *
 * Description: Follows a laser point around (using TrackGroundPointAction) and tries to pounce on it.
 *              Adapted from PounceOnMotion
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#include "anki/cozmo/basestation/behaviors/sparkable/behaviorTrackLaser.h"

#include "anki/common/basestation/jsonTools.h"
#include "anki/common/basestation/math/point.h"
#include "anki/common/basestation/math/pose.h"
#include "anki/common/basestation/utils/timer.h"

#include "anki/cozmo/basestation/actions/animActions.h"
#include "anki/cozmo/basestation/actions/basicActions.h"
#include "anki/cozmo/basestation/actions/trackGroundPointAction.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorPreReqs/behaviorPreReqRobot.h"
#include "anki/cozmo/basestation/components/movementComponent.h"
#include "anki/cozmo/basestation/components/visionComponent.h"
#include "anki/cozmo/basestation/cozmoContext.h"
#include "anki/cozmo/basestation/drivingAnimationHandler.h"
#include "anki/cozmo/basestation/events/ankiEvent.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/basestation/utils/cozmoFeatureGate.h"

#include "clad/externalInterface/messageEngineToGame.h"

#include "util/helpers/quoteMacro.h"

#define SET_STATE(s) SetState_internal(State::s, #s)

namespace Anki {
namespace Cozmo {

using namespace ExternalInterface;
  
namespace {
  
static const constexpr char* const kLogChannelName = "Behaviors";
  
} 
  
// Cozmo's low head angle for watching for lasers
static const Radians kAbsHeadDown_rad(MIN_HEAD_ANGLE);

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorTrackLaser::BehaviorTrackLaser(Robot& robot, const Json::Value& config)
: IBehavior(robot, config)
, _cumulativeTurn_rad(0)
, _lastTimeRotate(0.0f)
{
  SetDefaultName("TrackLaser");

  SubscribeToTags({EngineToGameTag::RobotObservedLaserPoint});

  SetParamsFromConfig(config);

  SET_STATE(Inactive);
  
  _lastLaserObservation.type     = LaserObservation::Type::None;
  _lastLaserObservation.time_sec = -1000.f;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorTrackLaser::SetParamsFromConfig(const Json::Value& config)
{
  // Helper macro to grab the key with the same name as the param struct member from the Json and
  // set it as a float
# define SET_FLOAT_HELPER(__name__) do { _params.__name__ = JsonTools::ParseFloat(config, QUOTE(__name__), "BehaviorTrackLaser.SetParamsFromConfig"); } while(0)
  
  // In same order as defined in the params struct
  SET_FLOAT_HELPER(startIfLaserSeenWithin_sec);
  SET_FLOAT_HELPER(maxDistToGetAttention_mm);
  
  SET_FLOAT_HELPER(darkenedExposure_ms);
  SET_FLOAT_HELPER(darkenedGain);
  SET_FLOAT_HELPER(timeToWaitForExposureChange_ms);
  
  SET_FLOAT_HELPER(maxTimeToConfirm_ms);
  SET_FLOAT_HELPER(searchAmplitude_deg);
  
  SET_FLOAT_HELPER(maxTimeSinceNoLaser_running_sec);
  SET_FLOAT_HELPER(maxTimeSinceNoLaser_notRunning_sec);
  SET_FLOAT_HELPER(maxTimeBehaviorTimeout_sec);
  SET_FLOAT_HELPER(maxTimeBeforeRotate_sec);
  SET_FLOAT_HELPER(trackingTimeout_sec);
  
  SET_FLOAT_HELPER(pounceAfterTrackingFor_sec);
  SET_FLOAT_HELPER(pounceIfWithinDist_mm);
  SET_FLOAT_HELPER(pouncePanTol_deg);
  SET_FLOAT_HELPER(pounceTiltTol_deg);
  SET_FLOAT_HELPER(backupDistAfterPounce_mm);
  SET_FLOAT_HELPER(backupDurationAfterPounce_sec);
  
  SET_FLOAT_HELPER(randomInitialSearchPanMin_deg);
  SET_FLOAT_HELPER(randomInitialSearchPanMax_deg);
  SET_FLOAT_HELPER(minPanDuration_sec);
  SET_FLOAT_HELPER(maxPanDuration_sec);
  SET_FLOAT_HELPER(minTimeToReachLaser_sec);
  SET_FLOAT_HELPER(maxTimeToReachLaser_sec);
  SET_FLOAT_HELPER(predictionDuration_sec);
  SET_FLOAT_HELPER(trackingTimeToAchieveObjective_sec);
  
# undef SET_FLOAT_HELPER
  
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorTrackLaser::IsRunnableInternal(const BehaviorPreReqRobot& preReqData) const
{
  const CozmoContext* context = preReqData.GetRobot().GetContext();
  const bool featureEnabled = context->GetFeatureGate()->IsFeatureEnabled(Anki::Cozmo::FeatureType::Laser);
  if(!featureEnabled)
  {
    return false;
  }
  
  if(_shouldStreamline)
  {
    return true;
  }
  else
  {
    // Have we seen a potential laser observation recently?
    if(_lastLaserObservation.type != LaserObservation::Type::None)
    {
      // Note that we are reasoning about last seen in basestation time here. One could argue
      // that it might better to reason about "last image processed time" and use image
      // timestamps (in ms instead of sec), but given the relatively long duration of the
      // startIfLaserSeenWithin, I think it makes more sense to ask if we've seen a laser
      // in that amount of basestation time. If we're dropping enough frames to not see
      // a laser for _seconds_, we're probably not going to be able to track the laser
      // anyway.
      const float currentTime = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
      
      if(_lastLaserObservation.time_sec > currentTime - _params.startIfLaserSeenWithin_sec) {
        return true;
      }
    }
    
    return false;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result BehaviorTrackLaser::InitInternal(Robot& robot)
{
  InitHelper(robot);
  const bool haveSeenLaser = (_lastLaserObservation.type != LaserObservation::Type::None);
  if(haveSeenLaser)
  {
    // Pretend we just rotated so we don't immediately rotate to start
    _lastTimeRotate = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
    
    TransitionToWaitForLaser(robot);
  }
  else
  {
    TransitionToInitialSearch(robot);
  }
  return Result::RESULT_OK;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorTrackLaser::InitHelper(Robot& robot)
{
  const float currentTime_sec = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  _lastLaserObservation.time_sec = currentTime_sec;
  _haveConfirmedLaser = false;
  
  _originalCameraSettings.colorEnabled = robot.GetVisionComponent().AreColorImagesEnabled();
  _originalCameraSettings.exposureTime_ms = robot.GetVisionComponent().GetMaxCameraExposureTime_ms();
  _originalCameraSettings.gain = robot.GetVisionComponent().GetCurrentCameraGain();
  
  const bool kUseDefaultsForUnspecified = false; // disable all vision except laser point detection
  AllVisionModesSchedule schedule({{VisionMode::DetectingLaserPoints, VisionModeSchedule(true)}}, kUseDefaultsForUnspecified);
  robot.GetVisionComponent().PushNextModeSchedule(std::move(schedule));
  robot.GetVisionComponent().SetAndDisableAutoExposure(_params.darkenedExposure_ms, _params.darkenedGain);
  robot.GetVisionComponent().EnableColorImages(true);
  
  _changedExposureTime_ms = robot.GetLastImageTimeStamp();
  
  // Don't override sparks idle animation
  if(!_shouldStreamline)
  {
    robot.GetAnimationStreamer().PushIdleAnimation(AnimationTrigger::LaserFace);
    
    robot.GetDrivingAnimationHandler().PushDrivingAnimations({
      AnimationTrigger::LaserDriveStart,
      AnimationTrigger::LaserDriveLoop,
      AnimationTrigger::LaserDriveEnd
    });
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IBehavior::Status BehaviorTrackLaser::UpdateInternal(Robot& robot)
{
  switch(_state)
  {
    case State::WaitForStop:
    {
      return Status::Complete;
    }
  
    case State::WaitingForLaser:
    {
      if(LaserObservation::Type::Confirmed == _lastLaserObservation.type)
      {
        LOG_EVENT("robot.laser_behavior.confirmed_laser", "");
        TransitionToTrackLaser(robot);
        break;
      }
      else
      {
        const float currentTime_sec = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
        const float totalConfirmationWaitTime_sec = Util::MilliSecToSec(_params.maxTimeToConfirm_ms +
                                                                        _params.timeToWaitForExposureChange_ms);
        
        if((_lastLaserObservation.type == LaserObservation::Type::Unconfirmed) &&
           (_lastLaserObservation.time_sec + totalConfirmationWaitTime_sec) < currentTime_sec)
        {
          LOG_EVENT("robot.laser_behavior.laser_never_confirmed", "");
          return Status::Complete;
        }
        
        // Check to see if it's time to rotate again
        else if ( (_lastTimeRotate + _params.maxTimeBeforeRotate_sec) < currentTime_sec )
        {
          TransitionToRotateToWatchingNewArea(robot);
        }
      }
      break;
    }
  
    case State::GetOutBored:
    case State::Pouncing:
    {
      // Just let the get out or pouncing animations complete (don't want to time out until they are done)
      break;
    }
      
    default:
    {
      // If not already in the process of getting out, check to see if there has been a timeout.
      // We're done if we haven't seen motion in a long while or since start.
      // Note that we use a different timeout if we have not yet confirmed a laser while running.
      const float currentTime_sec = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
      const float maxTime = (_haveConfirmedLaser ?
                             _params.maxTimeSinceNoLaser_running_sec :
                             _params.maxTimeSinceNoLaser_notRunning_sec);
      
      if ( (_lastLaserObservation.time_sec + maxTime) < currentTime_sec )
      {
        PRINT_CH_INFO(kLogChannelName, "BehaviorTrackLaser.UpdateInternal.NoLaserTimeout",
                      "No %slaser found, giving up after %.1fsec",
                      _haveConfirmedLaser ? "" : "confirmed ", maxTime);
        
        Util::sEventF("robot.laser_behavior.no_laser_timeout", {{DDATA, (_haveConfirmedLaser ? "0" : "1")}},
                      "%f", maxTime);
        
        TransitionToGetOutBored(robot);
      }
      else if ( (GetTimeStartedRunning_s() + _params.maxTimeBehaviorTimeout_sec) < currentTime_sec)
      {
        PRINT_CH_INFO(kLogChannelName, "BehaviorTrackLaser.UpdateInternal.BehaviorTimeout",
                      "Started:%.1f, Now:%.1f, Timeout:%.1f",
                      GetTimeStartedRunning_s(), currentTime_sec, _params.maxTimeBehaviorTimeout_sec);
        
        LOG_EVENT("robot.laser_behavior.ran_until_max_timeout", "%f", _params.maxTimeBehaviorTimeout_sec);
        
        TransitionToGetOutBored(robot);
      }
      
      break;
    }
  }

  return Status::Running;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result BehaviorTrackLaser::ResumeInternal(Robot& robot)
{
  _lastLaserObservation.type = LaserObservation::Type::None;
  InitHelper(robot);
  TransitionToBringingHeadDown(robot);
  return Result::RESULT_OK;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorTrackLaser::StopInternal(Robot& robot)
{
  Cleanup(robot);
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorTrackLaser::TransitionToInitialSearch(Robot& robot)
{
  PRINT_CH_DEBUG(kLogChannelName, "BehaviorTrackLaser.TransitionToInitialSearch",
                 "BehaviorTrackLaser.TransitionToInitialSearch");
  SET_STATE(InitialSearch);
  
  CompoundActionSequential* fullAction = new CompoundActionSequential(robot);
  
  const bool kPanIsAbsolute  = false;
  const bool kTiltIsAbsolute = true;
  
  float panDirection = 1.0f;
  {
    // set pan and tilt
    Radians panAngle(DEG_TO_RAD(GetRNG().RandDblInRange(_params.randomInitialSearchPanMin_deg,
                                                        _params.randomInitialSearchPanMax_deg)));
    // randomize direction
    if( GetRNG().RandDbl() < 0.5 )
    {
      panDirection = -1.0f;
    }
    
    panAngle *= panDirection;
    
    IActionRunner* panAndTilt = new PanAndTiltAction(robot, panAngle, kAbsHeadDown_rad, kPanIsAbsolute, kTiltIsAbsolute);
    fullAction->AddAction(panAndTilt);
  }
  
  // pan another random amount in the other direction (should get us back close to where we started, but not
  // exactly)
  {
    // get new pan angle
    Radians panAngle(DEG_TO_RAD(GetRNG().RandDblInRange(_params.randomInitialSearchPanMin_deg,
                                                        _params.randomInitialSearchPanMax_deg)));
    // opposite direction
    panAngle *= -panDirection;
    
    IActionRunner* panAndTilt = new PanAndTiltAction(robot, panAngle, kAbsHeadDown_rad, kPanIsAbsolute, kTiltIsAbsolute);
    fullAction->AddAction(panAndTilt);
  }
  
  StartActing(fullAction, &BehaviorTrackLaser::TransitionToWaitForLaser);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorTrackLaser::TransitionToBringingHeadDown(Robot& robot)
{
  SET_STATE(BringingHeadDown);
  
  const f32 backupSpeed_mmps = std::abs(_params.backupDistAfterPounce_mm) / _params.backupDurationAfterPounce_sec;
  
  CompoundActionParallel* moveBackAndBringHeadDown = new CompoundActionParallel(robot, {
    new MoveHeadToAngleAction(robot, kAbsHeadDown_rad),
    new DriveStraightAction(robot, _params.backupDistAfterPounce_mm, backupSpeed_mmps),
  });
  
  StartActing(moveBackAndBringHeadDown, &BehaviorTrackLaser::TransitionToWaitForLaser);
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorTrackLaser::TransitionToRotateToWatchingNewArea(Robot& robot)
{
  SET_STATE( RotateToWatchingNewArea );
  _lastTimeRotate = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();

  Radians panAngle(DEG_TO_RAD(GetRNG().RandDblInRange(_params.randomInitialSearchPanMin_deg,
                                                      _params.randomInitialSearchPanMax_deg)));

  // other direction weighted based on the cumulative turn rads - constantly pulls robot towards center
  const double weightedSearchAmplitude = (_params.searchAmplitude_deg /
                                          (_params.searchAmplitude_deg - _cumulativeTurn_rad.getDegrees())) * 0.5;
  if( GetRNG().RandDbl() < weightedSearchAmplitude)
  {
    panAngle *= -1.f;
  }
  _cumulativeTurn_rad += panAngle;
  
  const bool kIsPanAbsolute = false;
  const bool kIsTiltAbsolute = true;
  IActionRunner* panAction = new PanAndTiltAction(robot, panAngle, kAbsHeadDown_rad, kIsPanAbsolute, kIsTiltAbsolute);
  
  _lastLaserObservation.type = LaserObservation::Type::None;
  
  StartActing(panAction, &BehaviorTrackLaser::TransitionToWaitForLaser);
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorTrackLaser::TransitionToWaitForLaser(Robot& robot)
{
  SET_STATE(WaitingForLaser);
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorTrackLaser::TransitionToTrackLaser(Robot& robot)
{
  SET_STATE(TrackLaser);
  
  const Point2f& pt = _lastLaserObservation.pointWrtRobot;
  const Pose3d laserPointPose(0.f, Z_AXIS_3D(), {pt.x(), pt.y(), 0.f}, &robot.GetPose());
  
  CompoundActionSequential* action = new CompoundActionSequential(robot, {
    new TurnTowardsPoseAction(robot, laserPointPose, M_PI_F),
    new TriggerAnimationAction(robot, AnimationTrigger::AcknowledgeObject, 1, true,
                               Util::EnumToUnderlying(AnimTrackFlag::HEAD_TRACK))
  });
  
  {
    auto const& laserMsgTag = ExternalInterface::MessageEngineToGameTag::RobotObservedLaserPoint;
    TrackGroundPointAction* trackAction = new TrackGroundPointAction(robot, laserMsgTag);
    
    trackAction->SetUpdateTimeout(_params.trackingTimeout_sec);
    trackAction->SetMoveEyes(true);
    
    const TimeStamp_t predictionDuration_ms = Util::SecToMilliSec(_params.predictionDuration_sec);
    if(predictionDuration_ms > 0)
    {
      const bool kUseXPrediction = false;
      const bool kUseYPrediction = true;
      trackAction->EnablePredictionWhenLost(kUseXPrediction, kUseYPrediction, predictionDuration_ms);
    }
    
    trackAction->EnableDrivingAnimation(true);
    
    // Add extra motion
    trackAction->SetTiltTolerance(DEG_TO_RAD(3));
    trackAction->SetPanTolerance(DEG_TO_RAD(5));
    trackAction->SetClampSmallAnglesToTolerances(true);
    
    trackAction->SetStopCriteria(DEG_TO_RAD(_params.pouncePanTol_deg),
                                 DEG_TO_RAD(_params.pounceTiltTol_deg),
                                 0.f, _params.pounceIfWithinDist_mm,
                                 _params.pounceAfterTrackingFor_sec);
    
    // Vary the pan speed slightly each time
    DEV_ASSERT(_params.minPanDuration_sec <= _params.maxPanDuration_sec,
               "BehaviorTrackLaser.TransitionToTrackLaser.BadPanDurations");
    const f32 panTime_sec = GetRNG().RandDblInRange(_params.minPanDuration_sec, _params.maxPanDuration_sec);
    trackAction->SetPanDuration(panTime_sec);
    
    // Vary the forward driving speed slightly each time
    DEV_ASSERT(_params.minTimeToReachLaser_sec <= _params.maxTimeToReachLaser_sec,
               "BehaviorTrackLaser.TransitionToTrackLaser.BadTimeToReachLaserParams");
    const f32 driveTime_sec = GetRNG().RandDblInRange(_params.minTimeToReachLaser_sec, _params.maxTimeToReachLaser_sec);
    trackAction->SetDesiredTimeToReachTarget(driveTime_sec);
    
    action->AddAction(trackAction);
  }
  
  // For logging how long tracking ran
  // NOTE: this actually includes the time it took to turn towards and play the acknowledge animation (in
  //       addition to the actual tracking time) but it's just used for logging an event so high precision
  //       isn't really worth the complexity of adding an additional behavior state so separate this
  //       compound action
  _startedTracking_sec = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  
  StartActing(action,
              [this,&robot](ActionResult result)
              {
                const bool doPounce = (ActionResult::SUCCESS == result);
                const f32  trackingTime_sec = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds() - _startedTracking_sec;
              
                PRINT_CH_DEBUG(kLogChannelName, "BehaviorTrackLaser.TransitionToTrackLaser.TrackingFinished",
                               "Result: %s Time: %fsec", EnumToString(result), trackingTime_sec);
                
                // Log DAS event indicating how long we tracked in seconds in s_val and whether we pounced in
                // ddata (1 for pounce, 0 otherwise)
                Util::sEventF("robot.laser_behavior.tracking_completed", {{DDATA, (doPounce ? "1" : "0")}}, "%f", trackingTime_sec);

                if(trackingTime_sec > _params.trackingTimeToAchieveObjective_sec)
                {
                  BehaviorObjectiveAchieved(BehaviorObjective::LaserTracked);
                }
                
                if(doPounce) {
                  TransitionToPounce(robot);
                } else {
                  TransitionToRotateToWatchingNewArea(robot);
                }
              });
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorTrackLaser::TransitionToPounce(Robot& robot)
{
  SET_STATE(Pouncing);
  
  IActionRunner* pounceAction = new TriggerLiftSafeAnimationAction(robot, AnimationTrigger::LaserPounce);
  
  StartActing(pounceAction, [this,&robot]() {
    BehaviorObjectiveAchieved(BehaviorObjective::LaserPounced);
    TransitionToBringingHeadDown(robot);
  });
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorTrackLaser::TransitionToGetOutBored(Robot& robot)
{
  SET_STATE(GetOutBored);
  
  StopActing(false);
  
  StartActing(new TriggerLiftSafeAnimationAction(robot, AnimationTrigger::LaserGetOut),
              [this]() {
                SET_STATE(WaitForStop);
              });
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorTrackLaser::AlwaysHandle(const EngineToGameEvent& event, const Robot& robot)
{
  switch (event.GetData().GetTag())
  {
    case MessageEngineToGameTag::RobotObservedLaserPoint:
    {
      // Note this helper does slightly different things depending on running/not running
      SetLastLaserObservation(robot, event);
      break;
    }
      
    default: {
      PRINT_NAMED_ERROR("BehaviorTrackLaser.AlwaysHandle.InvalidEvent", "");
      break;
    }
  }
  
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorTrackLaser::SetLastLaserObservation(const Robot& robot, const EngineToGameEvent& event)
{
  const auto & laserObserved = event.GetData().Get_RobotObservedLaserPoint();
  const bool inGroundPlane   = (laserObserved.ground_area_fraction > 0.f);
  const bool isRunning       = IsRunning();
  const bool closeEnough     = (isRunning || laserObserved.ground_x_mm < _params.maxDistToGetAttention_mm);
  
  if ( inGroundPlane && closeEnough )
  {
    // This observation is "confirmed" if we're running and exposure has been adjusted to dark
    const VisionComponent& visionComponent = robot.GetVisionComponent();
    const bool isConfirmed = (isRunning &&
                              (robot.GetLastImageTimeStamp() - _changedExposureTime_ms) > (TimeStamp_t)_params.timeToWaitForExposureChange_ms &&
                              visionComponent.GetCurrentCameraExposureTime_ms() == _params.darkenedExposure_ms &&
                              Util::IsFltNear(visionComponent.GetCurrentCameraGain(), _params.darkenedGain));
    
    PRINT_CH_DEBUG(kLogChannelName, "BehaviorTrackLaser.SetLastLaserObservation.SawLaser",
                   "%s laser at (%d,%d)",
                   isConfirmed ? "Confirmed" : "Saw possible",
                   laserObserved.ground_x_mm, laserObserved.ground_y_mm);
    
    _lastLaserObservation.type = (isConfirmed ? LaserObservation::Type::Confirmed : LaserObservation::Type::Unconfirmed);
    _lastLaserObservation.time_sec = event.GetCurrentTime();
    _lastLaserObservation.pointWrtRobot.x() = laserObserved.ground_x_mm;
    _lastLaserObservation.pointWrtRobot.y() = laserObserved.ground_y_mm;
    _haveConfirmedLaser = isConfirmed;
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorTrackLaser::Cleanup(Robot& robot)
{
  SET_STATE(Complete);
  
  _haveConfirmedLaser = false;
  _lastLaserObservation.type = LaserObservation::Type::None;
  
  // Leave the exposure/color settings as they were when we started
  robot.GetVisionComponent().PopCurrentModeSchedule();
  robot.GetVisionComponent().SetAndDisableAutoExposure(_originalCameraSettings.exposureTime_ms,
                                                       _originalCameraSettings.gain);
  robot.GetVisionComponent().EnableAutoExposure(true);
  robot.GetVisionComponent().EnableColorImages(_originalCameraSettings.colorEnabled);
  
  // Only pop animations if set within this behavior
  if(!_shouldStreamline){
    robot.GetAnimationStreamer().PopIdleAnimation();
    robot.GetDrivingAnimationHandler().PopDrivingAnimations();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorTrackLaser::SetState_internal(State state, const std::string& stateName)
{
  // don't log staying in same state (we do that a lot in this behavior with WaitForLaser)
  if(_state != state)
  {
    _state = state;
    SetDebugStateName(stateName);
  }
}

} // namespace Cozmo
} // namespace Anki
