#ifndef COZMO_CONFIG_H
#define COZMO_CONFIG_H

#include "anki/types.h"

#ifdef COZMO_ROBOT
#include "anki/common/constantsAndMacros.h"
#else
#include "util/math/math.h"
#endif


namespace Anki {
namespace Cozmo {

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
  
  const f32 MIN_HEAD_ANGLE = DEG_TO_RAD(-25.f);
  const f32 MAX_HEAD_ANGLE = DEG_TO_RAD( 44.5f);
  
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
  
  // Length of the forward collision sensor (with respect to origin)
  const u8 FORWARD_COLLISION_SENSOR_LENGTH_MM = 160;
  
  // The height of the lift at various configurations
  // Actual limit in proto is closer to 20.4mm, but there is a weird
  // issue with moving the lift when it is at a limit. The lift arm
  // flies off of the robot and comes back! So for now, we just don't
  // drive the lift down that far. We also skip calibration in sim.
  const f32 LIFT_HEIGHT_LOWDOCK  = 32.f;
  const f32 LIFT_HEIGHT_HIGHDOCK = 76.f;
  const f32 LIFT_HEIGHT_CARRY    = 92.f;
  const f32 LIFT_HEIGHT_LOW_ROLL = 68.f;
  
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
  const f32 HEAD_CAM_POSITION[3]   = {17.52f, 0.f, -8.f};
  
  // Upper shoulder joint relative to robot origin
  const f32 LIFT_BASE_POSITION[3]  = {-41.0f, 0.f, 30.5f + WHEEL_RAD_TO_MM}; // relative to robot origin
  
  // IMU position relative to neck joint
  const f32 IMU_POSITION[3] = {5.8f, 0.f, -13.5f};
  
  // Face LCD Screen size
  const f32 SCREEN_SIZE[2] = {26.f, 13.f};
  
  /***************************************************************************
   *
   *                          Geometries
   *
   **************************************************************************/
  
  
  // TODO: This needs to be sync'd with whatever is in BlockDefinitions.h
  const f32 DEFAULT_BLOCK_MARKER_WIDTH_MM = 25.f;
  
  // The distance to the bridge ground marker that the robot must
  // achieve before we can consider it aligned with the bridge enough
  // to start driving straight. This should be the minimum distance that
  // the robot can reliably "dock" to the marker.
  const f32 BRIDGE_ALIGNED_MARKER_DISTANCE = 60.f;
  
  // Distance between the marker at the end of the bridge
  // and the desired pose of the robot when it is considered
  // to be off the bridge.
  const f32 MARKER_TO_OFF_BRIDGE_POSE_DIST = 80.f;
  
  
  // Distance to the charger ramp marker that the robot must
  // achieve before we can consider it aligned with the charger enough
  // to reverse on to it.
  const f32 CHARGER_ALIGNED_MARKER_DISTANCE = 140.f;
  
  
  /***************************************************************************
   *
   *                          Camera Calibration
   *
   **************************************************************************/
  
  const u8 NUM_RADIAL_DISTORTION_COEFFS = 8;
  
  
  /***************************************************************************
   *
   *                          Cliff Sensor
   *
   **************************************************************************/
  
  // Default cliff detection threshold
  const u32 CLIFF_SENSOR_DROP_LEVEL = 400;
  
  // Cliff un-detection threshold (hysteresis)
  const u32 CLIFF_SENSOR_UNDROP_LEVEL = 600;
  
  
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
  
  // Maximum angular velocity
  // Determined experimentally by turning robot in place at max speed.
  // It can actually spin closer to 360 deg/s, but we use a conservative limit to be sure
  // the robot can actually achieve this top speed. If it can't, point turns could look jerky because
  // the robot can't keep up with the rotation profile.
  // Ideally, speed, in radians, should be (MAX_WHEEL_SPEED_MMPS / WHEEL_DIST_HALF_MM), but tread slip makes this not true.
  const f32 MAX_BODY_ROTATION_SPEED_DEG_PER_SEC = 300;
  const f32 MAX_BODY_ROTATION_SPEED_RAD_PER_SEC = DEG_TO_RAD(MAX_BODY_ROTATION_SPEED_DEG_PER_SEC);
  const f32 MAX_BODY_ROTATION_ACCEL_RAD_PER_SEC2 = MAX_BODY_ROTATION_SPEED_RAD_PER_SEC * MAX_WHEEL_ACCEL_MMPS2 / MAX_WHEEL_SPEED_MMPS;
  
  
  /***************************************************************************
   *
   *                          Timing (non-comms)
   *
   **************************************************************************/
  
