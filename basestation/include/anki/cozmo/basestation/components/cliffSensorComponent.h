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

#include "anki/common/types.h"

#include "clad/types/proxMessages.h"

#include "util/helpers/noncopyable.h"
#include "util/helpers/templateHelpers.h"

namespace Anki {
namespace Cozmo {

class Robot;
struct RobotState;
  
class CliffSensorComponent : private Util::noncopyable
{
public:

  // constructor
  CliffSensorComponent(Robot& robot);

  void UpdateRobotData(const RobotState& msg);
  
  bool IsCliffSensorEnabled() const { return _enableCliffSensor; }
  void SetEnableCliffSensor(const bool val) { _enableCliffSensor = val; }
  
  bool IsCliffDetected() const { return _isCliffDetected; }
  void SetCliffDetected(const bool isCliffDetected) { _isCliffDetected = isCliffDetected; }
  
  bool IsCliffDetectedStatusBitOn() const { return _cliffDetectedStatusBitOn; }
  
  // Returns current threshold for cliff detection
  u16 GetCliffDetectThreshold() const { return _cliffDetectThreshold; }
  
  void SendCliffDetectThresholdToRobot(const u16 thresh);
  
  // index corresponds to CliffSensor enum
  u16  GetCliffDataRaw(unsigned int ind = 0) const;
  
  // Assesses suspiciousness of current cliff and adjusts cliff threshold if necessary
  void UpdateCliffDetectThreshold();
  
  void UpdateCliffRunningStats(const RobotState& msg);
  void ClearCliffRunningStats();
  
  // Get the mean/variance of cliff readings over the past few seconds. (See kCliffSensorRunningStatsWindowSize)
  f32  GetCliffRunningMean() const { return _cliffRunningMean; }
  f32  GetCliffRunningVar()  const { return _cliffRunningVar; }
  
  // Evaluate how suspicious the cliff that caused robot to stop is based on how much the reading
  // actually rises while it's stopping in reaction to the supposed cliff.
  void EvaluateCliffSuspiciousnessWhenStopped();
  
private:
  
  // Returns true if floor is suspiciously cliff-y looking based on variance of cliff readings
  bool IsFloorSuspiciouslyCliffy() const;
  
  // Increments count of suspicious cliff. (i.e. Cliff was detected but data looks like maybe it's not real.)
  void IncrementSuspiciousCliffCount();  
  
// members:
  
  Robot& _robot;
  
  bool _enableCliffSensor = true;
  bool _isCliffDetected = false;
  
  // True if the robot is reporting CLIFF_DETECTED in its status message
  bool _cliffDetectedStatusBitOn = false;
  
  u16 _cliffDetectThreshold;
  
  std::array<uint16_t, Util::EnumToUnderlying(CliffSensor::CLIFF_COUNT)> _cliffDataRaw; // initialized in constructor
  
  u32  _suspiciousCliffCnt  = 0;
  u32  _cliffStartTimestamp = 0;
  
  // Cliff-yness tracking
  std::deque<u16> _cliffDataQueue;
  f32             _cliffRunningMean    = 0.f;
  f32             _cliffRunningVar     = 0.f;
  f32             _cliffRunningVar_acc = 0.f;
  
};


} // Cozmo namespace
} // Anki namespace

#endif // __Anki_Cozmo_Basestation_Components_CliffSensorComponent_H__
