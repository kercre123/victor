/**
 * File: hal.h
 *
 * Author: Andrew Stein (andrew)
 * Created: 10/10/2013
 * Author: Daniel Casner <daniel@anki.com>
 * Revised for Cozmo 2: 02/27/2017
 *
 * Information on last revision to this file:
 *    $LastChangedDate$
 *    $LastChangedBy$
 *    $LastChangedRevision$
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
#include "anki/types.h"

namespace Anki
{
  namespace Cozmo
  {

    namespace HAL
    {

/************************************************************************
 * \section Parameters and Constants
 */
       
      /// Number of time stamp ticks per second
      static const int TICKS_PER_SECOND = 200;
      /// Scale value for maximum motor power in HAL
      static const f32 MOTOR_MAX_POWER = 1.0f;
      /// Number of bytes of audio data fetche each time step
      static const size_t AUDIO_DATA_PER_TICK = 2560;
      /// Maximum number of object advertisements reported in a time step
      static const size_t MAX_ADVERTISEMENTS_PER_TICK = 16;

/************************************************************************
 * \section Time
 */

      /// Get the number of microseconds since boot
      u32 GetMicroCounter(void);
      
      /// Block main execution for specified number of microseconds
      void MicroWait(u32 microseconds);

      /** Retrieve current time robot time stamp
       * TimeStamps are in units of approximate miliseconds but are ticked by the hardware processor update at
       * TICKS_PER_SECOND. The engine sets timestamp on connect to the robot process but otherwise it increases
       * monotonically.
       */
      TimeStamp_t GetTimeStamp(void);
      
      /** Sets the robot timestamp to value
       * Set timing offset on engine connect.
       */
      void SetTimeStamp(TimeStamp_t t);

      /// Retrieve the number of times a hardware watchdog reset has occured.
      u8 GetWatchdogResetCounter(void);

/************************************************************************
 * \section IMU Interface
 */

      /// IMU_DataStructure contains 3-axis acceleration and 3-axis gyro data
      struct IMU_DataStructure
      {
        inline void Reset() {
          acc_x = acc_y = acc_z = 0.0f;
          rate_x = rate_y = rate_z = 0.0f;
        }
        
        f32 acc_x;  ///< mm/s/s
        f32 acc_y;  ///< mm/s/s
        f32 acc_z;  ///< mm/s/s
        f32 rate_x; ///< rad/s
        f32 rate_y; ///< rad/s
        f32 rate_z; ///< rad/s
      };

      /** Read acceleration and rate
       * x-axis points out cozmo's face
       * y-axis points out of cozmo's left
       * z-axis points out the top of cozmo's head
       */
      bool IMUReadData(IMU_DataStructure &IMUData);

      /// Read raw unscaled IMU values
      void IMUReadRawData(int16_t* accel, int16_t* gyro, uint8_t* timestamp);

/************************************************************************
 * \section Motors
 */

      /// IDs for motors
      typedef enum
      {
        MOTOR_LEFT_WHEEL = 0,
        MOTOR_RIGHT_WHEEL,
        MOTOR_LIFT,
        MOTOR_HEAD,
        MOTOR_COUNT
      } MotorID;

      /** Set motor drive voltage and direction
       * Positive numbers move the motor forward or up, negative is back or down
       * @param[in] motor The motor to Update
       * @param[in] power unitless range [-1.0, 1.0]
       */
      void MotorSetPower(const MotorID motor, const f32 power);

      /// Reset the internal position of the specified motor to 0
      void MotorResetPosition(const MotorID motor);

      /** Returns units based on the specified motor type:
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
      } CliffID;
      
      /// Face proximity sensor
      u16 GetRawProxData();

      /// Cliff sensors
      u16 GetRawCliffData(const CliffID cliff_id);

/************************************************************************
 * \section Microphones
 */
      
      /** Decompresses microphone data from this tick into buffer.
       * @param[out] A buffer with space for AUDIO_DATA_PER_TICK to receive audio data.
       */ 
      void GetMicrophoneData(u8* buffer);

