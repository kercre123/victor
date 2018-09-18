/**
 * File: hal.h
 *
 * Author: Andrew Stein (andrew)
 * Created: 10/10/2013
 * Author: Daniel Casner <daniel@anki.com>
 * Revised for Cozmo 2: 02/27/2017
 *
 * Description:
 *
 *   This is an "abstract class" defining an interface to lower level
 *   hardware functionality like getting an image from a camera,
 *   setting motor speeds, etc.  This is all the Robot should
 *   need to know in order to talk to its underlying hardware.
 *
 *   To avoid C++ class overhead running on a robot's embedded hardware,
 *   this is implemented as a namespace instead of a fullblown class.
 *
 *   This just defines the interface; the implementation (e.g., Real vs.
 *   Simulated) is given by a corresponding .cpp file.  Which type of
 *   is used for a given project/executable is decided by which .cpp
 *   file gets compiled in.
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#ifndef ANKI_COZMO_ROBOT_HARDWAREINTERFACE_H
#define ANKI_COZMO_ROBOT_HARDWAREINTERFACE_H
#include "coretech/common/shared/types.h"
#include "clad/types/motorTypes.h"
#include "clad/types/proxMessages.h"


#ifdef SIMULATOR
#include "sim_hal.h"
#endif

// Whether or not to read/process imu data on a thread
#define PROCESS_IMU_ON_THREAD 1

namespace Anki {
namespace Vector {
namespace HAL {


/************************************************************************
 * \section Parameters and Constants
 */

/// Scale value for maximum motor power in HAL
static const f32 MOTOR_MAX_POWER = 1.0f;

//
// Initialize HAL
// Blocks until initialization complete or shutdownSignal becomes non-zero
// Returns RESULT_OK or error
//
Result Init(const int * shutdownSignal);

//
// Perform a single HAL "tick"
// Returns RESULT_OK or error
//
Result Step(void);
void Stop(void);

void Shutdown();

/************************************************************************
 * \section Time
 */

/// Get the number of microseconds since boot
u32 GetMicroCounter(void);

/// Block main execution for specified number of microseconds
void MicroWait(u32 microseconds);

/// Retrieve current time (from steady_clock) in ms 
extern "C" TimeStamp_t GetTimeStamp(void);

/// Retrieve the number of times a hardware watchdog reset has occurred.
u8 GetWatchdogResetCounter(void);

/************************************************************************
 * \section IMU Interface
 */

/// IMU_DataStructure contains 3-axis acceleration and 3-axis gyro data
struct IMU_DataStructure
{
  f32 acc_x;  ///< mm/s/s
  f32 acc_y;  ///< mm/s/s
  f32 acc_z;  ///< mm/s/s
  f32 rate_x; ///< rad/s
  f32 rate_y; ///< rad/s
  f32 rate_z; ///< rad/s

  f32 temperature_degC;
};

/** Read acceleration and rate
 * x-axis points out cozmo's face
 * y-axis points out of cozmo's left
 * z-axis points out the top of cozmo's head
 */
bool IMUReadData(IMU_DataStructure &imuData);

/************************************************************************
 * \section Motors
 */

/** Returns the calibration power to be used for the specified motor
 * @param[in] Motor for which to retrieve calibration power
 * @return Calibration power in unitless range [-1.0, 1.0]
 */
float MotorGetCalibPower(MotorID motor);

/** Set motor drive voltage and direction
 * Positive numbers move the motor forward or up, negative is back or down
 * @param[in] motor The motor to Update
 * @param[in] power unitless range [-1.0, 1.0]
 */
void MotorSetPower(const MotorID motor, const f32 power);

/// Reset the internal position of the specified motor to 0
void MotorResetPosition(const MotorID motor);

/** Returns units based on the specified motor type:
 * Note: this function must be called once per tick for each motor
 * @param[in] Motor to retrieve
 * @return Wheels are in mm/s, everything else is in radians/s.
 */
f32 MotorGetSpeed(const MotorID motor);

/** Returns units based on the specified motor type:
 * @param[in] Motor to retrieve
 * @return Wheels are in mm since reset, everything else is in radians.
 */
f32 MotorGetPosition(const MotorID motor);

/// Measures the unitless load on all motors
s32 MotorGetLoad();

/************************************************************************
 * \section Proximity / Cliff sensors
 */

/// Ids for cliff sensors
typedef enum
{
  CLIFF_FL = 0, ///< Front left
  CLIFF_FR,     ///< Front right
  CLIFF_BL,     ///< Back left
  CLIFF_BR,     ///< Back right
  CLIFF_COUNT
} CliffID; //TODO: assert matches DropSensor, or use directly

