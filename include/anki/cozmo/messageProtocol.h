/**
 * File: cozmoMsgProtocol.h
 * 
 * Author: Kevin Yoon 
 * Created: 9/24/2013 
 *
 * Description: The file contains all the low level structs that
 *              define the messaging protocol. This is only a
 *              header file that is shared between the robot and
 *              the basestation.
 *
 * Copyright: Anki, Inc. 2013
 **/

#ifndef COZMO_MSG_PROTOCOL_H
#define COZMO_MSG_PROTOCOL_H

#include "anki/common/types.h"
    
// Base message size (just size byte)
#define SIZE_MSG_BASE_SIZE 1

// Basestation defines
#define MSG_FIRST_DATA_BYTE_IDX_BASESTATION 2

// Expected message receive latency
// It is assumed that this value does not fluctuate greatly.
// The more inaccurate this value is, the more invalid our
// handling of messages will be.
#define MSG_RECEIVE_LATENCY_SEC 0.03

// The effective latency of vehicle messages for basestation modelling purposes
// This is twice the MSG_RECEIVE_LATENCY_SEC so that the basestation maintains a model
// of the system one message cycle latency in the future. This way, commanded actions are applied
// at the time they are expected in the physical world.
#define BASESTATION_MODEL_LATENCY_SEC (2*MSG_RECEIVE_LATENCY_SEC)


////////// Simulator comms //////////

// Channel number used by CozmoWorldComm Webots receiver
// for robot messages bound for the basestation.
#define BASESTATION_SIM_COMM_CHANNEL 100

// Port on which CozmoWorldComms is listening for a connection from basestation.
#define COZMO_WORLD_LISTEN_PORT "5555"

// First two bytes of message which prefixes normal messages
#define COZMO_WORLD_MSG_HEADER_BYTE_1 (0xbe)
#define COZMO_WORLD_MSG_HEADER_BYTE_2 (0xef)

// Size of CozmoWorldMessage header.
// Includes COZMO_WORLD_MSG_HEADER_BYTE_1 and COZMO_WORLD_MSG_HEADER_BYTE_2 and 1 byte for robotID.
#define COZMO_WORLD_MSG_HEADER_SIZE 3

////////// End Simulator comms //////////



///////////////////////////////////////////////////////////////////////////////
//                      *** Message ID description ***
// 
// MSG_B2V_* -- Message from basestation to vehicle
// MSG_V2B_* -- Message from vehicle to basestation
// 
// Messages are in range of [0,255]. Reserved message blocks for categories:
// 
//  0  -  9  -- MSG_*2*_BOOT_* -- Bootloader-related messages
// 10  - 19  -- MSG_*2*_BTLE_* -- BTLE-related messages
// 20  - 119 -- MSG_*2*_CORE_* -- Messages related to core functionality (used
//                                in standard product use)
// 120 - 229 -- MSG_*2*_DEV_*  -- Development/diagnostics/testing messages. Used
//                                throughout development, manufacturing,
//                                diagnostics, logging, etc.
// 230 - 255 -- MSG_*2*_TMP_*  -- Messages for temporary use (development, etc.)
///////////////////////////////////////////////////////////////////////////////

