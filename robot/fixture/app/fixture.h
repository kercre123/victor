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

// Error numbers - these are thrown and eventually arrive on the display for the factory operator
// To aid memorization, error numbers are grouped logically:
//  3xy - motor errors - where X is the motor 1 (left), 2 (right), 3 (lift), or 4 (head) - and Y is the problem (reversed, encoder, etc)
//  4xy - testport errors - indicating problems communicating with the device or storing factory info
//  5xy - head errors - where X is the component (0 = CPU) and Y is the problem
//  6xy - body errors - where X is the component (0 = CPU) and Y is the problem
//  7xy - cube/charger errors - where X is the component (0 = CPU) and Y is the problem
#define ERROR_OK                    0

// Internal Errors
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

#define IS_INTERNAL_ERROR(e) (e < 100)

// Head errors
#define ERROR_HEAD_BOOTLOADER       400   // Can't load bootloader into K02
#define ERROR_HEAD_APP              401   // Can't load app into K02

#define ERROR_HEAD_RADIO_BOOT       410   // Can't sync with radio
#define ERROR_HEAD_RADIO_ERASE      411   // Problem erasing flash
#define ERROR_HEAD_RADIO_FLASH      412   // Problem programming radio

// Cube/charger errors
#define ERROR_CUBE_CANNOT_WRITE     700
#define ERROR_CUBE_NO_COMMUNICATION 701
#define ERROR_CUBE_VERIFY_FAILED    702
#define ERROR_CUBE_BLOCK_FAILED     703


#endif
