/***************************************************************************
 *
 *                          Physical Robot Geometry
 *
 **************************************************************************/

const f32 MIN_HEAD_ANGLE = DEG_TO_RAD(-25.f);
const f32 MAX_HEAD_ANGLE = DEG_TO_RAD( 35.f);

// Head angle may exceed limits by this amount before
// it is considered to be out of calibration.
const f32 HEAD_ANGLE_LIMIT_MARGIN = DEG_TO_RAD(2.0f);

// Safe head angle for the proximity sensors to be usable with the lift
// either up or down
const f32 HEAD_ANGLE_WHILE_FOLLOWING_PATH = -0.32f;

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

const s32 MAX_FACE_FRAME_SIZE = 1024;

const u32 AUDIO_SAMPLE_SIZE = 800;


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
