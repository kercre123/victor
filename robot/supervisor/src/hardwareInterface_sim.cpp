// System Includes
#include <cmath>

// Our Includes
#include "anki/cozmo/robot/hardwareInterface.h"
#include "anki/cozmo/robot/cozmoConfig.h"
#include "cozmo_physics.h"

// Webots Includes
#include <webots/Display.hpp>
#include <webots/Supervisor.hpp>


namespace Anki {
  namespace Cozmo {
    namespace HardwareInterface {

      // Const paramters / settings
      // TODO: some of these should be defined elsewhere (e.g. comms)
      const u16 RECV_BUFFER_SIZE = 1024;
      const s32 UNLOCK_HYSTERESIS = 50;
      const f32 DRIVE_VELOCITY_SLOW = 5.0f;
      const f32 TURN_VELOCITY_SLOW = 1.0f;
      const f32 LIFT_CENTER = -0.275;
      const f32 LIFT_UP = 0.635;
      const f32 LIFT_UPUP = 0.7;
      const f64 WEBOTS_INFINITY = std::numeric_limits<f64>::infinity();
      const u32 BASESTATION_SIM_COMM_CHANNEL = 100;
      const bool KEYBOARD_CONTROL_ENABLED = true;
      
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
      
      typedef struct {
        
        bool isInitialized;
        
        webots::Supervisor webotRobot_;
        
        s32 robotID_;
        
        // Motors
        webots::Motor* leftWheelMotor_;
        webots::Motor* rightWheelMotor_;
        
        webots::Motor* headMotor_;
        webots::Motor* liftMotor_;
        webots::Motor* liftMotor2_;
        
        // Gripper
        webots::Connector* con_;
        bool gripperEngaged_;
        s32 unlockhysteresis_;
        
        // Cameras / Vision Processing
        webots::Camera* matCam_;
        webots::Camera* headCam_;
        CameraInfo headCamInfo_;
        CameraInfo matCamInfo_;
        
        // For pose information
        webots::GPS* gps_;
        webots::Compass* compass_;
        char locStr[MAX_TEXT_DISPLAY_LENGTH];
        
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
        
        // Webots Keyboard / Display
        bool keyboardCtrlEnabled_;
        char displayStr_[MAX_TEXT_DISPLAY_LENGTH];
        
      } Members;
      
      // The actual static variable holding our members, initialized to
      // the default values:
      // TODO: add more defaults here
      static Members this_ = {
        .isInitialized = false,
        .robotID_      = -1,
        .gripperEngaged_ = false,
        .unlockhysteresis_ = UNLOCK_HYSTERESIS
      };
    
#pragma mark --- Simulated Hardware Interface "Private Methods" ---
      // Localization
      void GetGlobalPose(f32 &x, f32 &y, f32& rad);
      
      // Text overlay
      void SetOverlayText(OverlayTextID ot_id, const char* txt);
      
      // Path drawing functions
      void ErasePath(s32 path_id);
      void AppendPathSegmentLine(s32 path_id, f32 x_start_m, f32 y_start_m, f32 x_end_m, f32 y_end_m);
      void AppendPathSegmentArc(s32 path_id, f32 x_center_m, f32 y_center_m, f32 radius_m, f32 startRad, f32 endRad);
      void ShowPath(s32 path_id, bool show);
      void SetPathHeightOffset(f32 m);
      
      void ManageRecvBuffer();
      
      void RunKeyboardController();
      
      inline const u8* getHeadImage(void) {
        return this_.headCam_->getImage();
      }
      
      inline const u8* getMatImage(void) {
        return this_.matCam_->getImage();
      }
      
    } // namespace HardwareInterface
    
    
#pragma mark --- Simulated Hardware Method Implementations ---
    
    // Helper function to create a CameraInfo struct from Webots camera properties:
    void FillCameraInfo(const webots::Camera *camera,
                        HardwareInterface::CameraInfo &info)
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
      
      for(u8 i=0; i<HardwareInterface::NUM_RADIAL_DISTORTION_COEFFS; ++i) {
        info.distortionCoeffs[i] = 0.f;
      }
      
    } // FillCameraInfo
    
