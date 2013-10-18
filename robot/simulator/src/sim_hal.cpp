// System Includes
#include <cmath>

// Our Includes
#include "anki/cozmo/robot/hal.h"
#include "anki/cozmo/robot/cozmoConfig.h"
#include "anki/cozmo/messageProtocol.h"
#include "cozmo_physics.h"

// Webots Includes
#include <webots/Display.hpp>
#include <webots/Supervisor.hpp>


namespace Anki {
  namespace Cozmo {
    
    // Global pointer to the Webots robot so that we do other things like
    // keyboard control elsewhere.  Read: backdoor hack.  This feels dirty...
    webots::Supervisor* gCozmoBot = NULL;
    
    namespace { // "Private members"

      // Const paramters / settings
      // TODO: some of these should be defined elsewhere (e.g. comms)
      const u16 RECV_BUFFER_SIZE = 1024;
      const s32 UNLOCK_HYSTERESIS = 50;
      const f64 WEBOTS_INFINITY = std::numeric_limits<f64>::infinity();
      
      // For Webots Display:
      const f32 OVERLAY_TEXT_SIZE = 0.07;
      const u32 OVERLAY_TEXT_COLOR = 0x00ff00;
      const u16 MAX_TEXT_DISPLAY_LENGTH = 1024;
      
#pragma mark --- Simulated HardwareInterface "Member Variables" ---
      
      // Overlaid Text Display IDs:
      typedef enum {
        OT_CURR_POSE,
        OT_TARGET_POSE,
        OT_PATH_ERROR
      } OverlayTextID;
      
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
      
      // For pose information
      webots::GPS* gps_;
      webots::Compass* compass_;
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
      
      // For communications with the cozmo_physics plugin used for drawing
      // paths with OpenGL
      webots::Emitter *physicsComms_;
      
      // Webots Display
      char displayStr_[MAX_TEXT_DISPLAY_LENGTH];
      
#pragma mark --- Simulated Hardware Interface "Private Methods" ---
      // Localization
      //void GetGlobalPose(f32 &x, f32 &y, f32& rad);
    
      inline const u8* getHeadImage(void) {
        return headCam_->getImage();
      }
      
      inline const u8* getMatImage(void) {
        return matCam_->getImage();
      }
      
    } // "private" namespace
    
    // Path drawing functions
    void ErasePath(s32 path_id);
    void AppendPathSegmentLine(s32 path_id, f32 x_start_m, f32 y_start_m, f32 x_end_m, f32 y_end_m);
    void AppendPathSegmentArc(s32 path_id, f32 x_center_m, f32 y_center_m, f32 radius_m, f32 startRad, f32 endRad);
    void ShowPath(s32 path_id, bool show);
    void SetPathHeightOffset(f32 m);
    
#pragma mark --- Simulated Hardware Method Implementations ---
    
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
      
      for(u8 i=0; i<HAL::NUM_RADIAL_DISTORTION_COEFFS; ++i) {
        info.distortionCoeffs[i] = 0.f;
      }
      
    } // FillCameraInfo
    
