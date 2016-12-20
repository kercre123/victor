
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
#include "anki/types.h"
#include "anki/cozmo/shared/cozmoConfig.h"
#include "clad/types/animationKeyFrames.h"
#include "clad/types/imageTypes.h"
#include "clad/types/ledTypes.h"
#include "clad/types/motorTypes.h"
#include "clad/types/nvStorageTypes.h"
#include "clad/robotInterface/messageToActiveObject.h"
#include "clad/types/cameraParams.h"

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
#ifdef TARGET_K02
#define DISABLE_PRINT_MACROS 1
#else
#define DISABLE_PRINT_MACROS 0
#endif

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

#ifdef __cplusplus
extern "C"
#endif 
void FORCE_HARDFAULT(void);
#ifndef HAL_ASSERT
#define HAL_ASSERT(c) do { if(!(c)) FORCE_HARDFAULT(); } while(0)
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
#define PRINT(...) Anki::Cozmo::Messages::SendText(__VA_ARGS__)
#define PERIODIC_PRINT(num_calls_between_prints, ...)\
{ \
  static u16 cnt = num_calls_between_prints; \
  if (cnt++ >= num_calls_between_prints) { \
    Anki::Cozmo::Messages::SendText(__VA_ARGS__); \
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

#define PRINT(...) Anki::Cozmo::Messages::SendText(__VA_ARGS__)
#define PERIODIC_PRINT(num_calls_between_prints, ...)\
{ \
  static u16 cnt = num_calls_between_prints; \
  if (cnt++ >= num_calls_between_prints) { \
    Anki::Cozmo::Messages::SendText(__VA_ARGS__); \
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

      void AudioFill(void);

      // @return true if the audio clock says it is time for the next frame
      bool AudioReady();

      // Play one frame of audio or silence
      // @param frame - a pointer to an audio frame or NULL to play one frame of silence
      void AudioPlayFrame(AnimKeyFrame::AudioSample *msg);
      void AudioPlaySilence();

// #pragma mark --- Flash Memory ---
      /////////////////////////////////////////////////////////////////////
      // FLASH MEMORY
      //

      /** Write to flash
       * @param address The flash address to start writing at
       * @param data Pointer to the data to write
       * @param length How many bytes of data to write
       * @return NV_OKAY on success or other on error.
       */
      NVStorage::NVResult FlashWrite(u32 address, u32* data, u32 length);
      /** Read from flash
       * @param address The flash address to start reading from
       * @param data Pointer to buffer to read into, must have at least length bytes available
       * @param length How many bytes of data to read.
       * @return NV_OKAY on success or other on error.
       */
      NVStorage::NVResult FlashRead (u32 address, u32* data, u32 length);
      /** Erase a sector of flash.
       * @param address The address of the start of the sector to erase.
       * @return NV_OKAY on success or other on failure.
       */
      NVStorage::NVResult FlashErase(u32 address);

      /** Initalize flash controller
       */
      void FlashInit();

// #pragma mark --- IMU ---
      /////////////////////////////////////////////////////////////////////

      // IMU_DataStructure contains 3-axis acceleration and 3-axis gyro data
      struct IMU_DataStructure
      {
        void Reset() {
          acc_x = acc_y = acc_z = 0;
          rate_x = rate_y = rate_z = 0;
        }
        
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
      bool IMUReadData(IMU_DataStructure &IMUData);

      // Read raw unscaled IMU values
      void IMUReadRawData(int16_t* accel, int16_t* gyro, uint8_t* timestamp);

      // Calculate the camera relative time of the latest sample
      void IMUGetCameraTime(uint32_t* const frameNumber, uint8_t* const line2Number);
      
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
      
      void CameraGetDefaultParameters(DefaultCameraParams& params);

      // Sets the camera parameters (non-blocking call)
      void CameraSetParameters(u16 exposure_ms, f32 gain);

      // Starts camera frame synchronization (blocking call)
      void CameraGetFrame(u8* frame, ImageResolution res, bool enableLight);

      // Return the current scan line time
      u16 CameraGetScanLine();
      
      // Get the camera frame number -- counts from camera start
      u32 CameraGetFrameNumber();
      
      // Get the number of scan lines of delay due to current exposure settings
      u16 CameraGetExposureDelay();

#     ifdef SIMULATOR
      u32 GetCameraStartTime();
      u32 GetVisionTimeStep();
#     endif

      bool IsVideoEnabled();

      // Get the number of lines received so far for the specified camera
      //u32 CameraGetReceivedLines(CameraID cameraID);

      // Set the streaming mode of camera images
      void SetImageSendMode(const ImageSendMode mode, const ImageResolution res);

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

/*
      // Interrupt driven proxmity (CALL AT BEGINNING OF LOOP)
      // Note: this function is pipelined. // latency ~= 5 ms (1 main loop)
      //       - returns data (from last function call)
      //       - wake up the next sensor
      //       - wait ~3.5 ms
      //       - read from sensor
      // Only call once every 5ms (1 main loop)
      // current order is left -> right -> forward
      void GetProximity(ProximityValues *prox);
*/
      
      // Returns distance in mm
      // If 0, nothing is detected
      u8 GetForwardProxSensorCurrentValue();
      
// #pragma mark --- Battery ---
      /////////////////////////////////////////////////////////////////////
      // BATTERY
      //

      // Get the battery voltage in volts
      f32 BatteryGetVoltage();

      // Return whether or not the battery is charging
      bool BatteryIsCharging();

      // Return whether or not the robot is connected to a charger
      bool BatteryIsOnCharger();
      
      bool BatteryIsChargerOOS();

// #pragma mark --- UI LEDS ---
      /////////////////////////////////////////////////////////////////////
      // UI LEDs

      // Light up one of the eye LEDs to the specified 24-bit RGB color
      void SetLED(LEDId led_id, u16 color);

      // Turn headlights on (true) and off (false)
      void SetHeadlights(bool state);

      u16 GetRawCliffData();
      
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
      void FaceAnimate(u8* frame, const u16 length);

      // Move the face to an X, Y offset - where 0, 0 is centered, negative is left/up
      // This position is relative to the animation displayed when FaceAnimate() was
      // last called.
      void FaceMove(s32 x, s32 y);

      // Clear the currently animated face
      void FaceClear();

      // Print a message to the face - this will permanently replace the face with your message
      extern "C" void FacePrintf(const char *format, ...);

      // Restore normal operation of the face from a FacePrintf
      extern "C" void FaceUnPrintf(void);

// #pragma mark --- Radio ---
      /////////////////////////////////////////////////////////////////////
      // RADIO
      //
      bool RadioIsConnected();

      void RadioUpdateState(u8 wifi, u8 blue);

      int RadioQueueAvailable();

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
       * @return True if sucessfully queued, false otherwise
       */
      bool RadioSendMessage(const void *buffer, const u16 size, const u8 msgID);

      /////////////////////////////////////////////////////////////////////
      // BLOCK COMMS
      //
      void FlashBlockIDs();

#if defined(SIMULATOR)
      bool AssignSlot(u32 slot_id, u32 factory_id);
#endif
      
      // Set the color and flashing of each LED on a block separately
      Result SetBlockLight(const u32 activeID, const u16* colors);
      
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

      // Returns the unique serial number of the robot
      u32 GetID();

      // For board-level debugging only - read the comments in uart.cpp or find a better printf
      void BoardPrintf(const char *format, ...);
      namespace Power
      {
        void enterSleepMode(void);
      }
      
      // Retuns the watchdog reset counter
      u16 GetWatchdogResetCounter(void);
      
    } // namespace HAL
  } // namespace Cozmo
} // namespace Anki

#endif // ANKI_COZMO_ROBOT_HARDWAREINTERFACE_H
