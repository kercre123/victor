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
namespace Vector {

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
  
  float ApplyFilter(int input)
  {
    if(_winSize == 1) {
      return input;
    }
    
    _buffer.push_back(input);
    _sum += _buffer.back();
    if(_buffer.size()==(_winSize+1)) {
      _sum -= _buffer.front();
      _buffer.pop_front();
    }
    return float(_sum)/float(_buffer.size());
  }
  
private:
  std::deque<int> _buffer;
  int _winSize;
  int _sum;
};


// helper class with logic to contain touch calibration logic
class TouchBaselineCalibrator
{
public:
  TouchBaselineCalibrator(int maxAllowedOffsetFromBaseline)
  : _baseline(-1.0f)
  , _isCalibrated(false)
  , _numCalibrationReadings(0)
  , _numConsecNoTouch(0)
  , _maxAllowedOffsetFromBaseline(maxAllowedOffsetFromBaseline)
  {
  }
  
  bool IsCalibrated() const
  {
    return _isCalibrated;
  }
  
  void ResetCalibration()
  {
    _baseline = -1.0f;
    _isCalibrated = false;
    _numConsecNoTouch = 0;
    _numCalibrationReadings = 0;
    _numStableBaselineReadings = 0;
  }
  
  void UpdateBaseline(float reading, bool isPickedUp, bool isPressed);
  
  float GetBaseline() const
  {
    return _baseline;
  }
  
protected:
  float _baseline;
  
  // if not calibrated: fast calibration mode
  // if     calibrated: slow calibration mode
  bool _isCalibrated;
  
  // when enough raw readings are accumulated within
  // the baseline, then we can transition to calibrated
  size_t _numStableBaselineReadings;
  
  // number of readings collected so far (while not calibrated)
  size_t _numCalibrationReadings;
  
  // number of readings after the last detected touch
  size_t _numConsecNoTouch;
  
  // highest allowable offset from baseline that can be
  // accummulated into the filter per tick. This value
  // is used to prevent soft touches from raising the baseline
  int _maxAllowedOffsetFromBaseline;
};

class TouchSensorComponent : public ISensorComponent, public IDependencyManagedComponent<RobotComponentID>
{
public:
  
  // struct to organize the filter-specific parameters
  // used in the dynamic thresholding algorithm for
  // touch detection
  struct FilterParameters
  {
    // number of values to average over
    int boxFilterSize;
    
    // constant size difference between detect and undetect levels
    int dynamicThreshGap;
    
    // the offset from the input reading that detect level is set to
    int dynamicThreshDetectFollowOffset;
    
    // the offset from the input reading that the undetect level is set to
    int dynamicThreshUndetectFollowOffset;
    
    // the lowest reachable undetect level offset from baseline
    int dynamicThreshMaximumDetectOffset;
    
    // the highest reachable detect level offset from baseline
    int dynamicThreshMininumUndetectOffset;
    
    // minimum number of consecutive readings that are below/above detect level
    // before we update the thresholds dynamically
    // note: this counters the impact of noise on the dynamic thresholds
    int minConsecCountToUpdateThresholds;
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
  
  // returns the current detected press state using the input
  // signal and passed in baseline measurement
  bool ProcessWithDynamicThreshold(const int baseline, const int input);
  
  int GetDetectLevelOffset() const;
  
  int GetUndetectLevelOffset() const;

public:

  u16 GetLatestRawTouchValue() const
  {
    return _lastRawTouchValue;
  }
  
  bool IsCalibrated() const
  {
    return _baselineCalibrator.IsCalibrated();
  }
  
private:
  // Let Playpen behaviors have access to Start/Stop recording touch sensor data
  // TODO(Al): Could probably move the recording logic to IBehaviorPlaypen by handling
  // state messages
  friend class IBehaviorPlaypen;
  void StartRecordingData(TouchSensorValues* data);
  void StopRecordingData() { _dataToRecord = nullptr; }
  
  std::vector<Signal::SmartHandle> _signalHandles;
  
  FilterParameters _filterParams;
  
  static const FilterParameters _kFilterParamsForProd;
  
  // flag to guard against using touch sensor logic designed for
  // PVT hardware on DVT robots. This prevents the robot undefined
  // behavior as a result of syscon-level changes to touch sensor
  bool _enabled = true;

  // Pointer to a struct that should be populated with touch sensor data when recording
  TouchSensorValues* _dataToRecord = nullptr;

  u16 _lastRawTouchValue;
  
  // module to help with maintaining the detected baseline
  TouchBaselineCalibrator _baselineCalibrator;
  
  BoxFilter _boxFilterTouch;
  
  // current offset of the detect-level relative to baseline
  int _detectOffsetFromBase;
  
  // current offset of the undetect-level relative to baseline
  int _undetectOffsetFromBase;
  
  bool _isPressed;
  
  // counters to debounce the monotonic increase/decrease
  // of the detect and undetect thresholds
  // otherwise noise would constantly be shifting the thresholds
  int _countAboveDetectLevel;
  int _countBelowUndetectLevel;
  
  // time in seconds of the last touch press
  float _touchPressTime;
  
  // counter for press state: number of consecutive
  // detections for either pressed or released state
  unsigned int _counterPressState;
  
  // whether or not the touch press state is confirmed:
  // true, if pressed confirmed
  // false, if released confirmed
  bool _confirmedPressState;
  
  // dev-only logging members for debugging touch
  struct DevLogTouchRow
  {
    int rawTouch;
    float boxTouch;
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