    ReturnCode HAL::Init()
    {
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
      
      FillCameraInfo(headCam_, headCamInfo_);
      FillCameraInfo(matCam_,  matCamInfo_);
        
      leftWheelGyro_  = webotRobot_.getGyro("wheel_gyro_fl");
      rightWheelGyro_ = webotRobot_.getGyro("wheel_gyro_fr");
      
      tx_ = webotRobot_.getEmitter("radio_tx");
      rx_ = webotRobot_.getReceiver("radio_rx");
      
      // Plugin comms uses channel 0
      physicsComms_ = webotRobot_.getEmitter("cozmo_physics_comms");
      
      // Set ID
      // Expected format of name is <SomeName>_<robotID>
      std::string name = webotRobot_.getName();
      size_t lastDelimPos = name.rfind('_');
      if (lastDelimPos != std::string::npos) {
        robotID_ = atoi( name.substr(lastDelimPos+1).c_str() );
        if (robotID_ < 1) {
          fprintf(stdout, "***ERROR: Invalid robot name (%s). ID must be greater than 0\n", name.c_str());
          return EXIT_FAILURE;
        }
        fprintf(stdout, "Initializing robot ID: %d\n", robotID_);
      } else {
        fprintf(stdout, "***ERROR: Cozmo robot name %s is invalid.  Must end with '_<ID number>'\n.", name.c_str());
        return EXIT_FAILURE;
      }
      
      //Set the motors to velocity mode
      leftWheelMotor_->setPosition(WEBOTS_INFINITY);
      rightWheelMotor_->setPosition(WEBOTS_INFINITY);
      
      // Enable position measurements on wheel motors
      leftWheelMotor_->enablePosition(TIME_STEP);
      rightWheelMotor_->enablePosition(TIME_STEP);
      
      // Set speed to 0
      leftWheelMotor_->setVelocity(0);
      rightWheelMotor_->setVelocity(0);
      
      
      // Enable head/lift motor position measurements:
      headMotor_->enablePosition(TIME_STEP);
      liftMotor_->enablePosition(TIME_STEP);
      liftMotor2_->enablePosition(TIME_STEP);
      headMotor_->setPosition(0);
      liftMotor_->setPosition(-0.275);
      liftMotor2_->setPosition(0.275);
      
      // Get localization sensors
      gps_ = webotRobot_.getGPS("gps");
      compass_ = webotRobot_.getCompass("compass");
      gps_->enable(TIME_STEP);
      compass_->enable(TIME_STEP);
      
      // Get wheel speed sensors
      leftWheelGyro_->enable(TIME_STEP);
      rightWheelGyro_->enable(TIME_STEP);
      
      // Setup comms
      rx_->enable(TIME_STEP);
      rx_->setChannel(robotID_);
      tx_->setChannel(robotID_);
      recvBufSize_ = 0;
      
      // Initialize path drawing settings
      SetPathHeightOffset(0.05);
      
      // Global pointer to the Webots robot so that we do other things like
      // keyboard control elsewhere.  Read: backdoor hack.  This feels dirty...
      gCozmoBot = &webotRobot_;
      
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
    
    /*
    void HAL::GetGlobalPose(float &x, float &y, float& rad)
    {
      
      const double* position = gps_->getValues();
      const double* northVector = compass_->getValues();
      
      x = position[0];
      y = -position[2];
      
      rad = std::atan2(northVector[0], -northVector[2]);
      
      snprintf(displayStr_, MAX_TEXT_DISPLAY_LENGTH,
               "Pose: x=%f y=%f angle=%f\n", x, y, rad);
    }*/
    
    
    
    
    void HAL::SetLeftWheelAngularVelocity(float rad_per_sec)
    {
      leftWheelMotor_->setVelocity(-rad_per_sec);
    }
    
    void HAL::SetRightWheelAngularVelocity(float rad_per_sec)
    {
      rightWheelMotor_->setVelocity(-rad_per_sec);
    }
    
    void HAL::SetWheelAngularVelocity(float left_rad_per_sec, float right_rad_per_sec)
    {
      leftWheelMotor_->setVelocity(-left_rad_per_sec);
      rightWheelMotor_->setVelocity(-right_rad_per_sec);
    }
    
    float HAL::GetLeftWheelPosition()
    {
      return leftWheelMotor_->getPosition();
    }
    
    float HAL::GetRightWheelPosition()
    {
      return rightWheelMotor_->getPosition();
    }
    
    void HAL::GetWheelPositions(float &left_rad, float &right_rad)
    {
      left_rad = leftWheelMotor_->getPosition();
      right_rad = rightWheelMotor_->getPosition();
    }
    
    float HAL::GetLeftWheelSpeed()
    {
      const double* axesSpeeds_rad_per_s = leftWheelGyro_->getValues();
      float mm_per_s = -axesSpeeds_rad_per_s[0] * WHEEL_RAD_TO_MM;
      //printf("LEFT: %f rad/s, %f mm/s\n", -axesSpeeds_rad_per_s[0], mm_per_s);
      return mm_per_s;
    }
    
    float HAL::GetRightWheelSpeed()
    {
      const double* axesSpeeds_rad_per_s = rightWheelGyro_->getValues();
      float mm_per_s = -axesSpeeds_rad_per_s[0] * WHEEL_RAD_TO_MM;
      //printf("RIGHT: %f rad/s, %f mm/s\n", -axesSpeeds_rad_per_s[0], mm_per_s);
      return mm_per_s;
    }
    
    void HAL::SetHeadPitch(float pitch_rad)
    {
      headMotor_->setPosition(pitch_rad);
    }
    
    float HAL::GetHeadPitch()
    {
      return headMotor_->getPosition();
    }
    
    void HAL::SetLiftPitch(float pitch_rad)
    {
      liftMotor_->setPosition(pitch_rad);
      liftMotor2_->setPosition(-pitch_rad);
    }
    
    float HAL::GetLiftPitch()
    {
      return liftMotor_->getPosition();
    }
    
    
    void HAL::EngageGripper()
    {
      
    }
    
    void HAL::DisengageGripper()
    {
      if (gripperEngaged_)
      {
        gripperEngaged_ = false;
        unlockhysteresis_ = UNLOCK_HYSTERESIS;
        con_->unlock();
        //printf("UNLOCKED!\n");
      }
    }
    
    bool HAL::IsGripperEngaged() {
      return gripperEngaged_;
    }
    
    
    void HAL::ManageGripper()
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
    
    void SetOverlayText(OverlayTextID ot_id, const char* txt)
    {
      webotRobot_.setLabel(ot_id, txt, 0,
                                 0.7f + static_cast<f32>(ot_id) * (OVERLAY_TEXT_SIZE/3.f),
                                 OVERLAY_TEXT_SIZE, OVERLAY_TEXT_COLOR, 0);
    }
    
    void HAL::UpdateDisplay(void)
    {
     /*
      fprintf(stdout, "speedDes: %d, speedCur: %d, speedCtrl: %d, speedMeas: %d\n",
              GetUserCommandedDesiredVehicleSpeed(),
              GetUserCommandedCurrentVehicleSpeed(),
              GetControllerCommandedVehicleSpeed(),
              GetCurrentMeasuredVehicleSpeed());
      */
      
      // Print overlay text in main 3D view
      SetOverlayText(OT_CURR_POSE, displayStr_);
      
    } // HAL::UpdateDisplay()
    
    
    ReturnCode HAL::Step(void)
    {

      if(webotRobot_.step(Cozmo::TIME_STEP) == -1) {
        return EXIT_FAILURE;
      } else {
        return EXIT_SUCCESS;
      }
      
      
    } // step()
    
    
    
    /////////// Comms /////////////
    void HAL::ManageRecvBuffer()
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
    }
    
