#include "proxSensors.h"
#include "trig_fast.h"
#include "headController.h"
#include "liftController.h"
#include "messages.h"
#include "pickAndPlaceController.h"
#include "steeringController.h"
#include "wheelController.h"
#include "imuFilter.h"
#include "anki/cozmo/robot/hal.h"
#include "clad/robotInterface/messageRobotToEngine_send_helper.h"
#include "localization.h"
#include "anki/cozmo/robot/logging.h"
#include "anki/cozmo/shared/cozmoConfig.h"

#include <array>

namespace Anki {
  namespace Cozmo {
    namespace ProxSensors {

      namespace {

#ifndef SIMULATOR
        struct ProxSensorLookupEntry {
          u16 rawDistance_mm; // raw distance reading from the sensor
          int offset_mm;      // offset to add to the raw distance to get actual distance
        };
        // Piecewise linear look-up table for raw prox sensor reading offset. The table
        // must be ordered on 'first' element. Keep this table small (lookup is linear!)
#if !FACTORY_TEST
        const std::array<ProxSensorLookupEntry, 2> kProxSensorRawDistLUT_mm {{
          {100, -30},
          {300, -22}
        }};
#else
        const std::array<ProxSensorLookupEntry, 1> kProxSensorRawDistLUT_mm {{
          {0, 0}
        }};
#endif
#endif
        
        // Cliff sensor
        const int _nCliffSensors = HAL::CLIFF_COUNT;
        u16 _cliffVals[_nCliffSensors] = {0};
        
        // Bits correspond to each of the cliff sensors
        uint8_t _cliffDetectedFlags = 0;
        
        bool _enableCliffDetect = true;
        bool _wasAnyCliffDetected = false;
        bool _wasPickedup      = false;

        bool _stopOnCliff = true;
        
        u16 _cliffDetectThresh[4] = {CLIFF_SENSOR_THRESHOLD_DEFAULT, CLIFF_SENSOR_THRESHOLD_DEFAULT, CLIFF_SENSOR_THRESHOLD_DEFAULT, CLIFF_SENSOR_THRESHOLD_DEFAULT};
        
        CliffEvent _cliffMsg;
        TimeStamp_t _pendingCliffEvent = 0;
        TimeStamp_t _pendingUncliffEvent = 0;
      } // "private" namespace

      void QueueCliffEvent() {
        if (_pendingCliffEvent == 0) {
          _pendingCliffEvent = HAL::GetTimeStamp() + CLIFF_EVENT_DELAY_MS;
          _cliffMsg.timestamp = HAL::GetTimeStamp();
          _cliffMsg.detectedFlags = _cliffDetectedFlags;
          _cliffMsg.didStopForCliff = _stopOnCliff;
        }
      }
      
      // If no cliff event is queued, queue this undetected
      // event to go out immediately.
      // If a cliff detected event is already queued, queue
      // the undetected event to go out right after.
      void QueueUncliffEvent() {
        if (_pendingCliffEvent == 0) {
          _pendingUncliffEvent = HAL::GetTimeStamp();
        } else {
          _pendingUncliffEvent = _pendingCliffEvent;
        }
      }

      u8 GetCliffDetectedFlags()
      {
        return _cliffDetectedFlags;
      }
      
      u16 GetCliffValue(u32 ind)
      {
        AnkiConditionalErrorAndReturnValue(ind < HAL::CLIFF_COUNT, 0, "ProxSensors.GetCliffValue.InvalidIndex", "Index %d is not valid", ind);
        return _cliffVals[ind];
      }
      
