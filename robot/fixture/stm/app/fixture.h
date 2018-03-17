#ifndef FIXTURE_H
#define FIXTURE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "tests.h"

//-----------------------------------------------------------------------------
//                  Fixture Modes
//-----------------------------------------------------------------------------

#define FIXMODE_NONE           0   //Not assigned, or invalid
#define FIXMODE_DEBUG          1

#define FIXMODE_BODY0A         4
#define FIXMODE_BODY0          5
#define FIXMODE_BODY1          6
#define FIXMODE_BODY2          7
#define FIXMODE_BODY3          8

#define FIXMODE_HEAD1          11
#define FIXMODE_HEAD2          12

#define FIXMODE_BACKPACK1      16

#define FIXMODE_CUBE0          20 /*DEBUG*/
#define FIXMODE_CUBE1          21
#define FIXMODE_CUBE2          22

#define FIXMODE_ROBOT0         25
#define FIXMODE_ROBOT1         26
#define FIXMODE_ROBOT2         27
#define FIXMODE_ROBOT3         28

#define FIXMODE_INFO           31
#define FIXMODE_PLAYPEN        32
#define FIXMODE_PACKOUT        33
#define FIXMODE_LIFETEST       36
#define FIXMODE_RECHARGE0      37
#define FIXMODE_RECHARGE1      38
#define FIXMODE_SOUND1         39
#define FIXMODE_SOUND2         40

#define FIXMODE_MOTOR1L        46
#define FIXMODE_MOTOR1H        47
#define FIXMODE_MOTOR2L        48
#define FIXMODE_MOTOR2H        49
#define FIXMODE_MOTOR3L        50
#define FIXMODE_MOTOR3H        51

#define FIXMODE_FINISH1        56
#define FIXMODE_FINISH2        57
#define FIXMODE_FINISH3        58
#define FIXMODE_FINISHX        59

#define FIXMODE_EMCUBE         61
#define FIXMODE_EMROBOT        62

//-----------------------------------------------------------------------------
//                  Fixture Mode extended info
//-----------------------------------------------------------------------------

typedef struct {
  const char       *name;
  bool            (*Detect)(void); //true if test pcba/assembly is detected
  TestFunction*   (*GetTests)(void); //return ptr to NULL-terminated array of test functions
  void            (*Cleanup)(void); //called after tests (or after an exception) to de-init any resources and exit gracefully
  int               mode; //include the [redundant] mode field for runtime DDC (dynamic dummy check)
} fixmode_info_t;

//Global data
extern int                  g_fixmode; //runtime fixture mode
extern const fixmode_info_t g_fixmode_info[]; //mode information (see array init dat below)
extern const int            g_num_fixmodes; //number of modes
extern bool                 g_allowOutdated;

//Fixture prototypes
#ifdef __cplusplus
void          fixtureInit(void); //initialize fixture, board rev, pins etc.
const char*   fixtureName(void); //gets name of the current fixmode
bool          fixtureDetect(int min_delay_us = 1000); //runs detect on the current fixmode. 'min_delay_us' pads out a minimum execution time per call.
void          fixtureCleanup(void); //runs cleanup on the current fixmode
TestFunction* fixtureGetTests(void); //gets tests for the current fixmode
int           fixtureGetTestCount(void); //gets the # of test functions for the current fixmode
bool          fixtureValidateFixmodeInfo(bool print=0); //dev tool - validate const info array. print=1 -> print the array to console
uint32_t      fixtureGetSerial(void); // Get a serial number for a device in the normal 12.20 fixture.sequence format
#endif

