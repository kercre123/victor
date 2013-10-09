#ifndef SIM_ROBOT_H
#define SIM_ROBOT_H

#include "anki/common/types.h"
#include "anki/cozmo/C

#include <webots/Supervisor.hpp>

// If enabled, will use Matlab as the vision system for processing images
#define USE_MATLAB_FOR_HEAD_CAMERA
#define USE_MATLAB_FOR_MAT_CAMERA

#define USING_MATLAB_VISION (defined(USE_MATLAB_FOR_HEAD_CAMERA) || \
                             defined(USE_MATLAB_FOR_MAT_CAMERA))

#if USING_MATLAB_VISION
// If using Matlab for any vision processing, enable the Matlab engine
#include "engine.h"
#endif

#define DRIVE_VELOCITY_SLOW 5.0f
#define TURN_VELOCITY_SLOW 1.0f
#define HIST 50
#define LIFT_CENTER -0.275
#define LIFT_UP 0.635
#define LIFT_UPUP 0.7

#define MAX_TEXT_DISPLAY_LENGTH 1024

#define RECV_BUFFER_SIZE 1024

// TODO: Switch to embedded-friendly basic types from common/types.h

namespace Anki {
  namespace Cozmo {
    
    typedef enum {
      OT_CURR_POSE
      ,OT_TARGET_POSE
      ,OT_PATH_ERROR
    } OverlayTextID;
    
    
    //
    // This is an abstract class defining s32erface to lower level
    // hardware functionality like getting an image from a camera,
    // setting motor speeds, etc.  This is all the CozmoBot should
    // need to know in order to talk to its underlying hardware.
    //
    class HardwareInterface
    {
    public:
      
      // Stepping functions (see Robot class definition for difference)
      virtual void step_MainExecution() = 0;
      virtual void step_LongExecution() = 0;
      
      // Wheel motors
      virtual void SetLeftWheelAngularVelocity(f32 rad_per_sec) = 0;
      virtual void SetRightWheelAngularVelocity(f32 rad_per_sec) = 0;
      
      virtual void SetWheelAngularVelocity(f32 left_rad_per_sec,
                                           f32 right_rad_per_sec) = 0;
      
      virtual f32 GetLeftWheelPosition() = 0;
      virtual f32 GetRightWheelPosition() = 0;
      virtual void  GetWheelPositions(f32 &left_rad, f32 &right_rad) = 0;
      
      virtual f32 GetLeftWheelSpeed() = 0;
      virtual f32 GetRightWheelSpeed() = 0;
      
      // Head pitch
      virtual void  SetHeadPitch(f32 pitch_rad) = 0;
      virtual f32 GetHeadPitch() const = 0;
      
      // Lift position
      virtual void  SetLiftPitch(f32 pitch_rad) = 0;
      virtual f32 GetLiftPitch() const = 0;
      
      // Gripper control
      virtual void EngageGripper() = 0;
      virtual void DisengageGripper() = 0;
      virtual bool IsGripperEngaged() = 0;
      
      
      
      // Communications
      virtual void sendMessage(void* data, s32 size) = 0;
      virtual s32  recvMessage(void* data) = 0;
      virtual bool isConnected() = 0;
      
    }; // class HardwareInterface
    
    
    class VisionInterface
    {
      
    public:
      
      // Cameras
      // TODO: Add functions for adjusting ROI of cameras?
      virtual const u8* GetHeadImage() = 0;
      virtual const u8* GetMatImage() = 0;
      
      typedef struct {
        f32 focalLength_x, focalLength_y;
        f32 center_x, center_y;
        f32 skew;
        u16 nrows, ncols;
      } CameraInfo;
      
      virtual CameraInfo GetHeadCamInfo() = 0;
      virtual CameraInfo GetMatCamInfo()  = 0;
      
    }; // class VisionInterface
    
    
    class RealVisionInterface : VisionInterface
    {
      
    }; // class RealVisionInterface
    
    
    class SimulatedVisionInterface : VisionInterface
    {
      
      SimulatedVisionInterface(webots::Supervisor &sup);
      
      // Cameras
      // TODO: Add functions for adjusting ROI of cameras?
      virtual const u8* GetHeadImage();
      virtual const u8* GetMatImage();
      
      virtual CameraInfo GetHeadCamInfo();
      virtual CameraInfo GetMatCamInfo();
      
    protected:
      webots::Supervisor webotsRobot_;
      
    }; // class SimulatedVisionInterface
    
