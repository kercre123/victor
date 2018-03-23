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
#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior_fwd.h"
#include "util/entityComponent/entity.h"
#include "clad/types/factoryTestTypes.h"

#include "util/math/math.h"

#include <deque>

namespace Anki {
namespace Cozmo {

class IBehaviorPlaypen;
  

class BoxFilter
{
public:
  BoxFilter(int windowSize)
  : _buffer()
  , _winSize(windowSize)
  , _sum(0)
  {
  }
  
  int ApplyFilter(int input)
  {
    _buffer.push_back(input);
    _sum += _buffer.back();
    if(_buffer.size()==(_winSize+1)) {
      _sum -= _buffer.front();
      _buffer.pop_front();
    }
    return int( float(_sum)/_buffer.size() );
  }
  
private:
  std::deque<int> _buffer;
  int _winSize;
  int _sum;
};

class TouchSensorComponent : public ISensorComponent, public IDependencyManagedComponent<RobotComponentID>
{
public:

public:
  TouchSensorComponent();
  ~TouchSensorComponent() = default;

  //////
  // IDependencyManagedComponent functions
  //////
  virtual void GetInitDependencies(RobotCompIDSet& dependencies) const override {};
  virtual void InitDependent(Robot* robot, const RobotCompMap& dependentComponents) override {
    InitBase(robot);
  };
  //////
  // end IDependencyManagedComponent functions
  //////
  
  bool GetIsPressed() const;
  
  float GetTouchPressTime() const;
  
protected:
  virtual void NotifyOfRobotStateInternal(const RobotState& msg) override;
  
  virtual std::string GetLogHeader() override;
  virtual std::string GetLogRow() override;
  
  // returns true if there is a state change between pressed/released
  // additionally updates the state of press/release internally which
  // can be queried with GetIsPressed()
  bool ProcessWithDynamicThreshold(const int baseline, const int input);
  
  int GetDetectLevelOffset() const;
  
  int GetUndetectLevelOffset() const;

public:

  u16 GetLatestRawTouchValue() const
  {
    return _lastRawTouchValue;
  }
  
  bool IsCalibrated() const;
  
private:

  // Let Playpen behaviors have access to Start/Stop recording touch sensor data
  // TODO(Al): Could probably move the recording logic to IBehaviorPlaypen by handling
  // state messages
  friend class IBehaviorPlaypen;
  void StartRecordingData(TouchSensorValues* data);
  void StopRecordingData() { _dataToRecord = nullptr; } 

  // Pointer to a struct that should be populated with touch sensor data when recording
  TouchSensorValues* _dataToRecord = nullptr;

  u16 _lastRawTouchValue;

  // number of consecutive cycles seeing "no contact" reading
  size_t _noContactCounter;
  
  // calibration value from IIR filtering "no contact" signals
  float _baselineTouch;
  
  BoxFilter _boxFilterTouch;
  
  // current offset of the detect-level relative to baseline
  int _detectOffsetFromBase;
  
  // current offset of the undetect-level relative to baseline
  int _undetectOffsetFromBase;
  
  bool _isPressed;
  
  size_t _numConsecCalibReadings;
  
  // counters to debounce the monotonic increase/decrease
  // of the detect and undetect thresholds
  // otherwise noise would constantly be shifting the thresholds
  int _countAboveDetectLevel;
  int _countBelowUndetectLevel;
  
  // time in seconds of the last touch press
  float _touchPressTime;
};


} // Cozmo namespace
} // Anki namespace

#endif // __Engine_Components_TouchSensorComponent_H__
