#ifndef COZMO_CONFIG_H
#define COZMO_CONFIG_H

#include "anki/common/constantsAndMacros.h"
#include "anki/common/types.h"
#include "anki/cozmo/robot/debug.h"
#include "anki/vision/CameraSettings.h"

namespace Anki {
  namespace Cozmo {

    const f32 WHEEL_DIAMETER_MM = 28.4f;  // This should be in sync with the CozmoBot proto file
    const f32 HALF_WHEEL_CIRCUM = WHEEL_DIAMETER_MM * M_PI_2;
    const f32 WHEEL_RAD_TO_MM = WHEEL_DIAMETER_MM / 2.f;  // or HALF_WHEEL_CIRCUM / PI;
    const f32 WHEEL_DIST_MM = 47.7f; // distance b/w the front wheels
    const f32 WHEEL_DIST_HALF_MM = WHEEL_DIST_MM / 2.f;
    
    //const f32 MAT_CAM_HEIGHT_FROM_GROUND_MM = (0.5f*WHEEL_DIAMETER_MM) - 3.f;

    const u8 NUM_RADIAL_DISTORTION_COEFFS = 4;

    // Cozmo control loop is 200Hz.
    const s32 TIME_STEP = 5;
    
    // Basestation control loop
    const s32 BS_TIME_STEP = 60;
    
    // Packet headers/footers:
    // TODO: Do we need this?  Only used in simulation I think? (Add #ifdef SIMULATOR?)
    const u8 RADIO_PACKET_HEADER[2] = {0xBE, 0xEF};
    const u8 RADIO_PACKET_FOOTER[2] = {0xFF, 0x0F};

    // The base listening port for robot TCP server.
    // Each robot listens on port (ROBOT_RADIO_BASE_PORT + ROBOT_ID)
    const u16 ROBOT_RADIO_BASE_PORT = 5551;
    
    // Expected message receive latency
    // It is assumed that this value does not fluctuate greatly.
    // The more inaccurate this value is, the more invalid our
    // handling of messages will be.
    const f32 MSG_RECEIVE_LATENCY_SEC = 0.03;
    
    // The effective latency of vehicle messages for basestation modelling purposes
    // This is twice the MSG_RECEIVE_LATENCY_SEC so that the basestation maintains a model
    // of the system one message cycle latency in the future. This way, commanded actions are applied
    // at the time they are expected in the physical world.
    const f32 BASESTATION_MODEL_LATENCY_SEC = 2.f*MSG_RECEIVE_LATENCY_SEC;
  

    
    // Messages that are purely for advertising when using TCP/UDP. (Maybe belongs somewhere else...)
    // This will eventually be passed in via the GUI.
    // The default 127.0.0.1 works fine for simulated basestation running on the same machine as the simluator.
    const char* const ROBOT_SIM_WORLD_HOST = "127.0.0.1";
    
    // Port on which registered robots advertise.
    const u32 ROBOT_ADVERTISING_PORT = 5100;
    
    // Port on which simulated robot should connect to (de)register for advertisement
    const u32 ROBOT_ADVERTISEMENT_REGISTRATION_PORT = 5101;
    
    typedef struct {
      u16 port;                // Port that robot is accepting connections on
      u8 robotAddr[18];        // IP address as null terminated string
      u8 robotID;
      u8 enableAdvertisement;  // 1 when robot wants to advertise, 0 otherwise.
    } RobotAdvertisementRegistration;
    
    typedef struct  {
      u16 port;
      u8 robotAddr[17];        // IP address as null terminated string
      u8 robotID;
    } RobotAdvertisement;
    
    // If most recent advertisement message is older than this,
    // then it is no longer considered to be advertising.
    const f32 ROBOT_ADVERTISING_TIMEOUT_S = 0.25;

    // Time in between robot advertisements
    const f32 ROBOT_ADVERTISING_PERIOD_S = 0.03;
    
    
#ifdef SIMULATOR
    // Should be less than the timestep of offboard_vision_processing,
    // and roughly reflect image capture time.  If it's larger than or equal
    // to offboard_vision_processing timestep then we may send the same image
    // multiple times.
    const s32 VISION_TIME_STEP = 20;
    
    // Note that we are hard-coding an empircally-computed/calibrated focal
    // length, even for the simulated camera! So if we change the simulated FOV
    // of the camera, we need to recalibrate this focal length. This is because,
    // for completely unknown / not-understood reasons, computing the focal
    // length from fov_hor directly does not give us an accurate value for
    // the Webots camera.
    const f32 HEAD_CAM_CALIB_FOV          = 0.92f;
    const f32 HEAD_CAM_CALIB_FOCAL_LENGTH = 338.58f;
    
    ////////// Simulator comms //////////
    
    // TODO: are all these still used and should they be defined elsewhere?
    
    // Channel number used by CozmoWorldComm Webots receiver
    // for robot messages bound for the basestation.
#define BASESTATION_SIM_COMM_CHANNEL 100
    
