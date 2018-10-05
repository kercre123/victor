#ifndef COZMO_CONFIG_H
#define COZMO_CONFIG_H

#ifdef COZMO_ROBOT
#include "coretech/common/shared/types.h"
#include "anki/common/constantsAndMacros.h"
#else
#include "coretech/common/shared/types.h"
#include "util/math/math.h"
#endif

#include <math.h>

#ifndef ANKI_WHISKEY
#define ANKI_WHISKEY 0
#endif

namespace Anki {
namespace Vector {

  /***************************************************************************
   *
   *                          Physical Robot Geometry
   *
   **************************************************************************/
  const f32 WHEEL_DIAMETER_MM  = 29.f;
  const f32 HALF_WHEEL_CIRCUM  = WHEEL_DIAMETER_MM * M_PI_2_F;
  const f32 WHEEL_RAD_TO_MM    = WHEEL_DIAMETER_MM / 2.f;  // or HALF_WHEEL_CIRCUM / PI;
  const f32 WHEEL_DIST_MM      = 46.f; // approx distance b/w the center of the front treads
  const f32 WHEEL_DIST_HALF_MM = WHEEL_DIST_MM / 2.f;
  const f32 WHEEL_BASE_MM      = 48.f;
  
  // Tolerances on commanded target angles
  // i.e. If motor is within this tolerance of the target angle, it is done moving.
  const f32 HEAD_ANGLE_TOL       = DEG_TO_RAD(2.f);
  const f32 LIFT_ANGLE_TOL       = DEG_TO_RAD(1.5f);
  
  const f32 MIN_HEAD_ANGLE = DEG_TO_RAD(-22.f);
  const f32 MAX_HEAD_ANGLE = DEG_TO_RAD( 45.f);
  
  const f32 kIdealViewBlockHeadAngle = DEG_TO_RAD(-17.5f);
  const f32 kIdealViewBlockLiftUpHeadAngle = DEG_TO_RAD(-22.5f);
  const f32 kIdealViewFaceHeadAngle = DEG_TO_RAD(12.5f);
  
  // Head angle may exceed limits by this amount before
  // it is considered to be out of calibration.
  const f32 HEAD_ANGLE_LIMIT_MARGIN = DEG_TO_RAD(3.f);
  
  // Safe head angle for the proximity sensors to be usable with the lift
  // either up or down
  const f32 HEAD_ANGLE_WHILE_FOLLOWING_PATH = DEG_TO_RAD(-15.f);
  
  // Theoretically equivalent to ORIGIN_TO_LOW_LIFT_DIST_MM...
  const f32 ORIGIN_TO_LIFT_FRONT_FACE_DIST_MM = 29.f;
  
  // The x-offset from robot origin that the robot's drive center is
  // located for the treaded robot when not carrying a block.
  // (If you were to model the treaded robot as a two-wheel robot,
  // the drive center is the location between the two wheels)
  const f32 DRIVE_CENTER_OFFSET = -20.f;
  
  // Forward distance sensor measurements (TODO: finalize these dimensions on production robot)
  const float kProxSensorTiltAngle_rad = DEG_TO_RAD(6.5f);    // Angle that the prox sensor is tilted (upward is positive)
  const float kProxSensorPosition_mm[3] = {10.f, 0.f, 16.f};  // With respect to robot origin
  const float kProxSensorFullFOV_rad = DEG_TO_RAD(25.f);      // Full Field of View (FOV) of the sensor cone
  
  // The height of the lift at various configurations
  // Actual limit in proto is closer to 20.4mm, but there is a weird
  // issue with moving the lift when it is at a limit. The lift arm
  // flies off of the robot and comes back! So for now, we just don't
  // drive the lift down that far. We also skip calibration in sim.
  const f32 LIFT_PROTO_MIN_HEIGHT                 = 20.4f;
  const f32 LIFT_HEIGHT_LOWDOCK                   = 32.f; // For interfacing with a cube that is on the ground.
  const f32 LIFT_HEIGHT_HIGHDOCK                  = 76.f; // For interfacing with a cube that is stacked on top of another cube.
  const f32 LIFT_HEIGHT_CARRY                     = 92.f; // Cube carrying height.
  const f32 LIFT_HEIGHT_LOW_ROLL                  = 68.f; // For rolling a cube that is on the ground.
  const f32 LIFT_HEIGHT_ABOVE_PROX                = 59.f; // Just above prox sensor. Useful for driving in cluttered spaces.
  
