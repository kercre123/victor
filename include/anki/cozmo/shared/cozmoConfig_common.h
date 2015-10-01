/***************************************************************************
 *
 *                          Physical Robot Geometry
 *
 **************************************************************************/

const f32 WHEEL_DIAMETER_MM  = 29.f;
const f32 HALF_WHEEL_CIRCUM  = WHEEL_DIAMETER_MM * M_PI_2;
const f32 WHEEL_RAD_TO_MM    = WHEEL_DIAMETER_MM / 2.f;  // or HALF_WHEEL_CIRCUM / PI;
const f32 WHEEL_DIST_MM      = 47.7f; // approx distance b/w the center of the front treads
const f32 WHEEL_DIST_HALF_MM = WHEEL_DIST_MM / 2.f;
const f32 WHEEL_BASE_MM      = 48.f;


const f32 MIN_HEAD_ANGLE = DEG_TO_RAD(-20.f);
const f32 MAX_HEAD_ANGLE = DEG_TO_RAD( 45.f);

// Head angle may exceed limits by this amount before
// it is considered to be out of calibration.
const f32 HEAD_ANGLE_LIMIT_MARGIN = DEG_TO_RAD(2.0f);

// Safe head angle for the proximity sensors to be usable with the lift
// either up or down
const f32 HEAD_ANGLE_WHILE_FOLLOWING_PATH = -0.32f;

// Theoretically equivalent to ORIGIN_TO_LOW_LIFT_DIST_MM...
const f32 ORIGIN_TO_LIFT_FRONT_FACE_DIST_MM = 29.f;

// The x-offset from robot origin that the robot's drive center is
// located for the treaded robot when not carrying a block.
// (If you were to model the treaded robot as a two-wheel robot,
// the drive center is the location between the two wheels)
const f32 DRIVE_CENTER_OFFSET = 0.f;

// The height of the lift at various configurations
// Actual limit in proto is closer to 20.4mm, but there is a weird
// issue with moving the lift when it is at a limit. The lift arm
// flies off of the robot and comes back! So for now, we just don't
// drive the lift down that far. We also skip calibration in sim.
const f32 LIFT_HEIGHT_LOWDOCK  = 32.f;
const f32 LIFT_HEIGHT_HIGHDOCK = 76.f;
const f32 LIFT_HEIGHT_CARRY    = 90.f;
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

// The distance along the x axis from the wrist joint to the front of the lifter plate
const f32 LIFT_FRONT_WRT_WRIST_JOINT = 4.f;

// Neck joint relative to robot origin
const f32 NECK_JOINT_POSITION[3] = {-13.f, 0.f, 34.5f + WHEEL_RAD_TO_MM};

// camera relative to neck joint
const f32 HEAD_CAM_POSITION[3]   = {17.7f, 0.f, -8.f};

// Upper shoulder joint relative to robot origin
const f32 LIFT_BASE_POSITION[3]  = {-41.0f, 0.f, 31.3f + WHEEL_RAD_TO_MM}; // relative to robot origin

// IMU position relative to neck joint
const f32 IMU_POSITION[3] = {5.8f, 0.f, -13.5f};


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
// to drive forward/reverse on to it.
const f32 CHARGER_ALIGNED_MARKER_DISTANCE = 100.f;


/***************************************************************************
 *
 *                          Camera Calibration
 *
 **************************************************************************/

const u8 NUM_RADIAL_DISTORTION_COEFFS = 4;


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


/***************************************************************************
 *
 *                          Timing (non-comms)
 *
 **************************************************************************/

// Cozmo control loop is 200Hz.
const s32 TIME_STEP = 5;

const f32 CONTROL_DT = TIME_STEP*0.001f;

const f32 ONE_OVER_CONTROL_DT = 1.0f/CONTROL_DT;


/***************************************************************************
 *
 *                          Streaming Animation
 *
 **************************************************************************/

// Streaming KeyFrame buffer size, in bytes
const s32 KEYFRAME_BUFFER_SIZE = 16384;

// Now in clad
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
