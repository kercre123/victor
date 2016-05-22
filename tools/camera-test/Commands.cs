using System;

namespace Defines
{
    //The message header bytes
    public enum Header : byte
    {
      HEADER1 = 0xBE,
      HEADER2 = 0xEF
    }

    //Different pixel lengths
    public enum ImagerSize : byte
    {
        IS_CAM_160 = 160,
        IS_TSL3301_102 = 102
    }
    
    public enum Commands : byte
    {
        //requests a full scan from the vehicle
        CMD_SCAN_REQUEST = 2,
        CMD_SCAN_RESPONSE_LO = 3,
        CMD_SCAN_RESPONSE_HI = 4,

        CMD_SET_SCANLEDS = 5,

        //set the exposure time on the linear array
        CMD_SET_EXPOSURE = 6,

        CMD_SET_VEHICLE_SPEED = 7,

        //Request and receive the current battery voltage
        CMD_BATTERY_VOLTAGE_REQUEST = 8,
        CMD_BATTERY_VOLTAGE_RESPONSE = 9,

        //do we follow a line or not...
        CMD_LINE_FOLLOW = 10,

        //Sets the steering gains
        CMD_SET_STEERING_GAINS = 11,

        CMD_PING_REQUEST = 13,
        CMD_PING_RESPONSE = 14,

        //set high level lights: turn signal, brake, highbeam etc...
        CMD_LIGHTS = 15,

        //Bootloader messages
        CMD_BTLD_ENTER_BOOTLOADER = 16,
        CMD_BTLD_REQUEST_START = 17,
        CMD_BTLD_FLASH_UPDATE_INFO = 18,
        CMD_BTLD_REQUEST_CHUNK = 19,
        CMD_BTLD_NEXT_CHUNK = 20,
        CMD_BTLD_FLASH_UPDATED = 21,

        //Sent by the vehicle to tell the BS information about its current position etc.
        CMD_POSITION_UPDATE = 26,

        //BS tells a vehicle to change lanes
        CMD_LANE_CHANGE = 27,

        //A few commands used for logging
        CMD_LOG_DT_FIDX = 28,
        CMD_LOG_BINARY_SCAN = 22,
        CMD_LOG_PROFILING = 23,
        //24
        CMD_LOG_ERROR_STEER = 24,

        //Can activate or deactivate the logging.
        CMD_ACTIVATE_LOGGING = 29,

        //Relative position update in case absolute fails
        CMD_RELATIVE_POSITION_UPDATE = 30,

        //Cancel current laneswitch and converge to next lane
        CMD_CANCEL_LANE_SWITCH = 31,

        //some temporary commands, not used in the REAL vehicles

        //Sets the open loop steering angle of the vehicle. Units in microseconds
        //between -500 and 500, 0 is straight.
        CMD_SET_VEHICLE_OL_STEERING = 32,

        //sends a message to the car/cars to tell them about the road network
        //so steering etc. can be done more acurately
        CMD_INITIALIZE_ROAD_CURVATURES = 33,

        //expect road curvature or so... ask Boris

        //Set the vehicle in open loop speed mode
        CMD_SET_VEHICLE_MODE_OPEN_LOOP = 0,

        CMD_DRIVE_MOTORS_OPEN_LOOP = 32,

        CMD_TEMP = 39,

        CMD_DEBUG = 40,

        CMD_FLASH_PARAMETERS_REQUEST = 41,

        CMD_FLASH_PARAMETERS_RESPONSE = 42,

        CMD_FLASH_PARAMETERS_WRITE = 43,

        //The sinple light message, no brightness
        CMD_SIMPLE_LIGHTS = 44,

        CMD_CAR_OFF_ROAD = 45,

        CMD_SET_VEHICLE_SPEED_AFTER_LANE_CHANGE = 46,

        CMD_SET_PLAYBACK_ACTIONS = 47,

        CMD_STARTSTOP_PLAYBACK_ACTION = 48,

        CMD_SET_CAR_MODE = 49,
            
        CMD_CAR_INIT_COMPLETE = 50,

        CMD_CARD_READ = 51,

        CMD_SET_TRACK_ID = 52,

        CMD_IPHONE_PURCHASE = 53,

        // Messages for the updated (v2) bootloader that adds a recovery after timeout
        // feature. 
        CMD_BTLD_FLASH_STATUS_REQUEST = 54,
        CMD_BTLD_DESIRED_CHUNK = 55,
        CMD_BTLD_WRITE_NEXT_CHUNK = 56,

        // Message for sending version info
        CMD_REQUEST_CAR_VERSION = 57,
        CMD_SEND_CAR_VERSION_1 = 58,
        CMD_SEND_CAR_VERSION_2 = 59,

        // Logging HorizontalMotionTracking / LaneSwitchController
        CMD_LOG_HMT = 60,



        //Send and Image with 102 pixels, 1 byte per pixel, and a 4 byte timestamp, and 4 bytes of additional payload
        //[timestamp][image][additionalpayload]
        IMGCMD_IMGPLUS_160 = 61,



        CMD_GENERAL_PAYLOAD = 150,
        
        //Messages to request information about the wireless channels and respond to it        
        BSCMD_REQUEST_CHANNEL_CHANGE = 250,
        BSCMD_REQUEST_CHANNEL_INFO = 251,
        BSCMD_SEND_CHANNEL_INFO = 252,
        
        
        ////////////////////////////////////////////////////////////////////////
        //special commands from the coordinator node to the
        //basestation. Information about the network status etc.
        BSCMD_NODE_JOINED = 254,
        BSCMD_NODE_LOST = 253,

        
    }
}