  // Distance between the lift shoulder joint and the lift "wrist" joint where arm attaches to fork assembly
  const f32 LIFT_ARM_LENGTH = 66.f;
  
  // Height of the lifter front assembly above the gripper, used to compute
  // the overall height of the robot when the lift is up (i.e. the height is
  // is the lift height plus this)
  const f32 LIFT_HEIGHT_ABOVE_GRIPPER = 5.f; // approximate
  
  // The lift height is defined as the height of the upper lift arm's wrist joint plus this offset.
  const f32 LIFT_FORK_HEIGHT_REL_TO_ARM_END = 0;
  
  // The height of the top of the lift crossbar with respect to the wrist joint
  const f32 LIFT_XBAR_HEIGHT_WRT_WRIST_JOINT = -20.f;
  // Lift xbar height from top (where tool codes are) to bottom (the other side, the one at the bottom)
  const f32 LIFT_XBAR_HEIGHT = 8.5f; // measured manually (not in dimensions sheet)
  // Width of cross bar inside the two vertical sides (approximate)
  const f32 LIFT_XBAR_WIDTH = 30.f;
  // The height of the bottom of the lift crossbar with respect to the wrist joint.
  // This is also the lift's overall bottom wrt wrist
  const f32 LIFT_XBAR_BOTTOM_WRT_WRIST_JOINT = LIFT_XBAR_HEIGHT_WRT_WRIST_JOINT-LIFT_XBAR_HEIGHT;
  
  // The distance along the x axis from the wrist joint to the front of the lifter plate
  const f32 LIFT_FRONT_WRT_WRIST_JOINT = 4.f;
  // The distance along the x axis from the wrist joint to the back of the lifter plate
  const f32 LIFT_BACK_WRT_WRIST_JOINT = -1.f; // rsam: approx
  
  // I added this var because I have seen that physical robots have some slack that causes their lifts to
  // fall a little with respect to the position they should hold. This fact prevented accurate calculation
  // of where the borders of the lift would be in the camera. Applying this offset to the bottom of the lift,
  // provides a threshold where we expect the lift to be. This variable was calculated by hand, so it's up to
  // tweaks if different evidence is found.
  const f32 LIFT_HARDWARE_FALL_SLACK_MM = 8.0f;
  
  // Neck joint relative to robot origin
  const f32 NECK_JOINT_POSITION[3] = {-13.f, 0.f, 34.5f + WHEEL_RAD_TO_MM};
  
  // camera relative to neck joint
  const f32 HEAD_CAM_POSITION[3]   = {18.84f, 0.f, -7.96f};
  
  // Upper shoulder joint relative to robot origin
  const f32 LIFT_BASE_POSITION[3]  = {-41.0f, 0.f, 30.5f + WHEEL_RAD_TO_MM}; // relative to robot origin
  
  // IMU position relative to neck joint
  const f32 IMU_POSITION[3] = {5.8f, 0.f, -13.5f};
  
  // Face LCD Screen size
  const f32 SCREEN_SIZE[2] = {26.f, 13.f};
  
  // Face display resolution, in pixels
  const s32 FACE_DISPLAY_WIDTH = 184;
  const s32 FACE_DISPLAY_HEIGHT = 96;
  const s32 FACE_DISPLAY_NUM_PIXELS = FACE_DISPLAY_WIDTH * FACE_DISPLAY_HEIGHT;

