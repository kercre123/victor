
/**
 * File: hal.h
 *
 * Author: Andrew Stein (andrew)
 * Created: 10/10/2013
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
 * Copyright: Anki, Inc. 2013
 *
 **/


#ifndef ANKI_COZMO_ROBOT_HARDWAREINTERFACE_H
#define ANKI_COZMO_ROBOT_HARDWAREINTERFACE_H
#include "anki/common/robot/config.h"
#include "anki/common/robot/utilities_c.h"
#include "anki/common/types.h"
#include "anki/common/constantsAndMacros.h"
#include "anki/vision/CameraSettings.h"
#include "messages.h"

#include "anki/cozmo/robot/cozmoConfig.h"

#define HAVE_ACTIVE_GRIPPER 0

// Set to 0 if you want to read printf output in a terminal and you're not
// using UART as radio. The radio is effectively disabled in this case.
// Set to 1 if using UART as radio. This disables PRINT calls only from
// going out UART. printf will still do it and probably corrupt comms
#define USING_UART_RADIO 1

// Diverts PRINT() statements to radio.
// USING_UART_RADIO must be 1 for this to work.
#define DIVERT_PRINT_TO_RADIO 1

// Disables PRINT macros entirely.
// When 1, overrides all of the above macros.
#define DISABLE_PRINT_MACROS 0

// Enable to stream debug images via UART to Pete's tool.
// This disables PRINT macros so they don't disrupt the stream of debug and image data.
// Useful for debugging trackers.
#define STREAM_DEBUG_IMAGES 0


#ifdef __cplusplus
extern "C" {
#endif
#include <stdio.h>
#ifdef __cplusplus
}
#endif


#if(STREAM_DEBUG_IMAGES)
#undef DISABLE_PRINT_MACROS
#define DISABLE_PRINT_MACROS 1
#endif

#if(DISABLE_PRINT_MACROS)

#define PRINT(...)
#define PERIODIC_PRINT(num_calls_between_prints, ...)

#else // #if(DISABLE_PRINT_MACROS)

#ifndef SIMULATOR

#if(USING_UART_RADIO)

#if(DIVERT_PRINT_TO_RADIO)
#define PRINT(...) Messages::SendText(__VA_ARGS__)
#define PERIODIC_PRINT(num_calls_between_prints, ...)\
{ \
  static u16 cnt = num_calls_between_prints; \
  if (cnt++ >= num_calls_between_prints) { \
    Messages::SendText(__VA_ARGS__); \
    cnt = 0; \
  } \
}
#else
#define PRINT(...)
#define PERIODIC_PRINT(num_calls_between_prints, ...)
#endif // #if(DIVERT_PRINT_TO_RADIO)

#else  // #if(USING_UART_RADIO)

#define PRINT(...) printf(__VA_ARGS__)

// Prints once every num_calls_between_prints times you call it
#define PERIODIC_PRINT(num_calls_between_prints, ...)  \
{ \
  static u16 cnt = num_calls_between_prints; \
  if (cnt++ >= num_calls_between_prints) { \
    printf(__VA_ARGS__); \
    cnt = 0; \
  } \
}

#endif // if (USING_UART_RADIO)

#elif defined(SIMULATOR) // #ifndef SIMULATOR

#if(USING_UART_RADIO && DIVERT_PRINT_TO_RADIO)

#define PRINT(...) Messages::SendText(__VA_ARGS__)
#define PERIODIC_PRINT(num_calls_between_prints, ...)\
{ \
  static u16 cnt = num_calls_between_prints; \
  if (cnt++ >= num_calls_between_prints) { \
    Messages::SendText(__VA_ARGS__); \
    cnt = 0; \
  } \
}
#else

#define PRINT(...) fprintf(stdout, __VA_ARGS__)

#define PERIODIC_PRINT(num_calls_between_prints, ...)  \
{ \
  static u16 cnt = num_calls_between_prints; \
  if (cnt++ >= num_calls_between_prints) { \
    fprintf(stdout, __VA_ARGS__); \
    cnt = 0; \
  } \
}

#endif // #if(USING_UART_RADIO && DIVERT_PRINT_TO_RADIO)

#endif  // #elif defined(SIMULATOR)

#endif // #if(DISABLE_PRINT_MACROS)

namespace Anki
{
  namespace Cozmo
  {

    namespace HAL
    {

      //
      // Parameters / Constants
      //
      const f32 MOTOR_MAX_POWER = 1.0f;

      ///////////////////
      // TODO: The following are constants for a naive linear approximation of power to speed,
      // which is definitely a non-linear relationship. Eventually, we should figure out the true
      // relationship on the robot so that the simulator can approximate it.

      // The max angular speed the head can move when max power is commanded.
      const f32 MAX_HEAD_SPEED = 2*PI; // rad/s