typedef enum {

  ///////////////////////////////////////////////////////////////////////////////
  // Bootloader messages (0-9)
  ///////////////////////////////////////////////////////////////////////////////
/*
  MSG_B2V_BOOT_ENTER_BOOTLOADER = 0,  // Prepare to receive new firmware
  MSG_B2V_BOOT_REQUEST_START = 1,     // Try to re-run the application (after a crash)
  
  MSG_V2B_BOOT_REQUEST_CHUNK = 3,     // Request the next chunk of firmware
  MSG_V2B_BOOT_FLASH_UPDATED = 5,     // The new firmware is now runing
  
  MSG_V2B_BOOT_CRASH_REPORT = 6,      // The firmware has crashed or is incomplete
*/
  ///////////////////////////////////////////////////////////////////////////////
  // BTLE messages (10-19)
  ///////////////////////////////////////////////////////////////////////////////
/*
  MSG_B2V_BTLE_CHANGE_CONNECTION_PARAMS = 10,
  MSG_B2V_BTLE_CONNECTION_PARAMS_REQUEST = 11,
  MSG_V2B_BTLE_CONNECTION_PARAMS_RESPONSE = 12,
  MSG_B2V_BTLE_DISCONNECT = 13,
*/



  ///////////////////////////////////////////////////////////////////////////////
  // Core functionality messages (20-119)
  ///////////////////////////////////////////////////////////////////////////////
/*
  // Vehicle added or lost
  MSG_V2B_CORE_NODE_LOST = 20,   
  MSG_V2B_CORE_NODE_JOINED, 

  // Ping request / response
  MSG_B2V_CORE_PING_REQUEST,
  MSG_V2B_CORE_PING_RESPONSE,

  // Messages for checking vehicle code version info
  MSG_B2V_CORE_REQUEST_VEHICLE_VERSION,
  MSG_V2B_CORE_SEND_VEHICLE_VERSION_1,

  // Battery state messages
  MSG_B2V_CORE_BATTERY_VOLTAGE_REQUEST,
  MSG_V2B_CORE_BATTERY_VOLTAGE_RESPONSE,

  // Tell the vehicle to shut down (done playing)
  MSG_B2V_CORE_FORCE_VEHICLE_SHUTDOWN,
  
  //Message to set high-level light commands (headlights, brake-lights, etc.)
  MSG_B2V_CORE_SET_LIGHTS,

  //Evertime we change a state, the vehicle should let the basestation know (from normal to charging, etc...)
  MSG_V2B_CORE_STATE_CHANGED,

  //Basestation can send this message to a car to forcew a system state change (For example delocalized, direct access, etc.)
  MSG_B2V_CORE_SET_SYSTEM_STATE,
*/


  // ** Initialization related messages
/*
  // Tells the car which mode it's in (RC, computer controlled, etc). If
  // computer controlled, will behave differently when delocalized.
  MSG_B2V_CORE_SET_OPERATING_MODE,

  // Sent from car to basestation to tell it that it's initialization (mode,
  // trackID) is complete
  MSG_V2B_CORE_INIT_COMPLETE,
*/



  // ** Motion related commands **

  // Command a speed and curvature
  MSG_B2V_CORE_SET_MOTION,

  // ** Path related commands **
  MSG_B2V_CORE_CLEAR_PATH,
  MSG_B2V_CORE_SET_PATH_SEGMENT_LINE,
  MSG_B2V_CORE_SET_PATH_SEGMENT_ARC,


  // ** Localization related commands
/*
  // Absolute position update from vehicle: position / location ID 
  // TODO: Add direction 
  MSG_V2B_CORE_LOCALIZATION_POSITION_UPDATE,

  //Relative position update (offset from last known code, stop bar, transition bar, etc.)
  // TODO: Add direction; merge with transition / intersection updates?
  MSG_V2B_CORE_LOCALIZATION_RELATIVE_POSITION_UPDATE,
*/  
  
  // Message from car to basestation telling it that it's detected that it left
  // the road.  Puts it into a delocalization mode until it's relocalized again
  // (signified by a road piece index update from basestation).
  MSG_V2B_CORE_VEHICLE_DELOCALIZED,
  
  // ** Vision system commands
  
  // Robot saw a BlockMarker with its head camera
  //   Message will contain the block and face types, as well as the
  //   four corners of the square fiducial (as observed under perspective
  //   distortion)
  MSG_V2B_CORE_BLOCK_MARKER_OBSERVED,
  
  // Robot saw a MatMarker with its downward-facing camera
  //   Message will contain the identified mat square (X,Y), the image
  //   coordinates of the marker's center and the angle at which it was seen
  //   in the image.
  MSG_V2B_CORE_MAT_MARKER_OBSERVED,
  


  // ** Playback system related commands
/*
  // Clear existing playback sequence definitions on vehicle
  MSG_B2V_CORE_CLEAR_PLAYBACK_SEQUENCES,
  // Message from basestation to load a set of playback commands into the car
  MSG_B2V_CORE_SET_PLAYBACK_ACTIONS,
  // Message from basestation to tell the car to start or stop playback of the 
  // action sequences
  MSG_B2V_CORE_STARTSTOP_PLAYBACK_ACTION,

  // Message to set the default light pattern
  MSG_B2V_CORE_SET_DEFAULT_LIGHTS,
*/
  
/*
  // ** Game Flash Messages (for storing upgrades on vehicle) **
  // Specifies an 8-bit offset in the GameFlash area from which the Basestation wants to read 16 bytes of data.
  MSG_B2V_CORE_READ_FLASH_REQUEST,
  // The vehicle's response to MSG_B2V_CORE_READ_FLASH_REQUEST. Contains 16 bytes of flash data and the offset from which it is retrieved
  MSG_V2B_CORE_READ_FLASH_RESPONSE,
  // Specifies and 8-bit offset in the GameFlash area into which the Basestation wants to write 16 bytes of data. The data is also contained in the message.
  MSG_B2V_CORE_WRITE_FLASH,
*/

/*  
  // Updates the phone with general vehicle status. 
  // This message will be sent upon first connecting to a phone
  // and whenever any of the status fields change.
  MSG_V2B_CORE_STATUS_UPDATE,

  // Turn on/off test mode
  MSG_B2V_CORE_SET_TEST_MODE,
*/

  ///////////////////////////////////////////////////////////////////////////////
  // Development / diagnostics messages (120-229)
  ///////////////////////////////////////////////////////////////////////////////
/*
  // Scan request and response (response broken up in to chunks
  MSG_B2V_CORE_LAST_CORE_MSG,
  MSG_B2V_DEV_REQUEST_SCAN = 120,
  MSG_V2B_DEV_SCAN_RESPONSE = 121,

  MSG_B2V_DEV_SET_CAMERA_PARAMS = 122,

  // CHANGED
  MSG_B2V_DEV_SET_STEERING_GAINS = 123,

  //Sets the gains for the PID wheel controller, they are the same for both wheels
  MSG_B2V_DEV_SET_WHEEL_GAINS = 124,

  //Sets a desired speed for each wheel in mm/sec (forward speed)
  MSG_B2V_DEV_SET_WHEEL_SPEEDS = 125,

  //If the car is in direct access mode, those flags define the cars behavior (see structure)
  MSG_B2V_DEV_SET_DIRECT_ACCESS_FLAG = 126,


  // Set direct LEDs
  //TODO: expand with 2 bit masks: 1 for which to set, other for vals for those
  MSG_B2V_DEV_SET_LEDS = 127,

  // This message is sent by TestBasestation to request for the car to
  // send back the parameters stored in flash
  MSG_B2V_DEV_FLASH_PARAMETERS_REQUEST = 128,
 
  // This message is sent by the car to tell the test basestation what the 
  // current parameters are in flash
  MSG_V2B_DEV_FLASH_PARAMETERS_RESPONSE = 129,
 
  // This command is sent by the TestBasestation along with a buffer to tell
  // the car to write a new set of parameters to flash
  MSG_B2V_DEV_FLASH_PARAMETERS_WRITE =130,


  // Messages for parsed scan info
  MSG_B2V_DEV_REQUEST_PARSED_SCAN = 131,
  MSG_V2B_DEV_PARSED_SCAN_RESPONSE_1 = 132,
  MSG_V2B_DEV_PARSED_SCAN_RESPONSE_2 = 133,


  // Message for indicating when cycles take longer than expected
  MSG_V2B_DEV_CYCLE_OVERTIME = 134,


  // Message for forcing a reset of the radio
  // For testing purposes to reproduce unexpected radio resets.
  MSG_B2V_DEV_RESET_RADIO = 137,
*/


  ///////////////////////////////////////////////////////////////////////////////
  // In-vehicle stats messages 
  ///////////////////////////////////////////////////////////////////////////////
/*
  // Request stats from vehicle
  MSG_B2V_DEV_STATS_REQUEST = 138,

  // Stats message from vehicle
  MSG_V2B_DEV_STATS_RESPONSE = 139,

  // Reset in-RAM stats
  MSG_B2V_DEV_STATS_RESET = 140,
  

  // Generic DAS event message
  MSG_B2V_DEV_SET_DAS_CATEGORY = 141,
  MSG_V2B_DEV_DAS_EVENT = 142,

  // Message to start/stop SD card logging
  MSG_B2V_DEV_SD_LOGGING = 143,
*/

  ///////////////////////////////////////////////////////////////////////////////
  // Logging messages
  ///////////////////////////////////////////////////////////////////////////////
/*
  // Activate / deactivate logging on vehicle
  MSG_B2V_DEV_SET_LOGGING_MODE = 200,
  
  // Debugging messages -- will print out contents based on type field but not
  // process in any other way
  MSG_V2B_DEV_DEBUG_VAL = 201,

  MSG_V2B_DEV_LOG_FIDX = 202,
  MSG_V2B_DEV_LOG_BINARY_SCAN = 203,
  MSG_V2B_DEV_LOG_PROFILING = 204,
  MSG_V2B_DEV_LOG_WHEEL_SPEEDS = 205,

  // HorizontalMotionTracking / LaneSwitchController logging
  MSG_V2B_DEV_LOG_HMT,
*/

  // Messages specifically for communicating with simulator robots
  MSG_B2V_SIM_REQ_AVAIL_ROBOTS,
  MSG_V2B_SIM_AVAIL_ROBOT_IDS,
  MSG_B2V_SIM_REQ_ROBOT_CONNECT,
  MSG_V2B_SIM_CONFIRM_ROBOT_CONNECT,



  ///////////////////////////////////////////////////////////////////////////////
  // Temporary use messages (230 - 255)
  ///////////////////////////////////////////////////////////////////////////////

  
} CozmoMsg_Command;

