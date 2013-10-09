#include "cozmoBot.h"
#include "cozmoConfig.h"
#include "app/vehicleMath.h"
#include "app/mainExecution.h"
#include "app/vehicleSpeedController.h"
#include "app/pathFollower.h"
#include "hal/hal.h"
#include "comms/cozmoMsgProtocol.h"
#include "utilMessaging.h"
#include "keyboardController.h"
#include "cozmo_physics.h"
#include <cmath>
#include <cstdio>
#include <string>

#include <webots/Display.hpp>

#if ANKICORETECH_EMBEDDED_USE_MATLAB && USING_MATLAB_VISION
#include "anki/embeddedCommon/matlabConverters.h"
#endif

///////// TESTING //////////
#define EXECUTE_TEST_PATH 0

///////// END TESTING //////

//Names of the wheels used for steering
#define WHEEL_FL "wheel_fl"
#define WHEEL_FR "wheel_fr"
#define GYRO_FL "wheel_gyro_fl"
#define GYRO_FR "wheel_gyro_fr"
#define PITCH "motor_head_pitch"
#define LIFT "lift_motor"
#define LIFT2 "lift_motor2"
#define CONNECTOR "connector"
#define PLUGIN_COMMS "cozmo_physics_comms"
#define RX "radio_rx"
#define TX "radio_tx"

#define DOWN_CAMERA "cam_down"
#define HEAD_CAMERA "cam_head"
#define LIFT_CAMERA "cam_lift"

// Global Cozmo robot instance
CozmoBot gCozmoBot;


CozmoBot::CozmoBot() : Supervisor()
{
  robotID_ = -1;

  leftWheelMotor_ = getMotor(WHEEL_FL);
  rightWheelMotor_ = getMotor(WHEEL_FR);

  headMotor_ = getMotor(PITCH);
  liftMotor_ = getMotor(LIFT);
  liftMotor2_ = getMotor(LIFT2);

  con_ = getConnector(CONNECTOR);
  con_->enablePresence(TIME_STEP);
  gripperEngaged_ = false;
  unlockhysteresis_ = HIST;

  downCam_ = getCamera(DOWN_CAMERA);
  headCam_ = getCamera(HEAD_CAMERA);
  liftCam_ = getCamera(LIFT_CAMERA);
  downCam_->enable(TIME_STEP);
  headCam_->enable(TIME_STEP);
  liftCam_->enable(TIME_STEP);

  leftWheelGyro_ = getGyro(GYRO_FL);
  rightWheelGyro_ = getGyro(GYRO_FR);

  tx_ = getEmitter(TX);
  rx_ = getReceiver(RX);

  // Plugin comms uses channel 0
  physicsComms_ = getEmitter(PLUGIN_COMMS);
    
#if USING_MATLAB_VISION
  matlabEngine_ = NULL;
  if (!(matlabEngine_ = engOpen(""))) {
		fprintf(stderr, "\nCan't start MATLAB engine!\n");
    // TODO: how do we indicate construction failure on the robot?
	}
  
  engEvalString(matlabEngine_, "run('../../../matlab/initCozmoPath.m');");
  
  // TODO: Pass camera calibration data back to Matlab
  
  char cmd[256];
  snprintf(cmd, 255, "h_imgFig = figure; "
           "subplot(121); "
           "h_headImg = imshow(zeros(%d,%d)); "
           "title('Head Camera'); "
           "subplot(122); "
           "h_matImg = imshow(zeros(%d,%d)); "
           "title('Mat Camera');",
           headCam_->getHeight(), headCam_->getWidth(),
           downCam_->getHeight(), downCam_->getWidth());
  
  engEvalString(matlabEngine_, cmd);
  
#endif
} // Constructor: CozmoBot()

CozmoBot::~CozmoBot()
{
#if USING_MATLAB_VISION
  if(matlabEngine_ != NULL) {
  	engClose(matlabEngine_);
  }
#endif
} // Destructor