      ProxSensorDataRaw GetProxData()
      {
        auto proxData = HAL::GetRawProxData();
#ifndef SIMULATOR
        // Apply look-up table to convert from raw distance reading
        // to corrected reading. Piecewise linear interpolation.
        // Assumes kProxSensorRawDistLUT_mm is sorted on 'first' elements.
        const u16 rawDist_mm = proxData.distance_mm;
        int offset_mm = 0;
        const auto& lut = kProxSensorRawDistLUT_mm;
        if (rawDist_mm <= lut.front().rawDistance_mm) {
          offset_mm = lut.front().offset_mm;
        } else if (rawDist_mm >= lut.back().rawDistance_mm) {
          offset_mm = lut.back().offset_mm;
        } else {
          // Linearly interpolate
          for (auto low = lut.begin() ; low != lut.end() ; ++low) {
            const auto high = std::next(low);
            const auto x0 = low->rawDistance_mm;
            const auto x1 = high->rawDistance_mm;
            const auto y0 = low->offset_mm;
            const auto y1 = high->offset_mm;
            if ((rawDist_mm >= x0) &&
                (rawDist_mm <= x1)) {
              offset_mm = y0 + (y1 - y0) * (rawDist_mm - x0) / (x1 - x0);
              break;
            }
          }
        }
        if (rawDist_mm + offset_mm < 0) {
          proxData.distance_mm = 0;
        } else {
          proxData.distance_mm = rawDist_mm + offset_mm;
        }
#endif
        return proxData;
      }
      
      // Stops robot if cliff detected as wheels are driving forward.
      // Delays cliff event to allow pickup event to cancel it in case the
      // reason for the cliff was actually a pickup.
      void UpdateCliff()
      {
        // Update all cliff values
        for (int i=0 ; i < _nCliffSensors ; i++) {
          u16 rawVal = HAL::GetRawCliffData(static_cast<HAL::CliffID>(i));
          if (rawVal == 0) {
            // VIC-537: Cliff sensors sometimes return spurious readings
            //          like 0 (which is too low to be valid) or some crazy high numbers in the 10000s.
            //          Intercepting only 0s here and leaving previous reading as is
            //          so that we don't stop in reaction to a false cliff.
            AnkiWarn("ProxSensors.UpdateCliff.BadZeroCliffValue", "Index %d", i);
          } else {
            _cliffVals[i] = rawVal;
          }
        }
        
        // Compute bounds on cliff detect/undetect thresholds which may be adjusted according to the
        // intensity of ambient light as measured by the LED-off level.
        // TODO: Not doing it yet, but should do this for Cozmo 2 as well
        
        for (int i=0 ; i < _nCliffSensors ; i++) {
          // Update cliff status with hysteresis
          const u16 cliffDetectThresh = _cliffDetectThresh[i];
          const u16 cliffUndetectThresh = cliffDetectThresh + CLIFF_DETECT_HYSTERESIS;
          const bool alreadyDetected = (_cliffDetectedFlags & (1<<i)) != 0;
          if (!alreadyDetected && _cliffVals[i] < cliffDetectThresh) {
            _cliffDetectedFlags |= (1<<i);
          } else if (alreadyDetected && _cliffVals[i] > cliffUndetectThresh) {
            _cliffDetectedFlags &= ~(1<<i);
          }
        }
        
        f32 leftSpeed, rightSpeed;
        WheelController::GetFilteredWheelSpeeds(leftSpeed, rightSpeed);
        const bool isDriving = (ABS(leftSpeed) + ABS(rightSpeed)) > WheelController::WHEEL_SPEED_CONSIDER_STOPPED_MM_S;
        
        // Check for whether or not wheels are already stopping.
        // When reversing and stopping fast enough it's possible for the wheels to report forward speeds.
        // Not sure if because the speed change is too fast for encoders to register properly or if the wheels
        // are actually moving forward very briefly, but in any case, if the wheels are already stopping
        // there's no need to report StoppingDueToCliff.
        // Currently, this only seems to happen during the backup for face plant.
        f32 desiredLeftSpeed, desiredRightSpeed;
        WheelController::GetDesiredWheelSpeeds(desiredLeftSpeed, desiredRightSpeed);
        bool alreadyStopping = (desiredLeftSpeed == 0.f) && (desiredRightSpeed == 0.f);

        if (_enableCliffDetect &&
            IsAnyCliffDetected() &&
            !IMUFilter::IsPickedUp() &&
            isDriving && !alreadyStopping &&
            !_wasAnyCliffDetected) {
          
          // TODO (maybe): Check for cases where cliff detect should not stop motors
          // 1) Turning in place
          // 2) Driving over something (i.e. pitch is higher than some degrees).
          AnkiEvent( "ProxSensors.UpdateCliff.StoppingDueToCliff", "%d", _stopOnCliff);
          
          if(_stopOnCliff)
          {
            // Stop all motors and animations
            PickAndPlaceController::Reset();
            SteeringController::ExecuteDirectDrive(0,0);

            // Send stopped message
            RobotInterface::SendMessage(RobotInterface::RobotStopped());
          }
          else
          {
            // If we aren't stopping at this cliff then send a potential cliff message
            // because we might not be able to verify that it is indeed a cliff
            PotentialCliff msg;
            RobotInterface::SendMessage(msg);
          }
          
          // Queue cliff detected message
          QueueCliffEvent();
          
          _wasAnyCliffDetected = true;
        } else if (!IsAnyCliffDetected() && _wasAnyCliffDetected) {
          QueueUncliffEvent();
          _wasAnyCliffDetected = false;
        }

        
        // Clear queued cliff events if pickedup
        if (IMUFilter::IsPickedUp() && !_wasPickedup) {
          _pendingCliffEvent = 0;
        }
        _wasPickedup = IMUFilter::IsPickedUp();
        
        // Send or update queued cliff event
        if (_pendingCliffEvent != 0) {
          // Update the detectedFlags field if any new cliffs have been detected
          // since first queuing the message
          _cliffMsg.detectedFlags |= _cliffDetectedFlags;
          
          if (HAL::GetTimeStamp() >= _pendingCliffEvent) {
            RobotInterface::SendMessage(_cliffMsg);
            _pendingCliffEvent = 0;
          }
        }
        
        // Send queued uncliff event
        if (_pendingUncliffEvent != 0 && HAL::GetTimeStamp() >= _pendingUncliffEvent) {
          _cliffMsg.detectedFlags = 0;
          RobotInterface::SendMessage(_cliffMsg);
          _pendingUncliffEvent = 0;
        }
      }
      