    void HAL::SendMessage(const void* data, int size)
    {
      // Prefix data with message header (0xBEEF + robotID)
      u8 msg[1024] = {COZMO_WORLD_MSG_HEADER_BYTE_1,
        COZMO_WORLD_MSG_HEADER_BYTE_2, static_cast<u8>(robotID_)};
      
      if(size+3 > 1024) {
        fprintf(stdout, "Data too large to send with prepended header!\n");
      } else {
        memcpy(msg+3, data, size);
        tx_->send(msg, size+3);
      }
    }
    
    int HAL::RecvMessage(void* data)
    {
      // TODO: check for and remove 0xBEEF?
      
      // Is there any data in the receive buffer?
      if (recvBufSize_ > 0) {
        // Is there a complete message in the receive buffer?
        // The first byte should contain the size of the first message
        int firstMsgSize = recvBuf_[0];
        if (recvBufSize_ >= firstMsgSize) {
          
          // Copy to passed in buffer
          memcpy(data, recvBuf_, firstMsgSize);
          
          // Shift data down
          recvBufSize_ -= firstMsgSize;
          memmove(recvBuf_, &(recvBuf_[firstMsgSize]), recvBufSize_);
          
          return firstMsgSize;
        }
      }
      
      return NULL;
    }
    
    
    //////// Path drawing functions /////////
    void ErasePath(int path_id)
    {
      float msg[ERASE_PATH_MSG_SIZE];
      msg[0] = PLUGIN_MSG_ERASE_PATH;
      msg[PLUGIN_MSG_ROBOT_ID] = robotID_;
      msg[PLUGIN_MSG_PATH_ID] = path_id;
      physicsComms_->send(msg, sizeof(msg));
    }
    
