// System Includes
#include <cmath>
#include <cstdlib>

// Our Includes
#include "anki/cozmo/robot/hal.h"
#include "anki/cozmo/robot/cozmoConfig.h"
#include "anki/cozmo/messageProtocol.h"
#include "cozmo_physics.h"

#include "sim_overlayDisplay.h"

// Webots Includes
#include <webots/Robot.hpp>
#include <webots/Supervisor.hpp>

#ifndef SIMULATOR
#error SIMULATOR should be defined by any target using sim_hal.cpp
#endif

namespace Anki {
  namespace Cozmo {
    
    namespace { // "Private members"

      // Const paramters / settings
      // TODO: some of these should be defined elsewhere (e.g. comms)
      const u16 RECV_BUFFER_SIZE = 1024;
      const s32 UNLOCK_HYSTERESIS = 50;
      const f64 WEBOTS_INFINITY = std::numeric_limits<f64>::infinity();
      
      
      const u32 CAMERA_SINGLE_CAPTURE_TIME_US = 1000000 / 15;  // 15Hz, VGA
      const u32 CAMERA_CONTINUOUS_CAPTURE_TIME_US = 1000000 / 30;  // 30Hz, VGA
      
#pragma mark --- Simulated HardwareInterface "Member Variables" ---
      
      bool isInitialized = false;
      
      webots::Supervisor webotRobot_;
      
      s32 robotID_ = -1;
      
      // Motors
      webots::Motor* leftWheelMotor_;
      webots::Motor* rightWheelMotor_;
      
      webots::Motor* headMotor_;
      webots::Motor* liftMotor_;
      webots::Motor* liftMotor2_;
      
      // Gripper
      webots::Connector* con_;
      bool gripperEngaged_ = false;
      s32 unlockhysteresis_ = UNLOCK_HYSTERESIS;
      
      // Cameras / Vision Processing
      webots::Camera* matCam_;
      webots::Camera* headCam_;
      HAL::CameraInfo headCamInfo_;
      HAL::CameraInfo matCamInfo_;
      HAL::CameraMode headCamMode_;
      // HAL::CameraMode matCamMode_;
      //u8* headCamBuffer_;
      //u8* matCamBuffer_;
      u32 headCamCaptureTime_;
      u32 matCamCaptureTime_;
      u32 headCamStartCaptureTime_ = 0;
      u32 matCamStartCaptureTime_ = 0;
      HAL::CameraUpdateMode headCamUpdateMode_;
      HAL::CameraUpdateMode matCamUpdateMode_;
      
      // For pose information
      webots::GPS* gps_;
      webots::Compass* compass_;
      webots::Node* estPose_;
      //char locStr[MAX_TEXT_DISPLAY_LENGTH];
      
      // For measuring wheel speed
      webots::Gyro *leftWheelGyro_;
      webots::Gyro *rightWheelGyro_;
      
      // For communications with basestation
      webots::Emitter *tx_;
      webots::Receiver *rx_;
      bool isConnected_;
      unsigned char recvBuf_[RECV_BUFFER_SIZE];
      s32 recvBufSize_;
      
#pragma mark --- Simulated Hardware Interface "Private Methods" ---
      // Localization
      //void GetGlobalPose(f32 &x, f32 &y, f32& rad);
      

      float GetHeadAngle()
      {
        return headMotor_->getPosition();
      }
      
      float GetLiftAngle()
      {
        return liftMotor_->getPosition();
      }

      float GetLeftWheelSpeed()
      {
        const double* axesSpeeds_rad_per_s = leftWheelGyro_->getValues();
        //float mm_per_s = -axesSpeeds_rad_per_s[1] * WHEEL_RAD_TO_MM;   // true speed
        float mm_per_s = ABS(axesSpeeds_rad_per_s[1] * WHEEL_RAD_TO_MM); // non-quadrature encoder speed (i.e. always +ve)
        //PRINT("LEFT: %f rad/s, %f mm/s\n", -axesSpeeds_rad_per_s[1], mm_per_s);
        return mm_per_s;
      }
      
