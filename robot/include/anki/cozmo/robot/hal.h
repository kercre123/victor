
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

#include "messages.h"

#ifndef ANKI_COZMO_ROBOT_HARDWAREINTERFACE_H
#define ANKI_COZMO_ROBOT_HARDWAREINTERFACE_H
#include "anki/common/robot/config.h"
#include "anki/common/robot/utilities_c.h"
#include "anki/common/types.h"
#include "anki/common/constantsAndMacros.h"
#include "anki/vision/CameraSettings.h"
#include "anki/cozmo/shared/cozmoConfig.h"
#include "anki/cozmo/shared/cozmoTypes.h"
#include "anki/cozmo/shared/ledTypes.h"

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
      extern "C" TimeStamp_t GetTimeStamp(void);
      extern "C" void SetTimeStamp(TimeStamp_t t);

// #pragma mark --- Processor ---
      /////////////////////////////////////////////////////////////////////
      // PROCESSOR
      //
      extern "C" {
        void EnableIRQ();
        void DisableIRQ();
      }


// #pragma mark --- Audio ---
      /////////////////////////////////////////////////////////////////////
      // AUDIO
      //

      // @return true if the audio clock says it is time for the next frame
      bool AudioReady();

      // Play one frame of audio or silence
      // @param frame - a pointer to an audio frame or NULL to play one frame of silence
      void AudioPlayFrame(u8* frame);

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
      // x-axis points out cozmo's face
      // y-axis points out of cozmo's left
      // z-axis points out the top of cozmo's head
      // NB: DO NOT CALL THIS MORE THAN ONCE PER MAINEXECUTION TIC!!!
      void IMUReadData(IMU_DataStructure &IMUData);

// #pragma mark --- UART/Wifi ---
      /////////////////////////////////////////////////////////////////////
      // UART Debug Channel (aka Wifi)
      //

      int UARTPrintf(const char* format, ...);
      int UARTPutChar(int c);

      // Puts an entire message, with the usual header/footer
      // Returns false is there wasn't enough space to buffer the message
      bool UARTPutPacket(const u8* buffer, const u32 length, const u8 socket=0);

      void UARTPutString(const char* s);
      int UARTGetChar();

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

#     if SIMULATOR
      u32 GetCameraStartTime();
#     endif
      // Get the number of lines received so far for the specified camera
      //u32 CameraGetReceivedLines(CameraID cameraID);

      // Set the streaming mode of camera images
      void SetImageSendMode(const ImageSendMode_t mode, const Vision::CameraResolution res);

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

      // Get the battery voltage x10
      u8 BatteryGetVoltage10x();

      // Return whether or not the battery is charging
      bool BatteryIsCharging();

      // Return whether or not the robot is connected to a charger
      bool BatteryIsOnCharger();

// #pragma mark --- UI LEDS ---
      /////////////////////////////////////////////////////////////////////
      // UI LEDs

      // Light up one of the eye LEDs to the specified 24-bit RGB color
      void SetLED(LEDId led_id, u32 color);

      // Turn headlights on (true) and off (false)
      void SetHeadlights(bool state);

// #pragma mark --- Face ---
      /////////////////////////////////////////////////////////////////////
      // Face

      // Update the face to the next frame of an animation
      // @param frame - a pointer to a variable length frame of face animation data
      // Frame is in 8-bit RLE format:
      //  0 terminates the image
      //  1-63 draw N full lines (N*128 pixels) of black or blue
      //  64-255 draw 0-191 pixels (N-64) of black or blue, then invert the color for the next run
      // The decoder starts out drawing black, and inverts the color on every byte >= 64
      void FaceAnimate(u8* frame);

      // Move the face to an X, Y offset - where 0, 0 is centered, negative is left/up
      // This position is relative to the animation displayed when FaceAnimate() was
      // last called.
      void FaceMove(s32 x, s32 y);

      // Blink the eyes
      void FaceBlink();

// #pragma mark --- Radio ---
      /////////////////////////////////////////////////////////////////////
      // RADIO
      //
      bool RadioIsConnected();

      void RadioUpdateState(u8 wifi, u8 blue);

      void DisconnectRadio();

      /** Gets the next packet from the radio
       * @param buffer [out] A buffer into which to copy the packet. Must have MTU bytes available
       * return The number of bytes of the packet or 0 if no packet was available.
       */
      u32 RadioGetNextPacket(u8* buffer);

      /** Send a packet on the radio.
       * @param buffer [in] A pointer to the data to be sent
       * @param length [in] The number of bytes to be sent
       * @param socket [in] Socket number, default 0 (base station)
       * @return true if the packet was queued for transmission, false if it couldn't be queued.
       */
      bool RadioSendPacket(const void *buffer, const u32 length, const u8 socket=0);

      /** Wrapper method for sending messages NOT PACKETS
       * @param msgID The ID (tag) of the message to be sent
       * @param buffer A pointer to the message to be sent
       * @param reliable Specifify if the message should be transferred reliably. Default true.
       * @param hot Specify if the message is hot and needs to be sent imeediately. Default false.
       * @return True if sucessfully queued, false otherwise
       */
      bool RadioSendMessage(const int msgID, const void *buffer, const bool reliable=true, const bool hot=false);

      /** Special method for sending images (from long execution) for thread safety.
       * This method always sends the message as unreliable and hot but is queued until the main execution thread picks
       * it up and sends it through reliable transport.
       * @param chunkData a pointer to the imageChunk Message data
       * @param length The number of bytes of chunk data to send
       * @return true if the message was successfully queued, false otherwise.
       */
      bool RadioSendImageChunk(const void* chunkData, const uint16_t length);

      /////////////////////////////////////////////////////////////////////
      // BLOCK COMMS
      //
      void FlashBlockIDs();

      // Set the color and flashing of each LED on a block separately
      Result SetBlockLight(const u8 blockID, const u32* onColor, const u32* offColor,
                           const u32* onPeriod_ms, const u32* offPeriod_ms,
                           const u32* transitionOnPeriod_ms, const u32* transitionOffPeriod_ms);



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
