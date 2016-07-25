// Define constants used by fixtures
#ifndef FIXTURE_H
#define FIXTURE_H

#define FIXTURE_NONE          0     // No ID resistors

#define FIXTURE_BODY1_TEST    1     // ID 1
#define FIXTURE_BODY2_TEST    9     // ID 4 + 1
#define FIXTURE_BODY3_TEST    13    // ID 4 + 3 + 1

#define FIXTURE_HEAD1_TEST    2     // ID 2  

#define FIXTURE_MOTOR1A_TEST  10    // ID 4 + 2         MOTORxL = lift
#define FIXTURE_MOTOR2A_TEST  11    // ID 4 + 2 + 1
#define FIXTURE_MOTOR1B_TEST  3     // ID 2 + 1         MOTORxH = head
#define FIXTURE_MOTOR2B_TEST  12    // ID 4 + 3

// Note:  The following accessory tests must be in order (charger, cube1, cube2, etc..) 
#define FIXTURE_CHARGER_TEST  4     // ID 3
#define FIXTURE_CUBE1_TEST    5     // ID 3 + 1
#define FIXTURE_CUBE2_TEST    6     // ID 3 + 2
#define FIXTURE_CUBE3_TEST    7     // ID 3 + 2 + 1

#define FIXTURE_ROBOT_TEST    8     // ID 4
#define FIXTURE_INFO_TEST     14    // ID 4 + 3 + 2
#define FIXTURE_PLAYPEN_TEST  15    // ID 4 + 3 + 2 + 1

// These options are not selectable by dip switch (there are too many of them):
#define FIXTURE_FINISHC_TEST   16   // Must be in order (charger, cube1, cube2, etc..)
#define FIXTURE_FINISH1_TEST   17    
#define FIXTURE_FINISH2_TEST   18    
#define FIXTURE_FINISH3_TEST   19    
#define FIXTURE_FINISH_TEST    20   // Will connect to any type of accessory

#define FIXTURE_CUBEX_TEST     21   // Will verify (but not program-from-scratch) any cube type

#define FIXTURE_DEBUG          22   // Should be last ID

typedef unsigned char FixtureType;
#define FIXTURE_TYPES { "NO ID",  "BODY1",  "HEAD1",  "MOTOR1H","CHARGER", "CUBE1",  "CUBE2", "CUBE3", \
                        "ROBOT",  "BODY2",  "MOTOR1L","MOTOR2L","MOTOR2H", "BODY3",  "INFO",  "PLAYPEN", \
                        "FINISHC","FINISH1","FINISH2","FINISH3","FINISHX", "CUBEX", \
                        "DEBUG" }

extern FixtureType g_fixtureType;

extern char g_lotCode[15];
extern u32 g_time;
extern u32 g_dateCode;

// Get a serial number for a device in the normal 12.20 fixture.sequence format
u32 GetSerial();
                        
// Error numbers - these are thrown and eventually arrive on the display for the factory operator
// To aid memorization, error numbers are grouped logically:
//  3xy - motor errors - where X is the motor 1 (left), 2 (right), 3 (lift), or 4 (head) - and Y is the problem (reversed, encoder, etc)
//  4xy - testport errors - indicating problems communicating with the device or storing factory info
//  5xy - head errors - where X is the component (0 = CPU) and Y is the problem
//  6xy - body errors - where X is the component (0 = CPU) and Y is the problem
//  7xy - cube/charger errors - where X is the component (0 = CPU) and Y is the problem
#define ERROR_OK                    0

// Internal Errors - generally programming or hardware errors
#define ERROR_EMPTY_COMMAND         1
#define ERROR_ACK1                  2
#define ERROR_ACK2                  3
#define ERROR_RECEIVE               4
#define ERROR_UNKNOWN_MODE          5
#define ERROR_OUT_OF_RANGE          6
#define ERROR_ALIGNMENT             7

#define ERROR_SERIAL_EXISTS         8
#define ERROR_LOT_CODE              9
#define ERROR_OUT_OF_SERIALS        10    // When the fixture itself runs out of 500,000 serial numbers