    ReturnCode HardwareInterface::Init()
    {
      this_.leftWheelMotor_  = this_.webotRobot_.getMotor("wheel_fl");
      this_.rightWheelMotor_ = this_.webotRobot_.getMotor("wheel_fr");
      
      this_.headMotor_  = this_.webotRobot_.getMotor("motor_head_pitch");
      this_.liftMotor_  = this_.webotRobot_.getMotor("lift_motor");
      this_.liftMotor2_ = this_.webotRobot_.getMotor("lift_motor2");
      
      this_.con_ = this_.webotRobot_.getConnector("connector");
      this_.con_->enablePresence(TIME_STEP);
      
      this_.matCam_ = this_.webotRobot_.getCamera("cam_down");
      this_.headCam_ = this_.webotRobot_.getCamera("cam_head");
      
      this_.matCam_->enable(TIME_STEP);
      this_.headCam_->enable(TIME_STEP);
      
      FillCameraInfo(this_.headCam_, this_.headCamInfo_);
      FillCameraInfo(this_.matCam_,  this_.matCamInfo_);
        
      this_.leftWheelGyro_  = this_.webotRobot_.getGyro("wheel_gyro_fl");
      this_.rightWheelGyro_ = this_.webotRobot_.getGyro("wheel_gyro_fr");
      
      this_.tx_ = this_.webotRobot_.getEmitter("radio_tx");
      this_.rx_ = this_.webotRobot_.getReceiver("radio_rx");
      
      // Plugin comms uses channel 0
      this_.physicsComms_ = this_.webotRobot_.getEmitter("cozmo_physics_comms");
      
      // Set ID
      // Expected format of name is <SomeName>_<robotID>
      std::string name = this_.webotRobot_.getName();
      size_t lastDelimPos = name.rfind('_');
      if (lastDelimPos != std::string::npos) {
        this_.robotID_ = atoi( name.substr(lastDelimPos+1).c_str() );
        if (this_.robotID_ < 1) {
          fprintf(stdout, "***ERROR: Invalid robot name (%s). ID must be greater than 0\n", name.c_str());
          return EXIT_FAILURE;
        }
        fprintf(stdout, "Initializing robot ID: %d\n", this_.robotID_);
      } else {
        fprintf(stdout, "***ERROR: Cozmo robot name %s is invalid.  Must end with '_<ID number>'\n.", name.c_str());
        return EXIT_FAILURE;
      }
      
      //Set the motors to velocity mode
      this_.leftWheelMotor_->setPosition(WEBOTS_INFINITY);
      this_.rightWheelMotor_->setPosition(WEBOTS_INFINITY);
      
      // Enable position measurements on wheel motors
      this_.leftWheelMotor_->enablePosition(TIME_STEP);
      this_.rightWheelMotor_->enablePosition(TIME_STEP);
      
      // Set speed to 0
      this_.leftWheelMotor_->setVelocity(0);
      this_.rightWheelMotor_->setVelocity(0);
      
      
      //Set the head pitch to 0
      this_.headMotor_->setPosition(0);
      this_.liftMotor_->setPosition(LIFT_CENTER);
      this_.liftMotor2_->setPosition(-LIFT_CENTER);
      
      // Get localization sensors
      this_.gps_ = this_.webotRobot_.getGPS("gps");
      this_.compass_ = this_.webotRobot_.getCompass("compass");
      this_.gps_->enable(TIME_STEP);
      this_.compass_->enable(TIME_STEP);
      
      // Get wheel speed sensors
      this_.leftWheelGyro_->enable(TIME_STEP);
      this_.rightWheelGyro_->enable(TIME_STEP);
      
      // Setup comms
      this_.rx_->enable(TIME_STEP);
      this_.rx_->setChannel(this_.robotID_);
      this_.tx_->setChannel(BASESTATION_SIM_COMM_CHANNEL);
      this_.recvBufSize_ = 0;
      
      // Initialize path drawing settings
      SetPathHeightOffset(0.05);
      
      this_.isInitialized = true;
      return EXIT_SUCCESS;
      
    } // Init()
    
    void HardwareInterface::Destroy()
    {
      // Turn off components: (strictly necessary?)
      this_.matCam_->disable();
      this_.headCam_->disable();
      
      this_.gps_->disable();
      this_.compass_->disable();
      
      this_.leftWheelGyro_->disable();
      this_.rightWheelGyro_->disable();
      
      this_.rx_->disable();

    } // Destroy()
    
    bool HardwareInterface::IsInitialized(void)
    {
      return this_.isInitialized;
    }
    
    bool HardwareInterface::IsConnected(void)
    {
      return this_.isConnected_;
    }
    
