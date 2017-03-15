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
        bool _enableCliffDetect = true;
        bool _cliffDetected    = false;
        bool _wasCliffDetected = false;
        bool _wasPickedup      = false;

        u16 _cliffDetectThresh = CLIFF_SENSOR_DROP_LEVEL;
        
        CliffEvent _cliffMsg;
        TimeStamp_t _pendingCliffEvent = 0;
        TimeStamp_t _pendingUncliffEvent = 0;
        
        #ifdef SIMULATOR
        // Forward prox sensor
        u8 _lastForwardObstacleDetectedDist = FORWARD_COLLISION_SENSOR_LENGTH_MM + 1;
        const u32 PROX_EVENT_CYCLE_PERIOD = 6;
        #endif
        
        #ifdef COZMO_V2
        u16 _cliffVals[HAL::CLIFF_COUNT];
        HAL::CliffID _minCliffSensor;
        #endif
        
        bool _stopOnCliff = true;
        
      } // "private" namespace

      void QueueCliffEvent(f32 x, f32 y, f32 angle) {
        if (_pendingCliffEvent == 0) {
          _pendingCliffEvent = HAL::GetTimeStamp() + CLIFF_EVENT_DELAY_MS;
          _cliffMsg.x_mm = x;
          _cliffMsg.y_mm = y;
          _cliffMsg.angle_rad = angle;
          _cliffMsg.detected = true;
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

      u16 GetMinRawCliffValue()
      {
        #ifdef COZMO_V2
        return _cliffVals[_minCliffSensor];
        #else
        return HAL::GetRawCliffData();
        #endif // COZMO_V2
      }

      
      
      // Stops robot if cliff detected as wheels are driving forward.
      // Delays cliff event to allow pickup event to cancel it in case the
      // reason for the cliff was actually a pickup.
      void UpdateCliff()
      {
        #ifdef COZMO_V2
        {
          // Update all cliff values and store sensor with min value
          _cliffVals[HAL::CLIFF_FL] = HAL::GetRawCliffData(HAL::CLIFF_FL);
          _cliffVals[HAL::CLIFF_FR] = HAL::GetRawCliffData(HAL::CLIFF_FR);
          _cliffVals[HAL::CLIFF_BL] = HAL::GetRawCliffData(HAL::CLIFF_BL);
          _cliffVals[HAL::CLIFF_BR] = HAL::GetRawCliffData(HAL::CLIFF_BR);

          u16 minVal = _cliffVals[0];
          _minCliffSensor = (HAL::CliffID)0;
          for (int i=1; i<HAL::CLIFF_COUNT; ++i) {
            if (_cliffVals[i] < minVal) {
              minVal = _cliffVals[i];
              _minCliffSensor = (HAL::CliffID)i;
            }
          }
        }
        #endif  // COZMO_V2
        
        // Update cliff status with hysteresis
        u16 rawCliff = GetMinRawCliffValue();
        if (!_cliffDetected && rawCliff < _cliffDetectThresh) {
          _cliffDetected = true;
        } else if (_cliffDetected && rawCliff > CLIFF_SENSOR_UNDROP_LEVEL) {
          _cliffDetected = false;
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
            IsCliffDetected() &&
            !IMUFilter::IsPickedUp() &&
            isDriving && !alreadyStopping &&
            !_wasCliffDetected) {
          
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
          QueueCliffEvent(Localization::GetCurrPose_x(), Localization::GetCurrPose_y(), Localization::GetCurrPose_angle().ToFloat());
          
          _wasCliffDetected = true;
        } else if (!IsCliffDetected() && _wasCliffDetected) {
          QueueUncliffEvent();
          _wasCliffDetected = false;
        }

        
        // Clear queued cliff events if pickedup
        if (IMUFilter::IsPickedUp() && !_wasPickedup) {
          _pendingCliffEvent = 0;
        }
        _wasPickedup = IMUFilter::IsPickedUp();
        
        // Send queued cliff event
        if (_pendingCliffEvent != 0 && HAL::GetTimeStamp() >= _pendingCliffEvent) {
          RobotInterface::SendMessage(_cliffMsg);
          _pendingCliffEvent = 0;
        }
        
        // Send queued uncliff event
        if (_pendingUncliffEvent != 0 && HAL::GetTimeStamp() >= _pendingUncliffEvent) {
          _cliffMsg.detected = false;
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
                ProxObstacle msg;
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


      bool IsCliffDetected()
      {
        return _cliffDetected;
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
        _wasCliffDetected  = false;
        _wasPickedup       = false;
        
        _cliffDetected     = false;
        _cliffDetectThresh = CLIFF_SENSOR_DROP_LEVEL;
        
        _pendingCliffEvent   = 0;
        _pendingUncliffEvent = 0;
      }

    } // namespace ProxSensors
  } // namespace Cozmo
} // namespace Anki
