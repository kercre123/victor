/**
 * File: touchSensorHelpers.h
 *
 * Author: Arjun Menon
 * Created: 10/02/2017
 *
 * Description: Helper classes to manage Press debouncing, Baseline calibration,
 * and Gesture classification
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#ifndef __Engine_Components_TouchSensorHelpers_H__
#define __Engine_Components_TouchSensorHelpers_H__

#include "util/container/circularBuffer.h"
#include "clad/types/touchGestureTypes.h"

namespace Anki {
namespace Cozmo {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
// lightweight helper class to debounce switch values
// http://www.eng.utah.edu/~cs5780/debouncing.pdf
class DebounceHelper
{
public:
  DebounceHelper(int loLimit, int hiLimit);
  
  bool ProcessRawPress(bool rawPress);
  bool GetDebouncedPress() const
  {
    return _debouncedPress;
  }
  
private:
  int _loLimit;
  int _hiLimit;
  
  int _counter;
  
  bool _debouncedPress;
};
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 

// helper class to classify the stream of touch sensor signal over time
class TouchGestureClassifier
{
public:
  
  TouchGestureClassifier(float minHoldTime, 
                         float recentTouchTimeout, 
                         float slowStrokeTimeThreshold,
                         int maxBufferStrokeCount,
                         int minNumSlowStroke);
  
  void AddTouchPressed();
  
  void AddTouchReleased();
  
  TouchGesture CalcTouchGesture();
  
protected:
  
  TouchGesture CheckTransitionToStroking() const;
  
  TouchGesture CheckTransitionToNoTouch(const float now) const;
  
  TouchGesture CheckTransitionToSustain(const float now) const;
  
private:
  
  // minimum duration of time that the robot must be
  // physically contacted for it to be considered "held"
  const float _kMinTimeForHeld_s;
  
  // maximum time to wait for next press before timing out
  // and returning the "NoTouch" idle state
  const float _kMaxTimeForRecentTouch_s;
  
  // minimum duration of time between touch press and release
  // for it to be considered a "slow" stroke
  const float _kMinTouchDurationForSlowStroke_s;

  // min num strokes to classify past buffered strokes as "slow"
  const int _kMinNumSlowStroke;
  
  // the last N touch durations
  // used to classify stroking
  Util::CircularBuffer<float> _touchDurationBuffer;
  
  // timestamps of the last touch events
  float _touchPressTime_s;
  float _touchReleaseTime_s;
  
  // XXX: add an intermediate state for "primed" but not yet "idle"
  TouchGesture _gestureState; 
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 

class TouchBaselineCalibrator
{
public:
  TouchBaselineCalibrator(int numUpdateForFastCalib,
                          int windowSize,
                          float filterCoeffSlow,
                          float filterCoeffFast,
                          float restartMaxStdevFactor,
                          float maxAllowableBufferStdev,
                          float maxAllowStdevFactorForLowMean);

  // given a new touch sensor reading, compute the new baseline parameters
  // i.e. the mean and standard deviation of the "no contact" signal
  void UpdateCalibration(float value);

  bool IsCalibrated() const
  {
    return _isCalibrated;
  }

  float GetFilteredTouchMean() const
  {
    return _filteredTouchMean; 
  }

  float GetFilteredTouchStdev() const 
  { 
    return _filteredTouchStdev; 
  }

private:

  // switchover point from fast->slow timescale
  const int _kNumUpdatesForFastCalib;
  
  const float _kRestartMaxStdevFactor;

  const float _kMaxAllowableBufferStdev;

  // used to compute the lower bound on acceptable newMean
  // according to the formula:
  // threshold = FiltMean - Factor*FiltStdev
  const float _kMaxAllowStdevFactorForLowMean;

  // coefficient for lowpass filter
  // 0 = filtered value is static unchanging input
  // 1 = filtered value is always latest input
  const float _kFilterCoeffSlow;
  const float _kFilterCoeffFast;

  // output values for the baseline computation
  float _filteredTouchMean;
  float _filteredTouchStdev;

  // counter for readings until we are considered calibrated
  size_t _numConsecutiveReadings;

  bool _isCalibrated;

  // used to calculate windowed mean/stdev
  Util::CircularBuffer<int> _bufferedTouchIntensity;
};

} // end namespace
} // end namespace

#endif // __Engine_Components_TouchSensorHelpers_H__
