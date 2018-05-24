// Define constants used by fixtures
#ifndef FIXTURE_H
#define FIXTURE_H

#define FIXTURE_NONE          0     // No ID resistors

#define FIXTURE_BODY1_TEST    1     // ID 1
#define FIXTURE_BODY2_TEST    9     // ID 4 + 1
#define FIXTURE_BODY3_TEST    13    // ID 4 + 3 + 1

#define FIXTURE_HEAD1_TEST    2     // ID 2  

#define FIXTURE_MOTOR1L_TEST  10    // ID 4 + 2         MOTORxL = lift
#define FIXTURE_MOTOR1H_TEST  3     // ID 2 + 1         MOTORxH = head
#define FIXTURE_MOTOR2L_TEST  11    // ID 4 + 2 + 1
#define FIXTURE_MOTOR2H_TEST  12    // ID 4 + 3
#define FIXTURE_MOTOR3L_TEST  30    //
#define FIXTURE_MOTOR3H_TEST  31    //

// Note:  The following accessory tests must be in order (charger, cube1, cube2, etc..) 
#define FIXTURE_CHARGER_TEST  4     // ID 3
#define FIXTURE_CUBE1_TEST    5     // ID 3 + 1
#define FIXTURE_CUBE2_TEST    6     // ID 3 + 2
#define FIXTURE_CUBE3_TEST    7     // ID 3 + 2 + 1

#define FIXTURE_INFO_TEST     14    // ID 4 + 3 + 2
#define FIXTURE_PLAYPEN_TEST  15    // ID 4 + 3 + 2 + 1

// These options are not selectable by dip switch (there are too many of them):
#define FIXTURE_FINISHC_TEST   16   // Must be in order (charger, cube1, cube2, etc..)
#define FIXTURE_FINISH1_TEST   17    
#define FIXTURE_FINISH2_TEST   18    
#define FIXTURE_FINISH3_TEST   19    
#define FIXTURE_FINISHX_TEST   20   // Will connect to any type of accessory

#define FIXTURE_CUBEX_TEST     21   // Will verify (but not program-from-scratch) any cube type

// These can have any values, but must be in numeric order
#define FIXTURE_ROBOT1_TEST    8    // ID 4
#define FIXTURE_ROBOT2_TEST    22
#define FIXTURE_ROBOT3_TEST    23
#define FIXTURE_ROBOT3_CE_TEST 33
#define FIXTURE_ROBOT3_LE_TEST 38

#define FIXTURE_PACKOUT_TEST   24
#define FIXTURE_PACKOUT_CE_TEST 34
#define FIXTURE_PACKOUT_LE_TEST 39
#define FIXTURE_LIFETEST_TEST  25
#define FIXTURE_RECHARGE_TEST  26
#define FIXTURE_RECHARGE2_TEST 35

// 27 is reserved for the ill-fated JAM test

#define FIXTURE_HEAD2_TEST     28

#define FIXTURE_SOUND_TEST     29

#define FIXTURE_COZ187_TEST    32   // murder cozmo, kill code to fac-revert via charge-contact cmd

#define FIXTURE_EMROBOT_TEST   36
#define FIXTURE_EMCUBE_TEST    37

#define FIXTURE_DEBUG          40   // Should be last ID

//DEBUG must always be last entry!!!!!!!!!!!!!!!!!!1
#define NUM_FIXTYPES  (FIXTURE_DEBUG+1)

typedef unsigned char FixtureType;
#define FIXTURE_TYPES {                                                                                           \
  /*0-7*/   "NO ID",    "BODY1",      "HEAD1",      "MOTOR1H",  "CHARGER",  "CUBE1",  "CUBE2",      "CUBE3",      \
  /*8-15*/  "ROBOT1",   "BODY2",      "MOTOR1L",    "MOTOR2L",  "MOTOR2H",  "BODY3",  "INFO",       "PLAYPEN",    \
  /*16-23*/ "FINISHC",  "FINISH1",    "FINISH2",    "FINISH3",  "FINISHX",  "CUBEX",  "ROBOT2",     "ROBOT3",     \
  /*24-31*/ "PACKOUT",  "LIFETEST",   "RECHARGE",   "JAM",      "HEAD2",    "SOUND",  "MOTOR3L",    "MOTOR3H",    \
  /*32-39*/ "COZ187",   "ROBOT3-CE",  "PACKOUT-CE", "RECHARGE2","EMROBOT",  "EMCUBE", "ROBOT3-LE",  "PACKOUT-LE", \
  /*40-47*/ "DEBUG" }

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
#define ERROR_OUT_OF_SERIALS        10    // When the fixture itself runs out of 500000 serial numbers

