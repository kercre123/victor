/**
 * File: touchSensorHelpers.cpp
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

#include "engine/components/sensors/touchSensorHelpers.h"
#include "anki/common/basestation/utils/timer.h"
#include "util/logging/logging.h"
#include "util/math/math.h"
#include <cmath>

namespace Anki {
namespace Cozmo {
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
// DebounceHelper:
//
// lightweight helper class to debounce switch values
// http://www.eng.utah.edu/~cs5780/debouncing.pdf
  
DebounceHelper::DebounceHelper(int loLimit, int hiLimit)
: _loLimit(loLimit)
, _hiLimit(hiLimit)
, _counter(0)
, _debouncedPress(false)
{
}
  
bool DebounceHelper::ProcessRawPress(bool rawPress)
{
  if(rawPress == _debouncedPress) {
    // if the debounced state is true (pressed), then the counter should
    // count the number of cycles it is stably released
    _counter = _debouncedPress ? _loLimit : _hiLimit;
  } else {
    --_counter;
  }
  
  if(_counter==0) {
    _debouncedPress = rawPress;
    _counter = _debouncedPress ? _loLimit : _hiLimit;
    return true;
  } else {
    return false;
  }
  
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
// TouchGestureClassifier
// helper class to classify the stream of touch sensor signal over time

TouchGestureClassifier::TouchGestureClassifier(float minHoldTime, 
                                               float recentTouchTimeout, 
                                               float slowStrokeTimeThreshold,
                                               int maxBufferStrokeCount,
                                               int minNumSlowStroke)
: _kMinTimeForHeld_s(minHoldTime)
, _kMaxTimeForRecentTouch_s(recentTouchTimeout)
, _kMinTouchDurationForSlowStroke_s(slowStrokeTimeThreshold)
, _kMinNumSlowStroke(minNumSlowStroke)
, _touchDurationBuffer(maxBufferStrokeCount)
, _touchPressTime_s(0.0)
, _touchReleaseTime_s(0.0)
, _gestureState(TouchGesture::NoTouch)
{
}

void TouchGestureClassifier::AddTouchPressed()
{
  _touchPressTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
}

void TouchGestureClassifier::AddTouchReleased()
{
  _touchReleaseTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  
  // prune the touch duration buffer to remove ContactSustain level touches
  const bool orderingCorrect = _touchReleaseTime_s > _touchPressTime_s;
  const bool isSustainPress  = (_touchReleaseTime_s - _touchPressTime_s) > _kMinTimeForHeld_s;
  if( orderingCorrect && !isSustainPress ) {
    _touchDurationBuffer.push_back( _touchReleaseTime_s - _touchPressTime_s );
  }
}

TouchGesture TouchGestureClassifier::CalcTouchGesture()
{
  const auto now = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  
  const bool isInPressPhase = _touchPressTime_s > _touchReleaseTime_s;
  
  // transition to next gesture
  TouchGesture nextGestureState = TouchGesture::Count;
  switch(_gestureState) {
    case TouchGesture::NoTouch:
    {
      if(isInPressPhase) {
        nextGestureState = TouchGesture::ContactInitial;
      }
      break;
    }
    case TouchGesture::ContactInitial: // deliberate fallthrough
    case TouchGesture::ContactSustain: // deliberate fallthrough
    case TouchGesture::SlowRepeating:  // deliberate fallthrough
    case TouchGesture::FastRepeating:
    {
      if(isInPressPhase) {
        nextGestureState = CheckTransitionToSustain(now);
      } else {
        nextGestureState = CheckTransitionToNoTouch(now);
        if(nextGestureState == TouchGesture::Count) {
          nextGestureState = CheckTransitionToStroking();
        }
      }
      break;
    }
    case TouchGesture::Count:
    {
      DEV_ASSERT(false, "TouchSensorComponent.InvalidTouchGestureState");
      break;
    }
  }
  
  // no-op change to state
  if(nextGestureState == TouchGesture::Count) {
    nextGestureState = _gestureState;
  }
  
  // handle on-exit and on-enter actions
  
  const bool wasInStroking = (_gestureState==TouchGesture::SlowRepeating ||
                              _gestureState==TouchGesture::FastRepeating);
  const bool willNotBeStroking = !(nextGestureState==TouchGesture::SlowRepeating ||
                                  nextGestureState==TouchGesture::FastRepeating);
  const bool isExitingStroking = wasInStroking && willNotBeStroking;
  if(isExitingStroking) {
    _touchDurationBuffer.clear(); // prevents stale touch data when leaving StrokingX
  }
  
  // apply station transition
  _gestureState = nextGestureState;
  
  return _gestureState;
}

TouchGesture TouchGestureClassifier::CheckTransitionToStroking() const
{
  if( _touchDurationBuffer.size() == _touchDurationBuffer.capacity() ) {
    int numSlowStrokes = 0;
    for(int i=0; i<_touchDurationBuffer.size(); ++i) {
      const bool isSlowTouch = (_touchDurationBuffer[i] > 
                                _kMinTouchDurationForSlowStroke_s);
      if(isSlowTouch) {
        numSlowStrokes++;
      }
    }
    const bool isSlowRepeating = numSlowStrokes >= _kMinNumSlowStroke;
    return isSlowRepeating ? 
            TouchGesture::SlowRepeating : 
            TouchGesture::FastRepeating;
  }
  
  return TouchGesture::Count; // default do not transition
}

TouchGesture TouchGestureClassifier::CheckTransitionToNoTouch(const float now) const
{
  return ((now-_touchReleaseTime_s) > _kMaxTimeForRecentTouch_s) ?
          TouchGesture::NoTouch :
          TouchGesture::Count; // default do not transition
}

TouchGesture TouchGestureClassifier::CheckTransitionToSustain(const float now) const
{
  const bool isInPressPhase = _touchPressTime_s > _touchReleaseTime_s;
  const bool isPressedLong  = (now-_touchPressTime_s) > _kMinTimeForHeld_s;
  return (isInPressPhase && isPressedLong) ? 
          TouchGesture::ContactSustain : 
          TouchGesture::Count;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
// TouchBaselineCalibrator:

TouchBaselineCalibrator::TouchBaselineCalibrator(int numUpdateForFastCalib,
                                                 int windowSize,
                                                 float filterCoeffSlow,
                                                 float filterCoeffFast,
                                                 float restartMaxStdevFactor,
                                                 float maxAllowableBufferStdev,
                                                 float maxAllowStdevFactorForLowMean)
: _kNumUpdatesForFastCalib(numUpdateForFastCalib)
, _kRestartMaxStdevFactor(restartMaxStdevFactor)
, _kMaxAllowableBufferStdev(maxAllowableBufferStdev)
, _kMaxAllowStdevFactorForLowMean(maxAllowStdevFactorForLowMean)
, _kFilterCoeffSlow(filterCoeffSlow)
, _kFilterCoeffFast(filterCoeffFast)
, _filteredTouchMean(0)
, _filteredTouchStdev(0)
, _numConsecutiveReadings(0)
, _isCalibrated(false)
, _bufferedTouchIntensity(windowSize)
{
}

// given a new touch sensor reading, compute the new baseline parameters
// i.e. the mean and standard deviation of the "no contact" signal
void TouchBaselineCalibrator::UpdateCalibration(float value)
{
  _bufferedTouchIntensity.push_back(value);

  const bool bufferIsFull = (_bufferedTouchIntensity.size() == 
                              _bufferedTouchIntensity.capacity());
  if(bufferIsFull) {
    float newMean  = 0.0f;
    float newStdev = 0.0f;
    float minTouch = std::numeric_limits<float>::max();
    float maxTouch = std::numeric_limits<float>::lowest();

    // calculate mean, stdev, min, max on the buffer, and the low mean counter
    { 
      for(int i=0; i<_bufferedTouchIntensity.size();++i) {
        newMean += _bufferedTouchIntensity[i];
        if(minTouch > _bufferedTouchIntensity[i]) {
          minTouch = _bufferedTouchIntensity[i];
        }
        if(maxTouch < _bufferedTouchIntensity[i]) {
          maxTouch = _bufferedTouchIntensity[i];
        }
      }
      DEV_ASSERT(_bufferedTouchIntensity.size()!=0, 
          "TouchBaselineCalibrator.CalcMeanStdev.EmptyBufferTouchIntensity");
      newMean /= _bufferedTouchIntensity.size();
      
      DEV_ASSERT(_bufferedTouchIntensity.size()>=2, 
              "TouchBaselineCalibrator.CannotCaluclateStdevOnBufferOfSizeOne");

      for(int i=0; i<_bufferedTouchIntensity.size();++i) {
        const float tmp = _bufferedTouchIntensity[i]-newMean;
        newStdev += tmp*tmp;
      }

      newStdev /= _bufferedTouchIntensity.size()-1;
      newStdev = sqrt(newStdev);
    }

    // if it is just initialized then the filtered value is the first input
    if( _filteredTouchMean==0 && _filteredTouchStdev==0 ) {
      _filteredTouchMean = newMean;
      _filteredTouchStdev = newStdev;
      _bufferedTouchIntensity.clear();
      return;
    }

    // restart the calibration if:
    // 1. in FastCalib and the buffer properties are not within expectations
    // 2. We see a new mean that is far below the filtered mean
    //    (it implies that we calibrated while being held)

    // check the buffer properties are valid
    const float sigmaFactor = _kRestartMaxStdevFactor*GetFilteredTouchStdev();
    const bool bufferGood = (newStdev <= _kMaxAllowableBufferStdev) &&
                            (minTouch >= GetFilteredTouchMean()-sigmaFactor) &&
                            (maxTouch <= GetFilteredTouchMean()+sigmaFactor);
    const bool shouldRestartFastCalib = !_isCalibrated && !bufferGood;

    // check if the new mean is much lower than the filtered mean
    const float minAllowedMean = GetFilteredTouchMean() - 
                _kMaxAllowStdevFactorForLowMean*GetFilteredTouchStdev();
    const bool isLowMean = newMean < minAllowedMean;

    const bool shouldRestartCalib = shouldRestartFastCalib || isLowMean;
                                        
    if(shouldRestartCalib) {
      _numConsecutiveReadings = 0;
      _isCalibrated           = false;
      // when set to zero, these variables take on the next non-zero value
      _filteredTouchMean      = fmin(_filteredTouchMean, newMean);
      _filteredTouchStdev     = fmin(_filteredTouchStdev, newStdev);
    } else {
      // this reading is good: accumulate into the filter
      auto coeff = _isCalibrated ? _kFilterCoeffSlow : _kFilterCoeffFast;
      _filteredTouchMean = newMean*coeff + _filteredTouchMean*(1.0f-coeff);
      _filteredTouchStdev = newStdev*coeff + _filteredTouchStdev*(1.0f-coeff);

      _numConsecutiveReadings++;
      if(!_isCalibrated && _numConsecutiveReadings > _kNumUpdatesForFastCalib) {
        _isCalibrated = true;
        PRINT_NAMED_INFO("TouchBaseLineCalibrator.Calibrated","");
      }
    }

    _bufferedTouchIntensity.clear();
  }
}

} // end namespace
} // end namespace