    void AppendPathSegmentLine(int path_id, float x_start_m, float y_start_m, float x_end_m, float y_end_m)
    {
      float msg[LINE_MSG_SIZE];
      msg[0] = PLUGIN_MSG_APPEND_LINE;
      msg[PLUGIN_MSG_ROBOT_ID] = robotID_;
      msg[PLUGIN_MSG_PATH_ID] = path_id;
      msg[LINE_START_X] = x_start_m;
      msg[LINE_START_Y] = y_start_m;
      msg[LINE_END_X] = x_end_m;
      msg[LINE_END_Y] = y_end_m;
      
      physicsComms_->send(msg, sizeof(msg));
    }
    
    void AppendPathSegmentArc(int path_id, float x_center_m, float y_center_m, float radius_m, float startRad, float endRad)
    {
      float msg[ARC_MSG_SIZE];
      msg[0] = PLUGIN_MSG_APPEND_ARC;
      msg[PLUGIN_MSG_ROBOT_ID] = robotID_;
      msg[PLUGIN_MSG_PATH_ID] = path_id;
      msg[ARC_CENTER_X] = x_center_m;
      msg[ARC_CENTER_Y] = y_center_m;
      msg[ARC_RADIUS] = radius_m;
      msg[ARC_START_RAD] = startRad;
      msg[ARC_END_RAD] = endRad;
      
      physicsComms_->send(msg, sizeof(msg));
    }
    
    void ShowPath(int path_id, bool show)
    {
      float msg[SHOW_PATH_MSG_SIZE];
      msg[0] = PLUGIN_MSG_SHOW_PATH;
      msg[PLUGIN_MSG_ROBOT_ID] = robotID_;
      msg[PLUGIN_MSG_PATH_ID] = path_id;
      msg[SHOW_PATH] = show ? 1 : 0;
      physicsComms_->send(msg, sizeof(msg));
    }
    
    void SetPathHeightOffset(float m){
      float msg[SET_HEIGHT_OFFSET_MSG_SIZE];
      msg[0] = PLUGIN_MSG_SET_HEIGHT_OFFSET;
      msg[PLUGIN_MSG_ROBOT_ID] = robotID_;
      //msg[PLUGIN_MSG_PATH_ID] = path_id;
      msg[HEIGHT_OFFSET] = m;
      physicsComms_->send(msg, sizeof(msg));
    }
    
    const HAL::FrameGrabber HAL::GetHeadFrameGrabber(void)
    {
      return &getHeadImage;
    }
    
    const HAL::FrameGrabber HAL::GetMatFrameGrabber(void)
    {
      return &getMatImage;
    }
    
    const HAL::CameraInfo* HAL::GetHeadCamInfo(void)
    {
      return &(headCamInfo_);
    }
    
    const HAL::CameraInfo* HAL::GetMatCamInfo(void)
    {
      return &(matCamInfo_);
    }
    
    // Get the number of microseconds since boot
    u32 HAL::GetMicroCounter(void)
    {
      return static_cast<u32>(webotRobot_.getTime() * 1000000.0);
    }

    
  } // namespace Cozmo
} // namespace Anki