/*
///////////////////////////////////////////////////////////////////////////////
//   BTLE Messages
///////////////////////////////////////////////////////////////////////////////
// MSG_B2V_BTLE_CHANGE_CONNECTION_PARAMS
typedef struct {
  u8 size;             // 9
  u8 msgID;            // MSG_B2V_BTLE_CHANGE_CONNECTION_PARAMS
  u16 minConnInterval; // Min number of 1.25ms intervals to use
  u16 maxConnInterval; // Max number of 1.25ms intervals to use
  u16 slaveLatency;    // Desired slave latency, usually 0
  u16 timeoutMult;     // usually 0xFC, iphone timeout in ms / 10
} CarMsg_ChangeConnectionParams;
#define SIZE_MSG_B2V_BTLE_CHANGE_CONNECTION_PARAMS 10

// MSG_B2V_BTLE_CONNECTION_PARAMS_REQUEST
typedef struct {
  u8 size;             // 1
  u8 msgID;            // MSG_B2V_BTLE_CONNECTION_PARAMS_REQUEST
} CarMsg_ConnectionParamsRequest;
#define SIZE_MSG_B2V_BTLE_CONNECTION_PARAMS_REQUEST 2

// MSG_V2B_BTLE_CONNECTION_PARAMS_RESPONSE
typedef struct {
  u8 size;             // 7
  u8 msgID;            // MSG_V2B_BTLE_CONNECTION_PARAMS_RESPONSE
  u16 connInterval;    // Min number of 1.25ms intervals to use
  u16 slaveLatency;    // Desired slave latency, usually 0
  u16 timeoutMult;     // usually 0xFC, iphone timeout in ms / 10
} CarMsg_ConnectionParamsResponse;
#define SIZE_MSG_V2B_BTLE_CONNECTION_PARAMS_RESPONSE 8

// MSG_B2V_BTLE_DISCONNECT
typedef struct {
  u8 size;             // 1
  u8 msgID;            // MSG_B2V_BTLE_DISCONNECT
} CarMsg_Disconnect;
#define SIZE_MSG_B2V_BTLE_DISCONNECT 2


///////////////////////////////////////////////////////////////////////////////
// Debug messages (arbitrary data)
// 
// Format:
//  u8 header1
//  u8 header2
//  u8 sourceVehicleID
//  u8 msgSize
//  u8 msgID (CMD_DEBUG)
//  u8 debugType (one of the defines below)
//  <type> data (based on define above)
///////////////////////////////////////////////////////////////////////////////
#define CMD_DEBUG_U8  0
#define CMD_DEBUG_S8  1
#define CMD_DEBUG_U16 2
#define CMD_DEBUG_S16 3
#define CMD_DEBUG_U32 4
#define CMD_DEBUG_S32 5
#define CMD_DEBUG_FLT 6
#define CMD_DEBUG_ASSERT 7

///////////////////////////////////////////////////////////////////////////////
*/
// Flags defining the current HIGH LEVEL functionality of the vehicle:
  // ADD PLAYBACK (?)  <--- need to add ?