    class VisionMailbox
    {
    public:
      
      void putMessage(const CozmoMsg_);
      void getMessage();
      
    protected:
      
    };
    
    class VisionSystem
    {
    public:
      
      typedef struct {
        bool matMarker;
        u32
      } Mailbox;
      
      VisionSystem(VisionInterface *interface);
      
      int lookForBlocks();
      int visualServo();
      int localizeWithMat();
      
    protected:
      VisionInterface *visionInterface;
      
      VisionInterface::CameraInfo headCamInfo;
      VisionInterface::CameraInfo matCamInfo;
      
    }; // class VisionSystem
    
    class Robot // Cozmo::Robot a.k.a. "CozmoBot"
    {
    public:
      
      Robot();
      ~Robot();
      
      //
      // Stepping Functions
      //
      // We will have (at least?) two threads to step along:
      //
      // 1. The low-level functions which happen at a strict, determinstic rate
      //    (like motor control and sensor updates)
      //
      // 2. The slower functions which take longer and will be run "as quickly
      //    as possible (like the vision system for finding block markers)
      //
      void step_MainExecution();
      void step_LongExecution();
      
      //
      // State Machine Operation Modes
      //
      enum OperationMode {
        INITIATE_PRE_DOCK,
        PRE_DOCK,
        DOCK,
        LIFT_BLOCK,
        PLACE_BLOCK,
        FOLLOW_PATH
      };
      OperationMode get_operationMode() const;
      void set_operationMode(OperationMode newMode);
      
    protected:
      HardwareInterface *hwInterface;
      
      VisionSystem visionSystem;
      VisionMailbox visionMailbox;
      
      OperationMode mode, nextMode;
      
      // Communication with Basestation
      void Send(void* data, s32 size);
      s32  Recv(void* data);
      
#if USING_MATLAB_VISION
      Engine *matlabEngine_;
#endif
      
    }; // class Robot
    
    
    inline Robot::OperationMode Robot::get_operationMode() const
    { return this->mode; }
    
    inline void Robot::set_operationMode(OperationMode newMode)
    { this->mode = newMode; }
    
    
    //
    // This class defines the hardware s32erface to a real, physical robot.
    //
    class RealHardwareInterface : public HardwareInterface
    {
      
    }; // class RealCozmoBot
    
    
    //
    // This class defines the hardware s32erface to a Webots-simulated CozmoBot
    //
    class SimulatedHardwareInterface : public HardwareInterface
    {
    public:
      
      SimulatedHardwareInterface();
      
      virtual ~SimulatedHardwareInterface();
      
      
      //
      // Methods from abstract interface
      //
      
      virtual void step_RealTime();
      void step_AsFastAsPossible();
      
      // Wheel motors
      virtual void SetLeftWheelAngularVelocity(f32 rad_per_sec);
      virtual void SetRightWheelAngularVelocity(f32 rad_per_sec);
      
      virtual void SetWheelAngularVelocity(f32 left_rad_per_sec,
                                           f32 right_rad_per_sec);
      
      virtual f32  GetLeftWheelPosition();
      virtual f32  GetRightWheelPosition();
      virtual void GetWheelPositions(f32 &left_rad, f32 &right_rad);
      
      virtual f32 GetLeftWheelSpeed();
      virtual f32 GetRightWheelSpeed();
      
      // Head pitch
      virtual void SetHeadPitch(f32 pitch_rad);
      virtual f32  GetHeadPitch() const;
      
      // Lift position
      virtual void SetLiftPitch(f32 pitch_rad);
      virtual f32  GetLiftPitch() const;
      
      // Gripper control
      virtual void EngageGripper();
      virtual void DisengageGripper();
      virtual bool IsGripperEngaged();
      
      // Communications
      virtual void sendMessage(void* data, s32 size);
      virtual s32  recvMessage(void* data);
      virtual bool isConnected();
      
      
      //////
      void Init();
      void run();
      virtual s32 step(s32 ms); // Virtual step function inherited from Supervisor
      
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
      
    private:
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
      void ManageGripper();
      
      // Cameras / Vision Processing
      webots::Camera* downCam_;
      webots::Camera* headCam_;
      
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
      void ManageRecvBuffer();
      
      // For communications with the cozmo_physics plugin used for drawing paths with OpenGL
      webots::Emitter *physicsComms_;
      
    };
    
  } // namespace Cozmo
  
} // namespace Anki

#endif