      float GetRightWheelSpeed()
      {
        const double* axesSpeeds_rad_per_s = rightWheelGyro_->getValues();
        //float mm_per_s = -axesSpeeds_rad_per_s[1] * WHEEL_RAD_TO_MM;   // true speed
        float mm_per_s = ABS(axesSpeeds_rad_per_s[1] * WHEEL_RAD_TO_MM); // non-quadrature encoder speed (i.e. always +ve)
        //PRINT("RIGHT: %f rad/s, %f mm/s\n", -axesSpeeds_rad_per_s[1], mm_per_s);
        return mm_per_s;
      }

      
      void SetLeftWheelSpeed(f32 mm_per_s)
      {
        f32 rad_per_s = -mm_per_s / WHEEL_RAD_TO_MM;
        leftWheelMotor_->setVelocity(rad_per_s);
      }
      
      void SetRightWheelSpeed(f32 mm_per_s)
      {
        f32 rad_per_s = -mm_per_s / WHEEL_RAD_TO_MM;
        rightWheelMotor_->setVelocity(rad_per_s);
      }
      
      float GetLeftWheelPosition()
      {
        return leftWheelMotor_->getPosition();
      }
      
      float GetRightWheelPosition()
      {
        return rightWheelMotor_->getPosition();
      }
      
      
      void SetHeadAngularVelocity(const f32 rad_per_sec)
      {
        // Only tilt if we are within limits
        // (Webots MaxStop and MinStop don't seem to be working: perhaps
        //  because the motors are in velocity control mode?)
        const f32 currentHeadAngle = GetHeadAngle();
        if(currentHeadAngle >= MIN_HEAD_ANGLE &&
           currentHeadAngle <= MAX_HEAD_ANGLE)
        {
          headMotor_->setVelocity(rad_per_sec);
        } else {
          PRINT("Head at angular limit, refusing to tilt.\n");
          headMotor_->setVelocity(0.0);
          // TODO: return a failure?
        }
      }
      
      
      void SetLiftAngularVelocity(const f32 rad_per_sec)
      {
        liftMotor_->setVelocity(rad_per_sec);
        liftMotor2_->setVelocity(-rad_per_sec);
      }
      
      void EngageGripper()
      {
        //Should we lock to a block which is close to the connector?
        if (!gripperEngaged_ && con_->getPresence() == 1)
        {
          if (unlockhysteresis_ == 0)
          {
            con_->lock();
            gripperEngaged_ = true;
            //printf("LOCKED!\n");
          }else{
            unlockhysteresis_--;
          }
        }
      }
      
      void DisengageGripper()
      {
        if (gripperEngaged_)
        {
          gripperEngaged_ = false;
          unlockhysteresis_ = UNLOCK_HYSTERESIS;
          con_->unlock();
          //printf("UNLOCKED!\n");
        }
      }
      
      /////////// Comms /////////////
      void ManageRecvBuffer()
      {
        // Check for incoming data.
        // Add it to receive buffer.
        // Check for special "radio-level" messages (i.e. pings, connection requests)
        // and respond accordingly.
        
        int dataSize;
        const void* data;
        
        // Read receiver for as long as it is not empty.
        while (rx_->getQueueLength() > 0) {
          
          // Get head packet
          data = rx_->getData();
          dataSize = rx_->getDataSize();
          
          // Copy data to receive buffer
          memcpy(&recvBuf_[recvBufSize_], data, dataSize);
          recvBufSize_ += dataSize;
          
          // Delete processed packet from queue
          rx_->nextPacket();
        }
      } // ManageRecvBuffer()

      
    } // "private" namespace
    
    namespace Sim {
      // Create a pointer to the webots supervisor object within
      // a Simulator namespace so that other Simulation-specific code
      // can talk to it.  This avoids there being a global gCozmoBot
      // running around, accessible in non-simulator code.
      webots::Supervisor* CozmoBot = &webotRobot_;
    }
    
#pragma mark --- Simulated Hardware Method Implementations ---
    
