// Define constants used by fixtures
#ifndef FIXTURE_H
#define FIXTURE_H

#define FIXTURE_CHARGER_TEST  2    // ID 4, ID 1
#define FIXTURE_CUBE_TEST     3    // ID 4
#define FIXTURE_HEAD_TEST     5    // ID 2
#define FIXTURE_BODY_TEST     6    // ID 1
#define FIXTURE_NONE          7    // No ID resistors
  
#define FIXTURE_DEBUG         8

typedef unsigned char FixtureType;
#define FIXTURE_TYPES { "?", "?", "CHARGE", "CUBE", "?", "HEAD", "BODY", "NO ID", "DEBUG" }

extern char g_lotCode[15];
extern u32 g_time;
extern u32 g_dateCode;

// Diagnostic mode commands are used to speak to the robot over the test port (charge contacts)
typedef enum
{
  DMC_ENTER                 = 0xA3,
  DMC_FIRST                 = 0x01,
  
  DMC_NACK                  = 0x01,
  DMC_ACK                   = 0x02,
  
  DMC_LAST
} DiagnosticModeCommand;

#define ERROR_OK                    0

// Internal Errors
#define ERROR_EMPTY_COMMAND         1
#define ERROR_ACK1                  2
#define ERROR_ACK2                  3
#define ERROR_RECEIVE               4
#define ERROR_UNKNOWN_MODE          5
#define ERROR_OUT_OF_RANGE          6
#define ERROR_ALIGNMENT             7

#define IS_INTERNAL_ERROR(e) (e < 100)

// Vehicle Errors
#define ERROR_POWER_CONTACTS        100

#define ERROR_ENABLE_CAMERA_2D      210
#define ERROR_ENABLE_CAMERA_1D      211
#define ERROR_CAMERA_2D             220
#define ERROR_CAMERA_1D_RAW         221

#define ERROR_ENABLE_MOTORS         300
#define ERROR_DRIVE_MOTORS          310
#define ERROR_READ_ENCODERS         320
#define ERROR_GEAR_GAP              321
//#define ERROR_ENCODERS_NON_ZERO     330
#define ERROR_ENCODERS_ZERO         331
#define ERROR_LEFT_MOTOR_ZERO       332
#define ERROR_RIGHT_MOTOR_ZERO      333
#define ERROR_LEFT_MOTOR_NON_ZERO   334
#define ERROR_RIGHT_MOTOR_NON_ZERO  335
#define ERROR_LEFT_WHEEL_NEGATIVE   336
#define ERROR_LEFT_WHEEL_POSITIVE   337
#define ERROR_RIGHT_WHEEL_NEGATIVE  338
#define ERROR_RIGHT_WHEEL_POSITIVE  339
#define ERROR_NOT_STATIONARY        340

#define ERROR_WRITE_FACTORY_BLOCK   400
#define ERROR_SERIAL_EXISTS         401
#define ERROR_LOT_CODE              402
#define ERROR_INVALID_MODEL         403
#define ERROR_READ_FACTORY_BLOCK    410
//#define ERROR_ALREADY_FLASHED       411
#define ERROR_GET_VERSION           412
#define ERROR_ENTER_CHARGE_FLASH    420
#define ERROR_FLASH_BLOCK_ACK       421
#define ERROR_CANNOT_REENTER        422
#define ERROR_DIFFERENT_VERSION     423
#define ERROR_READ_USER_BLOCK       430

#define ERROR_RADIO_RESET           500
#define ERROR_RADIO_ENTER_DTM       501
#define ERROR_RADIO_ENTER_RECEIVER  502
#define ERROR_RADIO_END_TEST        503
#define ERROR_RADIO_TEST_RESULTS    504
#define ERROR_RADIO_PACKET_COUNT    505

#define ERROR_PCB_BOOTLOADER        600
#define ERROR_PCB_OUT_OF_SERIALS    601
#define ERROR_PCB_JTAG_LOCK         602
#define ERROR_PCB_ZERO_UID          603
#define ERROR_PCB_FLASH_VEHICLE     610
#define ERROR_PCB_ENTER_DIAG_MODE   611

#define ERROR_NO_PC                 700

#define ERROR_PCB_ENCODERS          800

#endif
