#ifndef COZMO_CONFIG_H
#define COZMO_CONFIG_H

#include "anki/common/constantsAndMacros.h"
#include "anki/common/types.h"
#include "anki/cozmo/robot/debug.h"
#include "anki/vision/CameraSettings.h"

namespace Anki {
  namespace Cozmo {

    /***************************************************************************
     *
     *                          Physical Robot Geometry
     *
     **************************************************************************/
    
    const f32 WHEEL_DIAMETER_MM  = 28.4f;  // This should be in sync with the CozmoBot proto file
    const f32 HALF_WHEEL_CIRCUM  = WHEEL_DIAMETER_MM * M_PI_2;
    const f32 WHEEL_RAD_TO_MM    = WHEEL_DIAMETER_MM / 2.f;  // or HALF_WHEEL_CIRCUM / PI;
    const f32 WHEEL_DIST_MM      = 47.7f; // distance b/w the front wheels
    const f32 WHEEL_DIST_HALF_MM = WHEEL_DIST_MM / 2.f;
    const f32 WHEEL_BASE_MM      = 30.f;
    
    
    // The height of the lift at various configurations
    // Actual limit in proto is closer to 20.4mm, but there is a weird
    // issue with moving the lift when it is at a limit. The lift arm
    // flies off of the robot and comes back! So for now, we just don't
    // drive the lift down that far. We also skip calibration in sim.

#ifdef SIMULATOR
    // Need a different height in simulation because of the position of the connectors
    const f32 LIFT_HEIGHT_LOWDOCK  = 29.f;
    const f32 LIFT_HEIGHT_HIGHDOCK = 73.f;
    const f32 LIFT_HEIGHT_CARRY    = 88.f;
    
    // Distance between the robot origin and the distance along the robot's x-axis
    // to the lift when it is in the low docking position.
    const f32 ORIGIN_TO_LOW_LIFT_DIST_MM = 25.f;
    const f32 ORIGIN_TO_HIGH_LIFT_DIST_MM = 16.5f;
    const f32 ORIGIN_TO_HIGH_PLACEMENT_DIST_MM = 16.f;  // TODO: Technically, this should be the same as ORIGIN_TO_HIGH_LIFT_DIST_MM

#else
    // Not the actual heights achieved by the lift, but the heights that would be achieved if there was no backlash.
    /*
    // Robot #1
    const f32 LIFT_HEIGHT_LOWDOCK  = 11.f;
    const f32 LIFT_HEIGHT_HIGHDOCK = 75.f;
    const f32 LIFT_HEIGHT_CARRY    = 98.f;
    */
    
    // Robot #2 (w/ (lightly stripped) clutch arm)
    /*
    const f32 LIFT_HEIGHT_LOWDOCK  = 26.f;
    const f32 LIFT_HEIGHT_HIGHDOCK = 75.f;
    const f32 LIFT_HEIGHT_CARRY    = 90.5f;
     */
    
    // Robot #1 (clutch-less arm)
    const f32 LIFT_HEIGHT_LOWDOCK  = 23.f;
    const f32 LIFT_HEIGHT_HIGHDOCK = 75.f;
    const f32 LIFT_HEIGHT_CARRY    = 95.f;
    
    // Distance between the robot origin and the distance along the robot's x-axis
    // to the lift when it is in the low docking position.
    const f32 ORIGIN_TO_LOW_LIFT_DIST_MM = 24.f;
    const f32 ORIGIN_TO_HIGH_LIFT_DIST_MM = 16.5f;
    const f32 ORIGIN_TO_HIGH_PLACEMENT_DIST_MM = 16.f;  // TODO: Technically, this should be the same as ORIGIN_TO_HIGH_LIFT_DIST_MM

#endif
    
    // The distance to the bridge ground marker that the robot must
    // achieve before we can consider it aligned with the bridge enough
    // to start driving straight. This should be the minimum distance that
    // the robot can reliably "dock" to the marker.
    const f32 BRIDGE_ALIGNED_MARKER_DISTANCE = 60.f;
    
    // Distance between the marker at the end of the bridge
    // and the desired pose of the robot when it is considered
    // to be off the bridge.
    const f32 MARKER_TO_OFF_BRIDGE_POSE_DIST = 80.f;
    
    // Distance between the lift shoulder joint and the lift "wrist" joint where arm attaches to fork assembly
    const f32 LIFT_ARM_LENGTH = 64.f;
    
    // Height of the lifter front assembly above the gripper, used to compute
    // the overall height of the robot when the lift is up (i.e. the height is
    // is the lift height plus this)
    const f32 LIFT_HEIGHT_ABOVE_GRIPPER = 5.f; // approximate
    