    ////////// End Simulator comms //////////
    
#else // Real robot:
    
    // TODO: Get these from calibration somehow
    const u16 HEAD_CAM_CALIB_WIDTH  = 320;
    const u16 HEAD_CAM_CALIB_HEIGHT = 240;
    
    // From Calibration on June 3, 2014
    const f32 HEAD_CAM_CALIB_FOCAL_LENGTH_X = 333.6684040730803f;
    const f32 HEAD_CAM_CALIB_FOCAL_LENGTH_Y = 335.2799828009748f;
    const f32 HEAD_CAM_CALIB_CENTER_X       = 153.5278504171585f;
    const f32 HEAD_CAM_CALIB_CENTER_Y       = 125.0506182422117f;
    const f32 HEAD_CAM_CALIB_DISTORTION[NUM_RADIAL_DISTORTION_COEFFS] = {
      -0.130117292078901f,
       0.043087399840388f,
      -0.000994462251660f,
      -0.000805198826767f
    };
    
  
#endif // #ifdef SIMULATOR
    
    const f32 CONTROL_DT = TIME_STEP*0.001f;
    const f32 ONE_OVER_CONTROL_DT = 1.0f/CONTROL_DT;
    
    // The height of the lift at various configurations
    const f32 LIFT_HEIGHT_LOWDOCK  = 29.f;  // Actual limit in proto is closer to 20.4mm, but there is a weird
    // issue with moving the lift when it is at a limit. The lift arm
    // flies off of the robot and comes back! So for now, we just don't
    // drive the lift down that far. We also skip calibration in sim.
#ifdef SIMULATOR
    // Need a different height in simulation because of the position of the connectors
    const f32 LIFT_HEIGHT_HIGHDOCK = 73.f;
#else
    const f32 LIFT_HEIGHT_HIGHDOCK = 69.f;
#endif
    const f32 LIFT_HEIGHT_CARRY    = 88.f;
    
    // Height of lift "shoulder" joint where the arm attaches to robot body
    const f32 LIFT_JOINT_HEIGHT = 41.7f;
    
    // Distance between the lift shoulder joint and the lift "wrist" joint where arm attaches to fork assembly
    const f32 LIFT_ARM_LENGTH = 64.f;
    
    // The lift height is defined as the height of the upper lift arm's wrist joint plus this offset.
    const f32 LIFT_FORK_HEIGHT_REL_TO_ARM_END = 0;

    // The height of the top of the lift crossbar with respect to the wrist joint
    const f32 LIFT_XBAR_HEIGHT_WRT_WRIST_JOINT = -18.f;
    
    // TODO: convert to using these in degree form?
    const f32 MIN_HEAD_ANGLE = DEG_TO_RAD(-25.f);
    const f32 MAX_HEAD_ANGLE = DEG_TO_RAD( 35.f);
    
    const f32 NECK_JOINT_POSITION[3] = {-13.f, 0.f, 33.5f + WHEEL_RAD_TO_MM}; // relative to robot origin
    
    // TODO: Get camera optical center relative to neck joint, not lens face (which is the 8.8 here)
    const f32 HEAD_CAM_POSITION[3]   = {8.8f, 0.f, -6.f}; // relative to neck joint
    
    const f32 LIFT_BASE_POSITION[3]  = {-40.0f, 0.f, 29.5f + WHEEL_RAD_TO_MM}; // relative to robot origin
    //const f32 MAT_CAM_POSITION[3]   =  {-25.0f, 0.f, -3.f}; // relative to robot origin
    
    const f32 ROBOT_BOUNDING_LENGTH = 88.f; // including gripper fingers
    const f32 ROBOT_BOUNDING_WIDTH  = 54.2f;
    const f32 ROBOT_BOUNDING_FRONT_DISTANCE = 32.1f; // distance from robot origin to front of bounding box
    
    const f32 IMU_POSITION[3] = {5.8f, 0.f, -13.5f};  // relative to neck joint
    
    const f32 PREDOCK_DISTANCE_MM = 80.f;
    
    /*
    // This is the width of the *outside* of the square fiducial!
    // Note that these don't affect Matlab, meaning offboard vision!
    #define BLOCKMARKER3D_USE_OUTSIDE_SQUARE true
    const f32 BLOCK_MARKER_WIDTH_MM = 32.f;
    */
    
    // TODO: This needs to be sync'd with whatever is in BlockDefinitions.h
    const f32 DEFAULT_BLOCK_MARKER_WIDTH_MM = 25.f;
    
    // Resolution of images that are streamed to basestation (dev purposes)
    const Vision::CameraResolution IMG_STREAM_RES = Vision::CAMERA_RES_QQQVGA;
    
    // Number of frames to skip when streaming images to basestation
    const u8 IMG_STREAM_SKIP_FRAMES = 2;
    
  } // namespace Cozmo
} // namespace Anki


#endif // COZMO_CONFIG_H