  // Common conversion functionality for lift height
  #define ConvertLiftHeightToLiftAngleRad(height_mm) asinf((CLIP(height_mm, LIFT_HEIGHT_LOWDOCK, LIFT_HEIGHT_CARRY) \
                                                     - LIFT_BASE_POSITION[2] - LIFT_FORK_HEIGHT_REL_TO_ARM_END)/LIFT_ARM_LENGTH)
  #define ConvertLiftAngleToLiftHeightMM(angle_rad) ((sinf(angle_rad) * LIFT_ARM_LENGTH) \
                                                    + LIFT_BASE_POSITION[2] + LIFT_FORK_HEIGHT_REL_TO_ARM_END)
  
  const f32 MIN_LIFT_ANGLE = ConvertLiftHeightToLiftAngleRad(LIFT_HEIGHT_LOWDOCK);
  #if ANKI_WHISKEY
  const f32 MAX_LIFT_ANGLE = MIN_LIFT_ANGLE + DEG_TO_RAD(190);
  #else
  const f32 MAX_LIFT_ANGLE = ConvertLiftHeightToLiftAngleRad(LIFT_HEIGHT_CARRY);
  #endif  
  
  // deadband around the min and max lift-angles beyond which we are considered not calibrated
  const f32 LIFT_ANGLE_LIMIT_MARGIN = DEG_TO_RAD(5.f);
  
  /***************************************************************************
   *
   *                          Geometries
   *
   **************************************************************************/
  
  
  // TODO: This needs to be sync'd with whatever is in BlockDefinitions.h
  const f32 DEFAULT_BLOCK_MARKER_WIDTH_MM = 25.f;  
  
  // Distance to the charger ramp marker that the robot must
  // achieve before we can consider it aligned with the charger enough
  // to reverse on to it.
  const f32 CHARGER_ALIGNED_MARKER_DISTANCE = 140.f;
  
  // Slope of the charger platform w.r.t. the ground
  const f32 kChargerSlopeAngle_rad = DEG_TO_RAD(7.0f);
  
  
  /***************************************************************************
   *
   *                          Camera
   *
   **************************************************************************/
  
  const u8 NUM_RADIAL_DISTORTION_COEFFS = 8;

  const u16 DEFAULT_CAMERA_RESOLUTION_WIDTH  = 640;
  const u16 DEFAULT_CAMERA_RESOLUTION_HEIGHT = 360;

  const u16 CAMERA_SENSOR_RESOLUTION_WIDTH  = 1280;
  const u16 CAMERA_SENSOR_RESOLUTION_HEIGHT = 720;
  
  const f32 MIN_CAMERA_EXPOSURE_TIME_MS = 1;
  const f32 MAX_CAMERA_EXPOSURE_TIME_MS = 66;
  
  // Range for exposure and white balance gains
  const f32 MIN_CAMERA_GAIN = 0.25f; // Real min should be 0.1, but using 0.25 as a bandaid for VIC-6653
  const f32 MAX_CAMERA_GAIN = 3.8f;
  
  /***************************************************************************
   *
   *                          Cliff Sensor
   *
   **************************************************************************/

  // Cliff detection thresholds (these come from testing with DVT1 robots - will need
  // to be adjusted for production hardware)
  #if FACTORY_TEST
  const u16 CLIFF_SENSOR_THRESHOLD_MAX = 180;
  #else
  const u16 CLIFF_SENSOR_THRESHOLD_MAX = 40;
  #endif
  const u16 CLIFF_SENSOR_THRESHOLD_MIN = 15;
  const u16 CLIFF_SENSOR_THRESHOLD_DEFAULT = CLIFF_SENSOR_THRESHOLD_MAX;
  
  // Cliff sensor value must rise this far above the threshold to 'untrigger' cliff detection.
  const u16 CLIFF_DETECT_HYSTERESIS = 30;
  
  // V2 cliff sensors (assumes 4 cliff sensors are arranged in a rectangle symmetric about the robot x axis)
  // NOTE: These values are approximate and should be verified for final V2 design.
  const f32 kCliffSensorYOffset_mm      = 14.f;  // y (lateral) distance from robot origin to the cliff sensors
  const f32 kCliffSensorXOffsetFront_mm = 2.f;   // x (longitudinal) offset from robot origin to front cliff sensors
  const f32 kCliffSensorXOffsetRear_mm  = -50.f; // x (longitudinal) offset from robot origin to rear cliff sensors
  

  // The minimum value expected of cliff sensor when
  // it's detecting a white line in the habitat
  // TODO (VIC-3550): Merge with kChargerCliffBlackThreshold?
  const u16 MIN_CLIFF_STOP_ON_WHITE_VAL_HIGH = 400;
  
  // if cliff-alignment fails, we may wish to retry cliff alignment
  // with a lowered threshold in case we are dealing with values
  // that are marginally lower than the threshold
  // note: this is to counteract variability in appearance of
  // habitat white w.r.t. individual robots
  const u16 MIN_CLIFF_STOP_ON_WHITE_VAL_LOW = 350;

  // Amount below MIN_CLIFF_STOP_ON_WHITE_VAL at which a white
  // value is undetected
  const u16 CLIFF_STOP_ON_WHITE_HYSTERSIS = 50;

  // In calm mode, cliffs are never detected
  const u16 CLIFF_CALM_MODE_VAL = 1000;

  // In calm mode, prox sensor returns this distance reading
  // but it's also of negative signal quality so it should never be used
  const u16 PROX_CALM_MODE_DIST_MM = 1000;

  /***************************************************************************
   *
   *                          Speeds and Accels
   *
   **************************************************************************/
  
  // Motor speed / accel limits
  // TODO: These were plucked out of the sky.
  const f32 MAX_HEAD_SPEED_RAD_PER_S = 1000;
  const f32 MAX_HEAD_ACCEL_RAD_PER_S2 = 10000;
  const f32 MAX_LIFT_SPEED_RAD_PER_S = 1000;
  const f32 MAX_LIFT_ACCEL_RAD_PER_S2 = 10000;
  
  // How fast (in mm/sec) can a wheel spin at max
  const f32 MAX_WHEEL_SPEED_MMPS = 220.f;
  const f32 MAX_WHEEL_ACCEL_MMPS2 = 10000.f;  // TODO: Actually measure this!
  
  // How fast (in mm/sec) can the robot drive without falling off 
  // (most) straight edge cliffs
  const f32 MAX_SAFE_WHEEL_SPEED_MMPS = 170.f;

  // How fast (in mm/sec) can the robot drive without falling off 
  // (most) straight edge cliffs while carrying a cube
  const f32 MAX_SAFE_WHILE_CARRYING_WHEEL_SPEED_MMPS = 60.f;

  // Maximum angular velocity
  // Determined experimentally by turning robot in place at max speed.
  // It can actually spin closer to 360 deg/s, but we use a conservative limit to be sure
  // the robot can actually achieve this top speed. If it can't, point turns could look jerky because
  // the robot can't keep up with the rotation profile.
  // Ideally, speed, in radians, should be (MAX_WHEEL_SPEED_MMPS / WHEEL_DIST_HALF_MM), but tread slip makes this not true.
  const f32 MAX_BODY_ROTATION_SPEED_DEG_PER_SEC = 300;
  const f32 MAX_BODY_ROTATION_SPEED_RAD_PER_SEC = DEG_TO_RAD(MAX_BODY_ROTATION_SPEED_DEG_PER_SEC);
  const f32 MAX_BODY_ROTATION_ACCEL_RAD_PER_SEC2 = MAX_BODY_ROTATION_SPEED_RAD_PER_SEC * MAX_WHEEL_ACCEL_MMPS2 / MAX_WHEEL_SPEED_MMPS;
  const f32 MAX_BODY_ROTATION_ACCEL_DEG_PER_SEC2 = RAD_TO_DEG(MAX_BODY_ROTATION_ACCEL_RAD_PER_SEC2);
  
  
  /***************************************************************************
   *
   *                          Timing (non-comms)
   *
   **************************************************************************/
  
  // Cozmo control loop is 200Hz.
  const s32 ROBOT_TIME_STEP_MS = 5;
  
  const f32 CONTROL_DT = ROBOT_TIME_STEP_MS * 0.001f;
  
  const f32 ONE_OVER_CONTROL_DT = 1.0f/CONTROL_DT;
  
  // how long there is between stopping the motors and issuing a cliff event (because we have decided there isn't a pickup event)
  const u32 CLIFF_EVENT_DELAY_MS = 500;
  
  // Anim process timing consts
  const u32 ANIM_TIME_STEP_MS = 33;
  const u32 ANIM_TIME_STEP_US = ANIM_TIME_STEP_MS * 1000;
  const s32 ANIM_OVERTIME_WARNING_THRESH_MS = 5;
  const s32 ANIM_OVERTIME_WARNING_THRESH_US = ANIM_OVERTIME_WARNING_THRESH_MS * 1000;
  
  // Web server process timing consts; much more lax
  const u32 WEB_SERVER_TIME_STEP_MS = 100;
  const u32 WEB_SERVER_TIME_STEP_US = WEB_SERVER_TIME_STEP_MS * 1000;
  const s32 WEB_SERVER_OVERTIME_WARNING_THRESH_MS = 500;
  const s32 WEB_SERVER_OVERTIME_WARNING_THRESH_US = WEB_SERVER_OVERTIME_WARNING_THRESH_MS * 1000;
  
  // Time step for cube tick
  const s32 CUBE_TIME_STEP_MS = 10;
  
  // Timestep for cube animation LED 'frames'
  const u32 CUBE_LED_FRAME_LENGTH_MS = 30;
  
  /***************************************************************************
   *
   *                          Communications
   *
   **************************************************************************/
  
  const u32 MAX_SENT_BYTES_PER_TIC_TO_ROBOT = 200;
  const u32 MAX_SENT_BYTES_PER_TIC_TO_UI = 0;
  
  // Packet headers/footers:
  // TODO: Do we need this?  Only used in simulation I think? (Add #ifdef SIMULATOR?)
  const u8 RADIO_PACKET_HEADER[2] = {0xBE, 0xEF};
  const u8 RADIO_PACKET_FOOTER[2] = {0xFF, 0x0F};
  
  // The base listening port for robot UDP server.
  // Each robot listens on port (ROBOT_RADIO_BASE_PORT + ROBOT_ID)
  const u16 ROBOT_RADIO_BASE_PORT = 5551;
  
  // The base listening port for anim process UDP server
  const u16 ANIM_PROCESS_SERVER_BASE_PORT = 5600;
  
  // Port on which registered UI devices advertise.
  const u32 UI_ADVERTISING_PORT = 5102;
  
  // Port on which UI device should connect to (de)register for advertisement
  const u32 UI_ADVERTISEMENT_REGISTRATION_PORT = 5103;

  const u32 SWITCHBOARD_TCP_PORT = 5107;
  
  // If most recent advertisement message is older than this,
  // then it is no longer considered to be advertising.
  const f32 ROBOT_ADVERTISING_TIMEOUT_S = 0.25;
  
  // Time in between robot advertisements
  const f32 ROBOT_ADVERTISING_PERIOD_S = 0.03f;
  
  // How frequently to send robot state messages (in number of main execution
  // loop increments).  So, 6 --> every 30ms, since our loop timestep is 5ms.
  const s32 STATE_MESSAGE_FREQUENCY = 6;

  // How frequently to send state messages in calm mode
  const s32 STATE_MESSAGE_FREQUENCY_CALM = 50;

  // UI device server port which listens for basestation/game clients
  const u32 UI_MESSAGE_SERVER_LISTEN_PORT = 5200;

  // Number of frames to skip when streaming images to basestation
  const u8 IMG_STREAM_SKIP_FRAMES = 2;

  // Default robot ID
  // Do not change this! It affects which ports are binded to.
  const u32 DEFAULT_ROBOT_ID = 0;

  // Defines whether animation process should broadcast the final face image (after processing/scan line application etc)
  // back to engine so that it can maintain an accurate reperesentation of robot state
  // THIS IS CURRENTLY USED FOR R&D ONLY - CONSULT KEVIN YOON ABOUT PERFORMANCE BEFORE TURNING
  // THIS ON IN MASTER
  #define SHOULD_SEND_DISPLAYED_FACE_TO_ENGINE false
  
} // namespace Vector
} // namespace Anki

#endif // COZMO_CONFIG_H