//Init data for g_fixmode_info[] - keep all mode info organized in one file!
//note: FIXMODE_x must equal array index - very important, don't screw it up.
#define FIXMODE_INFO_INIT_DATA                                                                                                        \
  { "NONE"        , NULL                , NULL                        , NULL                        , FIXMODE_NONE        },  /*0*/   \
  { "DEBUG"       , TestDebugDetect     , TestDebugGetTests           , TestDebugCleanup            , FIXMODE_DEBUG       },  /*1*/   \
  { NULL          , NULL                , NULL                        , NULL                        , -1 },                           \
  { NULL          , NULL                , NULL                        , NULL                        , -1 },                           \
  { "BODY0A"      , TestBodyDetect      , TestBody0GetTests           , TestBodyCleanup             , FIXMODE_BODY0A      },  /*4*/   \
  { "BODY0"       , TestBodyDetect      , TestBody0GetTests           , TestBodyCleanup             , FIXMODE_BODY0       },  /*5*/   \
  { "BODY1"       , TestBodyDetect      , TestBody1GetTests           , TestBodyCleanup             , FIXMODE_BODY1       },  /*6*/   \
  { "BODY2"       , TestBodyDetect      , TestBody2GetTests           , TestBodyCleanup             , FIXMODE_BODY2       },  /*7*/   \
  { "BODY3"       , TestBodyDetect      , TestBody3GetTests           , TestBodyCleanup             , FIXMODE_BODY3       },  /*8*/   \
  { NULL          , NULL                , NULL                        , NULL                        , -1 },                           \
  { NULL          , NULL                , NULL                        , NULL                        , -1 },                           \
  { "HEAD1"       , TestHeadDetect      , TestHead1GetTests           , TestHeadCleanup             , FIXMODE_HEAD1       },  /*11*/  \
  { "HEAD2"       , TestHeadDetect      , TestHead2GetTests           , TestHeadCleanup             , FIXMODE_HEAD2       },  /*12*/  \
  { NULL          , NULL                , NULL                        , NULL                        , -1 },                           \
  { NULL          , NULL                , NULL                        , NULL                        , -1 },                           \
  { NULL          , NULL                , NULL                        , NULL                        , -1 },                           \
  { "BACKPACK1"   , TestBackpackDetect  , TestBackpack1GetTests       , TestBackpackCleanup         , FIXMODE_BACKPACK1   },  /*16*/  \
  { NULL          , NULL                , NULL                        , NULL                        , -1 },                           \
  { NULL          , NULL                , NULL                        , NULL                        , -1 },                           \
  { NULL          , NULL                , NULL                        , NULL                        , -1 },                           \
  { "CUBE0"       , TestCubeDetect      , TestCube0GetTests           , TestCubeCleanup             , FIXMODE_CUBE0       },  /*20*/  \
  { "CUBE1"       , TestCubeDetect      , TestCube1GetTests           , TestCubeCleanup             , FIXMODE_CUBE1       },  /*21*/  \
  { "CUBE2"       , TestCubeDetect      , TestCube2GetTests           , TestCubeCleanup             , FIXMODE_CUBE2       },  /*22*/  \
  { NULL          , NULL                , NULL                        , NULL                        , -1 },                           \
  { NULL          , NULL                , NULL                        , NULL                        , -1 },                           \
  { "ROBOT0"      , TestRobotDetect     , TestRobot0GetTests          , TestRobotCleanup            , FIXMODE_ROBOT0      },  /*25*/  \
  { "ROBOT1"      , TestRobotDetect     , TestRobot1GetTests          , TestRobotCleanup            , FIXMODE_ROBOT1      },  /*26*/  \
  { "ROBOT2"      , TestRobotDetect     , TestRobot2GetTests          , TestRobotCleanup            , FIXMODE_ROBOT2      },  /*27*/  \
  { "ROBOT3"      , TestRobotDetect     , TestRobot3GetTests          , TestRobotCleanup            , FIXMODE_ROBOT3      },  /*28*/  \
  { NULL          , NULL                , NULL                        , NULL                        , -1 },                           \
  { NULL          , NULL                , NULL                        , NULL                        , -1 },                           \
  { "ROBOTINFO"   , TestRobotDetect     , TestRobotInfoGetTests       , TestRobotCleanup            , FIXMODE_INFO        },  /*31*/  \
  { "PLAYPEN"     , TestRobotDetect     , TestRobotPlaypenGetTests    , TestRobotCleanup            , FIXMODE_PLAYPEN     },  /*32*/  \
  { "PACKOUT"     , TestRobotDetect     , TestRobotPackoutGetTests    , TestRobotCleanup            , FIXMODE_PACKOUT     },  /*33*/  \
  { NULL          , NULL                , NULL                        , NULL                        , -1 },                           \
  { NULL          , NULL                , NULL                        , NULL                        , -1 },                           \
  { "LIFETEST"    , TestRobotDetect     , TestRobotLifetestGetTests   , TestRobotCleanup            , FIXMODE_LIFETEST    },  /*36*/  \
  { "RECHARGE0"   , TestRobotDetect     , TestRobotRecharge0GetTests  , TestRobotCleanup            , FIXMODE_RECHARGE0   },  /*37*/  \
  { "RECHARGE1"   , TestRobotDetect     , TestRobotRecharge1GetTests  , TestRobotCleanup            , FIXMODE_RECHARGE1   },  /*38*/  \
  { "SOUND1"      , TestRobotDetect     , TestRobotSoundGetTests      , TestRobotCleanup            , FIXMODE_SOUND1      },  /*39*/  \
  { "SOUND2"      , TestRobotDetect     , TestRobotSoundGetTests      , TestRobotCleanup            , FIXMODE_SOUND2      },  /*40*/  \
  { NULL          , NULL                , NULL                        , NULL                        , -1 },                           \
  { NULL          , NULL                , NULL                        , NULL                        , -1 },                           \
  { NULL          , NULL                , NULL                        , NULL                        , -1 },                           \
  { NULL          , NULL                , NULL                        , NULL                        , -1 },                           \
  { NULL          , NULL                , NULL                        , NULL                        , -1 },                           \
  { "MOTOR1L"     , TestMotorDetect     , TestMotor1LGetTests         , TestMotorCleanup            , FIXMODE_MOTOR1L     },  /*46*/  \
  { "MOTOR1H"     , TestMotorDetect     , TestMotor1HGetTests         , TestMotorCleanup            , FIXMODE_MOTOR1H     },  /*47*/  \
  { "MOTOR2L"     , TestMotorDetect     , TestMotor2LGetTests         , TestMotorCleanup            , FIXMODE_MOTOR2L     },  /*48*/  \
  { "MOTOR2H"     , TestMotorDetect     , TestMotor2HGetTests         , TestMotorCleanup            , FIXMODE_MOTOR2H     },  /*49*/  \
  { "MOTOR3L"     , TestMotorDetect     , TestMotor3LGetTests         , TestMotorCleanup            , FIXMODE_MOTOR3L     },  /*50*/  \
  { "MOTOR3H"     , TestMotorDetect     , TestMotor3HGetTests         , TestMotorCleanup            , FIXMODE_MOTOR3H     },  /*51*/  \
  { NULL          , NULL                , NULL                        , NULL                        , -1 },                           \
  { NULL          , NULL                , NULL                        , NULL                        , -1 },                           \
  { NULL          , NULL                , NULL                        , NULL                        , -1 },                           \
  { NULL          , NULL                , NULL                        , NULL                        , -1 },                           \
  { "FINISH1"     , TestCubeFinishDetect, TestCubeFinish1GetTests     , TestCubeCleanup             , FIXMODE_FINISH1     },  /*56*/  \
  { "FINISH2"     , TestCubeFinishDetect, TestCubeFinish2GetTests     , TestCubeCleanup             , FIXMODE_FINISH2     },  /*57*/  \
  { "FINISH3"     , TestCubeFinishDetect, TestCubeFinish3GetTests     , TestCubeCleanup             , FIXMODE_FINISH3     },  /*58*/  \
  { "FINISHX"     , TestCubeFinishDetect, TestCubeFinishXGetTests     , TestCubeCleanup             , FIXMODE_FINISHX     },  /*59*/  \
  { NULL          , NULL                , NULL                        , NULL                        , -1 },                           \
  { "EMCUBE"      , TestCubeFinishDetect, TestCubeFinishXGetTests     , TestCubeCleanup             , FIXMODE_EMCUBE      },  /*61*/  \
  { "EMROBOT"     , TestRobotDetect     , TestRobotEMGetTests         , TestRobotCleanup            , FIXMODE_EMROBOT     },  /*62*/  \