void CozmoBot::Init() 
{
  // Set ID
  // Expected format of name is <SomeName>_<robotID>
  std::string name = getName();
  size_t lastDelimPos = name.rfind('_');
  if (lastDelimPos != std::string::npos) {
    robotID_ = atoi( name.substr(lastDelimPos+1).c_str() );
    if (robotID_ < 1) {
      fprintf(stdout, "***ERROR: Invalid robot name (%s). ID must be greater than 0\n", name.c_str());
      return;
    }
    fprintf(stdout, "Initializing robot ID: %d\n", robotID_);
  } else {
    fprintf(stdout, "***ERROR: Cozmo robot name %s is invalid.  Must end with '_<ID number>'\n.", name.c_str());
    return;
  }

  //Set the motors to velocity mode
  leftWheelMotor_->setPosition(INFINITY);
  rightWheelMotor_->setPosition(INFINITY);
  
  // Enable position measurements on wheel motors
  leftWheelMotor_->enablePosition(TIME_STEP);
  rightWheelMotor_->enablePosition(TIME_STEP);

  // Set speed to 0
  leftWheelMotor_->setVelocity(0);
  rightWheelMotor_->setVelocity(0);


  //Set the head pitch to 0
  headMotor_->setPosition(0);
  liftMotor_->setPosition(LIFT_CENTER);
  liftMotor2_->setPosition(-LIFT_CENTER);

  // Get localization sensors
  gps_ = getGPS("gps");
  compass_ = getCompass("compass");
  gps_->enable(TIME_STEP);
  compass_->enable(TIME_STEP);

  // Get wheel speed sensors
  leftWheelGyro_->enable(TIME_STEP);
  rightWheelGyro_->enable(TIME_STEP);

  // Setup comms
  rx_->enable(TIME_STEP);
  rx_->setChannel(robotID_);
  tx_->setChannel(BASESTATION_SIM_COMM_CHANNEL);
  recvBufSize_ = 0;

  // Initialize path drawing settings
  SetPathHeightOffset(0.05);
}


void CozmoBot::GetGlobalPose(float &x, float &y, float& rad)
{

  const double* position = gps_->getValues();
  const double* northVector = compass_->getValues();

  x = position[0];
  y = -position[2];

  rad = atan2(northVector[0], -northVector[2]);

  snprintf(locStr, MAX_TEXT_DISPLAY_LENGTH, "Pose: x=%f y=%f angle=%f\n", x, y, rad);
}



void CozmoBot::SetLeftWheelAngularVelocity(float rad_per_sec)
{
  leftWheelMotor_->setVelocity(-rad_per_sec);
}

void CozmoBot::SetRightWheelAngularVelocity(float rad_per_sec)
{
  rightWheelMotor_->setVelocity(-rad_per_sec);
}

void CozmoBot::SetWheelAngularVelocity(float left_rad_per_sec, float right_rad_per_sec)
{
  leftWheelMotor_->setVelocity(-left_rad_per_sec);
  rightWheelMotor_->setVelocity(-right_rad_per_sec);
}

float CozmoBot::GetLeftWheelPosition()
{
  return leftWheelMotor_->getPosition();
}

float CozmoBot::GetRightWheelPosition()
{
  return rightWheelMotor_->getPosition();
}

void CozmoBot::GetWheelPositions(float &left_rad, float &right_rad)
{
  left_rad = leftWheelMotor_->getPosition();
  right_rad = rightWheelMotor_->getPosition();
}

float CozmoBot::GetLeftWheelSpeed()
{
  const double* axesSpeeds_rad_per_s = leftWheelGyro_->getValues();  
  float mm_per_s = -axesSpeeds_rad_per_s[0] * WHEEL_RAD_TO_MM;
  //printf("LEFT: %f rad/s, %f mm/s\n", -axesSpeeds_rad_per_s[0], mm_per_s);
  return mm_per_s;
}

