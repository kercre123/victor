#include "proxSensors.h"
#include "trig_fast.h"
#include "headController.h"
#include "liftController.h"
#include "animationController.h"
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
        #ifdef COZMO_V2
        const int _nCliffSensors = CLIFF_COUNT;
        #else
        const int _nCliffSensors = 1;
        // The upper bound on the cliff detection threshold is computed by subtracting
        // cliff off value from this.
        const u16 MAX_CLIFF_SENSOR_DETECT_THRESH_UPPER_BOUND = 2*CLIFF_SENSOR_DROP_LEVEL;
        #endif
        
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
        
        #ifdef SIMULATOR
        // Forward prox sensor
        u8 _lastForwardObstacleDetectedDist = FORWARD_COLLISION_SENSOR_LENGTH_MM + 1;
        const u32 PROX_EVENT_CYCLE_PERIOD = 6;
        #endif
        
      } // "private" namespace

      void QueueCliffEvent() {
        if (_pendingCliffEvent == 0) {
          _pendingCliffEvent = HAL::GetTimeStamp() + CLIFF_EVENT_DELAY_MS;
          _cliffMsg.timestamp = HAL::GetTimeStamp();
          #ifdef COZMO_V2
          _cliffMsg.detectedFlags = _cliffDetectedFlags;
          #else
          // For pre-V2 robots, there is only one cliff sensor in the front. However,
          // send the message as if the front left and front right cliffs were detected
          // so that the engine will handle it appropriately.
          _cliffMsg.detectedFlags = (1<<CLIFF_FL) | (1<<CLIFF_FR);
          #endif // COZMO_V2
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
        #ifdef COZMO_V2
        AnkiConditionalErrorAndReturnValue(ind < CLIFF_COUNT, 0, 1233, "ProxSensors.GetRawCliffValue.InvalidIndex", 647, "Index %d is not valid", 1, ind);
        return HAL::GetRawCliffData(static_cast<HAL::CliffID>(ind));
        #else
        AnkiConditionalErrorAndReturnValue(ind == 0, 0, 1233, "ProxSensors.GetRawCliffValue.InvalidIndex", 647, "Index %d is not valid", 1, ind);
        return HAL::GetRawCliffData();
        #endif // COZMO_V2
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
        #ifdef COZMO_V2
        
        // TODO: Not doing it yet, but should do this for Cozmo 2 as well
        const u16 cliffDetectThresh = _cliffDetectThresh;
        const u16 cliffUndetectThresh = CLIFF_SENSOR_UNDROP_LEVEL;
        
        #else
        
        const u16 offLevel = HAL::GetCliffOffLevel();
        
        // Compute cliff detect threshold
        const u16 maxCliffDetectThresh = (offLevel > MAX_CLIFF_SENSOR_DETECT_THRESH_UPPER_BOUND) ? 0
                                         : (MAX_CLIFF_SENSOR_DETECT_THRESH_UPPER_BOUND - offLevel);
        const u16 cliffDetectThresh = MIN(_cliffDetectThresh, maxCliffDetectThresh);
        
        // Lower the undetect threshold at a proportional rate as the detect threshold.
        // i.e. As the detect threshold falls from CLIFF_SENSOR_DROP_LEVEL to 0,
        //      the undetect threshold falls from CLIFF_SENSOR_UNDROP_LEVEL to CLIFF_SENSOR_UNDROP_LEVEL_MIN
        const u16 cliffUndetectThresh = CLIFF_SENSOR_UNDROP_LEVEL_MIN +
                                        (CLIFF_SENSOR_UNDROP_LEVEL - CLIFF_SENSOR_UNDROP_LEVEL_MIN) *
                                        ((f32)(cliffDetectThresh) / CLIFF_SENSOR_DROP_LEVEL);
        
        //AnkiDebugPeriodic(100, 1208, "ProxSensors.UpdateCliff.CliffData", 635, "raw: %d, off: %d, thresh: %d / %d", 4, GetMinRawCliffValue(), offLevel, cliffDetectThresh, cliffUndetectThresh);
        
        #endif  // ifdef COZMO_V2
        
        for (int i=0 ; i < _nCliffSensors ; i++) {
          // Update cliff status with hysteresis
          const bool alreadyDetected = (_cliffDetectedFlags & (1<<i)) != 0;
          if (!alreadyDetected && _cliffVals[i] < cliffDetectThresh) {
            _cliffDetectedFlags |= (1<<i);
          } else if (alreadyDetected && _cliffVals[i] > cliffUndetectThresh) {
            _cliffDetectedFlags &= ~(1<<i);
          }
        }
        
        #ifdef COZMO_V2
        f32 leftSpeed, rightSpeed;
        WheelController::GetFilteredWheelSpeeds(leftSpeed, rightSpeed);
        const bool isDriving = (ABS(leftSpeed) + ABS(rightSpeed)) > WheelController::WHEEL_SPEED_CONSIDER_STOPPED_MM_S;
        #else
        const f32 avgWheelSpeed = WheelController::GetAverageFilteredWheelSpeed();
        const bool isDriving = avgWheelSpeed > WheelController::WHEEL_SPEED_CONSIDER_STOPPED_MM_S;
        #endif
        
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
          AnkiEvent( 339, "ProxSensors.UpdateCliff.StoppingDueToCliff", 347, "%d", 1, _stopOnCliff);
          
          if(_stopOnCliff)
          {
            // Stop all motors and animations
            PickAndPlaceController::Reset();
            SteeringController::ExecuteDirectDrive(0,0);
            
            #ifdef SIMULATOR
            // TODO: On K02, need way to tell Espressif to cancel animations
            AnimationController::Clear();
            #endif

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
          #ifdef COZMO_V2
          // Update the detectedFlags field if any new cliffs have been detected
          // since first queuing the message
          _cliffMsg.detectedFlags |= _cliffDetectedFlags;
          #endif // COZMO_V2
          
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

        // FAKING obstacle detection via prox sensor.
        // TODO: This will eventually be done entirely on the engine using images.
        #ifdef SIMULATOR
        {
          
          if ( HAL::RadioIsConnected() )
          {
            static u32 proxCycleCnt = 0;
            if (++proxCycleCnt == PROX_EVENT_CYCLE_PERIOD) {
              u16 proxVal = HAL::GetRawProxData();
              const bool eventChanged = (proxVal != _lastForwardObstacleDetectedDist);
              if ( eventChanged )
              {
                // send changes in events
                ProxObsDetection msg;
                msg.distance_mm = proxVal;
                RobotInterface::SendMessage(msg);
                _lastForwardObstacleDetectedDist = proxVal;
              }
              proxCycleCnt = 0;
            } // period
          }
          else {
            // reset last since we are not connected anymore
            _lastForwardObstacleDetectedDist = FORWARD_COLLISION_SENSOR_LENGTH_MM + 1;
          }
          
        }
        #endif

        UpdateCliff();
        
        return retVal;

      } // Update()


      bool IsAnyCliffDetected()
      {
        return _cliffDetectedFlags != 0;
      }
      
      void EnableCliffDetector(bool enable) {
        AnkiEvent( 340, "ProxSensors.EnableCliffDetector", 347, "%d", 1, enable);
        _enableCliffDetect = enable;
      }
      
      void EnableStopOnCliff(bool enable) {
        AnkiEvent( 341, "ProxSensors.EnableStopOnCliff", 347, "%d", 1, enable);
        _stopOnCliff = enable;
      }

      void SetCliffDetectThreshold(u16 level)
      {
        if (level > CLIFF_SENSOR_UNDROP_LEVEL) {
          AnkiWarn( 416, "ProxSensors.SetCliffDetectThreshold.TooHigh", 347, "%d", 1, level);
        } else {
          AnkiEvent( 417, "ProxSensors.SetCliffDetectThreshold.NewLevel", 347, "%d", 1, level);
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
