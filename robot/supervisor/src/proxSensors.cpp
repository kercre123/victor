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

namespace Anki {
  namespace Cozmo {
    namespace ProxSensors {

      namespace {

        // Cliff sensor
        const int _nCliffSensors = HAL::CLIFF_COUNT;
        u16 _cliffVals[_nCliffSensors] = {0};
        
        // Bits correspond to each of the cliff sensors (4 for V2, 1 otherwise)
        uint8_t _cliffDetectedFlags = 0;
        
        bool _enableCliffDetect = true;
        bool _wasAnyCliffDetected = false;
        bool _wasPickedup      = false;

        bool _stopOnCliff = true;
        
        u16 _cliffDetectThresh = CLIFF_SENSOR_DROP_LEVEL;
        
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

      u16 GetRawCliffValue(unsigned int ind)
      {
        AnkiConditionalErrorAndReturnValue(ind < HAL::CLIFF_COUNT, 0, "ProxSensors.GetRawCliffValue.InvalidIndex", "Index %d is not valid", ind);
        return HAL::GetRawCliffData(static_cast<HAL::CliffID>(ind));
      }
      
      u16 GetRawProxValue()
      {
        return HAL::GetRawProxData().distance_mm;
      }
      // Stops robot if cliff detected as wheels are driving forward.
      // Delays cliff event to allow pickup event to cancel it in case the
      // reason for the cliff was actually a pickup.
      void UpdateCliff()
      {
        // Update all cliff values
        for (int i=0 ; i < _nCliffSensors ; i++) {
          _cliffVals[i] = GetRawCliffValue(i);
        }
        
        // Compute bounds on cliff detect/undetect thresholds which may be adjusted according to the
        // intensity of ambient light as measured by the LED-off level.
        
        // TODO: Not doing it yet, but should do this for Cozmo 2 as well
        const u16 cliffDetectThresh = _cliffDetectThresh;
        const u16 cliffUndetectThresh = CLIFF_SENSOR_UNDROP_LEVEL;
        
        for (int i=0 ; i < _nCliffSensors ; i++) {
          // Update cliff status with hysteresis
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
            RobotInterface::RobotStopped msg;
            RobotInterface::SendMessage(msg);
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
        Result retVal = RESULT_OK;
        UpdateCliff();

        return retVal;

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

      void SetCliffDetectThreshold(u16 level)
      {
        if (level > CLIFF_SENSOR_UNDROP_LEVEL) {
          AnkiWarn( "ProxSensors.SetCliffDetectThreshold.TooHigh", "%d", level);
        } else {
          AnkiEvent( "ProxSensors.SetCliffDetectThreshold.NewLevel", "%d", level);
          _cliffDetectThresh = level;
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
        
        _cliffDetectThresh = CLIFF_SENSOR_DROP_LEVEL;
        
        _pendingCliffEvent   = 0;
        _pendingUncliffEvent = 0;
      }

    } // namespace ProxSensors
  } // namespace Cozmo
} // namespace Anki