    ReturnCode HAL::Init()
    {
      // TODO: need to check return code?
      UARTInit();
      
      leftWheelMotor_  = webotRobot_.getMotor("wheel_fl");
      rightWheelMotor_ = webotRobot_.getMotor("wheel_fr");
      
      headMotor_  = webotRobot_.getMotor("motor_head_pitch");
      liftMotor_  = webotRobot_.getMotor("lift_motor");
      liftMotor2_ = webotRobot_.getMotor("lift_motor2");
      
      con_ = webotRobot_.getConnector("connector");
      con_->enablePresence(TIME_STEP);
      
      matCam_ = webotRobot_.getCamera("cam_down");
      headCam_ = webotRobot_.getCamera("cam_head");
      
      matCam_->enable(TIME_STEP);
      headCam_->enable(TIME_STEP);
      
      leftWheelGyro_  = webotRobot_.getGyro("wheel_gyro_fl");
      rightWheelGyro_ = webotRobot_.getGyro("wheel_gyro_fr");
      
      tx_ = webotRobot_.getEmitter("radio_tx");
      rx_ = webotRobot_.getReceiver("radio_rx");
      
      
      // Set ID
      // Expected format of name is <SomeName>_<robotID>
      std::string name = webotRobot_.getName();
      size_t lastDelimPos = name.rfind('_');
      if (lastDelimPos != std::string::npos) {
        robotID_ = atoi( name.substr(lastDelimPos+1).c_str() );
        if (robotID_ < 1) {
          PRINT("***ERROR: Invalid robot name (%s). ID must be greater than 0\n", name.c_str());
          return EXIT_FAILURE;
        }
        PRINT("Initializing robot ID: %d\n", robotID_);
      } else {
        PRINT("***ERROR: Cozmo robot name %s is invalid.  Must end with '_<ID number>'\n.", name.c_str());
        return EXIT_FAILURE;
      }
      
      //Set the motors to velocity mode
      headMotor_->setPosition(WEBOTS_INFINITY);
      liftMotor_->setPosition(WEBOTS_INFINITY);
      liftMotor2_->setPosition(WEBOTS_INFINITY);
      leftWheelMotor_->setPosition(WEBOTS_INFINITY);
      rightWheelMotor_->setPosition(WEBOTS_INFINITY);
      
      // Enable position measurements on head, lift, and wheel motors
      leftWheelMotor_->enablePosition(TIME_STEP);
      rightWheelMotor_->enablePosition(TIME_STEP);
      headMotor_->enablePosition(TIME_STEP);
      liftMotor_->enablePosition(TIME_STEP);
      liftMotor2_->enablePosition(TIME_STEP);
      
      // Set speeds to 0
      leftWheelMotor_->setVelocity(0);
      rightWheelMotor_->setVelocity(0);
      headMotor_->setVelocity(0);
      liftMotor_->setVelocity(0);
      liftMotor2_->setVelocity(0);
           
      //headMotor_->setPosition(0);
      //liftMotor_->setPosition(-0.275);
      //liftMotor2_->setPosition(0.275);
      
      // Get localization sensors
      gps_ = webotRobot_.getGPS("gps");
      compass_ = webotRobot_.getCompass("compass");
      gps_->enable(TIME_STEP);
      compass_->enable(TIME_STEP);
      estPose_ = webotRobot_.getFromDef("CozmoBotPose");
      
      // Get wheel speed sensors
      leftWheelGyro_->enable(TIME_STEP);
      rightWheelGyro_->enable(TIME_STEP);
      
      // Setup comms
      rx_->enable(TIME_STEP);
      rx_->setChannel(robotID_);
      tx_->setChannel(robotID_);
      recvBufSize_ = 0;
      
      isInitialized = true;
      return EXIT_SUCCESS;
      
    } // Init()
    
    void HAL::Destroy()
    {
      // Turn off components: (strictly necessary?)
      matCam_->disable();
      headCam_->disable();
      
      gps_->disable();
      compass_->disable();
      
      leftWheelGyro_->disable();
      rightWheelGyro_->disable();
      
      rx_->disable();

    } // Destroy()
    
    bool HAL::IsInitialized(void)
    {
      return isInitialized;
    }
    
    bool HAL::IsConnected(void)
    {
      return isConnected_;
    }
    
    
    void HAL::GetGroundTruthPose(f32 &x, f32 &y, f32& rad)
    {
      
      const double* position = gps_->getValues();
      const double* northVector = compass_->getValues();
      
      x = position[0];
      y = -position[2];
      
      rad = std::atan2(northVector[2], northVector[0]);
      
      //PRINT("GroundTruth:  pos %f %f %f   rad %f %f %f\n", position[0], position[1], position[2],
      //      northVector[0], northVector[1], northVector[2]);
      
      
    } // GetGroundTruthPose()
    
    /* Won't be able to do this on real robot, can only command power/speed
    void HAL::SetHeadPitch(float pitch_rad)
    {
      headMotor_->setPosition(pitch_rad);
    }
     */
    
    /* Won't be able to command angular position directly on real robot, can 
     only set power/speed
    void HAL::SetLiftPitch(float pitch_rad)
    {
      liftMotor_->setPosition(pitch_rad);
      liftMotor2_->setPosition(-pitch_rad);
    }
     */
    