float CozmoBot::GetRightWheelSpeed()
{
  const double* axesSpeeds_rad_per_s = rightWheelGyro_->getValues();  
  float mm_per_s = -axesSpeeds_rad_per_s[0] * WHEEL_RAD_TO_MM;
  //printf("RIGHT: %f rad/s, %f mm/s\n", -axesSpeeds_rad_per_s[0], mm_per_s);
  return mm_per_s;
}



void CozmoBot::SetHeadPitch(float pitch_rad)
{
  headMotor_->setPosition(pitch_rad);
}

float CozmoBot::GetHeadPitch() const
{
  return headMotor_->getPosition();
}

void CozmoBot::SetLiftPitch(float pitch_rad)
{
  liftMotor_->setPosition(pitch_rad);
  liftMotor2_->setPosition(-pitch_rad);
}

float CozmoBot::GetLiftPitch() const
{
  return liftMotor_->getPosition();
}


void CozmoBot::EngageGripper()
{

}

void CozmoBot::DisengageGripper()
{
  if (gripperEngaged_)
  {
    gripperEngaged_ = false;
    unlockhysteresis_ = HIST;
    con_->unlock();
    //printf("UNLOCKED!\n");
  }
}

bool CozmoBot::IsGripperEngaged() {
  return gripperEngaged_;
}


void CozmoBot::ManageGripper()
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

int CozmoBot::processHeadImage()
{
  int retVal = -1;
  
  const u8 *image = this->headCam_->getImage();
  const int nrows = this->headCam_->getHeight();
  const int ncols = this->headCam_->getWidth();
  
#if defined(USE_MATLAB_FOR_HEAD_CAMERA)
  mxArray *mxImg = Anki::Embedded::imageArrayToMxArray(image, nrows, ncols, 4);
  
  if(mxImg != NULL) {
    // Send the image to matlab
    engPutVariable(matlabEngine_, "headCamImage", mxImg);
    
    // Convert from GBRA format to RGB:
    engEvalString(matlabEngine_, "headCamImage = headCamImage(:,:,[3 2 1]);");
    
    // Display (optional)
    engEvalString(matlabEngine_, "set(h_headImg, 'CData', headCamImage);");
    
    // Detect BlockMarkers
    engEvalString(matlabEngine_, "blockMarkers = simpleDetector(headCamImage); "
                  "numMarkers = length(blockMarkers);");
    
    int numMarkers = static_cast<int>(mxGetScalar(engGetVariable(matlabEngine_, "numMarkers")));
    
    fprintf(stdout, "Found %d block markers.\n", numMarkers);
    
    // Can't get the blockMarkers directly because they are Matlab objects
    // which are not happy with engGetVariable.
    //mxArray *mxBlockMarkers = engGetVariable(matlabEngine_, "blockMarkers");
    for(int i_marker=0; i_marker<numMarkers; ++i_marker) {
      
      // Get the pieces of each block marker we need individually
      char cmd[256];
      snprintf(cmd, 255,
               "currentMarker = blockMarkers{%d}; "
               "blockType = currentMarker.blockType; "
               "faceType  = currentMarker.faceType; "
               "corners   = currentMarker.corners;", i_marker+1);
      
      engEvalString(matlabEngine_, cmd);
      
      
      // Create a message from those pieces
      
      CozmoMsg_ObservedBlockMarker msg;
      
      mxArray *mxBlockType = engGetVariable(matlabEngine_, "blockType");
      msg.blockType = static_cast<u32>(mxGetScalar(mxBlockType));
      
      mxArray *mxFaceType = engGetVariable(matlabEngine_, "faceType");
      msg.faceType = static_cast<u32>(mxGetScalar(mxFaceType));
      
      mxArray *mxCorners = engGetVariable(matlabEngine_, "corners");
      
      mxAssert(mxGetM(mxCorners)==4 && mxGetN(mxCorners)==2,
               "BlockMarker's corners should be 4x2 in size.");
      
      double *corners_x = mxGetPr(mxCorners);
      double *corners_y = corners_x + 4;
      
      msg.x_imgUpperLeft  = static_cast<f32>(corners_x[0]);
      msg.y_imgUpperLeft  = static_cast<f32>(corners_y[0]);
      
      msg.x_imgLowerLeft  = static_cast<f32>(corners_x[1]);
      msg.y_imgLowerLeft  = static_cast<f32>(corners_y[1]);
      
      msg.x_imgUpperRight = static_cast<f32>(corners_x[2]);
      msg.y_imgUpperRight = static_cast<f32>(corners_y[2]);
      
      msg.x_imgLowerRight = static_cast<f32>(corners_x[3]);
      msg.y_imgLowerRight = static_cast<f32>(corners_y[3]);
      
      fprintf(stdout, "Sending ObsevedBlockMarker message: Block %d, Face %d "
              "at [(%.1f,%.1f) (%.1f,%.1f) (%.1f,%.1f) (%.1f,%.1f)]\n",
              msg.blockType, msg.faceType,
              msg.x_imgUpperLeft,  msg.y_imgUpperLeft,
              msg.x_imgLowerLeft,  msg.y_imgLowerLeft,
              msg.x_imgUpperRight, msg.y_imgUpperRight,
              msg.x_imgLowerRight, msg.y_imgLowerRight);
      
      this->Send(&msg, sizeof(CozmoMsg_ObservedBlockMarker));
      
    } // FOR each block Marker
    
  } // IF any blockMarkers found
  
  retVal = 0;
  
#else  // NOT defined(USE_MATLAB_FOR_HEAD_CAMERA)

  // TODO: Hook this up to Pete's vision code
  fprintf(stderr, "CozmoBot::processHeadImage(): embedded vision "
          "processing not hooked up yet.\n");
  retVal = -1;
  
#endif // defined(USE_MATLAB_FOR_HEAD_CAMERA)
  
  return retVal;
  
} // processHeadImage()