/*FIXMODE_INFO_ARRAY_INIT*/

//-----------------------------------------------------------------------------
//                  Fixture Errors
//-----------------------------------------------------------------------------

typedef int error_t;

//RESERVED  RANGES:
//001-099	  Playpen
//100-199	  Syscon
//200-219	  Recovery / OTA
//220-299	  Robot Process
//300-999	  Factory fixtures

#define IS_INTERNAL_ERROR(e) (e >= 900)

// Error numbers - these are thrown and eventually arrive on the display for the factory operator
// To aid memorization, error numbers are grouped logically:
//  3xy - motor errors - where X is the motor 1 (left), 2 (right), 3 (lift), or 4 (head) - and Y is the problem (reversed, encoder, etc)
//  4xy - testport errors - indicating problems communicating with the device or storing factory info
//  5xy - head errors - where X is the component (0 = CPU) and Y is the problem
//  6xy - body errors - where X is the component (0 = CPU) and Y is the problem
//  7xy - cube/charger errors - where X is the component (0 = CPU) and Y is the problem
#define ERROR_OK                    0

//Note: export tags, <export...>, are used by a script to export error code listing to a CSV file
//<export start>
//<export heading> ANKI VICTOR FIXTURE ERROR CODES

//<export heading> General Motor Errors
#define ERROR_MOTOR_LEFT            310   // Problem driving left motor (sticky or broken wire)
//#define ERROR_MOTOR_LEFT_SPEED      311   // Problem driving left motor at full speed
//#define ERROR_MOTOR_LEFT_JAM        316   // Jamming test failed on left motor

