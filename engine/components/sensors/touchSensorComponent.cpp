/**
 * File: touchSensorComponent.cpp
 *
 * Author: Arjun Menon
 * Created: 9/7/2017
 *
 * Description: Component for managing the capacitative touch sensor
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#include "engine/components/sensors/touchSensorComponent.h"

#include "engine/externalInterface/externalInterface.h"
#include "engine/robot.h"

#include "anki/common/basestation/utils/timer.h"

#include "clad/types/touchGestureTypes.h"

#include "util/container/circularBuffer.h"

namespace Anki {
namespace Cozmo {
  
namespace {
  const std::string kLogDirectory = "touchSensor";
  
  const int kMaxBufferedStrokeCount = 3;
  
  const int kMinNumSlowStrokes = 2;
} // end anonymous namespace

// lightweight helper class to debounce switch values
// http://www.eng.utah.edu/~cs5780/debouncing.pdf
class DebounceHelper
{
public:
  DebounceHelper(int loLimit, int hiLimit)
  : _loLimit(loLimit)
  , _hiLimit(hiLimit)
  , _counter(0)
  , _debouncedKeyPress(false)
  {
  }
  
  bool ProcessRawKeyPress(bool rawKeyPress)
  {
    if(rawKeyPress == _debouncedKeyPress) {
      // if the debounced state is true (pressed), then the counter should
      // count the number of cycles it is stably released
      _counter = _debouncedKeyPress ? _loLimit : _hiLimit;
    } else {
      --_counter;
    }
    
    if(_counter==0) {
      _debouncedKeyPress = rawKeyPress;
      _counter = _debouncedKeyPress ? _loLimit : _hiLimit;
      return true;
    } else {
      return false;
    }
    
    return false;
  }
  
  bool GetDebouncedKeyPress() const
  {
    return _debouncedKeyPress;
  }
  
  
private:
  const int _loLimit;
  const int _hiLimit;
  
  int _counter;
  
  bool _debouncedKeyPress;
};
  

// helper class to classify the stream of touch sensor signal over time
class TouchGestureClassifier
{
public:
  
  TouchGestureClassifier(float minHoldTime, float recentTouchTimeout, float slowStrokeTimeThreshold)
  : _kMinTimeForHeld_s(minHoldTime)
  , _kMaxTimeForRecentTouch_s(recentTouchTimeout)
  , _kMinTouchDurationForSlowStroke_s(slowStrokeTimeThreshold)
  , _touchDurationBuffer(kMaxBufferedStrokeCount)
  , _touchPressTime_s(0.0)
  , _touchReleaseTime_s(0.0)
  {
  }
  
  void AddTouchPressed()
  {
    _touchPressTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  }
  
  void AddTouchReleased()
  {
    _touchReleaseTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
    
    // prune the touch duration buffer to remove ContactSustain level touches
    const bool orderingCorrect = _touchReleaseTime_s > _touchPressTime_s;
    const bool isSustainPress  = (_touchReleaseTime_s - _touchPressTime_s) > _kMinTimeForHeld_s;
    if( orderingCorrect && !isSustainPress ) {
      _touchDurationBuffer.push_back( _touchReleaseTime_s - _touchPressTime_s );
    }
  }
  
  TouchGesture CalcTouchGesture()
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
  
  TouchGesture CheckTransitionToStroking()
  {
    if( _touchDurationBuffer.size() == _touchDurationBuffer.capacity() ) {
      int numSlowStrokes = 0;
      for(int i=0; i<_touchDurationBuffer.size(); ++i) {
        const bool isSlowTouch = (_touchDurationBuffer[i] > _kMinTouchDurationForSlowStroke_s);
        numSlowStrokes += isSlowTouch;
      }
      const bool isSlowRepeating = numSlowStrokes >= kMinNumSlowStrokes;
      return isSlowRepeating ? TouchGesture::SlowRepeating : TouchGesture::FastRepeating;
    }
    
    return TouchGesture::Count; // default do not transition
  }
  
  TouchGesture CheckTransitionToNoTouch(const float now)
  {
    return ((now-_touchReleaseTime_s) > _kMaxTimeForRecentTouch_s) ?
            TouchGesture::NoTouch :
            TouchGesture::Count; // default do not transition
  }
  
  TouchGesture CheckTransitionToSustain(const float now)
  {
    const bool isInPressPhase = _touchPressTime_s > _touchReleaseTime_s;
    const bool isPressedLong  = (now-_touchPressTime_s) > _kMinTimeForHeld_s;
    return (isInPressPhase && isPressedLong) ? TouchGesture::ContactSustain : TouchGesture::Count;
  }
  
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
  
  // the last N touch durations
  // used to classify stroking
  Util::CircularBuffer<float> _touchDurationBuffer;
  
  // timestamps of the last touch events
  float _touchPressTime_s;
  float _touchReleaseTime_s;
  
  TouchGesture _gestureState; // XXX: add an intermediate state for "primed" but not yet "idle"
};
  
  
  TouchSensorComponent::TouchSensorComponent(Robot& robot) : ISensorComponent(robot, kLogDirectory)
  , _touchGesture(TouchGesture::NoTouch)
{
}


void TouchSensorComponent::UpdateInternal(const RobotState& msg)
{
  const float kMaxTouchIntensityInvalid = 800;       // arbitrarily chosen for Victor-G
  static float kTouchIntensityThreshold = 560;        // arbitrarily chosen for Victor-G
  static DebounceHelper debouncer( 3, 4);            // arbitrarily chosen for Victor-G
  static TouchGestureClassifier tgc(2.0, 5.0, 0.48); // arbitrarily chosen for Victor-G

  // Quick and dirty static touch bias calculation for factory test
  // until a proper on is implemented
  #ifdef FACTORY_TEST
  static int c = 1;
  static u32 avg = 0;
  if(c > 0 && c < 100)
  {
    c++;
    avg += msg.backpackTouchSensorRaw;
  }
  else if(c == 100)
  {
    avg /= 100;
    c = 0;
    kTouchIntensityThreshold = avg + 30;
  }
  #endif

  // sometimes spurious values that are absurdly high come through the sensor
  if(msg.backpackTouchSensorRaw > kMaxTouchIntensityInvalid) {
    return;
  }
  

  const bool isTouched = msg.backpackTouchSensorRaw > kTouchIntensityThreshold;
  
  if( debouncer.ProcessRawKeyPress(isTouched) ) {
    if( debouncer.GetDebouncedKeyPress() ) {
      tgc.AddTouchPressed();
    } else {
      tgc.AddTouchReleased();
    }
  }
  
  _touchGesture = tgc.CalcTouchGesture();
}


std::string TouchSensorComponent::GetLogHeader()
{
  return std::string("timestamp_ms, touchIntensity");
}


std::string TouchSensorComponent::GetLogRow()
{
  std::string str;
  // XXX(agm): figure out what to log
  return str;
}

  
} // Cozmo namespace
} // Anki namespace
