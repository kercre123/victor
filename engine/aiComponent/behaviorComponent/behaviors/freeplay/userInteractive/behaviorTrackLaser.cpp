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

#include "engine/aiComponent/behaviorComponent/behaviors/freeplay/userInteractive/behaviorTrackLaser.h"

#include "coretech/common/engine/jsonTools.h"
#include "coretech/common/engine/math/point.h"
#include "coretech/common/engine/math/pose.h"
#include "coretech/common/engine/utils/timer.h"

#include "engine/actions/animActions.h"
#include "engine/actions/basicActions.h"
#include "engine/actions/trackGroundPointAction.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/components/carryingComponent.h"
#include "engine/components/dockingComponent.h"
#include "engine/components/movementComponent.h"
#include "engine/components/visionComponent.h"
#include "engine/cozmoContext.h"
#include "engine/drivingAnimationHandler.h"
#include "engine/events/ankiEvent.h"
#include "engine/externalInterface/externalInterface.h"
#include "engine/utils/cozmoFeatureGate.h"

#include "clad/externalInterface/messageEngineToGame.h"

#include "util/helpers/quoteMacro.h"

#define SET_STATE(s) SetState_internal(State::s, #s)

namespace Anki {
namespace Vector {

using namespace ExternalInterface;

namespace {

static const constexpr char* const kLogChannelName              = "Behaviors";
static const constexpr char* const kSkipGetOutAnimKey           = "skipGetOutAnim";
static const constexpr char* const kMaxLostLaserTimeoutGraphKey = "maxLostLaserTimeoutGraph_sec";
static const constexpr float kInvalidStartedTime_sec            = -1.f;
}

// Cozmo's low head angle for watching for lasers
static const Radians kAbsHeadDown_rad(MIN_HEAD_ANGLE);


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorTrackLaser::InstanceConfig::InstanceConfig()
{
  skipGetOutAnim = false;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorTrackLaser::InstanceConfig::InstanceConfig(const Json::Value& config)
{
  // Helper macro to grab the key with the same name as the param struct member from the Json and
  // set it as a float
  # define SET_FLOAT_HELPER(__name__) do { \
    __name__ = JsonTools::ParseFloat(config, QUOTE(__name__), "BehaviorTrackLaser.SetParamsFromConfig");\
    if( ANKI_DEVELOPER_CODE ) {\
      varNames.emplace_back( QUOTE(__name__) );\
    }\
  } while(0)

  // In same order as defined in the params struct
  SET_FLOAT_HELPER(startIfLaserSeenWithin_sec);
  SET_FLOAT_HELPER(maxDistToGetAttention_mm);

  SET_FLOAT_HELPER(darkenedExposure_ms);
  SET_FLOAT_HELPER(darkenedGain);
  SET_FLOAT_HELPER(numImagesToWaitForExposureChange);
  SET_FLOAT_HELPER(imageMeanFractionForExposureChange);

  SET_FLOAT_HELPER(maxTimeToConfirm_ms);
  SET_FLOAT_HELPER(searchAmplitude_deg);

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

  skipGetOutAnim = false;
  JsonTools::GetValueOptional(config, kSkipGetOutAnimKey, skipGetOutAnim);

  if(ANKI_VERIFY(config.isMember(kMaxLostLaserTimeoutGraphKey),
                 "BehaviorTrackLaser.SetParamsFromConfig.MissingLostLaserGraphKey",
                 "No key found for lost laser timeout graph")){
    const Json::Value& maxLostLaserTimeoutGraph = config[kMaxLostLaserTimeoutGraphKey];
    maxLostLaserTimeoutGraph_sec.ReadFromJson(maxLostLaserTimeoutGraph);
  }


# undef SET_FLOAT_HELPER

}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorTrackLaser::DynamicVariables::DynamicVariables()
{
  haveEverConfirmedLaser              = false;
  haveAdjustedAnimations              = false;
  shouldSendTrackingObjectiveAchieved = false;

  imageMean                 = -1;
  exposureChangedTime_ms    = 0;
  lastTimeRotate            = 0.f;
  startedTracking_sec       = 0.f;
  currentLostLaserTimeout_s = 0.f;
  cumulativeTurn_rad        = 0;
  lastTimeRotate            = 0.0f;

  state = State::Inactive;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorTrackLaser::BehaviorTrackLaser(const Json::Value& config)
: ICozmoBehavior(config)
{
  SubscribeToTags({{
    EngineToGameTag::RobotObservedLaserPoint,
    EngineToGameTag::RobotProcessedImage
  }});

  _iConfig = InstanceConfig(config);

  SET_STATE(Inactive);

  _dVars.lastLaserObservation.type         = LaserObservation::Type::None;
  _dVars.lastLaserObservation.timestamp_ms      = 0;
  _dVars.lastLaserObservation.timestamp_prev_ms = 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorTrackLaser::GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const
{
  const char* list[] = {
    kSkipGetOutAnimKey,
    kMaxLostLaserTimeoutGraphKey
  };
  expectedKeys.insert( std::begin(list), std::end(list) );
  for( const auto& str : _iConfig.varNames ) {
    expectedKeys.insert( str.c_str() );
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorTrackLaser::WantsToBeActivatedBehavior() const
{
  const auto& robotInfo = GetBEI().GetRobotInfo();
  const bool featureEnabled = robotInfo.GetContext()->GetFeatureGate()->IsFeatureEnabled(Anki::Vector::FeatureType::Laser);
  if(!featureEnabled)
  {
    return false;
  }

  if(robotInfo.GetDockingComponent().IsPickingOrPlacing() ||
     robotInfo.GetCarryingComponent().IsCarryingObject())
  {
    // Don't interrupt while docking (since exposure change for confirmation can be really bad
    // and noticeable while tracking a cube). Also don't get distracted while carrying a cube.
    return false;
  }

  if(ShouldStreamline())
  {
    return true;
  }
  else
  {
    // Have we seen a potential laser observation recently?
    if(_dVars.lastLaserObservation.type != LaserObservation::Type::None)
    {
      const RobotTimeStamp_t lastImgTime_ms = robotInfo.GetLastImageTimeStamp();
      const TimeStamp_t seenWithin_ms  = Util::SecToMilliSec(_iConfig.startIfLaserSeenWithin_sec);

      const bool crntSeenRecently = ((_dVars.lastLaserObservation.timestamp_ms + seenWithin_ms) > lastImgTime_ms);
      const bool prevSeenRecently = ((_dVars.lastLaserObservation.timestamp_prev_ms + seenWithin_ms) > lastImgTime_ms);

      if(crntSeenRecently && prevSeenRecently)
      {
        PRINT_CH_DEBUG(kLogChannelName, "BehaviorTrackLaser.WantsToBeActivatedBehavior.CanStart",
                       "LastObs:%dms PrevObs:%dms LastImgTime:%dms Threshold:%.2fs",
                       (TimeStamp_t)_dVars.lastLaserObservation.timestamp_ms,
                       (TimeStamp_t)_dVars.lastLaserObservation.timestamp_prev_ms,
                       (TimeStamp_t)lastImgTime_ms, _iConfig.startIfLaserSeenWithin_sec);
        return true;
      }
    }

    return false;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorTrackLaser::OnBehaviorActivated()
{
  InitHelper();
  const bool haveSeenLaser = (_dVars.lastLaserObservation.type != LaserObservation::Type::None);
  if(haveSeenLaser || ShouldStreamline())
  {
    // Pretend we just rotated so we don't immediately rotate to start
    _dVars.lastTimeRotate = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();

    TransitionToWaitForExposureChange();
  }
  else
  {
    TransitionToInitialSearch();
  }

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorTrackLaser::InitHelper()
{
  _dVars.haveEverConfirmedLaser = false;
  _dVars.shouldSendTrackingObjectiveAchieved = false;
  _dVars.originalCameraSettings = GetBEI().GetVisionComponent().GetCurrentCameraParams();

  // disable all vision except what's needed laser point detection and confirmation
  const bool kUseDefaultsForUnspecified = false;
  AllVisionModesSchedule schedule({
    {VisionMode::DetectingLaserPoints, VisionModeSchedule(true)},
    {VisionMode::ComputingStatistics,  VisionModeSchedule(true)},
  }, kUseDefaultsForUnspecified);
  GetBEI().GetVisionComponent().PushNextModeSchedule(std::move(schedule));
  
  const Vision::CameraParams darkenedParams(_iConfig.darkenedExposure_ms, _iConfig.darkenedGain,
                                            _dVars.originalCameraSettings.whiteBalanceGainR,
                                            _dVars.originalCameraSettings.whiteBalanceGainG,
                                            _dVars.originalCameraSettings.whiteBalanceGainB);
  
  GetBEI().GetVisionComponent().SetAndDisableCameraControl(darkenedParams);

  _dVars.exposureChangedTime_ms = 0;
  _dVars.imageMean = -1;
  _dVars.currentLostLaserTimeout_s = _iConfig.maxLostLaserTimeoutGraph_sec.EvaluateY(0);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorTrackLaser::BehaviorUpdate()
{
  if(!IsActivated()){
    return;
  }

  switch(_dVars.state)
  {
    case State::WaitForStop:
    {
      CancelSelf();
      return;
    }

    case State::WaitingForExposureChange:
    {
      if(_dVars.exposureChangedTime_ms == 0)
      {
        // Still waiting.
        break;
      }

      // We observed the mean change in a RobotProcessedImage message, so go ahead and
      // transition to WaitingForLaser. Immediately fall through to the case below
      // to avoid waiting a tick to check if we also saw a confirmed laser in the same image.
      CancelDelegates(false); // Stop the WaitForImages action
      TransitionToWaitForLaser();

      // NOTE: Deliberate fallthrough!!
    }

    case State::WaitingForLaser:
    {
      if(LaserObservation::Type::Confirmed == _dVars.lastLaserObservation.type)
      {
        PRINT_NAMED_INFO("robot.laser_behavior.confirmed_laser", "");
        TransitionToRespondToLaser();
        break;
      }
      else
      {
        if(!CheckForTimeout())
        {
          // Check to see if it's time to rotate again
          const float currentTime_sec = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
          if ( (_dVars.lastTimeRotate + _iConfig.maxTimeBeforeRotate_sec) < currentTime_sec )
          {
            TransitionToRotateToWatchingNewArea();
          }
        }
        else // Have timed out
        {
          if(_dVars.haveEverConfirmedLaser)
          {
            TransitionToGetOutBored();
          }
          else
          {
            CancelSelf();
            return;
          }
        }
      }
      break;
    }

    case State::TrackLaser: // Let tracking action deal with timing out
    {
      if(_dVars.lastLaserObservation.type != LaserObservation::None){
        if(_dVars.startedTracking_sec == kInvalidStartedTime_sec){
          _dVars.startedTracking_sec = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
        }

        const f32  trackingTime_sec = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds() - _dVars.startedTracking_sec;
        if(trackingTime_sec > _iConfig.trackingTimeToAchieveObjective_sec)
        {
          _dVars.shouldSendTrackingObjectiveAchieved = true;
        }
        break;
      }
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
      if(CheckForTimeout())
      {
        if(_dVars.haveEverConfirmedLaser)
        {
          TransitionToGetOutBored();
        }
        else
        {
          CancelSelf();
          return;
        }
      }

      break;
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**void BehaviorTrackLaser::ResumeInternal()
{
  _dVars.lastLaserObservation.type = LaserObservation::Type::None;
  InitHelper();
  TransitionToBringingHeadDown();

}**/

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorTrackLaser::OnBehaviorDeactivated()
{
  if(_dVars.shouldSendTrackingObjectiveAchieved){
    BehaviorObjectiveAchieved(BehaviorObjective::LaserTracked);
  }

  Cleanup();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorTrackLaser::CheckForTimeout()
{
  // We're done if we haven't seen a laser in a long while or we've exceeded the max time for the behavior
  bool isTimedOut = false;

  const RobotTimeStamp_t lastImgTime_ms = GetBEI().GetRobotInfo().GetLastImageTimeStamp();

  if (_dVars.haveEverConfirmedLaser &&
      (_dVars.lastLaserObservation.timestamp_ms + (_dVars.currentLostLaserTimeout_s * 1000)) < lastImgTime_ms )
  {
    PRINT_CH_INFO(kLogChannelName, "BehaviorTrackLaser.CheckForTimeout.NoLaserTimeout",
                  "No laser found, giving up after %fs",
                  _dVars.currentLostLaserTimeout_s);

    Util::sInfoF("robot.laser_behavior.no_laser_timeout", {{DDATA, (_dVars.haveEverConfirmedLaser ? "0" : "1")}},
                 "%f", (_dVars.currentLostLaserTimeout_s * 1000));

    isTimedOut = true;
  }
  else if(!_dVars.haveEverConfirmedLaser &&
          _dVars.state == State::WaitingForLaser &&
          _dVars.exposureChangedTime_ms + _iConfig.maxTimeToConfirm_ms < lastImgTime_ms)
  {
    // Ran out of time confirming the unconfirmed laser we had seen, immediately complete
    // to get out of the behavior without really having done anything
    PRINT_CH_DEBUG(kLogChannelName, "BehaviorTrackLaser.CheckForTimeout.NeverConfirmed",
                   "LastImg:%dms LastObs:%dms MaxTime:%dms",
                   (TimeStamp_t)lastImgTime_ms,
                   (TimeStamp_t)_dVars.lastLaserObservation.timestamp_ms,
                   (TimeStamp_t)_iConfig.maxTimeToConfirm_ms);

    PRINT_NAMED_INFO("robot.laser_behavior.laser_never_confirmed", "");

    isTimedOut = true;
  }
  else
  {
    const float currentTime_sec = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
    if ( (GetTimeActivated_s() + _iConfig.maxTimeBehaviorTimeout_sec) < currentTime_sec)
    {
      PRINT_CH_INFO(kLogChannelName, "BehaviorTrackLaser.CheckForTimeout.BehaviorTimeout",
                    "Started:%.1f, Now:%.1f, Timeout:%.1f",
                    GetTimeActivated_s(), currentTime_sec, _iConfig.maxTimeBehaviorTimeout_sec);

      PRINT_NAMED_INFO("robot.laser_behavior.ran_until_max_timeout", "%f", _iConfig.maxTimeBehaviorTimeout_sec);

      isTimedOut = true;
    }
  }

  return isTimedOut;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorTrackLaser::TransitionToInitialSearch()
{
  PRINT_CH_DEBUG(kLogChannelName, "BehaviorTrackLaser.TransitionToInitialSearch",
                 "BehaviorTrackLaser.TransitionToInitialSearch");
  SET_STATE(InitialSearch);

  CompoundActionSequential* fullAction = new CompoundActionSequential();

  const bool kPanIsAbsolute  = false;
  const bool kTiltIsAbsolute = true;

  float panDirection = 1.0f;
  {
    // set pan and tilt
    Radians panAngle(DEG_TO_RAD(GetRNG().RandDblInRange(_iConfig.randomInitialSearchPanMin_deg,
                                                        _iConfig.randomInitialSearchPanMax_deg)));
    // randomize direction
    if( GetRNG().RandDbl() < 0.5 )
    {
      panDirection = -1.0f;
    }

    panAngle *= panDirection;

    IActionRunner* panAndTilt = new PanAndTiltAction(panAngle, kAbsHeadDown_rad, kPanIsAbsolute, kTiltIsAbsolute);
    fullAction->AddAction(panAndTilt);
  }

  // pan another random amount in the other direction (should get us back close to where we started, but not
  // exactly)
  {
    // get new pan angle
    Radians panAngle(DEG_TO_RAD(GetRNG().RandDblInRange(_iConfig.randomInitialSearchPanMin_deg,
                                                        _iConfig.randomInitialSearchPanMax_deg)));
    // opposite direction
    panAngle *= -panDirection;

    IActionRunner* panAndTilt = new PanAndTiltAction(panAngle, kAbsHeadDown_rad, kPanIsAbsolute, kTiltIsAbsolute);
    fullAction->AddAction(panAndTilt);
  }

  DelegateIfInControl(fullAction, &BehaviorTrackLaser::TransitionToWaitForLaser);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorTrackLaser::TransitionToBringingHeadDown()
{
  SET_STATE(BringingHeadDown);

  const f32 backupSpeed_mmps = std::abs(_iConfig.backupDistAfterPounce_mm) / _iConfig.backupDurationAfterPounce_sec;

  CompoundActionParallel* moveBackAndBringHeadDown = new CompoundActionParallel({
    new MoveHeadToAngleAction(kAbsHeadDown_rad),
    new DriveStraightAction(_iConfig.backupDistAfterPounce_mm, backupSpeed_mmps),
  });

  DelegateIfInControl(moveBackAndBringHeadDown, &BehaviorTrackLaser::TransitionToWaitForLaser);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorTrackLaser::TransitionToRotateToWatchingNewArea()
{
  SET_STATE( RotateToWatchingNewArea );
  _dVars.lastTimeRotate = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();

  Radians panAngle(DEG_TO_RAD(GetRNG().RandDblInRange(_iConfig.randomInitialSearchPanMin_deg,
                                                      _iConfig.randomInitialSearchPanMax_deg)));

  // other direction weighted based on the cumulative turn rads - constantly pulls robot towards center
  const double weightedSearchAmplitude = (_iConfig.searchAmplitude_deg /
                                          (_iConfig.searchAmplitude_deg - _dVars.cumulativeTurn_rad.getDegrees())) * 0.5;
  if( GetRNG().RandDbl() < weightedSearchAmplitude)
  {
    panAngle *= -1.f;
  }
  _dVars.cumulativeTurn_rad += panAngle;

  const bool kIsPanAbsolute = false;
  const bool kIsTiltAbsolute = true;
  IActionRunner* panAction = new PanAndTiltAction(panAngle, kAbsHeadDown_rad, kIsPanAbsolute, kIsTiltAbsolute);

  _dVars.lastLaserObservation.type = LaserObservation::Type::None;

  DelegateIfInControl(panAction, &BehaviorTrackLaser::TransitionToWaitForLaser);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorTrackLaser::TransitionToWaitForExposureChange()
{
  SET_STATE(WaitingForExposureChange);

  // Wait a couple of images for the exposure change to take effect.
  // Note that we don't care what kind of processing occurred in those images;
  // any image will do.
  const s32 numImagesToWait = (s32)_iConfig.numImagesToWaitForExposureChange;
  CompoundActionParallel* action = new CompoundActionParallel({new WaitForImagesAction(numImagesToWait)});

  if(ShouldStreamline())
  {
    action->AddAction(new TriggerAnimationAction(AnimationTrigger::DEPRECATED_LookDownForLaser));
  }

  // Once we've gotten a couple of images, switch to looking for a laser dot
  // to confirm it.
  DelegateIfInControl(action, [this]()
  {
    // We *assume* exposure has changed after seeing enough images, no way to know for sure!
    // Once this is true, observed lasers will be considered "confirmed".
    _dVars.exposureChangedTime_ms = GetBEI().GetRobotInfo().GetLastImageTimeStamp();
    TransitionToWaitForLaser();

    PRINT_CH_DEBUG(kLogChannelName, "BehaviorTrackLaser.TransitionToWaitForExposureChange.AssumingChanged",
                   "%d images have passed. Assuming exposure change has taken effect at t=%dms",
                   (s32)_iConfig.numImagesToWaitForExposureChange,
                   (TimeStamp_t)_dVars.exposureChangedTime_ms);
  });
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorTrackLaser::TransitionToWaitForLaser()
{
  SET_STATE(WaitingForLaser);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorTrackLaser::TransitionToRespondToLaser()
{
  SET_STATE(RespondToLaser);
  auto& robotInfo = GetBEI().GetRobotInfo();
  // Wait until we actually confirm a laser to adjust the idle/driving animations, to
  // avoid eyes changing if we see a possible laser and end up _not_ confirming.
  // However, don't override sparks driving or idle animations (i.e. when "streamlining").
  if(!ShouldStreamline() && !_dVars.haveAdjustedAnimations)
  {
    robotInfo.GetDrivingAnimationHandler().PushDrivingAnimations(
                                                             {AnimationTrigger::DEPRECATED_LaserDriveStart,
                                                               AnimationTrigger::DEPRECATED_LaserDriveLoop,
                                                               AnimationTrigger::DEPRECATED_LaserDriveEnd},
                                                             GetDebugLabel());

    _dVars.haveAdjustedAnimations = true;
  }

  const Point2f& pt = _dVars.lastLaserObservation.pointWrtRobot;
  const Pose3d laserPointPose(0.f, Z_AXIS_3D(), {pt.x(), pt.y(), 0.f}, robotInfo.GetPose());

  CompoundActionSequential* action = new CompoundActionSequential({
    new TurnTowardsPoseAction(laserPointPose, M_PI_F),
    new TriggerAnimationAction(AnimationTrigger::DEPRECATED_LaserAcknowledge, 1, true,
                               Util::EnumToUnderlying(AnimTrackFlag::HEAD_TRACK))
  });

  DelegateIfInControl(action, &BehaviorTrackLaser::TransitionToTrackLaser);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorTrackLaser::TransitionToTrackLaser()
{
  SET_STATE(TrackLaser);

  // re-set laser observation so that we can ensure Cozmo reacquire the laser
  // after playing his react to laser animation
  _dVars.lastLaserObservation.type = LaserObservation::Type::None;
  _dVars.startedTracking_sec = kInvalidStartedTime_sec;

  auto const& laserMsgTag = ExternalInterface::MessageEngineToGameTag::RobotObservedLaserPoint;
  TrackGroundPointAction* trackAction = new TrackGroundPointAction(laserMsgTag);

  // set params on track action
  {
    trackAction->SetUpdateTimeout(_iConfig.trackingTimeout_sec);
    trackAction->SetMoveEyes(true);

    const TimeStamp_t predictionDuration_ms = Util::SecToMilliSec(_iConfig.predictionDuration_sec);
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
    trackAction->SetStopCriteria(DEG_TO_RAD(_iConfig.pouncePanTol_deg),
                                 DEG_TO_RAD(_iConfig.pounceTiltTol_deg),
                                 0.f, _iConfig.pounceIfWithinDist_mm,
                                 _iConfig.pounceAfterTrackingFor_sec,
                                 kInterruptDrivingAnimOnSuccess);

    // Vary the pan speed slightly each time
    DEV_ASSERT(_iConfig.minPanDuration_sec <= _iConfig.maxPanDuration_sec,
               "BehaviorTrackLaser.TransitionToTrackLaser.BadPanDurations");
    const f32 panTime_sec = GetRNG().RandDblInRange(_iConfig.minPanDuration_sec, _iConfig.maxPanDuration_sec);
    trackAction->SetPanDuration(panTime_sec);

    // Vary the forward driving speed slightly each time
    DEV_ASSERT(_iConfig.minTimeToReachLaser_sec <= _iConfig.maxTimeToReachLaser_sec,
               "BehaviorTrackLaser.TransitionToTrackLaser.BadTimeToReachLaserParams");
    const f32 driveTime_sec = GetRNG().RandDblInRange(_iConfig.minTimeToReachLaser_sec, _iConfig.maxTimeToReachLaser_sec);
    trackAction->SetDesiredTimeToReachTarget(driveTime_sec);
  }


  const float startedActionTime_sec = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();

  DelegateIfInControl(trackAction,
              [this, startedActionTime_sec](ActionResult result)
              {
                const bool doPounce = (ActionResult::SUCCESS == result);
                const f32  actionTrackingTime_sec = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds() - startedActionTime_sec;

                const float behaviorRunningElapsed = (BaseStationTimer::getInstance()->GetCurrentTimeInSeconds() - GetTimeActivated_s());
                _dVars.currentLostLaserTimeout_s = _iConfig.maxLostLaserTimeoutGraph_sec.EvaluateY(behaviorRunningElapsed);

                PRINT_CH_DEBUG(kLogChannelName, "BehaviorTrackLaser.TransitionToTrackLaser.TrackingFinished",
                               "Result: %s Time: %fsec WillPounce: %s", EnumToString(result), actionTrackingTime_sec,
                               doPounce ? "Y" : "N");

                // Log DAS event indicating how long we tracked in seconds in s_val and whether we pounced in
                // ddata (1 for pounce, 0 otherwise)
                Util::sInfoF("robot.laser_behavior.tracking_completed",
                             {{DDATA, (doPounce ? "1" : "0")}}, "%f", actionTrackingTime_sec);

                if(doPounce) {
                  TransitionToPounce();
                } else {
                  TransitionToRotateToWatchingNewArea();
                }
              });
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorTrackLaser::TransitionToPounce()
{
  SET_STATE(Pouncing);

  IActionRunner* pounceAction = new TriggerLiftSafeAnimationAction(AnimationTrigger::DEPRECATED_LaserPounce);

  DelegateIfInControl(pounceAction, [this]() {
    BehaviorObjectiveAchieved(BehaviorObjective::LaserPounced);
    TransitionToBringingHeadDown();
  });
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorTrackLaser::TransitionToGetOutBored()
{
  SET_STATE(GetOutBored);

  CancelDelegates(false);

  auto callback = [this](){ SET_STATE(WaitForStop); };

  // Call callback immediately if not playing the get out anim
  if(_iConfig.skipGetOutAnim)
  {
    callback();
  }
  else
  {
    DelegateIfInControl(new TriggerLiftSafeAnimationAction(AnimationTrigger::DEPRECATED_LaserGetOut), callback);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorTrackLaser::AlwaysHandleInScope(const EngineToGameEvent& event)
{
  const EngineToGameTag& tag = event.GetData().GetTag();
  switch( tag )
  {
    case MessageEngineToGameTag::RobotObservedLaserPoint:
    {
      // Note this helper does slightly different things depending on running/not running
      SetLastLaserObservation(event);
      break;
    }

    case MessageEngineToGameTag::RobotProcessedImage:
    {
      // If we see the image mean drop significantly while waiting for the exposure change,
      // assume the change has happened so we can more immediately confirm a possible laser
      // and start tracking.
      const bool exposureChangeAlreadyObserved = (_dVars.exposureChangedTime_ms > 0);
      if(State::WaitingForExposureChange == _dVars.state && !exposureChangeAlreadyObserved)
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

            if(_dVars.imageMean < 0)
            {
              _dVars.imageMean = Util::numeric_cast<s16>(crntImgMean);
            }
            else if(crntImgMean < Util::numeric_cast<u8>((f32)_dVars.imageMean * _iConfig.imageMeanFractionForExposureChange))
            {
              PRINT_CH_DEBUG(kLogChannelName, "BehaviorTrackLaser.AlwaysHandle.ObservedExposureChange",
                             "Image mean dropped from %d to %d. Assuming exposure changed at t=%dms.",
                             _dVars.imageMean, crntImgMean, robotProcessedImage.timestamp);

              _dVars.exposureChangedTime_ms = robotProcessedImage.timestamp;

              // RobotProcessedImage messages come _after_ RobotObservedLaserPoint messages from that image.
              // Check to see if we saw a (possible) laser in this image and if so, mark it as confirmed, now
              // that we know the exposure had already dropped.
              if(_dVars.lastLaserObservation.timestamp_ms == _dVars.exposureChangedTime_ms)
              {
                _dVars.lastLaserObservation.type = LaserObservation::Type::Confirmed;
                _dVars.haveEverConfirmedLaser |= true;
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
void BehaviorTrackLaser::SetLastLaserObservation(const EngineToGameEvent& event)
{
  const auto & laserObserved = event.GetData().Get_RobotObservedLaserPoint();
  const bool inGroundPlane   = (laserObserved.ground_area_fraction > 0.f);
  const bool closeEnough     = (IsActivated() || laserObserved.ground_x_mm < _iConfig.maxDistToGetAttention_mm);

  if ( inGroundPlane && closeEnough )
  {
    // This observation is "confirmed" if exposure has been adjusted to dark
    const bool isConfirmed = (_dVars.exposureChangedTime_ms > 0);

    PRINT_CH_DEBUG(kLogChannelName, "BehaviorTrackLaser.SetLastLaserObservation.SawLaser",
                   "%s laser at (%d,%d), t=%d",
                   isConfirmed ? "Confirmed" : "Saw possible",
                   laserObserved.ground_x_mm, laserObserved.ground_y_mm, laserObserved.timestamp);

    _dVars.lastLaserObservation.type = (isConfirmed ? LaserObservation::Type::Confirmed : LaserObservation::Type::Unconfirmed);
    _dVars.lastLaserObservation.timestamp_prev_ms = _dVars.lastLaserObservation.timestamp_ms; // keep track of previous
    _dVars.lastLaserObservation.timestamp_ms = laserObserved.timestamp;
    _dVars.lastLaserObservation.pointWrtRobot.x() = laserObserved.ground_x_mm;
    _dVars.lastLaserObservation.pointWrtRobot.y() = laserObserved.ground_y_mm;
    _dVars.haveEverConfirmedLaser |= isConfirmed; // Once we've ever confirmed, stay true
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorTrackLaser::Cleanup()
{
  SET_STATE(Complete);

  _dVars.haveEverConfirmedLaser = false;
  _dVars.exposureChangedTime_ms = 0;
  _dVars.lastLaserObservation.type = LaserObservation::Type::None;

  // Leave the exposure/color settings as they were when we started
  GetBEI().GetVisionComponent().PopCurrentModeSchedule();
  GetBEI().GetVisionComponent().SetAndDisableCameraControl(_dVars.originalCameraSettings);
  GetBEI().GetVisionComponent().EnableAutoExposure(true);

  // Only pop animations if set within this behavior
  if(_dVars.haveAdjustedAnimations)
  {
    GetBEI().GetRobotInfo().GetDrivingAnimationHandler().RemoveDrivingAnimations(GetDebugLabel());
    _dVars.haveAdjustedAnimations = false;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorTrackLaser::SetState_internal(State state, const std::string& stateName)
{
  // don't log staying in same state (we do that a lot in this behavior with WaitForLaser)
  if(_dVars.state != state)
  {
    _dVars.state = state;
    SetDebugStateName(stateName);
  }
}

} // namespace Vector
} // namespace Anki