    // The lift height is defined as the height of the upper lift arm's wrist joint plus this offset.
    const f32 LIFT_FORK_HEIGHT_REL_TO_ARM_END = 0;
    
    // The height of the top of the lift crossbar with respect to the wrist joint
    const f32 LIFT_XBAR_HEIGHT_WRT_WRIST_JOINT = -22.f;
    
    // The distance along the x axis from the wrist joint to the front of the lifter plate
    const f32 LIFT_FRONT_WRT_WRIST_JOINT = 3.5f;
    
    // The height of the "fingers"
    const f32 LIFT_FINGER_HEIGHT = 3.8f;
    
    // TODO: convert to using these in degree form?
    const f32 MIN_HEAD_ANGLE = DEG_TO_RAD(-25.f);
    const f32 MAX_HEAD_ANGLE = DEG_TO_RAD( 40.f);
    
    // Head angle may exceed limits by this amount before
    // it is considered to be out of calibration.
    const f32 HEAD_ANGLE_LIMIT_MARGIN = DEG_TO_RAD(2.0f);

    // Safe head angle for the proximity sensors to be usable with the lift
    // either up or down
    const f32 HEAD_ANGLE_WHILE_FOLLOWING_PATH = -0.32f;
    
    const f32 NECK_JOINT_POSITION[3] = {-13.f, 0.f, 33.5f + WHEEL_RAD_TO_MM}; // relative to robot origin
    
    //const f32 HEAD_CAM_POSITION[3]   = {8.8f, 0.f, -6.f}; // lens face relative to neck joint
    const f32 HEAD_CAM_POSITION[3]   = {4.8f, 0.f, -6.f}; // camera/PCB interface relative to neck joint
    
    const f32 LIFT_BASE_POSITION[3]  = {-40.0f, 0.f, 29.5f + WHEEL_RAD_TO_MM}; // relative to robot origin
    
    // Amount to recede the front boundary of the robot when we don't want to include the lift.
    // TEMP: Applied by default to robot bounding box params below mostly for demo purposes
    //       so we don't prematurely delete blocks, but we may eventually want to apply it
    //       conditionally (e.g. when carrying a block)
    const f32 ROBOT_BOUNDING_X_LIFT  = 10.f;
    
    const f32 ROBOT_BOUNDING_X       = 88.f - ROBOT_BOUNDING_X_LIFT; // including gripper fingers
    const f32 ROBOT_BOUNDING_Y       = 54.2f;
    const f32 ROBOT_BOUNDING_X_FRONT = 32.1f - ROBOT_BOUNDING_X_LIFT; // distance from robot origin to front of bounding box
    const f32 ROBOT_BOUNDING_Z       = 67.7f; // from ground to top of head
    const f32 ROBOT_BOUNDING_RADIUS  = sqrtf((0.25f*ROBOT_BOUNDING_X*ROBOT_BOUNDING_X) +
                                             (0.25f*ROBOT_BOUNDING_Y*ROBOT_BOUNDING_Y));
    
    const f32 IMU_POSITION[3] = {5.8f, 0.f, -13.5f};  // relative to neck joint
  
    // TODO: This needs to be sync'd with whatever is in BlockDefinitions.h
    const f32 DEFAULT_BLOCK_MARKER_WIDTH_MM = 25.f;
    
    
    // Motor speed / accel limits
    // TODO: These were plucked out of the sky.
    const f32 MAX_HEAD_SPEED_RAD_PER_S = 1000;
    const f32 MAX_HEAD_ACCEL_RAD_PER_S2 = 10000;
    const f32 MAX_LIFT_SPEED_RAD_PER_S = 1000;
    const f32 MAX_LIFT_ACCEL_RAD_PER_S2 = 10000;

    
    /***************************************************************************
     *
     *                          Camera Calibration
     *
     **************************************************************************/
    
    const u8 NUM_RADIAL_DISTORTION_COEFFS = 4;

#ifdef SIMULATOR
    // Note that we are hard-coding an empircally-computed/calibrated focal
    // length, even for the simulated camera! So if we change the simulated FOV
    // of the camera, we need to recalibrate this focal length. This is because,
    // for completely unknown / not-understood reasons, computing the focal
    // length from fov_hor directly does not give us an accurate value for
    // the Webots camera.
    const f32 HEAD_CAM_CALIB_FOV          = 0.92f;
    const f32 HEAD_CAM_CALIB_FOCAL_LENGTH = 338.58f;
    
#else // Real robot:
    
    // TODO: Get these from calibration somehow
    const u16 HEAD_CAM_CALIB_WIDTH  = 320;
    const u16 HEAD_CAM_CALIB_HEIGHT = 240;
    