    bool HAL::IsGripperEngaged() {
      return gripperEngaged_;
    }
    
    void HAL::UpdateDisplay(void)
    {
      using namespace Sim::OverlayDisplay;
     /*
      PRINT("speedDes: %d, speedCur: %d, speedCtrl: %d, speedMeas: %d\n",
              GetUserCommandedDesiredVehicleSpeed(),
              GetUserCommandedCurrentVehicleSpeed(),
              GetControllerCommandedVehicleSpeed(),
              GetCurrentMeasuredVehicleSpeed());
      */
       
    } // HAL::UpdateDisplay()
    
    
    
    // Set the motor power in the unitless range [-1.0, 1.0]
    void HAL::MotorSetPower(MotorID motor, f32 power)
    {
      switch(motor) {
        case MOTOR_LEFT_WHEEL:
          // TODO: Assuming linear relationship, but it's not!
          SetLeftWheelSpeed(power * MAX_WHEEL_SPEED);
          break;
        case MOTOR_RIGHT_WHEEL:
          // TODO: Assuming linear relationship, but it's not!
          SetRightWheelSpeed(power * MAX_WHEEL_SPEED);
          break;
        case MOTOR_LIFT:
          // TODO: Assuming linear relationship, but it's not!
          SetLiftAngularVelocity(power * MAX_LIFT_SPEED);
          break;
        case MOTOR_GRIP:
          if (power > 0) {
            EngageGripper();
          } else {
            DisengageGripper();
          }
          break;
        case MOTOR_HEAD:
          // TODO: Assuming linear relationship, but it's not!
          SetHeadAngularVelocity(power * MAX_HEAD_SPEED);
          break;
        default:
          PRINT("ERROR (HAL::MotorSetPower) - Undefined motor type %d\n", motor);
          return;
      }
    }
    
    // Reset the internal position of the specified motor to 0
    void HAL::MotorResetPosition(MotorID motor)
    {
      // TODO
      switch(motor) {
        case MOTOR_LEFT_WHEEL:
          break;
        case MOTOR_RIGHT_WHEEL:
          break;
        case MOTOR_LIFT:
          break;
        case MOTOR_GRIP:
          break;
        case MOTOR_HEAD:
          break;
        default:
          PRINT("ERROR (HAL::MotorResetPosition) - Undefined motor type %d\n", motor);
          return;
      }
    }
    
    // Returns units based on the specified motor type:
    // Wheels are in mm/s, everything else is in degrees/s.
    f32 HAL::MotorGetSpeed(MotorID motor)
    {
      switch(motor) {
        case MOTOR_LEFT_WHEEL:
          return GetLeftWheelSpeed();
        case MOTOR_RIGHT_WHEEL:
          return GetRightWheelSpeed();
        case MOTOR_LIFT:
          // TODO: add gyros
          break;
        case MOTOR_GRIP:
          // TODO
          break;
        case MOTOR_HEAD:
          // TODO
          break;
        default:
          PRINT("ERROR (HAL::MotorGetSpeed) - Undefined motor type %d\n", motor);
          break;
      }
      return 0;
    }
    
    // Returns units based on the specified motor type:
    // Wheels are in mm since reset, everything else is in degrees.
    f32 HAL::MotorGetPosition(MotorID motor)
    {
      switch(motor) {
        case MOTOR_LEFT_WHEEL:
          return GetLeftWheelPosition();
        case MOTOR_RIGHT_WHEEL:
          return GetRightWheelPosition(); // TODO: Change to mm/s!
        case MOTOR_LIFT:
          return GetLiftAngle();
        case MOTOR_GRIP:
          // TODO
          break;
        case MOTOR_HEAD:
          return GetHeadAngle();
        default:
          PRINT("ERROR (HAL::MotorGetPosition) - Undefined motor type %d\n", motor);
          break;
      }
      return 0;
    }
    
    
    
    ReturnCode HAL::Step(void)
    {

      if(webotRobot_.step(Cozmo::TIME_STEP) == -1) {
        return EXIT_FAILURE;
      } else {
        return EXIT_SUCCESS;
      }
      
      
    } // step()
    