#define ERROR_CUBE_ROM_OVERSIZE     11    // When you link a too-big cube ROM
#define ERROR_CUBE_ROM_MISPATCH     12    // When you can't patch the cube ROM
#define ERROR_SERIAL_INVALID        13    // When the serial number of this fixture exceeds 255, it can't make cubes!

#define ERROR_RADIO_TIMEOUT         14    // On-board radio firmware failed to boot

#define IS_INTERNAL_ERROR(e) (e < 100)

#define ERROR_MOTOR_LEFT            310   // General problem driving left motor
#define ERROR_MOTOR_RIGHT           320   // General problem driving r motor

// Body/finished robot testport errors
#define ERROR_NO_PULSE              400   // Robot is not in debug/test mode
#define ERROR_NO_PULSE_ACK          401   // Robot can't hear test fixture
#define ERROR_TESTPORT_TIMEOUT      402   // Robot didn't reply to test fixture
#define ERROR_TESTPORT_TMI          403   // Robot misunderstood request (too much info in reply)
#define ERROR_TESTPORT_PADDING      404   // Test fixture can't hear robot

// SWD errors - in head or body test, these are CPU failures
// In finished good test, these are fixture (radio) failures
#define ERROR_SWD_IDCODE            450   // IDCODE is unrecognized
#define ERROR_SWD_READ_FAULT        451   // SWD read failed
#define ERROR_SWD_WRITE_FAULT       452   // SWD write failed
#define ERROR_SWD_WRITE_STUB        453   // Can't write stub
#define ERROR_SWD_WRITE_BLOCK       454   // Can't write block
#define ERROR_SWD_NOSTUB            455   // Stub did not start up
#define ERROR_SWD_MISMATCH          456   // Flash failed - contents do not match
#define ERROR_SWD_FLASH_TIMEOUT     457   // Flash failed - stub timed out during flash operation

// Head errors
#define ERROR_HEAD_BOOTLOADER       500   // Can't load bootloader into K02

#define ERROR_HEAD_RADIO_SYNC       510   // Can't sync with radio
#define ERROR_HEAD_RADIO_ERASE      511   // Problem erasing flash
#define ERROR_HEAD_RADIO_FLASH      512   // Problem programming radio
#define ERROR_HEAD_RADIO_TIMEOUT    513   // Unable to send command due to ESP timeout (broken connection?)

// Body errors
#define ERROR_BODY_BOOTLOADER       600   // Can't load bootloader onto body

// Motor harness errors
#define ERROR_BACKPACK_LED          650   // Backpack LED miswired
#define ERROR_ENCODER_FAULT         651   // Encoder broken
#define ERROR_MOTOR_BACKWARD        652   // Motor harness or encoder is miswired
#define ERROR_MOTOR_SLOW            653   // Motor (or encoder) problems at low speed
#define ERROR_MOTOR_FAST            654   // Encoder problem at high speed
#define ERROR_ENCODER_UNDERVOLT     655   // Encoder can't meet minimum voltage

// Cube/charger errors
#define ERROR_CUBE_CANNOT_WRITE     700   // MCU is locked
#define ERROR_CUBE_NO_COMMUNICATION 701   // MCU is not working (bad crystal?)
#define ERROR_CUBE_VERIFY_FAILED    702   // OTP is not empty or did not program correctly
#define ERROR_CUBE_CANNOT_READ      705   // Broken wire or MCU is locked 
#define ERROR_CUBEX_NOT_SET         706   // Cube not programmed - CUBEX requires cube to be already programmed

// 710-713 for cube/charger types 0-3
#define ERROR_CUBE_TYPE_CHANGE      710   // Cube type (1,2,3) does not match fixture type (1,2,3)

#define ERROR_CUBE_NO_BOOT          750   // Bad regulator, IMU, or crystal
#define ERROR_CUBE_MISSING_LED      751   // LED wiring problem
#define ERROR_CUBE_UNDERPOWER       752   // Bad power regulator
#define ERROR_CUBE_OVERPOWER        753   // Too much power in active mode
#define ERROR_CUBE_STANDBY          754   // Too much power in standby mode
#define ERROR_CUBE_RADIO            755   // Bad radio/antenna

#endif
