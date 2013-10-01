#ifndef SIM_ROBOT_H
#define SIM_ROBOT_H

#include <webots/Supervisor.hpp>

#define DRIVE_VELOCITY_SLOW 5.0f
#define TURN_VELOCITY_SLOW 1.0f
#define HIST 50
#define LIFT_CENTER -0.275
#define LIFT_UP 0.635
#define LIFT_UPUP 0.7

#define MAX_TEXT_DISPLAY_LENGTH 1024

#define RECV_BUFFER_SIZE 1024

using namespace webots;


typedef enum {
  OT_CURR_POSE
  ,OT_TARGET_POSE
  ,OT_PATH_ERROR
} OverlayTextID;


class CozmoBot : public Supervisor
{

private:
  int robotID_;

  // Motors
  Motor* leftWheelMotor_;
  Motor* rightWheelMotor_;

  Motor* headMotor_;
  Motor* liftMotor_;
  Motor* liftMotor2_;

  // Gripper
  Connector* con_;
  bool gripperEngaged_; 
  int unlockhysteresis_;
  void ManageGripper();

  // Cameras
  Camera* downCam_;
  Camera* headCam_;
  Camera* liftCam_;

  // For pose information
  GPS* gps_;
  Compass* compass_;
  char locStr[MAX_TEXT_DISPLAY_LENGTH];

  // For measuring wheel speed
  Gyro *leftWheelGyro_;
  Gyro *rightWheelGyro_;

  // For communications with basestation
  Emitter *tx_;
  Receiver *rx_;
  bool isConnected_;
  unsigned char recvBuf_[RECV_BUFFER_SIZE];
  int recvBufSize_;
  void ManageRecvBuffer();

  // For communications with the cozmo_physics plugin used for drawing paths with OpenGL
  Emitter *physicsComms_; 

public:
  CozmoBot();

  virtual ~CozmoBot() {}

  void Init();
  void run();
  
   
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
};


#endif