// MSG_B2V_CORE_SET_MOTION
typedef struct {
  u8  size;
  u8  msgID;
  s16 speed_mmPerSec;      // Commanded speed in mm/sec
  s16 accel_mmPerSec2;     // Commanded max absolute value of...
                           // ...acceleration/decceleration in mm/sec^2
  s16 curvatureRadius_mm;  // +ve: curves left, -ve: curves right, ...
                           // ...u16_MAX: point turn left, 16_MIN: point ...
                           // ...turn right, 0: straight
} CozmoMsg_SetMotion;
#define SIZE_MSG_B2V_CORE_SET_MOTION 8
  


// MSG_B2V_CORE_CLEAR_PATH
typedef struct {
  u8  size;
  u8  msgID;
  u16 pathID;
} CozmoMsg_ClearPath;
#define SIZE_MSG_B2V_CORE_CLEAR_PATH 4


#define COMMON_PATH_SEGMENT_PARAMS \
  u8  size; \
  u8  msgID; \
  u16 pathID; \
  u8  segmentID; \
  s16 desiredSpeed_mmPerSec; \
  s16 terminalSpeed_mmPerSec; 

// MSG_B2V_CORE_SET_PATH_SEGMENT_LINE
typedef struct {
  COMMON_PATH_SEGMENT_PARAMS

  f32 x_start_m;
  f32 y_start_m;
  f32 x_end_m;
  f32 y_end_m;
} CozmoMsg_SetPathSegmentLine;
#define SIZE_MSG_B2V_CORE_SET_PATH_SEGMENT_LINE 25

// MSG_B2V_CORE_SET_PATH_SEGMENT_ARC
typedef struct {
  COMMON_PATH_SEGMENT_PARAMS

  f32 x_center_m;
  f32 y_center_m;
  f32 radius_m;
  f32 startRad;
  f32 endRad;  
} CozmoMsg_SetPathSegmentArc;
#define SIZE_MSG_B2V_CORE_SET_PATH_SEGMENT_ARC 29

// MSG_V2B_CORE_BLOCK_MARKER_OBSERVED
typedef struct {
  u32 blockType;
  u32 faceType;
  f32 x_imgUpperLeft,  y_imgUpperLeft;
  f32 x_imgLowerLeft,  y_imgLowerLeft;
  f32 x_imgUpperRight, y_imgUpperRight;
  f32 x_imgLowerRight, y_imgLowerRight;
} CozmoMsg_ObservedBlockMarker;

// MSG_V2B_CORE_MAT_MARKER_OBSERVED
typedef struct {
  u32 x_MatSquare;
  u32 y_MatSquare;
  f32 x_imgCenter,  y_imgCenter;
  f32 angle;
} CozmoMsg_ObservedMatMarker;


#endif  // #ifndef COZMO_MSG_PROTOCOL