    void HardwareInterface::GetGlobalPose(float &x, float &y, float& rad)
    {
      
      const double* position = this_.gps_->getValues();
      const double* northVector = this_.compass_->getValues();
      
      x = position[0];
      y = -position[2];
      
      rad = std::atan2(northVector[0], -northVector[2]);
      
      snprintf(this_.displayStr_, MAX_TEXT_DISPLAY_LENGTH,
               "Pose: x=%f y=%f angle=%f\n", x, y, rad);
    }
    
    
    
    void HardwareInterface::SetLeftWheelAngularVelocity(float rad_per_sec)
    {
      this_.leftWheelMotor_->setVelocity(-rad_per_sec);
    }
    
    void HardwareInterface::SetRightWheelAngularVelocity(float rad_per_sec)
    {
      this_.rightWheelMotor_->setVelocity(-rad_per_sec);
    }
    
    void HardwareInterface::SetWheelAngularVelocity(float left_rad_per_sec, float right_rad_per_sec)
    {
      this_.leftWheelMotor_->setVelocity(-left_rad_per_sec);
      this_.rightWheelMotor_->setVelocity(-right_rad_per_sec);
    }
    
    float HardwareInterface::GetLeftWheelPosition()
    {
      return this_.leftWheelMotor_->getPosition();
    }
    
    float HardwareInterface::GetRightWheelPosition()
    {
      return this_.rightWheelMotor_->getPosition();
    }
    
    void HardwareInterface::GetWheelPositions(float &left_rad, float &right_rad)
    {
      left_rad = this_.leftWheelMotor_->getPosition();
      right_rad = this_.rightWheelMotor_->getPosition();
    }
    
    float HardwareInterface::GetLeftWheelSpeed()
    {
      const double* axesSpeeds_rad_per_s = this_.leftWheelGyro_->getValues();
      float mm_per_s = -axesSpeeds_rad_per_s[0] * WHEEL_RAD_TO_MM;
      //printf("LEFT: %f rad/s, %f mm/s\n", -axesSpeeds_rad_per_s[0], mm_per_s);
      return mm_per_s;
    }
    
    float HardwareInterface::GetRightWheelSpeed()
    {
      const double* axesSpeeds_rad_per_s = this_.rightWheelGyro_->getValues();
      float mm_per_s = -axesSpeeds_rad_per_s[0] * WHEEL_RAD_TO_MM;
      //printf("RIGHT: %f rad/s, %f mm/s\n", -axesSpeeds_rad_per_s[0], mm_per_s);
      return mm_per_s;
    }
    
    void HardwareInterface::SetHeadPitch(float pitch_rad)
    {
      this_.headMotor_->setPosition(pitch_rad);
    }
    
    float HardwareInterface::GetHeadPitch()
    {
      return this_.headMotor_->getPosition();
    }
    
    void HardwareInterface::SetLiftPitch(float pitch_rad)
    {
      this_.liftMotor_->setPosition(pitch_rad);
      this_.liftMotor2_->setPosition(-pitch_rad);
    }
    
    float HardwareInterface::GetLiftPitch()
    {
      return this_.liftMotor_->getPosition();
    }
    
    
    void HardwareInterface::EngageGripper()
    {
      
    }
    
    void HardwareInterface::DisengageGripper()
    {
      if (this_.gripperEngaged_)
      {
        this_.gripperEngaged_ = false;
        this_.unlockhysteresis_ = UNLOCK_HYSTERESIS;
        this_.con_->unlock();
        //printf("UNLOCKED!\n");
      }
    }
    
    bool HardwareInterface::IsGripperEngaged() {
      return this_.gripperEngaged_;
    }
    
    
    void HardwareInterface::ManageGripper()
    {
      //Should we lock to a block which is close to the connector?
      if (!this_.gripperEngaged_ && this_.con_->getPresence() == 1)
      {
        if (this_.unlockhysteresis_ == 0)
        {
          this_.con_->lock();
          this_.gripperEngaged_ = true;
          //printf("LOCKED!\n");
        }else{
          this_.unlockhysteresis_--;
        }
      }
    }
    
    void HardwareInterface::SetOverlayText(OverlayTextID ot_id, const char* txt)
    {
      this_.webotRobot_.setLabel(ot_id, txt, 0,
                                 0.7f + static_cast<f32>(ot_id) * (OVERLAY_TEXT_SIZE/3.f),
                                 OVERLAY_TEXT_SIZE, OVERLAY_TEXT_COLOR, 0);
    }
    