/************************************************************************
 * \section Buttons
 */

      /// Button IDs
      typedef enum 
      {
        BUTTON_POWER = 0,
        BUTTON_CAPACITIVE,
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

      /// Get the battery voltage in volts
      f32 BatteryGetVoltage();

      /// Return whether or not the battery is charging
      bool BatteryIsCharging();

      /// Return whether or not the robot is connected to a charger
      bool BatteryIsOnCharger();
      
      /// Return whether the USB charger is out of spec (cannot supply enough current)
      bool BatteryIsChargerOOS();

/************************************************************************
 * \section Leds
 */

      /// LED identifiers
      typedef enum 
      {
        LED_BACKPACK_FRONT = 0, ///< Front / top most backpack LED
        LED_BACKPACK_MIDDLE,    ///< Middle backpack LED
        LED_BACKPACK_BACK,      ///< Back / bottom most backpack LED. Includes IR channel
        LED_HEADLIGHT,          ///< Forward facing IR illuminator
        LED_COUNT
      };

      /** Set LED to specific color, includes backpack, headlight and backpack IR
       * @param[in] led_id The LED to Set
       * @param[in] color 16 bit packed color value 5 bits red, 5 bits green, 5 bits blue, 1 bit ir (if present)
       */
      void SetLED(const LEDId led_id, const u16 color);
      
/************************************************************************
 * \section Accessory Interface
 */

      /// Data about an unconnected object advertisement
      struct ObjectAdvertisement
      {
        u32 factory_id,    ///< Factory ID of object, used for slot assignment
        u32 lastHeardTick, ///< Last tick heard from
        s32 rssi           ///< RSSI of last advertisement packet
      }

      /** Retrieves the list of most recently heard from unassigned object advertisements.
       * @param[out] objects An array to fill with advertisement data. Must have room for MAX_ADVERTISEMENTS_PER_TICK
       * entries.
       * @return Number of entries populated.
       */
      int GetObjectAdvertisements(ObjectAdvertisement* objects);

      /// Assign a cube to a slot
      Result AssignSlot(const u32 activeID, const u32 factory_id);
      
      /** Set the color of each LED on the accessory
       * @param[in] activeID Accessory slot to update
       * @param[in] colors an array of 4 16 bit packed color values
       */
      Result SetBlockLight(const u32 activeID, const u16[4] colors);
      
      /// Enable or disable streaming accelerometer data from object at activeID
      Result StreamObjectAccel(const u32 activeID, const bool enable);
      
      /** Retrieve cube accelerometer data returned this tick
       * @param[out] activeID Will be set to the ID of the accessory who's data has been received
       * @return Pointer to raw accelerometer data received for that cube this tick or NULL if no data available.
       */
      u8* GetObjectAccelData(u32* activeID);
      
/************************************************************************
 * \section Power management
 */
      
      /// Run levels for the hardware
      typedef enum 
      {
        POWER_STATE_OFF               = 0x00,
        POWER_STATE_OFF_WAKE_ON_RADIO = 0x01,
        POWER_STATE_ON                = 0x02,
        POWER_STATE_IDLE              = 0x03,
        POWER_STATE_FORCE_RECOVERY    = 0x04,
        POWER_STATE_OTA_MODE          = 0x08,
        POWER_STATE_CHARGER_TEST_MODE = 0x41,
        POWER_STATE_DTM_MODE          = 0x42,
      } PowerState;
      
      /** Command hardware to enter specified power state
       * @warning Some power states will power off the android processor
       */
      void PowerSetMode(const PowerState state);
      
      
/************************************************************************
 * \section Hardware / Firmware Version information
 */
      
      /// Returns the unique serial number of the robot
      u32 GetID();
      
/************************************************************************
 * \section Error reporting
 */
      
      /// Sends formatted text out for debugging
      Result SendText(const LogLevel level, const char *format, ...);
      
      /// Force a hard fault in the processor, used for hardware assert
      void FORCE_HARDFAULT();
      #define HAL_ASSERT(c) do { if(!(c)) FORCE_HARDFAULT(); } while(0)
      
    } // namespace HAL
  } // namespace Cozmo
} // namespace Anki

#endif // ANKI_COZMO_ROBOT_HARDWAREINTERFACE_H
