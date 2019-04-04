/**
 * File: cliffSensorComponent.h
 *
 * Author: Matt Michini
 * Created: 6/27/2017
 *
 * Description: Component for managing cliff sensors and cliff sensor-related functions such as carpet detection (and
 *              corresponding cliff threshold adjustment), 'suspicious' cliff detection, and raw data access.
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#ifndef __Anki_Cozmo_Basestation_Components_CliffSensorComponent_H__
#define __Anki_Cozmo_Basestation_Components_CliffSensorComponent_H__

#include "engine/components/sensors/iSensorComponent.h"
#include "engine/externalInterface/externalInterface.h"

#include "clad/types/proxMessages.h"
#include "clad/types/robotStatusAndActions.h"
#include "clad/externalInterface/messageEngineToGame.h"

#include "util/bitFlags/bitFlags.h"

#include <list>

namespace Anki {
class Pose3d;
namespace Vector {

class CliffSensorComponent : public ISensorComponent, public IDependencyManagedComponent<RobotComponentID>
{
public:
  static const int kNumCliffSensors = static_cast<int>(CliffSensor::CLIFF_COUNT);
  
  using CliffSensorDataArray = std::array<uint16_t, kNumCliffSensors>;
  static_assert(std::is_same<CliffSensorDataArray, decltype(RobotState::cliffDataRaw)>::value, "CliffSensorDataArray must be same as type used in RobotState");
  
  using CliffSensorFiltDataArray = std::array<f32, kNumCliffSensors>;

  CliffSensorComponent();

  ~CliffSensorComponent() = default;

  //////
  // IDependencyManagedComponent functions
  //////
  virtual void GetInitDependencies(RobotCompIDSet& dependencies) const override {};
  virtual void InitDependent(Robot* robot, const RobotCompMap& dependentComps) override;
  //////
  // end IDependencyManagedComponent functions
  //////
  
  void HandleMessage(const ExternalInterface::RobotStopped& msg);
  
  void HandleMessage(const ExternalInterface::RobotOffTreadsStateChanged& msg);

protected:
  virtual void NotifyOfRobotStateInternal(const RobotState& msg) override;
  
  virtual std::string GetLogHeader() override;
  virtual std::string GetLogRow() override;

public:

  void SetPause(const bool b) { _isPaused = b; }
  
  bool IsCliffSensorEnabled() const { return _enableCliffSensor; }
  void SetEnableCliffSensor(const bool val) { _enableCliffSensor = val; }
  
  // Cliff detection based on RobotState
  bool IsCliffDetected() const { return _cliffDetectedFlags.AreAnyFlagsSet(); }
  bool IsCliffDetected(const CliffSensor sensor) const { return _cliffDetectedFlags.IsBitFlagSet(sensor); }
  uint8_t GetCliffDetectedFlags() const { return _cliffDetectedFlags.GetFlags(); }
  
  // White detection based on RobotState
  bool IsWhiteDetected() const { return _whiteDetectedFlags.AreAnyFlagsSet(); }
  bool IsWhiteDetected(const CliffSensor sensor) const { return _whiteDetectedFlags.IsBitFlagSet(sensor); }
  uint8_t GetWhiteDetectedFlags() const { return _whiteDetectedFlags.GetFlags(); }
  
  // Adjusts cliff threshold if necessary
  void UpdateCliffDetectThresholds();
  
  // Returns current threshold for cliff detection for the given cliff sensor
  uint16_t GetCliffDetectThreshold(unsigned int ind) const;
  
  const CliffSensorDataArray& GetCliffDetectThresholds() const { return _cliffDetectThresholds; }
  
  // Sends message to the robot process containing all of the current cliff detection thresholds
  void SendCliffDetectThresholdsToRobot();
  
  // Raw reported cliff sensor values
  const CliffSensorDataArray& GetCliffDataRaw() const { return _cliffDataRaw; }
  
  // Compute the estimated pose of the cliff obstacle for the given cliff event.
  // - timestamp represents the time at which the cliff was detected (used to calculate
  //   the robot's historical pose)
  // - cliff detected flags used to estimate the cliff's pose relative to the robot
  bool ComputeCliffPose(uint32_t timestampOfCliff, uint8_t cliffDetectedFlags, Pose3d& cliffPose) const;

  // given a set of cliff detected flags, gets the pose of the cliff relative to the robot
  // returns true if the output pose is valid
  bool GetCliffPoseRelativeToRobot(const uint8_t cliffDetectedFlags, Pose3d& relativePose) const;

  // mark a quad of fixed dimensions as cliff-type obstacles in the navmap
  void UpdateNavMapWithCliffAt(const Pose3d& pose, const uint32_t timestamp) const;

  // Enable/Disable stopping the robot when cliff sensor detects something 
  // as reflective as the white border of the habitat.
  void EnableStopOnWhite(bool stopOnWhite);

  // Returns whether or not stop-on-white is enabled
  bool IsStopOnWhiteEnabled() const { return _stopOnWhiteEnabled; }
  
  // Returns latest timestamp in seconds of when the robot was put down on a cliff.
  TimeStamp_t GetLatestPutDownOnCliffTime_ms() const { return _lastPutDownOnCliffTime_ms; }
  
  // Returns latest timestamp in seconds of a stop due to a cliff detection
  TimeStamp_t GetLatestStopDueToCliffTime_ms() const { return _lastStopDueToCliffTime_ms; }
  
  const CliffSensorDataArray& GetCliffDataRawAtLastStop() const { return _cliffDataRawAtLastStop; }
  
  const CliffSensorFiltDataArray & GetCliffDataFiltered() const { return _cliffDataFilt; }
  
  // Retrieves the number of milliseconds that the cliff sensors have reported @param numCliffs
  // being detected.
  //
  // If @param numCliffs is a positive value, this assumes that reports of N cliff detections
  // from the sensors, where N is greater than @param numCliffs, also counts as valid time to
  // include in the reported duration.
  //
  // If @param numCliffs is 0, then this assumes that the user wants only the duration of time
  // that the cliff sensors have currently been reporting _exactly_ zero cliffs detections.
  u32 GetDurationForAtLeastNCliffDetections_ms(const int minNumCliffs) const;

  // Returns the amount of time that @param numCliff or fewer cliffs have been detected for.
  // (Returns MAX_UINT for numCliffs == 4)
  u32 GetDurationForAtMostNCliffDetections_ms(const int numCliffs) const;
    
  int GetNumCliffsDetected() const { return _latestNumCliffsDetected; }

  // Returns the max number of cliffs observed while picked up (according to RobotState.status)
  // Returns 0 if not currently picked up.
  int GetMaxNumCliffsDetectedWhilePickedUp() const { return _maxNumCliffsDetectedWhilePickedUp; } 
  
private:
  
  void InitEventHandlers(IExternalInterface& interface);
  
  void QueueCliffThresholdUpdate();
  
  void SetCliffDetectThreshold(unsigned int ind, uint16_t newThresh);
  
  void UpdateLatestCliffDetectionDuration();
  
  bool _isPaused = false;
  
  bool _enableCliffSensor = true;
  Util::BitFlags8<CliffSensor> _cliffDetectedFlags;
  Util::BitFlags8<CliffSensor> _whiteDetectedFlags;
  
  int _latestNumCliffsDetected = -1;
    
  std::array<u32, kNumCliffSensors + 1> _cliffDetectionTimes_ms;

  // Stores the last time that N cliffs were detected
  // unlike _cliffDetectionTimes_ms which contains only the times
  // of cliffs that are _currently_ being detected.
  std::array<u32, kNumCliffSensors + 1> _cliffLastDetectedTimes_ms;

  int _maxNumCliffsDetectedWhilePickedUp = 0;
  
  uint32_t _latestMsgTimestamp = 0;
  
  std::list<Signal::SmartHandle> _eventHandles;
  
  // Cliff detection thresholds for each sensor
  CliffSensorDataArray _cliffDetectThresholds;
  
  // Raw cliff sensor data from robot process
  CliffSensorDataArray _cliffDataRaw;
  
  // Filtered cliff sensor data, which smooths out noise in the data
  CliffSensorFiltDataArray _cliffDataFilt;
  
  // Minimum observed cliff sensor values (used to adaptively adjust detection thresholds)
  CliffSensorDataArray _cliffMinObserved;
  
  // This is the allowed delta above the minimum-ever-seen cliff sensor value below which cliff detection is triggered.
  // In other words, if the lowest-ever-seen value for a given cliff sensor is 90, and _cliffDetectAllowedDelta is 40, then
  // the cliff detection threshold will be set at 130 (90+40). If the raw cliff value ever falls below the threshold, cliff
  // detection is triggered. This value can also be dynamically adjusted if it is suspected that we are driving on a surface
  // with poor IR reflectivity (e.g. a carpet).
  uint16_t _cliffDetectAllowedDelta;

  uint32_t _nextCliffThresholdUpdateToRobot_ms = 0;
  
  bool _stopOnWhiteEnabled = false;
  
  // Timestamp of latest event where robot stopped due to a cliff detection
  TimeStamp_t _lastStopDueToCliffTime_ms = 0;
  // Raw cliff sensor data when the last RobotStopped event occurred due to a cliff detection.
  CliffSensorDataArray _cliffDataRawAtLastStop;
  
  // Timestamp of latest event where robot was put down on a cliff
  TimeStamp_t _lastPutDownOnCliffTime_ms = 0;
};


} // Vector namespace
} // Anki namespace

#endif // __Anki_Cozmo_Basestation_Components_CliffSensorComponent_H__