    bool HAL::RadioToBase(const void *buffer, const CozmoMessageID msgID)
    {
      // Prefix data with message header (0xBEEF + robotID + msgID)
      const u8 HEADER_LENGTH = 4;
      u8 msg[256 + HEADER_LENGTH] = {
        COZMO_WORLD_MSG_HEADER_BYTE_1,
        COZMO_WORLD_MSG_HEADER_BYTE_2,
        static_cast<u8>(robotID_),
        static_cast<u8>(msgID)};
      
      const u8 size = MessageTable[msgID].size;
      
      if(size+HEADER_LENGTH > 256) {
        PRINT("Data too large to send with prepended header!\n");
      } else {
        memcpy(msg+HEADER_LENGTH, buffer, size);
        tx_->send(msg, size+HEADER_LENGTH);
      }
      
      return true;
    } // RadioToBase()
    
    u8 HAL::RadioFromBase(u8 buffer[RADIO_BUFFER_SIZE])
    {
      ManageRecvBuffer();
      
      // TODO: check for and remove 0xBEEF and robotID?
      
      // Is there any data in the receive buffer?
      if (recvBufSize_ > 0) {
        // Is there a complete message in the receive buffer?
        // The first byte is the message ID, from which we can determine the size.
        const CozmoMessageID msgID = static_cast<CozmoMessageID>(recvBuf_[0]);
        
        const u8 size = MessageTable[msgID].size;

        if (recvBufSize_ >= size) {
          
          // Copy to passed in buffer
          memcpy(buffer, recvBuf_, size);
          
          // Shift data down
          recvBufSize_ -= size;
          memmove(recvBuf_, &(recvBuf_[size]), recvBufSize_);
          
          return size;
        }
      }
      
      return 0;
    } // RadioFromBase()

    // Helper function to create a CameraInfo struct from Webots camera properties:
    void FillCameraInfo(const webots::Camera *camera,
                        HAL::CameraInfo &info)
    {
      
      u16 nrows  = static_cast<u16>(camera->getHeight());
      u16 ncols  = static_cast<u16>(camera->getWidth());
      f32 width  = static_cast<f32>(ncols);
      f32 height = static_cast<f32>(nrows);
      f32 aspect = width/height;
      
      f32 fov_hor = camera->getFov();
      f32 fov_ver = fov_hor / aspect;
      
      f32 fy = height / (2.f * std::tan(0.5f*fov_ver));
      
      info.focalLength_x = fy;
      info.focalLength_y = fy;
      info.fov_ver       = fov_ver;
      info.center_x      = 0.5f*width;
      info.center_y      = 0.5f*height;
      info.skew          = 0.f;
      info.nrows         = nrows;
      info.ncols         = ncols;
      
      for(u8 i=0; i<NUM_RADIAL_DISTORTION_COEFFS; ++i) {
        info.distortionCoeffs[i] = 0.f;
      }
      
    } // FillCameraInfo
    
    const HAL::CameraInfo* HAL::GetHeadCamInfo(void)
    {
      if(isInitialized) {
        FillCameraInfo(headCam_, headCamInfo_);
        return &headCamInfo_;
      }
      else {
        PRINT("HeadCam calibration requested before HAL initialized.\n");
        return NULL;
      }
    }
    
    const HAL::CameraInfo* HAL::GetMatCamInfo(void)
    {
      if(isInitialized) {
        FillCameraInfo(matCam_, matCamInfo_);
        return &matCamInfo_;
      }
      else {
        PRINT("MatCam calibration requested before HAL initialized.\n");
        return NULL;
      }
    }
    
    HAL::CameraMode HAL::GetHeadCamMode(void)
    {
      return headCamMode_;
    }
    
    // TODO: there is a copy of this in hal.cpp -- consolidate into one location.
    void HAL::SetHeadCamMode(const u8 frameResHeader)
    {
      bool found = false;
      for(CameraMode mode = CAMERA_MODE_VGA;
          not found && mode != CAMERA_MODE_COUNT; ++mode)
      {
        if(frameResHeader == CameraModeInfo[mode].header) {
          headCamMode_ = mode;
          found = true;
        }
      }
      
      if(not found) {
        PRINT("ERROR(SetCameraMode): Unknown frame res: %d", frameResHeader);
      }
    } //SetHeadCamMode()
    
