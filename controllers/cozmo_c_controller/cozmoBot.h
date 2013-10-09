#ifndef SIM_ROBOT_H
#define SIM_ROBOT_H

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

typedef enum {
  OT_CURR_POSE
  ,OT_TARGET_POSE
  ,OT_PATH_ERROR
} OverlayTextID;


class CozmoBot : public webots::Supervisor
{
public:
  
  CozmoBot();

  virtual ~CozmoBot();

  void Init();
  void run();
  virtual int step(int ms); // Virtual step function inherited from Supervisor
   
  // Localization
  void GetGlobalPose(float &x, float &y, float& rad);


  // Motor functions
  void SetLeftWheelAngularVelocity(float rad_per_sec);
  void SetRightWheelAngularVelocity(float rad_per_sec);
  void SetWheelAngularVelocity(float left_rad_per_sec, float right_rad_per_sec);

  float GetLeftWheelPosition();
  float GetRightWheelPosition();
  void GetWheelPositions(float &left_rad, float &right_rad);

  float GetLeftWheelSpeed();
  float GetRightWheelSpeed();

  void SetHeadPitch(float pitch_rad);
  float GetHeadPitch() const;
  void SetLiftPitch(float pitch_rad);
  float GetLiftPitch() const;

  void EngageGripper();
  void DisengageGripper();
  bool IsGripperEngaged();


  // Text overlay
  void SetOverlayText(OverlayTextID ot_id, const char* txt);


  // Comms
  void Send(void* data, int size);
  int Recv(void* data);
  bool IsConnected();

  // Path drawing functions
  void ErasePath(int path_id);
  void AppendPathSegmentLine(int path_id, float x_start_m, float y_start_m, float x_end_m, float y_end_m);
  void AppendPathSegmentArc(int path_id, float x_center_m, float y_center_m, float radius_m, float startRad, float endRad);
  void ShowPath(int path_id, bool show);
  void SetPathHeightOffset(float m);
  
private:
  int robotID_;
  
  // Motors
  webots::Motor* leftWheelMotor_;
  webots::Motor* rightWheelMotor_;
  
  webots::Motor* headMotor_;
  webots::Motor* liftMotor_;
  webots::Motor* liftMotor2_;
  
  // Gripper
  webots::Connector* con_;
  bool gripperEngaged_;
  int unlockhysteresis_;
  void ManageGripper();
  
  // Cameras / Vision Processing
  webots::Camera* downCam_;
  webots::Camera* headCam_;
  webots::Camera* liftCam_;
  int processHeadImage();
  int processMatImage();
  
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
  int recvBufSize_;
  void ManageRecvBuffer();
  
  // For communications with the cozmo_physics plugin used for drawing paths with OpenGL
  webots::Emitter *physicsComms_;

#if USING_MATLAB_VISION
  Engine *matlabEngine_;
#endif
};


#endif