    void HardwareInterface::UpdateDisplay(void)
    {
     /*
      fprintf(stdout, "speedDes: %d, speedCur: %d, speedCtrl: %d, speedMeas: %d\n",
              GetUserCommandedDesiredVehicleSpeed(),
              GetUserCommandedCurrentVehicleSpeed(),
              GetControllerCommandedVehicleSpeed(),
              GetCurrentMeasuredVehicleSpeed());
      */
      
      // Print overlay text in main 3D view
      SetOverlayText(OT_CURR_POSE, this_.displayStr_);
      
    } // HardwareInterface::UpdateDisplay()
    
    
    ReturnCode HardwareInterface::Step(void)
    {

      if(this_.webotRobot_.step(Cozmo::TIME_STEP) == -1) {
        return EXIT_FAILURE;
      } else {
        return EXIT_SUCCESS;
      }
      
      
    } // step()
    
    
    
    /////////// Comms /////////////
    void HardwareInterface::ManageRecvBuffer()
    {
      // Check for incoming data.
      // Add it to receive buffer.
      // Check for special "radio-level" messages (i.e. pings, connection requests)
      // and respond accordingly.
      
      int dataSize;
      const void* data;
      
      // Read receiver for as long as it is not empty.
      while (this_.rx_->getQueueLength() > 0) {
        
        // Get head packet
        data = this_.rx_->getData();
        dataSize = this_.rx_->getDataSize();
        
        // Copy data to receive buffer
        memcpy(&this_.recvBuf_[this_.recvBufSize_], data, dataSize);
        this_.recvBufSize_ += dataSize;
        
        // Delete processed packet from queue
        this_.rx_->nextPacket();
      }
    }
    
    void HardwareInterface::SendMessage(const void* data, int size)
    {
      this_.tx_->send(data, size);
    }
    
    int HardwareInterface::RecvMessage(void* data)
    {
      // Is there any data in the receive buffer?
      if (this_.recvBufSize_ > 0) {
        // Is there a complete message in the receive buffer?
        // The first byte should contain the size of the first message
        int firstMsgSize = this_.recvBuf_[0];
        if (this_.recvBufSize_ >= firstMsgSize) {
          
          // Copy to passed in buffer
          memcpy(data, this_.recvBuf_, firstMsgSize);
          
          // Shift data down
          this_.recvBufSize_ -= firstMsgSize;
          memmove(this_.recvBuf_, &(this_.recvBuf_[firstMsgSize]), this_.recvBufSize_);
          
          return firstMsgSize;
        }
      }
      
      return NULL;
    }
    
    
    //////// Path drawing functions /////////
    void HardwareInterface::ErasePath(int path_id)
    {
      float msg[ERASE_PATH_MSG_SIZE];
      msg[0] = PLUGIN_MSG_ERASE_PATH;
      msg[PLUGIN_MSG_ROBOT_ID] = this_.robotID_;
      msg[PLUGIN_MSG_PATH_ID] = path_id;
      this_.physicsComms_->send(msg, sizeof(msg));
    }
    
    void HardwareInterface::AppendPathSegmentLine(int path_id, float x_start_m, float y_start_m, float x_end_m, float y_end_m)
    {
      float msg[LINE_MSG_SIZE];
      msg[0] = PLUGIN_MSG_APPEND_LINE;
      msg[PLUGIN_MSG_ROBOT_ID] = this_.robotID_;
      msg[PLUGIN_MSG_PATH_ID] = path_id;
      msg[LINE_START_X] = x_start_m;
      msg[LINE_START_Y] = y_start_m;
      msg[LINE_END_X] = x_end_m;
      msg[LINE_END_Y] = y_end_m;
      
      this_.physicsComms_->send(msg, sizeof(msg));
    }
    
    void HardwareInterface::AppendPathSegmentArc(int path_id, float x_center_m, float y_center_m, float radius_m, float startRad, float endRad)
    {
      float msg[ARC_MSG_SIZE];
      msg[0] = PLUGIN_MSG_APPEND_ARC;
      msg[PLUGIN_MSG_ROBOT_ID] = this_.robotID_;
      msg[PLUGIN_MSG_PATH_ID] = path_id;
      msg[ARC_CENTER_X] = x_center_m;
      msg[ARC_CENTER_Y] = y_center_m;
      msg[ARC_RADIUS] = radius_m;
      msg[ARC_START_RAD] = startRad;
      msg[ARC_END_RAD] = endRad;
      
      this_.physicsComms_->send(msg, sizeof(msg));
    }
    