      Result Update()
      {
        UpdateCliff();

        return RESULT_OK;
      } // Update()  

      bool IsAnyCliffDetected()
      {
        return _cliffDetectedFlags != 0;
      }
      
      void EnableCliffDetector(bool enable) {
        AnkiEvent( "ProxSensors.EnableCliffDetector", "%d", enable);
        _enableCliffDetect = enable;
      }
      
      void EnableStopOnCliff(bool enable) {
        AnkiEvent( "ProxSensors.EnableStopOnCliff", "%d", enable);
        _stopOnCliff = enable;
      }

      void SetCliffDetectThreshold(u32 ind, u16 level)
      {
        AnkiConditionalError(ind < HAL::CLIFF_COUNT, "ProxSensors.SetCliffDetectThreshold.InvalidIndex", "Index %d is not valid", ind);
        if (level > CLIFF_SENSOR_THRESHOLD_MAX) {
          AnkiWarn( "ProxSensors.SetCliffDetectThreshold.TooHigh", "cliff sensor %d, threshold %d", ind, level);
        } else if (level < CLIFF_SENSOR_THRESHOLD_MIN) {
          AnkiWarn( "ProxSensors.SetCliffDetectThreshold.TooLow", "cliff sensor %d, threshold %d", ind, level);
        }else {
          AnkiEvent( "ProxSensors.SetCliffDetectThreshold.NewLevel", "cliff sensor %d, threshold %d", ind, level);
          _cliffDetectThresh[ind] = level;
        }
      }
      
      void SetAllCliffDetectThresholds(u16 level)
      {
        for (int i=0 ; i < HAL::CLIFF_COUNT ; i++) {
          SetCliffDetectThreshold(i, level);
        }
      }
      
      // Since this re-enables 'cliff detect' and 'stop on cliff'
      // it should only be called when the robot disconnects,
      // otherwise you could desync stopOnCliff state with engine.
      void Reset() {
        _enableCliffDetect = true;
        _stopOnCliff       = true;
        _wasAnyCliffDetected  = false;
        _wasPickedup       = false;
        
        _cliffDetectedFlags = 0;
        
        SetAllCliffDetectThresholds(CLIFF_SENSOR_THRESHOLD_DEFAULT);
        
        _pendingCliffEvent   = 0;
        _pendingUncliffEvent = 0;
      }

    } // namespace ProxSensors
  } // namespace Cozmo
} // namespace Anki