int CozmoBot::processMatImage()
{
  int retVal = -1;
  
  const u8 *image = this->downCam_->getImage();
  const int nrows = this->headCam_->getHeight();
  const int ncols = this->headCam_->getWidth();
  
#if defined(USE_MATLAB_FOR_MAT_CAMERA)
  mxArray *mxImg = Anki::Embedded::imageArrayToMxArray(image, nrows, ncols, 4);

  if(mxImg != NULL) {
    engPutVariable(matlabEngine_, "matCamImage", mxImg);
    engEvalString(matlabEngine_, "matCamImage = matCamImage(:,:,[3 2 1]);");
    engEvalString(matlabEngine_, "set(h_matImg, 'CData', matCamImage);");
    
    retVal = 0;
  } else {
    fprintf(stderr, "CozmoBot::processHeadImage(): could not convert image to mxArray.");
  }

#else  // NOT defined(USE_MATLAB_FOR_MAT_CAMERA)
  
  // TODO: Hook this up to Pete's vision code
  fprintf(stderr, "CozmoBot::processMatImage(): embedded vision "
          "processing not hooked up yet.\n");
  retVal = -1;
  
#endif // defined(USE_MATLAB_FOR_MAT_CAMERA)
  
  return retVal;
  
} // processMatImage()


void CozmoBot::SetOverlayText(OverlayTextID ot_id, const char* txt)
{
  const float overlayTextSize = 0.07;
  const u32 overlayTextColor = 0x00ff00;
  setLabel(ot_id, txt, 0, 0.7 + (float)ot_id * overlayTextSize/3, overlayTextSize, overlayTextColor, 0);
}