#define ERROR_MOTOR_RIGHT           320   // Problem driving right motor (sticky or broken wire)
//#define ERROR_MOTOR_RIGHT_SPEED     321   // Problem driving right motor at full speed
//#define ERROR_MOTOR_RIGHT_JAM       326   // Jamming test failed on right motor

#define ERROR_MOTOR_LIFT            330   // Problem driving lift motor (sticky or broken wire)
#define ERROR_MOTOR_LIFT_RANGE      331   // Lift can't reach full range (bad encoder/mechanical blockage)
#define ERROR_MOTOR_LIFT_BACKWARD   332   // Lift is wired backward
#define ERROR_MOTOR_LIFT_NOSTOP     333   // Lift does not hit stop (bad encoder/missing lift arm)
//#define ERROR_MOTOR_LIFT_JAM        336   // Jamming test failed on lift motor

#define ERROR_MOTOR_HEAD            340   // Problem driving head motor (sticky or broken wire)
#define ERROR_MOTOR_HEAD_RANGE      341   // Head can't reach full range (bad encoder/mechanical blockage)
#define ERROR_MOTOR_HEAD_BACKWARD   342   // Head is wired backward
#define ERROR_MOTOR_HEAD_NOSTOP     343   // Head does not hit stop (bad encoder/missing head arm)
//#define ERROR_MOTOR_HEAD_SLOW_RANGE 345   // Head can't reach full range when run at low voltage
//#define ERROR_MOTOR_HEAD_JAM        346   // Jamming test failed on head motor

//<export heading> Testport Errors - charge contact communications (BODY ROBOT PACKOUT)
#define ERROR_TESTPORT_CMD_TIMEOUT        400 //timeout waiting for response to a command
#define ERROR_TESTPORT_RSP_MISMATCH       401 //response doesn't match command
#define ERROR_TESTPORT_RSP_MISSING_ARGS   402 //response is missing required arguments
#define ERROR_TESTPORT_RSP_BAD_ARG        403 //response arg incorrect or improperly formatted
#define ERROR_TESTPORT_CMD_FAILED         404 //returned fail code
#define ERROR_TESTPORT_RX_ERROR           405 //driver (uart) overflow or dropped chars detected

//<export heading> SWD Errors (mcu programming interface)
//#define ERROR_SWD_IDCODE            450   // IDCODE is unrecognized
//#define ERROR_SWD_READ_FAULT        451   // SWD read failed
//#define ERROR_SWD_WRITE_FAULT       452   // SWD write failed
//#define ERROR_SWD_WRITE_STUB        453   // Can't write stub
//#define ERROR_SWD_WRITE_BLOCK       454   // Can't write block
//#define ERROR_SWD_NOSTUB            455   // Stub did not start up
//#define ERROR_SWD_MISMATCH          456   // Flash failed - contents do not match
//#define ERROR_SWD_VERIFY_FAILED     456   // failed flash write verification
//#define ERROR_SWD_FLASH_TIMEOUT     457   // Flash failed - stub timed out during flash operation
//#define ERROR_SWD_JTAG_LOCK         458   // failed to lock jtag

//<export heading> Head Errors
//#define ERROR_HEAD_BOOTLOADER       500   // Can't load bootloader into K02
//#define ERROR_HEAD_ROM_SIZE         501   // binary image too large for target region
//#define ERROR_HEAD_ESP_MEM_TEST     502   // espressif memory self test failed