/// Forward proximity sensor
ProxSensorDataRaw GetRawProxData();

/// Cliff sensors
u16 GetRawCliffData(const CliffID cliff_id);

/************************************************************************
 * \section Microphones
 */

using SendDataFunction = Result (*)(const s16* latestMicData, uint32_t numSamples);

/** Grants access to microphone data from this tick.
 * @param[in] Provides a function pointer for actually sending out the message using the mic data
 * @return true if more data needs to be sent (and this should be called again) false otherwise
 */
bool HandleLatestMicData(SendDataFunction sendDataFunc);

/************************************************************************
 * \section Buttons
 */

/// Button IDs
typedef enum
{
  BUTTON_CAPACITIVE = 0,
  BUTTON_POWER = 1,
  BUTTON_COUNT
} ButtonID;

/** Retrieve current button state
 * @param[in] button_id The button to retrieve
 * @return Mechanical buttons return 0 or 1. Capacitive buttons return an analog value
 */
u16 GetButtonState(const ButtonID button_id);
 
/************************************************************************
 * \section Battery
 */

/// Get the battery voltage in volts.
// Note: To get a more accurate battery voltage, use the linux system
//       voltage available from the OSState library. This is a voltage
//       estimate from syscon which is not as accurate as the system voltage.
f32 BatteryGetVoltage();

/// Return whether or not the battery is charging
bool BatteryIsCharging();

/// Return whether or not the robot is connected to a charger
bool BatteryIsOnCharger();

/// Return whether or not the battery has been disconnected from the charging circuit
/// after being on charge base for more than 30 min.
bool BatteryIsDisconnected();

// Return temperature of battery in C
u8 BatteryGetTemperature_C();

// Whether or not the battery is overheating.
// Syscon will shutoff 30s after this first becomes true.
bool BatteryIsOverheated();

/// Return detected charger voltage
f32 ChargerGetVoltage();
/************************************************************************
 * \section LEDs
 */

/// LED identifiers
typedef enum
{
  LED_BACKPACK_FRONT = 0,
  LED_BACKPACK_MIDDLE,
  LED_BACKPACK_BACK,
  LED_COUNT
} LEDId;

enum {
    LED_RED_SHIFT= 24,
    LED_GRN_SHIFT= 16,
    LED_BLU_SHIFT= 8,
    LED_CHANNEL_MASK= 0xFF
};


/** Set LED to specific color
 * @param[in] led_id The LED to Set
 * @param[in] color 32 bit RGBA
 */
void SetLED(const LEDId led_id, const u32 color);
void SetSystemLED(u32 color);

/************************************************************************
 * \section Power management
 */

/// Run levels for the hardware
typedef enum
{
  POWER_MODE_ACTIVE           = 0x0,
  POWER_MODE_CALM             = 0x1,
} PowerState;

/** Command syscon to enter specified power state
 */
void PowerSetDesiredMode(const PowerState state);

/** Get last desired syscon mode that was commanded
 */
PowerState PowerGetDesiredMode();

/** Get syscon's current power state
 */
PowerState PowerGetMode();

/** Get syscon's current power state
 */
PowerState PowerGetMode();

/************************************************************************
 * \section "Radio" comms to/from engine
 */
bool RadioIsConnected();

void DisconnectRadio(bool sendDisconnectMsg = true);

/** Gets the next packet from the radio
 * @param buffer [out] A buffer into which to copy the packet. Must have MTU bytes available
 * return The number of bytes of the packet or 0 if no packet was available.
 */
u32 RadioGetNextPacket(u8* buffer);

/** Send a packet on the radio.
 * @param buffer [in] A pointer to the data to be sent
 * @param length [in] The number of bytes to be sent
 * @return true if the packet was queued for transmission, false if it couldn't be queued.
 */
bool RadioSendPacket(const void *buffer, const size_t length);

/** Wrapper method for sending messages NOT PACKETS
 * @param msgID The ID (tag) of the message to be sent
 * @param buffer A pointer to the message to be sent
 * @return True if successfully queued, false otherwise
 */
bool RadioSendMessage(const void *buffer, const u16 size, const u8 msgID);


/************************************************************************
 * \section Hardware / Firmware Version information
 */

/// Returns the unique serial number of the robot
u32 GetID();

/************************************************************************
 * \section Error reporting
 */

/// Force a hard fault in the processor, used for hardware assert
void FORCE_HARDFAULT();
#define HAL_ASSERT(c) do { if(!(c)) FORCE_HARDFAULT(); } while(0)

} // namespace HAL
} // namespace Vector
} // namespace Anki

#endif // ANKI_COZMO_ROBOT_HARDWAREINTERFACE_H