#define ERROR_CUBE_ROM_OVERSIZE     11    // When you link a too-big cube ROM
#define ERROR_CUBE_ROM_MISPATCH     12    // When you can't patch the cube ROM
#define ERROR_SERIAL_INVALID        13    // When the serial number of this fixture exceeds 255 it can't make cubes!

#define ERROR_RADIO_TIMEOUT         14    // On-board radio firmware failed to boot

#define ERROR_INCOMPATIBLE_FIX_REV  15    // Test is incompatible with the current fixture hardware revision

#define IS_INTERNAL_ERROR(e) (e < 100)

// General errors
#define ERROR_TIMEOUT               101   // The operation timed out

// General motor errors
#define ERROR_MOTOR_LEFT            310   // Problem driving left motor (sticky or broken wire)
#define ERROR_MOTOR_LEFT_SPEED      311   // Problem driving left motor at full speed
#define ERROR_MOTOR_LEFT_JAM        316   // Jamming test failed on left motor

#define ERROR_MOTOR_RIGHT           320   // Problem driving right motor (sticky or broken wire)
#define ERROR_MOTOR_RIGHT_SPEED     321   // Problem driving right motor at full speed
#define ERROR_MOTOR_RIGHT_JAM       326   // Jamming test failed on right motor

#define ERROR_MOTOR_LIFT            330   // Problem driving lift motor (sticky or broken wire)
#define ERROR_MOTOR_LIFT_RANGE      331   // Lift can't reach full range (bad encoder/mechanical blockage)
#define ERROR_MOTOR_LIFT_BACKWARD   332   // Lift is wired backward
#define ERROR_MOTOR_LIFT_NOSTOP     333   // Lift does not hit stop (bad encoder/missing lift arm)
#define ERROR_MOTOR_LIFT_JAM        336   // Jamming test failed on lift motor

#define ERROR_MOTOR_HEAD            340   // Problem driving head motor (sticky or broken wire)
#define ERROR_MOTOR_HEAD_RANGE      341   // Head can't reach full range (bad encoder/mechanical blockage)
#define ERROR_MOTOR_HEAD_BACKWARD   342   // Head is wired backward
#define ERROR_MOTOR_HEAD_NOSTOP     343   // Head does not hit stop (bad encoder/missing head arm)
#define ERROR_MOTOR_HEAD_SLOW_RANGE 345   // Head can't reach full range when run at low voltage
#define ERROR_MOTOR_HEAD_JAM        346   // Jamming test failed on head motor

// Body/finished robot testport errors
#define ERROR_NO_PULSE              400   // Robot is not in debug/test mode
#define ERROR_NO_PULSE_ACK          401   // Robot can't hear test fixture
#define ERROR_TESTPORT_TIMEOUT      402   // Robot didn't reply to test fixture
#define ERROR_TESTPORT_TMI          403   // Robot misunderstood request (too much info in reply)
#define ERROR_TESTPORT_PADDING      404   // Test fixture can't hear robot

// SWD errors - in head or body test these are CPU failures
// In finished good test these are fixture (radio) failures
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
#define ERROR_HEAD_ROM_SIZE         501   // binary image too large for target region
#define ERROR_HEAD_ESP_MEM_TEST     502   // espressif memory self test failed

#define ERROR_HEAD_RADIO_SYNC       510   // Can't sync with radio
#define ERROR_HEAD_RADIO_ERASE      511   // Problem erasing flash
#define ERROR_HEAD_RADIO_FLASH      512   // Problem programming radio
#define ERROR_HEAD_RADIO_TIMEOUT    513   // Unable to send command due to ESP timeout (broken connection?)

#define ERROR_HEAD_SPEAKER          520   // Speaker not connected/damaged
#define ERROR_HEAD_Q1               530   // Q1 is out of spec