  // Cozmo control loop is 200Hz.
  const s32 TIME_STEP = 5;
  
  const f32 CONTROL_DT = TIME_STEP*0.001f;
  
  const f32 ONE_OVER_CONTROL_DT = 1.0f/CONTROL_DT;
  
  // how long there is between stopping the motors and issuing a cliff event (because we have decided there isn't a pickup event)
  const u32 CLIFF_EVENT_DELAY_MS = 500;
  
  /***************************************************************************
   *
   *                          Streaming Animation
   *
   **************************************************************************/
  
  // Now in clad
  // Streaming KeyFrame buffer size, in bytes
  //const s32 KEYFRAME_BUFFER_SIZE = 16384;
  //const s32 MAX_FACE_FRAME_SIZE = 1024;
  //const u32 AUDIO_SAMPLE_SIZE = 800;
  
  /***************************************************************************
   *
   *                          Communications
   *
   **************************************************************************/
  
  // Comms type for Basestation-robot comms
  // 0: Use TCP
  // 1: Use UDP
#define USE_UDP_ROBOT_COMMS 1
  
  // Comms types for UI-game comms
#define USE_UDP_UI_COMMS 1
  
  const u32 MAX_SENT_BYTES_PER_TIC_TO_ROBOT = 200;
  const u32 MAX_SENT_BYTES_PER_TIC_TO_UI = 0;
  
  // Packet headers/footers:
  // TODO: Do we need this?  Only used in simulation I think? (Add #ifdef SIMULATOR?)
  const u8 RADIO_PACKET_HEADER[2] = {0xBE, 0xEF};
  const u8 RADIO_PACKET_FOOTER[2] = {0xFF, 0x0F};
  
  // The base listening port for robot TCP server.
  // Each robot listens on port (ROBOT_RADIO_BASE_PORT + ROBOT_ID)
  const u16 ROBOT_RADIO_BASE_PORT = 5551;
  
  /*
   THESE LATENCY VALUES ARE NOT BEING USED -- SEE ALSO multiClientChannel.h
   
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
   */
  
  // Header required at front of all AdvertisementRegistrationMsg CLAD messages sent to a Robot Ad Service
  const u8 ROBOT_ADVERTISING_HEADER_TAG = 0xCA;
  
  // Rate at which the robot advertises itself
  const u32 ROBOT_ADVERTISING_PERIOD_MS = 100;
  
  // Port on which registered robots advertise.
  const u32 ROBOT_ADVERTISING_PORT = 5100;
  
  // Port on which simulated robot should connect to (de)register for advertisement
  const u32 ROBOT_ADVERTISEMENT_REGISTRATION_PORT = 5101;
  
  // Port on which registered UI devices advertise.
  const u32 UI_ADVERTISING_PORT = 5102;
  
  // Port on which UI device should connect to (de)register for advertisement
  const u32 UI_ADVERTISEMENT_REGISTRATION_PORT = 5103;
  
  // Port on which registered SDK devices advertise.
  const u32 SDK_ADVERTISING_PORT = 5104;
  
  // Port on which SDK device should connect to (de)register for advertisement
  const u32 SDK_ADVERTISEMENT_REGISTRATION_PORT = 5105;
  
  // Port for TCP/IP based version of SDK to communicate over
  const u32 SDK_ON_DEVICE_TCP_PORT = 5106;
  // See SDK_ON_COMPUTER_TCP_PORT in engineInterface.py for corresponding port on attached PC
  
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
  
  
  
  // Number of frames to skip when streaming images to basestation
  const u8 IMG_STREAM_SKIP_FRAMES = 2;
  
} // namespace Cozmo
} // namespace Anki

#endif // COZMO_CONFIG_H
