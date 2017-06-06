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

#include "anki/cozmo/basestation/behaviorSystem/behaviors/freeplay/userInteractive/behaviorTrackLaser.h"

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
  SubscribeToTags({{
    EngineToGameTag::RobotObservedLaserPoint,
    EngineToGameTag::RobotProcessedImage
  }});

  SetParamsFromConfig(config);

  SET_STATE(Inactive);
  
  _lastLaserObservation.type         = LaserObservation::Type::None;
  _lastLaserObservation.timestamp_ms      = 0;
  _lastLaserObservation.timestamp_prev_ms = 0;
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
  SET_FLOAT_HELPER(numImagesToWaitForExposureChange);
  SET_FLOAT_HELPER(imageMeanFractionForExposureChange);
  
  SET_FLOAT_HELPER(maxTimeToConfirm_ms);
  SET_FLOAT_HELPER(searchAmplitude_deg);
  
  SET_FLOAT_HELPER(maxTimeSinceNoLaser_ms);
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
  const Robot& robot = preReqData.GetRobot();
  const bool featureEnabled = robot.GetContext()->GetFeatureGate()->IsFeatureEnabled(Anki::Cozmo::FeatureType::Laser);
  if(!featureEnabled)
  {
    return false;
  }
  
  if(robot.IsPickingOrPlacing() || robot.IsCarryingObject())
  {
    // Don't interrupt while docking (since exposure change for confirmation can be really bad
    // and noticeable while tracking a cube). Also don't get distracted while carrying a cube.
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
      const TimeStamp_t lastImgTime_ms = preReqData.GetRobot().GetLastImageTimeStamp();
      const TimeStamp_t seenWithin_ms  = Util::SecToMilliSec(_params.startIfLaserSeenWithin_sec);
      
      const bool crntSeenRecently = ((_lastLaserObservation.timestamp_ms + seenWithin_ms) > lastImgTime_ms);
      const bool prevSeenRecently = ((_lastLaserObservation.timestamp_prev_ms + seenWithin_ms) > lastImgTime_ms);
      
      if(crntSeenRecently && prevSeenRecently)
      {
        PRINT_CH_DEBUG(kLogChannelName, "BehaviorTrackLaser.IsRunnableInternal.CanStart",
                       "LastObs:%dms PrevObs:%dms LastImgTime:%dms Threshold:%.2fs",
                       _lastLaserObservation.timestamp_ms,
                       _lastLaserObservation.timestamp_prev_ms,
                       lastImgTime_ms, _params.startIfLaserSeenWithin_sec);
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
    
    TransitionToWaitForExposureChange(robot);
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
  _haveEverConfirmedLaser = false;
  
  _originalCameraSettings.colorEnabled = robot.GetVisionComponent().AreColorImagesEnabled();
  _originalCameraSettings.exposureTime_ms = robot.GetVisionComponent().GetCurrentCameraExposureTime_ms();
  _originalCameraSettings.gain = robot.GetVisionComponent().GetCurrentCameraGain();
  
  // disable all vision except what's needed laser point detection and confirmation
  const bool kUseDefaultsForUnspecified = false;
  AllVisionModesSchedule schedule({
    {VisionMode::DetectingLaserPoints, VisionModeSchedule(true)},
    {VisionMode::ComputingStatistics,  VisionModeSchedule(true)},
  }, kUseDefaultsForUnspecified);
  robot.GetVisionComponent().PushNextModeSchedule(std::move(schedule));
  robot.GetVisionComponent().SetAndDisableAutoExposure(_params.darkenedExposure_ms, _params.darkenedGain);
  robot.GetVisionComponent().EnableColorImages(true);
  
  _exposureChangedTime_ms = 0;
  _imageMean = -1;
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
  
    case State::WaitingForExposureChange:
    {
      if(_exposureChangedTime_ms == 0)
      {
        // Still waiting.
        break;
      }
      
      // We observed the mean change in a RobotProcessedImage message, so go ahead and
      // transition to WaitingForLaser. Immediately fall through to the case below
      // to avoid waiting a tick to check if we also saw a confirmed laser in the same image.
      StopActing(false); // Stop the WaitForImages action
      TransitionToWaitForLaser(robot);
      
      // NOTE: Deliberate fallthrough!!
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
        const TimeStamp_t lastImgTime_ms = robot.GetLastImageTimeStamp();
        const TimeStamp_t maxConfirmTime_ms = _exposureChangedTime_ms + (TimeStamp_t)_params.maxTimeToConfirm_ms;
        
        if(!_haveEverConfirmedLaser && (lastImgTime_ms > maxConfirmTime_ms))
        {
          // Ran out of time confirming the unconfirmed laser we had seen, immediately complete
          // to get out of the behavior without really having done anything
          PRINT_CH_DEBUG(kLogChannelName, "BehaviorTrackLaser.UpdateInternal.NeverConfirmed",
                         "LastImg:%dms LastObs:%dms MaxTime:%dms",
                         lastImgTime_ms, _lastLaserObservation.timestamp_ms, (TimeStamp_t)_params.maxTimeToConfirm_ms);
          
          LOG_EVENT("robot.laser_behavior.laser_never_confirmed", "");
          return Status::Complete;
        }
        else if(!CheckForTimeout(robot))
        {
          // Check to see if it's time to rotate again
          const float currentTime_sec = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
          if ( (_lastTimeRotate + _params.maxTimeBeforeRotate_sec) < currentTime_sec )
          {
            TransitionToRotateToWatchingNewArea(robot);
          }
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
      CheckForTimeout(robot);
      
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
bool BehaviorTrackLaser::CheckForTimeout(Robot& robot)
{
  // We're done if we haven't seen a laser in a long while or we've exceeded the max time for the behavior
  bool isTimedOut = false;
  
  const TimeStamp_t lastImgTime_ms = robot.GetLastImageTimeStamp();
  
  const TimeStamp_t maxTimeSinceLastLaser_ms = (TimeStamp_t) _params.maxTimeSinceNoLaser_ms;
  
  if ( (_lastLaserObservation.timestamp_ms + maxTimeSinceLastLaser_ms) < lastImgTime_ms )
  {
    PRINT_CH_INFO(kLogChannelName, "BehaviorTrackLaser.UpdateInternal.NoLaserTimeout",
                  "No laser found, giving up after %dms",
                  maxTimeSinceLastLaser_ms);
    
    Util::sEventF("robot.laser_behavior.no_laser_timeout", {{DDATA, (_haveEverConfirmedLaser ? "0" : "1")}},
                  "%d", maxTimeSinceLastLaser_ms);
    
    isTimedOut = true;
  }
  else
  {
    const float currentTime_sec = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
    if ( (GetTimeStartedRunning_s() + _params.maxTimeBehaviorTimeout_sec) < currentTime_sec)
    {
      PRINT_CH_INFO(kLogChannelName, "BehaviorTrackLaser.UpdateInternal.BehaviorTimeout",
                    "Started:%.1f, Now:%.1f, Timeout:%.1f",
                    GetTimeStartedRunning_s(), currentTime_sec, _params.maxTimeBehaviorTimeout_sec);
      
      LOG_EVENT("robot.laser_behavior.ran_until_max_timeout", "%f", _params.maxTimeBehaviorTimeout_sec);
      
      isTimedOut = true;
    }
  }
  
  if(isTimedOut)
  {
    if(_haveEverConfirmedLaser)
    {
      TransitionToGetOutBored(robot);
    }
    else
    {
      SET_STATE(WaitForStop);
    }
  }
  return false;
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
void BehaviorTrackLaser::TransitionToWaitForExposureChange(Robot& robot)
{
  SET_STATE(WaitingForExposureChange);
  
  // Wait a couple of images for the exposure change to take effect.
  // Note that we don't care what kind of processing occurred in those images;
  // any image will do.
  const s32 numImagesToWait = (s32)_params.numImagesToWaitForExposureChange;
  WaitForImagesAction* waitAction = new WaitForImagesAction(robot, numImagesToWait);
  
  // Once we've gottena a couple of images, switch to looking for a laser dot
  // to confirm it.
  StartActing(waitAction, [this](Robot& robot)
  {
    // We *assume* exposure has changed after seeing enough images, no way to know for sure!
    // Once this is true, observed lasers will be considered "confirmed".
    _exposureChangedTime_ms = robot.GetLastImageTimeStamp();
    TransitionToWaitForLaser(robot);
    
    PRINT_CH_DEBUG(kLogChannelName, "BehaviorTrackLaser.TransitionToWaitForExposureChange.AssumingChanged",
                   "%d images have passed. Assuming exposure change has taken effect at t=%dms",
                   (s32)_params.numImagesToWaitForExposureChange,
                   _exposureChangedTime_ms);
  });
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
  
  // Wait until we actually confirm a laser to adjust the idle/driving animations, to
  // avoid eyes changing if we see a possible laser and end up _not_ confirming.
  // However, don't override sparks driving or idle animations (i.e. when "streamlining").
  if(!_shouldStreamline && !_haveAdjustedAnimations)
  {
    robot.GetAnimationStreamer().PushIdleAnimation(AnimationTrigger::LaserFace);
    
    robot.GetDrivingAnimationHandler().PushDrivingAnimations({
      AnimationTrigger::LaserDriveStart,
      AnimationTrigger::LaserDriveLoop,
      AnimationTrigger::LaserDriveEnd
    });
    
    _haveAdjustedAnimations = true;
  }
  
  const Point2f& pt = _lastLaserObservation.pointWrtRobot;
  const Pose3d laserPointPose(0.f, Z_AXIS_3D(), {pt.x(), pt.y(), 0.f}, &robot.GetPose());
  
  CompoundActionSequential* action = new CompoundActionSequential(robot, {
    new TurnTowardsPoseAction(robot, laserPointPose, M_PI_F),
    new TriggerAnimationAction(robot, AnimationTrigger::LaserAcknowledge, 1, true,
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
    
    const bool kInterruptDrivingAnimOnSuccess = true;
    trackAction->SetStopCriteria(DEG_TO_RAD(_params.pouncePanTol_deg),
                                 DEG_TO_RAD(_params.pounceTiltTol_deg),
                                 0.f, _params.pounceIfWithinDist_mm,
                                 _params.pounceAfterTrackingFor_sec,
                                 kInterruptDrivingAnimOnSuccess);
    
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
  //       isn't really worth the complexity of adding an additional behavior state to separate this
  //       compound action.
  _startedTracking_sec = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  
  StartActing(action,
              [this,&robot](ActionResult result)
              {
                const bool doPounce = (ActionResult::SUCCESS == result);
                const f32  trackingTime_sec = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds() - _startedTracking_sec;
              
                PRINT_CH_DEBUG(kLogChannelName, "BehaviorTrackLaser.TransitionToTrackLaser.TrackingFinished",
                               "Result: %s Time: %fsec WillPounce: %s", EnumToString(result), trackingTime_sec,
                               doPounce ? "Y" : "N");
                
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
    NeedActionCompleted(NeedsActionId::Pounce);
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
  const EngineToGameTag& tag = event.GetData().GetTag();
  switch( tag )
  {
    case MessageEngineToGameTag::RobotObservedLaserPoint:
    {
      // Note this helper does slightly different things depending on running/not running
      SetLastLaserObservation(robot, event);
      break;
    }
      
    case MessageEngineToGameTag::RobotProcessedImage:
    {
      // If we see the image mean drop significantly while waiting for the exposure change,
      // assume the change has happened so we can more immediately confirm a possible laser
      // and start tracking.
      const bool exposureChangeAlreadyObserved = (_exposureChangedTime_ms > 0);
      if(State::WaitingForExposureChange == _state && !exposureChangeAlreadyObserved)
      {
        auto const& robotProcessedImage = event.GetData().Get_RobotProcessedImage();
        
        auto visionModeIter = robotProcessedImage.visionModes.begin();
        while(visionModeIter != robotProcessedImage.visionModes.end())
        {
          if(VisionMode::ComputingStatistics == (*visionModeIter))
          {
            const u8 crntImgMean = robotProcessedImage.mean;
            
            PRINT_CH_DEBUG(kLogChannelName, "BehaviorTrackLaser.AlwaysHandle.ImageMean",
                           "Waiting for exposure change. Current mean: %d", crntImgMean);
            
            if(_imageMean < 0)
            {
              _imageMean = Util::numeric_cast<s16>(crntImgMean);
            }
            else if(crntImgMean < Util::numeric_cast<u8>((f32)_imageMean * _params.imageMeanFractionForExposureChange))
            {
              PRINT_CH_DEBUG(kLogChannelName, "BehaviorTrackLaser.AlwaysHandle.ObservedExposureChange",
                             "Image mean dropped from %d to %d. Assuming exposure changed at t=%dms.",
                             _imageMean, crntImgMean, robotProcessedImage.timestamp);
              
              _exposureChangedTime_ms = robotProcessedImage.timestamp;
              
              // RobotProcessedImage messages come _after_ RobotObservedLaserPoint messages from that image.
              // Check to see if we saw a (possible) laser in this image and if so, mark it as confirmed, now
              // that we know the exposure had already dropped.
              if(_lastLaserObservation.timestamp_ms == _exposureChangedTime_ms)
              {
                _lastLaserObservation.type = LaserObservation::Type::Confirmed;
                _haveEverConfirmedLaser |= true;
              }
            }
            
            break;
          }
          
          ++visionModeIter;
        }
        
      }
      break;
    }
 
    default: {
      PRINT_NAMED_ERROR("BehaviorTrackLaser.AlwaysHandle.InvalidEvent",
                        "%s", MessageEngineToGameTagToString(tag));
      break;
    }
  }
  
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorTrackLaser::SetLastLaserObservation(const Robot& robot, const EngineToGameEvent& event)
{
  const auto & laserObserved = event.GetData().Get_RobotObservedLaserPoint();
  const bool inGroundPlane   = (laserObserved.ground_area_fraction > 0.f);
  const bool closeEnough     = (IsRunning() || laserObserved.ground_x_mm < _params.maxDistToGetAttention_mm);
  
  if ( inGroundPlane && closeEnough )
  {
    // This observation is "confirmed" if exposure has been adjusted to dark
    const bool isConfirmed = (_exposureChangedTime_ms > 0);
    
    PRINT_CH_DEBUG(kLogChannelName, "BehaviorTrackLaser.SetLastLaserObservation.SawLaser",
                   "%s laser at (%d,%d), t=%d",
                   isConfirmed ? "Confirmed" : "Saw possible",
                   laserObserved.ground_x_mm, laserObserved.ground_y_mm, laserObserved.timestamp);
    
    _lastLaserObservation.type = (isConfirmed ? LaserObservation::Type::Confirmed : LaserObservation::Type::Unconfirmed);
    _lastLaserObservation.timestamp_prev_ms = _lastLaserObservation.timestamp_ms; // keep track of previous
    _lastLaserObservation.timestamp_ms = laserObserved.timestamp;
    _lastLaserObservation.pointWrtRobot.x() = laserObserved.ground_x_mm;
    _lastLaserObservation.pointWrtRobot.y() = laserObserved.ground_y_mm;
    _haveEverConfirmedLaser |= isConfirmed; // Once we've ever confirmed, stay true
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorTrackLaser::Cleanup(Robot& robot)
{
  SET_STATE(Complete);
  
  _haveEverConfirmedLaser = false;
  _exposureChangedTime_ms = 0;
  _lastLaserObservation.type = LaserObservation::Type::None;
  
  // Leave the exposure/color settings as they were when we started
  robot.GetVisionComponent().PopCurrentModeSchedule();
  robot.GetVisionComponent().SetAndDisableAutoExposure(_originalCameraSettings.exposureTime_ms,
                                                       _originalCameraSettings.gain);
  robot.GetVisionComponent().EnableAutoExposure(true);
  robot.GetVisionComponent().EnableColorImages(_originalCameraSettings.colorEnabled);
  
  // Only pop animations if set within this behavior
  if(_haveAdjustedAnimations)
  {
    robot.GetAnimationStreamer().PopIdleAnimation();
    robot.GetDrivingAnimationHandler().PopDrivingAnimations();
    _haveAdjustedAnimations = false;
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