// Body errors
#define ERROR_BODY_BOOTLOADER       600   // Can't load bootloader onto body
#define ERROR_BODY_OUTOFDATE        601   // Body board is running out of date firmware
#define ERROR_BODY_TREAD_ENC_LEFT   602   // left tread encoder failed self test
#define ERROR_BODY_TREAD_ENC_RIGHT  603   // right tread encoder failed self test
#define ERROR_BODY_BACKPACK_PULL    604   // backpack pull-up incorrect
#define ERROR_BODYCOLOR_INVALID     605   // an invalid color code was detected
#define ERROR_BODYCOLOR_FULL        606   // no space to write new bodycolor. requires full erase/re-program.
#define ERROR_BODY_FLASHLIGHT       607   // forward IR LED (flashlight) failure

// Drop sensor errors
#define ERROR_DROP_LEAKAGE          610   // Drop leakage detected
#define ERROR_DROP_TOO_DIM          611   // Drop sensor bad LED (or reading too dim)
#define ERROR_DROP_TOO_BRIGHT       612   // Drop sensor bad photodiode (or too much ambient light)

// Power system errors
#define ERROR_BAT_LEAKAGE           620   // Too much leakage through battery when turned off
#define ERROR_BAT_UNDERVOLT         621   // Battery voltage too low - must charge
#define ERROR_BAT_CHARGER           622   // Battery charger not working
#define ERROR_BAT_OVERVOLT          623   // Battery voltage too high for this test (e.g. charge test)

// Motor harness errors
#define ERROR_BACKPACK_LED          650   // Backpack LED miswired or bad LED
#define ERROR_ENCODER_FAULT         651   // Encoder wire/solder broken
#define ERROR_MOTOR_BACKWARD        652   // Motor or encoder is wired backward
#define ERROR_MOTOR_SLOW            653   // Motor cannot turn easily (too tight or debris inside)
#define ERROR_BACKPACK_BTN_THRESH   654   // Button voltage detected outside digital thresholds
#define ERROR_BACKPACK_BTN_PRESS_TIMEOUT    655 // Timeout waiting for backpack button to be pressed
#define ERROR_BACKPACK_BTN_RELEASE_TIMEOUT  656 // Timeout waiting for backpack button to be released

#define ERROR_MOTOR_FAST            664   // Encoder does not meet Anki spec (can't count every tick at speed)
#define ERROR_ENCODER_UNDERVOLT     665   // Encoder does not meet Anki spec (can't meet minimum voltage)
#define ERROR_ENCODER_SPEED_FAULT   666   // Encoder does not meet Anki spec (rise/fall threshold)
#define ERROR_ENCODER_RISE_TIME     667   // Encoder does not meet Anki spec (A vs B rise time)

// Cube/charger errors
#define ERROR_CUBE_CANNOT_WRITE     700   // MCU is locked
#define ERROR_CUBE_NO_COMMUNICATION 701   // MCU is not working (bad crystal?)
#define ERROR_CUBE_VERIFY_FAILED    702   // OTP is not empty or did not program correctly
#define ERROR_CUBE_CANNOT_READ      705   // Broken wire or MCU is locked 
#define ERROR_CUBEX_NOT_SET         706   // Cube not programmed - CUBEX requires cube to be already programmed
#define ERROR_CUBE_SCAN_FAILED      707   // Did not detect advertising packets from the cube's radio

// Errors 710-713 for cube/charger types 0-3
#define ERROR_CUBE_TYPE_CHANGE      710   // Cube type (1 2 3) does not match fixture type (1 2 3)
#define ERROR_CUBE1_TYPE_CHANGE     711   // Cube type (1 2 3) does not match fixture type (1 2 3)
#define ERROR_CUBE2_TYPE_CHANGE     712   // Cube type (1 2 3) does not match fixture type (1 2 3)
#define ERROR_CUBE3_TYPE_CHANGE     713   // Cube type (1 2 3) does not match fixture type (1 2 3)

#define ERROR_CUBE_NO_BOOT          750   // Bad regulator IMU or crystal
#define ERROR_CUBE_MISSING_LED      751   // LED wiring problem
#define ERROR_CUBE_UNDERPOWER       752   // Bad power regulator
#define ERROR_CUBE_OVERPOWER        753   // Too much power in active mode
#define ERROR_CUBE_STANDBY          754   // Too much power in standby mode
#define ERROR_CUBE_RADIO            755   // Bad radio/antenna

#endif