    void HardwareInterface::ShowPath(int path_id, bool show)
    {
      float msg[SHOW_PATH_MSG_SIZE];
      msg[0] = PLUGIN_MSG_SHOW_PATH;
      msg[PLUGIN_MSG_ROBOT_ID] = this_.robotID_;
      msg[PLUGIN_MSG_PATH_ID] = path_id;
      msg[SHOW_PATH] = show ? 1 : 0;
      this_.physicsComms_->send(msg, sizeof(msg));
    }
    
    void HardwareInterface::SetPathHeightOffset(float m){
      float msg[SET_HEIGHT_OFFSET_MSG_SIZE];
      msg[0] = PLUGIN_MSG_SET_HEIGHT_OFFSET;
      msg[PLUGIN_MSG_ROBOT_ID] = this_.robotID_;
      //msg[PLUGIN_MSG_PATH_ID] = path_id;
      msg[HEIGHT_OFFSET] = m;
      this_.physicsComms_->send(msg, sizeof(msg));
    }
    
    const HardwareInterface::FrameGrabber HardwareInterface::GetHeadFrameGrabber(void)
    {
      return &getHeadImage;
    }
    
    const HardwareInterface::FrameGrabber HardwareInterface::GetMatFrameGrabber(void)
    {
      return &getMatImage;
    }
    
    const HardwareInterface::CameraInfo* HardwareInterface::GetHeadCamInfo(void)
    {
      return &(this_.headCamInfo_);
    }
    
    const HardwareInterface::CameraInfo* HardwareInterface::GetMatCamInfo(void)
    {
      return &(this_.matCamInfo_);
    }
    
    //Check the keyboard keys and issue robot commands
    void HardwareInterface::RunKeyboardController()
    {
      //Why do some of those not match ASCII codes?
      //Numbers, spacebar etc. work, letters are different, why?
      //a, z, s, x, Space
      const s32 CKEY_LIFT_UP    = 65;
      const s32 CKEY_LIFT_DOWN  = 90;
      const s32 CKEY_HEAD_UP    = 83;
      const s32 CKEY_HEAD_DOWN  = 88;
      const s32 CKEY_UNLOCK     = 32;
      
      if (KEYBOARD_CONTROL_ENABLED)
      {
        int key = this_.webotRobot_.keyboardGetKey();
        
        switch (key)
        {
          case webots::Robot::KEYBOARD_UP:
          {
            SetWheelAngularVelocity(DRIVE_VELOCITY_SLOW, DRIVE_VELOCITY_SLOW);
            break;
          }
            
          case webots::Robot::KEYBOARD_DOWN:
          {
            SetWheelAngularVelocity(-DRIVE_VELOCITY_SLOW, -DRIVE_VELOCITY_SLOW);
            break;
          }
            
          case webots::Robot::KEYBOARD_LEFT:
          {
            SetWheelAngularVelocity(-TURN_VELOCITY_SLOW, TURN_VELOCITY_SLOW);
            break;
          }
            
          case webots::Robot::KEYBOARD_RIGHT:
          {
            SetWheelAngularVelocity(TURN_VELOCITY_SLOW, -TURN_VELOCITY_SLOW);
            break;
          }
            
          case CKEY_HEAD_UP: //s-key: move head up
          {
            SetHeadPitch(GetHeadPitch() + 0.01f);
            break;
          }
            
          case CKEY_HEAD_DOWN: //x-key: move head down
          {
            SetHeadPitch(GetHeadPitch() - 0.01f);
            break;
          }
          case CKEY_LIFT_UP: //a-key: move lift up
          {
            SetLiftPitch(GetLiftPitch() + 0.02f);
            break;
          }
            
          case CKEY_LIFT_DOWN: //z-key: move lift down
          {
            SetLiftPitch(GetLiftPitch() - 0.02f);
            break;
          }
          case '1': //set lift to pickup position
          {
            SetLiftPitch(LIFT_CENTER);
            break;
          }
          case '2': //set lift to block +1 position
          {
            SetLiftPitch(LIFT_UP);
            break;
          }
          case '3': //set lift to highest position
          {
            SetLiftPitch(LIFT_UPUP);
            break;
          }
            
          case CKEY_UNLOCK: //spacebar-key: unlock
          {
            DisengageGripper();
            break;
          }
            
            
          default:
          {
            SetWheelAngularVelocity(0, 0);
          }
            
        } // switch(key)
        
      } // if KEYBOARD_CONTROL_ENABLED
      
    } // RunKeyboardController()
    
    
  } // namespace Cozmo
} // namespace Anki
