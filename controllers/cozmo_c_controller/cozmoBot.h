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

using namespace webots;


typedef enum {
  OT_CURR_POSE
  ,OT_TARGET_POSE
  ,OT_PATH_ERROR
} OverlayTextID;


class CozmoBot : public Supervisor
{

private:
  Motor* leftWheelMotor_;
  Motor* rightWheelMotor_;

  Motor* headMotor_;
  Motor* liftMotor_;
  Motor* liftMotor2_;

  Connector* con_;
  bool gripperEngaged_; 
  int unlockhysteresis_;
  void ManageGripper();

  Camera* downCam_;
  Camera* headCam_;
  Camera* liftCam_;

  GPS* gps_;
  Compass* compass_;
  char locStr[MAX_TEXT_DISPLAY_LENGTH];

  Gyro *leftWheelGyro_;
  Gyro *rightWheelGyro_;

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
};


#endif