      // The max angular speed the head can move when max power is commanded.
      const f32 MAX_LIFT_SPEED = PI/2; // rad/s

      //////////////////////

      //
      // Simulator-only functions - not needed by real hardware
      // TBD:  If these aren't hardware features, can they go elsewhere?
      //
#ifndef ROBOT_HARDWARE
      // Define a function pointer type for returning a frame like so:
      //   img = fcn();
      typedef const u8* (*FrameGrabber)(void);

      Result Init(void);
      void Destroy(void);

      // Gripper control
      bool IsGripperEngaged();

      // Misc simulator-only stuff
      bool IsInitialized();
      void UpdateDisplay();

      s32 GetRobotID(void);

      // Take a step (needed for webots only)
      Result Step(void);

      // Ground truth (no-op if not in simulation)
      void GetGroundTruthPose(f32 &x, f32 &y, f32& rad);

      // For simulator only to activate connector for gripping.
      // Does not actually guarantee a physical connection.
      void EngageGripper();
      void DisengageGripper();
      
      // Returns pointer to IPv4 address
      const char* const GetLocalIP();
#endif

      //
      // Hardware Interface Methods:
      //
            
      // Get the number of microseconds since boot
      u32 GetMicroCounter(void);
      void MicroWait(u32 microseconds);

      // Get a sync'd timestamp (e.g. for messages), in milliseconds
      TimeStamp_t GetTimeStamp(void);
      void SetTimeStamp(TimeStamp_t t);

// #pragma mark --- Audio ---
      /////////////////////////////////////////////////////////////////////
      // AUDIO
      //

      const u32 AUDIO_SAMPLE_SIZE = 480;

      // Play an audio sample at 24 kHz. Returns true if it was played.
      bool AudioPlay(s16 buffer[AUDIO_SAMPLE_SIZE]);

// #pragma mark --- Flash Memory ---
      /////////////////////////////////////////////////////////////////////
      // FLASH MEMORY
      //

      const u32 FLASH_PAGE_SIZE = 4 * 1024;
      void FlashWrite(u32 page, u8 data[FLASH_PAGE_SIZE]);
      void FlashRead(u32 page, u8 data[FLASH_PAGE_SIZE]);

// #pragma mark --- IMU ---
      /////////////////////////////////////////////////////////////////////

      // IMU_DataStructure contains 3-axis acceleration and 3-axis gyro data
      struct IMU_DataStructure
      {
        f32 acc_x;      // mm/s/s    
        f32 acc_y;
        f32 acc_z;
        f32 rate_x;     // rad/s
        f32 rate_y;
        f32 rate_z;
      };

      // Read acceleration and rate
      void IMUReadData(IMU_DataStructure &IMUData);

// #pragma mark --- UART/Wifi ---
      /////////////////////////////////////////////////////////////////////
      // UART Debug Channel (aka Wifi)
      //

      int UARTPrintf(const char* format, ...);
      int UARTPutChar(int c);

      // Puts an entire message, with the usual header/footer
      // Returns false is there wasn't enough space to buffer the message
      bool UARTPutMessage(u8 msgID, u32 timestamp, u8* buffer, u32 length);
      
      void UARTPutString(const char* s);
      int UARTGetChar(u32 timeout = 0);

// #pragma mark --- Motors ---
      /////////////////////////////////////////////////////////////////////
      // MOTORS
      //

      enum MotorID
      {
        MOTOR_LEFT_WHEEL = 0,
        MOTOR_RIGHT_WHEEL,
        MOTOR_LIFT,
        MOTOR_HEAD,
        //MOTOR_GRIP,
        MOTOR_COUNT
      };

      // Positive numbers move the motor forward or up, negative is back or down
      // Set the motor power in the unitless range [-1.0, 1.0]
      void MotorSetPower(MotorID motor, f32 power);

      // Reset the internal position of the specified motor to 0
      void MotorResetPosition(MotorID motor);

      // Returns units based on the specified motor type:
      // Wheels are in mm/s, everything else is in radians/s.
      f32 MotorGetSpeed(MotorID motor);

      // Returns units based on the specified motor type:
      // Wheels are in mm since reset, everything else is in radians.
      f32 MotorGetPosition(MotorID motor);

      // Measures the unitless load on all motors
      s32 MotorGetLoad();

// #pragma mark --- Cameras ---
      /////////////////////////////////////////////////////////////////////
      // CAMERAS
      // TODO: Add functions for adjusting ROI of cameras?
      //

      // Intrinsic calibration:
      // A struct for holding intrinsic camera calibration parameters
      typedef struct {
        f32 focalLength_x, focalLength_y;
        f32 center_x, center_y;
        f32 skew;
        u16 nrows, ncols;
        f32 distortionCoeffs[NUM_RADIAL_DISTORTION_COEFFS];
      } CameraInfo;
      