    /*
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
     */
    
    // From Calibration on June 28, 2014, Cozmo Proto 2.1, #2
    const f32 HEAD_CAM_CALIB_FOCAL_LENGTH_X = 328.87251f;
    const f32 HEAD_CAM_CALIB_FOCAL_LENGTH_Y = 331.17332f;
    const f32 HEAD_CAM_CALIB_CENTER_X       = 160.10795f;
    const f32 HEAD_CAM_CALIB_CENTER_Y       = 117.64527f;
    const f32 HEAD_CAM_CALIB_DISTORTION[NUM_RADIAL_DISTORTION_COEFFS] = {
      -0.099922561981334f,
      -0.027386549049312f,
      -0.003826021624417f,
       0.000534816869611f
    };
    
#endif // #ifdef SIMULATOR


    /***************************************************************************
     *
     *                          Timing (non-comms)
     *
     **************************************************************************/
    
    // Cozmo control loop is 200Hz.
    const s32 TIME_STEP = 5;
    
    // Basestation control loop
    const s32 BS_TIME_STEP = 60;
    
    const f32 CONTROL_DT = TIME_STEP*0.001f;

    const f32 ONE_OVER_CONTROL_DT = 1.0f/CONTROL_DT;

#ifdef SIMULATOR
    // Should be less than the timestep of offboard_vision_processing,
    // and roughly reflect image capture time.  If it's larger than or equal
    // to offboard_vision_processing timestep then we may send the same image
    // multiple times.
    const s32 VISION_TIME_STEP = 20;
#endif
    
    
    /***************************************************************************
     *
     *                          Communications
     *
     **************************************************************************/
    
    // Comms type for Basestation-robot comms
    // 0: Use TCP
    // 1: Use UDP
    #define USE_UDP_ROBOT_COMMS 1
    
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
        
    // Port on which registered robots advertise.
    const u32 ROBOT_ADVERTISING_PORT = 5100;
    
    // Port on which simulated robot should connect to (de)register for advertisement
    const u32 ROBOT_ADVERTISEMENT_REGISTRATION_PORT = 5101;
    
    // Port on which registered UI devices advertise.
    const u32 UI_ADVERTISING_PORT = 5102;
    
    // Port on which UI device should connect to (de)register for advertisement
    const u32 UI_ADVERTISEMENT_REGISTRATION_PORT = 5103;
    
    // If most recent advertisement message is older than this,
    // then it is no longer considered to be advertising.
    const f32 ROBOT_ADVERTISING_TIMEOUT_S = 0.25;

    // Time in between robot advertisements
    const f32 ROBOT_ADVERTISING_PERIOD_S = 0.03;
    
    // How frequently to send robot state messages (in number of main execution
    // loop increments).  So, 6 --> every 30ms, since our loop timestep is 5ms.
    const s32 STATE_MESSAGE_FREQUENCY = 6;
    
    // UI device server port which listens for basestation/game clients
    const u32 UI_MESSAGE_SERVER_LISTEN_PORT = 5200;
    
#if SIMULATOR
    // Channel number used by CozmoWorldComm Webots receiver
    // for robot messages bound for the basestation.
#define BASESTATION_SIM_COMM_CHANNEL 100
    
#endif

    // Resolution of images that are streamed to basestation (dev purposes)
    const Vision::CameraResolution IMG_STREAM_RES = Vision::CAMERA_RES_QVGA;
    
    // Number of frames to skip when streaming images to basestation
    const u8 IMG_STREAM_SKIP_FRAMES = 2;

    
    /***************************************************************************
     *
     *                          Poses and Planner
     *
     **************************************************************************/
    
    // A common distance threshold for pose equality comparison.
    // If two poses are this close to each other, they are considered to be equal
    // (at least in terms of translation).
    const f32 DEFAULT_POSE_EQUAL_DIST_THRESOLD_MM = 5.0f;
    
    // A common angle threshold for pose equality comparison
    // If two poses are this close in terms of angle, they are considered equal.
    const f32 DEFAULT_POSE_EQUAL_ANGLE_THRESHOLD_RAD = DEG_TO_RAD(10);
    
    
    /***************************************************************************
     *
     *                  ~ ~ ~ ~ ~ MAGIC NUMBERS ~ ~ ~ ~
     *
     ***************************************************************************/
    // Cozmo #2 seems to always dock to the right of the marker by a few mm.
    // There could be any number of factors contributing to this so for now we just adjust the y-offset of the
    // docking error signal by this much.
    const f32 COZMO2_CAM_LATERAL_POSITION_HACK = 2.f;
    
    
  } // namespace Cozmo
} // namespace Anki


#endif // COZMO_CONFIG_H
