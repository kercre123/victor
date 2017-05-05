/**
 * File: behaviorTrackLaser.h
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

#ifndef __Cozmo_Basestation_Behaviors_BehaviorTrackLaser_H__
#define __Cozmo_Basestation_Behaviors_BehaviorTrackLaser_H__

#include "anki/cozmo/basestation/behaviors/iBehavior.h"

namespace Anki {
namespace Cozmo {

class BehaviorTrackLaser : public IBehavior
{
private:
  
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  BehaviorTrackLaser(Robot& robot, const Json::Value& config);
  
public:
  
  virtual bool IsRunnableInternal(const BehaviorPreReqRobot& preReqData) const override;
  virtual bool CarryingObjectHandledInternally() const override {return false;}

protected:
  
  virtual void AlwaysHandle(const EngineToGameEvent& event, const Robot& robot) override;

  virtual Result InitInternal(Robot& robot) override;
  virtual Status UpdateInternal(Robot& robot) override;
  virtual Result ResumeInternal(Robot& robot) override;
  virtual void   StopInternal(Robot& robot) override;

private:

  enum class State {
    Inactive,
    InitialSearch,
    BringingHeadDown,
    RotateToWatchingNewArea,
    WaitingForLaser,
    TrackLaser,
    Pouncing,
    GetOutBored,
    WaitForStop,
    Complete,
  };
  
  // used for tracking total turn angle
  Radians _cumulativeTurn_rad;
  
  // Set from Json config (use keys named exactly the same as the struct members).
  // Rough "reasonable" values are provided in comments to give you an idea of where to start.
  // NOTE: All are stored as floats to make it easy to set them from Json via one simple macro
  struct {
    
    float    startIfLaserSeenWithin_sec; // E.g. 1.0sec
    
    // Must see possible laser within this distance to start to try to confirm.
    // This distance threshold does not apply while already tracking a confirmed laser.
    float    maxDistToGetAttention_mm; // E.g. = 80mm
    
    // Once we see a possible laser (w/ camera at normal exposure), we darken the exposure
    // to confirm the laser. We wait a small amount of time to let the exposure settings
    // take effect.
    float    darkenedExposure_ms; // E.g. 1ms
    float    darkenedGain; // E.g. 0.1
    float    timeToWaitForExposureChange_ms; // E.g. 100ms
    
    // After changing exposure, we'll wait this long to confirm the laser
    float    maxTimeToConfirm_ms; // E.g. 65ms
    
    float    searchAmplitude_deg; // E.g. 90deg
    
    // Various timeouts
    float    maxTimeSinceNoLaser_running_sec; // Eg.g 3.5sec
    float    maxTimeSinceNoLaser_notRunning_sec;  // E.g. 1.5sec
    float    maxTimeBehaviorTimeout_sec;  // E.g. 30sec
    float    maxTimeBeforeRotate_sec;  // E.g. 4sec
    float    trackingTimeout_sec;  // E.g. 1.5fsec
    
    // Pounce settings
    // Cozmo pounces after maintaining the laser point within the given distance and angle tolerances
    // for the given amoung of time. After pouncing, he backs up a little.
    float    pounceAfterTrackingFor_sec;  // E.g. 1sec
    float    pounceIfWithinDist_mm;  // E.g. 50mm
    float    pouncePanTol_deg;  // E.g. 10deg
    float    pounceTiltTol_deg;  // E.g. 15deg
    float    backupDistAfterPounce_mm;  // E.g. -15mm
    float    backupDurationAfterPounce_sec;  // E.g. 0.25sec

    // For randomly searching for the laser if forcibly started (i.e. sparked)
    float    randomInitialSearchPanMin_deg;  // E.g. 20deg
    float    randomInitialSearchPanMax_deg;  // E.g. 45deg
    
    // Control how fast the robot rotates when point turning towards laser by
    // adjusting the time spent doing so. Chosen randomly each time a tracking
    // action is created, between min and max.
    float    minPanDuration_sec;  // E.g. 0.2sec
    float    maxPanDuration_sec;  // E.g. 0.4sec
    
    // Control how fast the robot will drive to reach the laser, but adjusting the time to
    // to drive the distance to it. Chosen randomly  each time
    float    minTimeToReachLaser_sec;  // E.g. 0.6sec
    float    maxTimeToReachLaser_sec;  // E.g. 0.8sec
    
    // For how long after losing the laser the robot will try to predict where it went and turn there
    // Set to 0 to disable
    float    predictionDuration_sec;  // E.g. 1sec
    
    // If we track the laser for this long, we achieve the LaserTracked objective
    float    trackingTimeToAchieveObjective_sec;  // E.g. 5sec
    
  } _params;
  
  struct LaserObservation {
    enum Type {
      None,         // Have not observed anything
      Unconfirmed,  // Seen while not running (and auto exposure on)
      Confirmed     // Seen while running (with reduced exposure)
    };
    
    Type    type;
    float   time_sec;
    Point2f pointWrtRobot;
  };
  
  LaserObservation _lastLaserObservation;
  bool  _haveConfirmedLaser = false;
  TimeStamp_t _changedExposureTime_ms = 0;
  
  float _lastTimeRotate = 0.f;
  float _startedTracking_sec = 0.f;
  
  State _state = State::Inactive;
  
  // So that we can restore when done
  struct {
    bool colorEnabled;
    s32  exposureTime_ms;
    f32  gain;
  } _originalCameraSettings;
  
  // reset everything for when the behavior is finished
  void Cleanup(Robot& robot);
  
  void SetState_internal(State state, const std::string& stateName);
  void TransitionToInitialSearch(Robot& robot);
  void TransitionToBringingHeadDown(Robot& robot);
  void TransitionToRotateToWatchingNewArea(Robot& robot);
  void TransitionToWaitForLaser(Robot& robot);
  void TransitionToTrackLaser(Robot& robot);
  void TransitionToPounce(Robot& robot);
  void TransitionToGetOutBored(Robot& robot);
  
  void InitHelper(Robot& robot);
  
  void SetParamsFromConfig(const Json::Value& config);
  
  void SetLastLaserObservation(const Robot& robot, const EngineToGameEvent& event);
  
};

} // namespace Cozmo
} // namespace Anki

#endif /* __Cozmo_Basestation_Behaviors_BehaviorTrackLaser_H__ */
