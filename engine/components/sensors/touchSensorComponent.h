/**
 * File: touchSensorComponent.h
 *
 * Author: Arjun Menon
 * Created: 9/7/2017
 *
 * Description: Component for managing the capacitative touch sensor
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#ifndef __Engine_Components_TouchSensorComponent_H__
#define __Engine_Components_TouchSensorComponent_H__

#include "engine/components/sensors/iSensorComponent.h"
#include "engine/components/sensors/touchSensorHelpers.h"
#include "clad/types/touchGestureTypes.h"
#include "clad/types/factoryTestTypes.h"

namespace Anki {
namespace Cozmo {

class IBehaviorPlaypen;

class TouchSensorComponent : public ISensorComponent
{
public:
  TouchSensorComponent(Robot& robot);
  ~TouchSensorComponent() = default;
  
protected:
  virtual void UpdateInternal(const RobotState& msg) override;
  
  virtual std::string GetLogHeader() override;
  virtual std::string GetLogRow() override;

public:
  
  TouchGesture GetLatestTouchGesture() const {
    return _touchGesture;
  }

  u32 GetLatestRawTouchValue() const { return _lastRawTouchValue; }
  
  bool IsCalibrated() const {
    return _baselineCalib.IsCalibrated();
  }
  
private:

  // Let Playpen behaviors have access to Start/Stop recording touch sensor data
  // TODO(Al): Could probably move the recording logic to IBehaviorPlaypen by handling
  // state messages
  friend class IBehaviorPlaypen;
  void StartRecordingData(TouchSensorValues* data);
  void StopRecordingData() { _dataToRecord = nullptr; } 

  // Pointer to a struct that should be populated with touch sensor data when recording
  TouchSensorValues* _dataToRecord = nullptr;

  DebounceHelper _debouncer;

  TouchGestureClassifier _gestureClassifier;

  TouchBaselineCalibrator _baselineCalib;

  float _touchDetectStdevFactor;

  // the latest computed result of touch gesture
  TouchGesture _touchGesture;

  u16 _lastRawTouchValue = 0;

  // number of consecutive cycles seeing "no contact" reading
  size_t _noContactCounter;
};


} // Cozmo namespace
} // Anki namespace

#endif // __Engine_Components_TouchSensorComponent_H__