//#define ERROR_HEAD_RADIO_SYNC       510   // Can't sync with radio
//#define ERROR_HEAD_RADIO_ERASE      511   // Problem erasing flash
//#define ERROR_HEAD_RADIO_FLASH      512   // Problem programming radio
//#define ERROR_HEAD_RADIO_TIMEOUT    513   // Unable to send command due to ESP timeout (broken connection?)

//#define ERROR_HEAD_SPEAKER          520   // Speaker not connected/damaged
//#define ERROR_HEAD_Q1               530   // Q1 is out of spec

//<export heading> Body Errors
//#define ERROR_BODY_BOOTLOADER       600   // Can't load bootloader onto body
//#define ERROR_BODY_OUTOFDATE        601   // Body board is running out of date firmware
//#define ERROR_BODY_TREAD_ENC_LEFT   602   // left tread encoder failed self test
//#define ERROR_BODY_TREAD_ENC_RIGHT  603   // right tread encoder failed self test
//#define ERROR_BODY_BACKPACK_PULL    604   // backpack pull-up incorrect
//#define ERROR_BODYCOLOR_INVALID     605   // an invalid color code was detected
//#define ERROR_BODYCOLOR_FULL        606   // no space to write new bodycolor. requires full erase/re-program.
//#define ERROR_BODY_FLASHLIGHT       607   // forward IR LED (flashlight) failure
#define ERROR_BODY_NO_BOOT_MSG        608   // Could not detect production fw boot msg

//<export heading> Drop Sensor Errors
//#define ERROR_DROP_LEAKAGE          610   // Drop leakage detected
//#define ERROR_DROP_TOO_DIM          611   // Drop sensor bad LED (or reading too dim)
//#define ERROR_DROP_TOO_BRIGHT       612   // Drop sensor bad photodiode (or too much ambient light)

//<export heading> Power System Errors
//#define ERROR_BAT_LEAKAGE           620   // Too much leakage through battery when turned off
#define ERROR_BAT_UNDERVOLT         621   // Battery voltage too low
#define ERROR_BAT_CHARGER           622   // Battery charger not working
#define ERROR_BAT_OVERVOLT          623   // Battery voltage too high for this test (e.g. charge test)
#define ERROR_OUTPUT_VOLTAGE_LOW    624   // Fixture's output voltage is too low - check supply and wiring
#define ERROR_OUTPUT_VOLTAGE_HIGH   625   // Fixture's output voltage is too high - check supply and wiring

//<export heading> Backpack Errors
#define ERROR_BACKPACK_LED          650   // Backpack LED miswired or bad LED
//#define ERROR_ENCODER_FAULT         651   // Encoder wire/solder broken
//#define ERROR_MOTOR_BACKWARD        652   // Motor or encoder is wired backward
//#define ERROR_MOTOR_SLOW            653   // Motor cannot turn easily (too tight or debris inside)
//#define ERROR_BACKPACK_BTN_THRESH   654   // Button voltage detected outside digital thresholds
//#define ERROR_BACKPACK_BTN_PRESS_TIMEOUT    655 // Timeout waiting for backpack button to be pressed
//#define ERROR_BACKPACK_BTN_RELEASE_TIMEOUT  656 // Timeout waiting for backpack button to be released

//#define ERROR_MOTOR_FAST            664   // Encoder does not meet Anki spec (can't count every tick at speed)
//#define ERROR_ENCODER_UNDERVOLT     665   // Encoder does not meet Anki spec (can't meet minimum voltage)
//#define ERROR_ENCODER_SPEED_FAULT   666   // Encoder does not meet Anki spec (rise/fall threshold)
//#define ERROR_ENCODER_RISE_TIME     667   // Encoder does not meet Anki spec (A vs B rise time)