int CozmoBot::step(int ms)
{
  int retVal = webots::Supervisor::step(ms);
  
  RunKeyboardController();
  
  // Do vision processing:
  this->processHeadImage();
  this->processMatImage();
  
#if(EXECUTE_TEST_PATH)
  // TESTING
  static u32 startDriveTime_us = 1000000;
  static BOOL driving = FALSE;
  if (!IsKeyboardControllerEnabled() && !driving && getMicroCounter() > startDriveTime_us) {
    SetUserCommandedAcceleration( MAX(ONE_OVER_CONTROL_DT + 1, 500) );  // This can't be smaller than 1/CONTROL_DT!
    SetUserCommandedDesiredVehicleSpeed(160);
    fprintf(stdout, "Speed commanded: %d mm/s\n", GetUserCommandedDesiredVehicleSpeed() );
    
    
    // Create a path and follow it
    PathFollower::AppendPathSegment_Line(0, 0.0, 0.0, 0.3, -0.3);
    float arc1_radius = sqrt(0.005);  // Radius of sqrt(0.05^2 + 0.05^2)
    PathFollower::AppendPathSegment_Arc(0, 0.35, -0.25, arc1_radius, -0.75*PI, 0);
    PathFollower::AppendPathSegment_Line(0, 0.35 + arc1_radius, -0.25, 0.35 + arc1_radius, 0.2);
    float arc2_radius = sqrt(0.02); // Radius of sqrt(0.1^2 + 0.1^2)
    PathFollower::AppendPathSegment_Arc(0, 0.35 + arc1_radius - arc2_radius, 0.2, arc2_radius, 0, PIDIV2);
    PathFollower::StartPathTraversal();
    
    driving = TRUE;
  }
#endif //EXECUTE_TEST_PATH
  
  
  fprintf(stdout, "speedDes: %d, speedCur: %d, speedCtrl: %d, speedMeas: %d\n",
          GetUserCommandedDesiredVehicleSpeed(),
          GetUserCommandedCurrentVehicleSpeed(),
          GetControllerCommandedVehicleSpeed(),
          GetCurrentMeasuredVehicleSpeed());
  
  CozmoMainExecution();
  
  
  // Buffer any incoming data from basestation
  ManageRecvBuffer();
  
  // Check if connector attaches to anything
  ManageGripper();
  
  // Print overlay text in main 3D view
  SetOverlayText(OT_CURR_POSE, locStr);
  
  return retVal;
  
} // step()


void CozmoBot::run()
{
  
  while (this->step(TIME_STEP) != -1) {
  
  } // while(step)
  
} // run()



/////////// Comms /////////////
void CozmoBot::ManageRecvBuffer()
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

void CozmoBot::Send(void* data, int size)
{
  tx_->send(data, size);
}

int CozmoBot::Recv(void* data)
{
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
void CozmoBot::ErasePath(int path_id)
{
  float msg[ERASE_PATH_MSG_SIZE];
  msg[0] = PLUGIN_MSG_ERASE_PATH;
  msg[PLUGIN_MSG_ROBOT_ID] = robotID_;
  msg[PLUGIN_MSG_PATH_ID] = path_id;
  physicsComms_->send(msg, sizeof(msg));
}

void CozmoBot::AppendPathSegmentLine(int path_id, float x_start_m, float y_start_m, float x_end_m, float y_end_m)
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

void CozmoBot::AppendPathSegmentArc(int path_id, float x_center_m, float y_center_m, float radius_m, float startRad, float endRad)
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

void CozmoBot::ShowPath(int path_id, bool show){
  float msg[SHOW_PATH_MSG_SIZE];
  msg[0] = PLUGIN_MSG_SHOW_PATH;
  msg[PLUGIN_MSG_ROBOT_ID] = robotID_;
  msg[PLUGIN_MSG_PATH_ID] = path_id;
  msg[SHOW_PATH] = show ? 1 : 0;
  physicsComms_->send(msg, sizeof(msg));
}

void CozmoBot::SetPathHeightOffset(float m){
  float msg[SET_HEIGHT_OFFSET_MSG_SIZE];
  msg[0] = PLUGIN_MSG_SET_HEIGHT_OFFSET;
  msg[PLUGIN_MSG_ROBOT_ID] = robotID_;
  //msg[PLUGIN_MSG_PATH_ID] = path_id;
  msg[HEIGHT_OFFSET] = m;
  physicsComms_->send(msg, sizeof(msg));
}