    void GetGrayscaleFrameHelper(webots::Camera* cam, u8* buffer)
    {
      // Acquire grey image
      // (Closest thing to Y channel?)
      const u8* image = cam->getImage();
      if(image == NULL) {
        PRINT("GetGrayscaleFrameHelper(): no image captured!");
      }
      else {
        u32 pixel = 0;
        for (int y=0; y < cam->getHeight(); y++ ) {
          for (int x=0; x < cam->getWidth(); x++ ) {
            buffer[pixel++] = webots::Camera::imageGetGrey(image, cam->getWidth(), x, y);
          }
        }
      }
    } // GetGrayscaleFrameHelper()
    
    
    // Starts camera frame synchronization
    void HAL::CameraStartFrame(CameraID cameraID, u8* frameBuffer,
                               CameraMode mode, CameraUpdateMode updateMode,
                               u16 exposure, bool enableLight)
    {
      // TODO: exposure? enableLight?
      
      switch(cameraID) {
        case CAMERA_FRONT:
        {
          if (mode != CAMERA_MODE_VGA) {
            PRINT("ERROR (CameraStartFrame): Head camera only supports VGA\n");
            return;
          }
          
          headCamUpdateMode_ = updateMode;
          /* Not trying to simulate capture time, so don't need this...
          headCamCaptureTime_ = headCamUpdateMode_ == CAMERA_UPDATE_SINGLE ? CAMERA_SINGLE_CAPTURE_TIME_US : CAMERA_CONTINUOUS_CAPTURE_TIME_US;
          headCamStartCaptureTime_ = GetMicroCounter();
          */
          
          GetGrayscaleFrameHelper(headCam_, frameBuffer);

          break;
        }
          
        case CAMERA_MAT:
        {
          if (mode != CAMERA_MODE_VGA) {
            PRINT("ERROR (CameraStartFrame): Mat camera only supports VGA\n");
            return;
          }
          
          matCamUpdateMode_ = updateMode;
          /* Not trying to simulate capture time, so don't need this...
          matCamCaptureTime_ = matCamUpdateMode_ == CAMERA_UPDATE_SINGLE ? CAMERA_SINGLE_CAPTURE_TIME_US : CAMERA_CONTINUOUS_CAPTURE_TIME_US;
          matCamStartCaptureTime_ = GetMicroCounter();
           */

          GetGrayscaleFrameHelper(matCam_, frameBuffer);
          
          break;
        }
          
        default:
          PRINT("ERROR (CameraStartFrame): Invalid camera %d\n", cameraID);
          break;
      }
    }
    
    // Get the number of lines received so far for the specified camera
    u32 HAL::CameraGetReceivedLines(CameraID cameraID)
    {
      switch(cameraID) {
        case CAMERA_FRONT:
          return headCamInfo_.nrows;
        case CAMERA_MAT:
          return matCamInfo_.nrows;
        default:
          PRINT("ERROR (CameraGetReceivedLines): Invalid camera %d\n", cameraID);
          return 0;
      }

    }
    
    // Returns whether or not the specfied camera has received a full frame
    bool HAL::CameraIsEndOfFrame(CameraID cameraID)
    {
      // Simulated cameras return frames instantaneously. Yay.
      return true;
      
      /* Don't try to be so fancy.
      switch(cameraID) {
        case CAMERA_FRONT:
        
          if (headCamStartCaptureTime_ != 0 && GetMicroCounter() - headCamStartCaptureTime_ > headCamCaptureTime_) {
            if (headCamUpdateMode_ == CAMERA_UPDATE_CONTINUOUS) {
              headCamStartCaptureTime_ = GetMicroCounter();
              //CaptureHeadCamFrame();
            } else { // Single mode
              headCamStartCaptureTime_ = 0;
            }
            return true;
          }
          break;
        
        case CAMERA_MAT:
        
          if (matCamStartCaptureTime_ != 0 && GetMicroCounter() - matCamStartCaptureTime_ > matCamCaptureTime_) {
            if (matCamUpdateMode_ == CAMERA_UPDATE_CONTINUOUS) {
              matCamStartCaptureTime_ = GetMicroCounter();
              //CaptureMatCamFrame();
            } else { // Single mode
              matCamStartCaptureTime_ = 0;
            }
            return true;
          }
          break;
        
        default:
          PRINT("ERROR (CameraIsEndOfFrame): Invalid camera %d\n", cameraID);
          break;
      }
      return false;
       */
    }
    
    
    // Get the number of microseconds since boot
    u32 HAL::GetMicroCounter(void)
    {
      return static_cast<u32>(webotRobot_.getTime() * 1000000.0);
    }
    
    s32 HAL::GetRobotID(void)
    {
      return robotID_;
    }

  } // namespace Cozmo
} // namespace Anki