      const CameraInfo* GetHeadCamInfo();

      // Set the camera capture resolution with CAMERA_RES_XXXXX_HEADER.
      //void       SetHeadCamMode(const u8 frameResHeader);
      //CameraMode GetHeadCamMode(void);

      // Sets the camera exposure (non-blocking call)
      // exposure is clipped to [0.0, 1.0]
      void CameraSetParameters(f32 exposure, bool enableVignettingCorrection);
      
      // Starts camera frame synchronization (blocking call)
      void CameraGetFrame(u8* frame, Vision::CameraResolution res, bool enableLight);

      // Get the number of lines received so far for the specified camera
      //u32 CameraGetReceivedLines(CameraID cameraID);

      
      /////////////////////////////////////////////////////////////////////
      // PROXIMITY SENSORS
      //

      enum sharpID
      {
        IRleft,
        IRforward,
        IRright
      };

      typedef struct
      {
        u16           left;
        u16           right;
        u16           forward;
        sharpID       latest;   // Most up to date sensor value
      } ProximityValues;
      
      
      // Interrupt driven proxmity (CALL AT BEGINNING OF LOOP)
      // Note: this function is pipelined. // latency ~= 5 ms (1 main loop)
      //       - returns data (from last function call)
      //       - wake up the next sensor
      //       - wait ~3.5 ms
      //       - read from sensor
      // Only call once every 5ms (1 main loop)
      // current order is left -> right -> forward
      void GetProximity(ProximityValues *prox);
      
// #pragma mark --- Battery ---
      /////////////////////////////////////////////////////////////////////
      // BATTERY
      //

      // Get the battery percent between [0, 100]
      u8 BatteryGetPercent();

      // Return whether or not the battery is charging
      bool BatteryIsCharging();

      // Return whether or not the robot is connected to a charger
      bool BatteryIsOnCharger();

// #pragma mark --- UI LEDS ---
      /////////////////////////////////////////////////////////////////////
      // UI LEDs
      // Updated for "neutral" (non-hardware specific) order in 2.1
      enum LEDId {
        LED_RIGHT_EYE_TOP = 0,
        LED_RIGHT_EYE_RIGHT,
        LED_RIGHT_EYE_BOTTOM,
        LED_RIGHT_EYE_LEFT,
        LED_LEFT_EYE_TOP,
        LED_LEFT_EYE_RIGHT,
        LED_LEFT_EYE_BOTTOM,
        LED_LEFT_EYE_LEFT,
        NUM_LEDS
      };

      // Set the intensity for each LED channel in the range [0, 255]
      void LEDSet(u8 leds[NUM_LEDS]);
      
      // The color format is identical to HTML Hex Triplets (RGB)
      enum LEDColor {
        LED_OFF =   0x000000,
        LED_RED =   0xff0000,
        LED_GREEN = 0x00ff00,
        LED_YELLOW= 0xffff00,
        LED_BLUE =  0x0000ff,
        LED_PURPLE= 0xff00ff,
        LED_CYAN =  0x00ffff,
        LED_WHITE = 0xffffff
      };
      
      // Light up one of the eye LEDs to the specified 24-bit RGB color
      void SetLED(LEDId led_id, u32 color);
      
      // Turn headlights on (true) and off (false)
      void SetHeadlights(bool state);
      
// #pragma mark --- Radio ---
      /////////////////////////////////////////////////////////////////////
      // RADIO
      //
      bool RadioIsConnected();

      Messages::ID RadioGetNextMessage(u8* buffer);

      // Returns true if the message has been sent to the basestation
      bool RadioSendMessage(const Messages::ID msgID, const void *buffer, TimeStamp_t ts = HAL::GetTimeStamp());

      
      /////////////////////////////////////////////////////////////////////
      // POWER MANAGEMENT
      //

      enum PowerState
      {
        POWER_STATE_OFF = 0,
        POWER_STATE_OFF_WAKE_ON_RADIO,
        POWER_STATE_ON,
        POWER_STATE_REBOOT,
        POWER_STATE_IDLE,
        POWER_STATE_CHARGING
      };

      void PowerSetMode(PowerState state);

      // Accelerometer
      // Returns each axis in Gs
      void AccelGetXYZ(f32& x, f32& y, f32& z);

      // Permanent robot information written at factory
      struct IDCard
      {
        u32 esn;
        u32 modelNumber;
        u32 lotCode;
        u32 birthday;
        u32 hwVersion;
      };

      IDCard* GetIDCard();
    } // namespace HAL
  } // namespace Cozmo
} // namespace Anki

#endif // ANKI_COZMO_ROBOT_HARDWAREINTERFACE_H