//<export heading> Cube Errors
#define ERROR_CUBE_CANNOT_WRITE     700   // MCU is locked
#define ERROR_CUBE_NO_COMMUNICATION 701   // MCU broken wire or locked. Already OTP'd? Bad crystal?
#define ERROR_CUBE_VERIFY_FAILED    702   // OTP is not empty or did not program correctly
#define ERROR_CUBE_LED              703   // Detected bad LED
#define ERROR_CUBE_LED_D1           704   // Detected bad LED [D1]
#define ERROR_CUBE_LED_D2           705   // Detected bad LED [D2]
#define ERROR_CUBE_LED_D3           706   // Detected bad LED [D3]
#define ERROR_CUBE_LED_D4           707   // Detected bad LED [D4]
#define ERROR_CUBE_ACCEL            708   // Accelerometer cannot communicate
#define ERROR_CUBE_ACCEL_PWR        709   // Accelerometer power fault
#define ERROR_CUBE_BAD_BINARY       710   // Fixture has corrupted binary (contact Anki for firmware update)
#define ERROR_CUBE_FW_MISMATCH      711   // Cube already has fw burned to OTP -- different than current version
#define ERROR_CUBE_FW_MATCH         712   // Cube already has fw burned to OTP -- same as current version
//#define ERROR_CUBE_CANNOT_READ      705   // Broken wire or MCU is locked 
//#define ERROR_CUBEX_NOT_SET         706   // Cube not programmed - CUBEX requires cube to be already programmed
//#define ERROR_CUBE_SCAN_FAILED      707   // Did not detect advertising packets from the cube's radio

// Errors 710-713 for cube/charger types 0-3
//#define ERROR_CUBE_TYPE_CHANGE      710   // Cube type (1 2 3) does not match fixture type (1 2 3)
//#define ERROR_CUBE1_TYPE_CHANGE     711   // Cube type (1 2 3) does not match fixture type (1 2 3)
//#define ERROR_CUBE2_TYPE_CHANGE     712   // Cube type (1 2 3) does not match fixture type (1 2 3)
//#define ERROR_CUBE3_TYPE_CHANGE     713   // Cube type (1 2 3) does not match fixture type (1 2 3)

//#define ERROR_CUBE_NO_BOOT          750   // Bad regulator IMU or crystal
//#define ERROR_CUBE_MISSING_LED      751   // LED wiring problem
//#define ERROR_CUBE_UNDERPOWER       752   // Bad power regulator
//#define ERROR_CUBE_OVERPOWER        753   // Too much power in active mode
//#define ERROR_CUBE_STANDBY          754   // Too much power in standby mode
//#define ERROR_CUBE_RADIO            755   // Bad radio/antenna

#define ERROR_ROBOT_TEST_SEQUENCE     800   // This test cannot run until all previous tests have passed

//<export heading> Internal Errors - generally programming or hardware errors
#define ERROR_EMPTY_COMMAND         901
#define ERROR_ACK1                  902
#define ERROR_ACK2                  903
//#define ERROR_RECEIVE               904
#define ERROR_UNKNOWN_MODE          905
#define ERROR_OUT_OF_RANGE          906
//#define ERROR_ALIGNMENT             907

#define ERROR_SERIAL_EXISTS         908
#define ERROR_LOT_CODE              909
#define ERROR_OUT_OF_SERIALS        910   // When the fixture itself runs out of 500000 serial numbers
#define ERROR_OUT_OF_CLOUD_CERTS    911   // The fixture has run out of cloud certificates

//#define ERROR_BIN_SIZE              911   // Binary too large
#define ERROR_FLASH_VERIFY          912   // Flash verification failed
////#define ERROR_CUBE_ROM_OVERSIZE     911   // When you link a too-big cube ROM
////#define ERROR_CUBE_ROM_MISPATCH     912   // When you can't patch the cube ROM
#define ERROR_SERIAL_INVALID        913   // valid fixture serial # {1-4095} required to generate ESN for production programming
//#define ERROR_RADIO_TIMEOUT         914   // On-board radio firmware failed to boot
#define ERROR_INCOMPATIBLE_FIX_REV  915   // Test is incompatible with the current fixture hardware revision
#define ERROR_INVALID_STATE         916   // Internal state is out of sync
#define ERROR_BAD_ARG               917   // Argument errror
//#define ERROR_UNHANDLED_EXCEPTION   918   // 'safe' operation threw an uncategorized error

//<export heading> General/Debug Errors
#define ERROR_TIMEOUT               951   // The operation timed out
#define ERROR_POWER_SHORT           952   // Power short circuit detected
//#define ERROR_DEVICE_NOT_DETECTED   953   // Manual test start - DUT was not detected

//<export end>

#ifdef __cplusplus
}
#endif

#endif //FIXTURE_H
