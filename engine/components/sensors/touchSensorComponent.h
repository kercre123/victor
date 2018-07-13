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

#include "util/signals/simpleSignal_fwd.h"
#include "util/math/math.h"

#include <deque>

namespace Anki {
namespace Cozmo {

class IBehaviorPlaypen;
struct FilterParameters;

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
  
  // struct to organize the filter-specific parameters
  // used in the dynamic thresholding algorithm for
  // touch detection.
  // Note: the default values are determined using the
  // test harness `TouchSensorTuningPVT` in file
  // testTouchSensor.cpp
  struct FilterParameters
  {
    int boxFilterSize = 55;
    
    // constant size difference between detect and undetect levels
    int dynamicThreshGap = 1;
    
    // the offset from the input reading that detect level is set to
    int dynamicThreshDetectFollowOffset = 2;
    
    // the offset from the input reading that the undetect level is set to
    int dynamicThreshUndetectFollowOffset = 2;
    
    // the lowest reachable undetect level offset from baseline
    int dynamicThreshMaximumDetectOffset = 20;
    
    // the highest reachable detect level offset from baseline
    int dynamicThreshMininumUndetectOffset = 8;
    
    // minimum number of consecutive readings that are below/above detect level
    // before we update the thresholds dynamically
    // note: this counters the impact of noise on the dynamic thresholds
    int minConsecCountToUpdateThresholds = 6;
  };
  
public:
  TouchSensorComponent();

  virtual ~TouchSensorComponent();

  //////
  // IDependencyManagedComponent functions
  //////
  virtual void GetInitDependencies(RobotCompIDSet& dependencies) const override {};
  virtual void InitDependent(Robot* robot, const RobotCompMap& dependentComps) override;
  //////
  // end IDependencyManagedComponent functions
  //////
  
  bool GetIsPressed() const;
  
  float GetTouchPressTime() const;
  
  // dev-only feature for logging values around the last received touch
  void DevLogTouchSensorData() const;
  
  // helper method to set the filter parameter values external
  // note: this is to be used in the test harness to perform a
  //    parameter sweep to find optimal setting
  void UnitTestOnly_SetFilterParameters(FilterParameters params)
  {
    _filterParams = params;
    
    _boxFilterTouch = BoxFilter(params.boxFilterSize);
    
    _detectOffsetFromBase = params.dynamicThreshMininumUndetectOffset +
                            params.dynamicThreshGap;
    
    _undetectOffsetFromBase = params.dynamicThreshMininumUndetectOffset;
  }
  
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
  
  std::vector<Signal::SmartHandle> _signalHandles;
  
  FilterParameters _filterParams;
  
  static const FilterParameters _kFilterParamsForDVT;
  static const FilterParameters _kFilterParamsForProd;
  
  // backwards-compatibility flag that uses a different scheme for touch sensor filtering
  // based on the version of hardware the robot has
  bool _useFilterLogicForDVT;

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
  
  // dev-only logging members for debugging touch
  struct DevLogTouchRow
  {
    int rawTouch;
    int boxTouch;
    int calibBaseline;
    
    int detOffsetFromBase;
    int undOffsetFromBase;
    
    bool isPressed;
  };
  
  enum DevLogMode
  {
    PreTouch,
    PostTouch
  };
  
  // phase of logging with respect to the touch event that we
  // are interested in logging around
  DevLogMode _devLogMode;

  // staging buffer for holding the pre-touch and post-touch samples
  std::deque<DevLogTouchRow> _devLogBuffer;
  
  // buffer that only accumulates non-press touch samples
  std::deque<DevLogTouchRow> _devLogBufferPreTouch;

  // last complete buffer that is saved for writing to disk
  std::deque<DevLogTouchRow> _devLogBufferSaved;

  // number of consecutive touch samples taken after a release->press event
  size_t _devLogSampleCount;
};


} // Cozmo namespace
} // Anki namespace

#endif // __Engine_Components_TouchSensorComponent_H__
