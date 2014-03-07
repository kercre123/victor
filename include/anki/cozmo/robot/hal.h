
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

#include "anki/cozmo/robot/messages.h"

#include "anki/cozmo/robot/cozmoConfig.h"

#ifndef USE_OFFBOARD_VISION
#define USE_OFFBOARD_VISION 1
#endif

#ifdef __cplusplus
extern "C" {
#endif
#include <stdio.h>
#ifdef __cplusplus
}
#endif

#ifndef SIMULATOR

#ifdef USE_CAPTURE_IMAGES

#define PRINT(...)
#define PERIODIC_PRINT(num_calls_between_prints, ...)

#else // #ifdef USE_CAPTURE_IMAGES

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
#endif // #ifdef USE_CAPTURE_IMAGES ... #else

#elif defined(SIMULATOR) // #ifndef SIMULATOR

#define PRINT(...) fprintf(stdout, __VA_ARGS__)

#define PERIODIC_PRINT(num_calls_between_prints, ...)  \
{ \
  static u16 cnt = num_calls_between_prints; \
  if (cnt++ >= num_calls_between_prints) { \
    fprintf(stdout, __VA_ARGS__); \
    cnt = 0; \
  } \
}

// Whether or not to use TCP server (0) or Webots emitter/receiver (1)
// as the BTLE channel. (TODO: Phase out webots emitter/receiver)
#define USE_WEBOTS_TXRX 0

#endif  // #elif defined(SIMULATOR)

#define REG_WORD(x) *(volatile u32*)(x)

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
      // Typedefs
      //

      // Define a function pointer type for returning a frame like so:
      //   img = fcn();
      typedef const u8* (*FrameGrabber)(void);


      //
      // Hardware Interface Methods:
      //

      ReturnCode Init(void);
      void Destroy(void);

      // Gripper control
      bool IsGripperEngaged();

      // Misc
      bool IsInitialized();
      void UpdateDisplay();

      // Get the number of microseconds since boot
      u32 GetMicroCounter(void);
      void MicroWait(u32 microseconds);

      // Get the CPU's core frequency
      u32 GetCoreFrequencyMHz();

      // Get a sync'd timestamp (e.g. for messages), in milliseconds
      TimeStamp_t GetTimeStamp(void);

      s32 GetRobotID(void);

      // Take a step (needed for webots, possibly a no-op for real robot?)
      ReturnCode Step(void);

      // Ground truth (no-op if not in simulation?)
      void GetGroundTruthPose(f32 &x, f32 &y, f32& rad);

      // .....................

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

// #pragma mark --- USB / UART ---
      /////////////////////////////////////////////////////////////////////
      // USB / UART
      //

      int UARTPrintf(const char* format, ...);
      int UARTPutChar(int c);
      void UARTPutHex(u8 c);
      void UARTPutHex32(u32 data);
      void UARTPutString(const char* s);
      int UARTGetChar(u32 timeout = 0);

      // Send a variable length buffer
      void USBSendBuffer(const u8* buffer, const u32 size);

      // Returns the number of bytes that are in the serial buffer
      u32 USBGetNumBytesToRead();

      // Get a character from the serial buffer.
      // Timeout is in microseconds.
      // Returns < 0 if no character available within timeout.
      s32 USBGetChar(u32 timeout = 0);

      // Peeks at the offset'th character in serial receive buffer.
      // (The next character available is at an offset of 0.)
      // Returns < 0 if no character available.
      s32 USBPeekChar(u32 offset = 0);

      // Returns an entire message packet in buffer, if one is available.
      // Until a valid packet header is found and the entire packet is
      // available, NO_MESSAGE_ID will be returned.  Once a valid header
      // is found and returned, its MessageID is returned.
      Messages::ID USBGetNextMessage(u8* buffer);

      // Send a byte.
      // Prototype matches putc for printf.
      int USBPutChar(int c);

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

      typedef enum
      {
        CAMERA_FRONT = 0,
        CAMERA_MAT,
        CAMERA_COUNT
      } CameraID;

      typedef enum
      {
        CAMERA_MODE_VGA = 0,
        CAMERA_MODE_QVGA,
        CAMERA_MODE_QQVGA,
        CAMERA_MODE_QQQVGA,
        CAMERA_MODE_QQQQVGA,
        CAMERA_MODE_COUNT,

        CAMERA_MODE_NONE = CAMERA_MODE_COUNT
      } CameraMode;

      typedef struct {
        u8 header; // used to specify a frame's resolution in a packet
        u16 width, height;
        u8 downsamplePower[CAMERA_MODE_COUNT];
      } CameraModeInfo_t;

      const CameraModeInfo_t CameraModeInfo[CAMERA_MODE_COUNT] =
      {
        // VGA
        { 0xBA, 640, 480, {0, 0, 0, 0, 0} },
        // QVGA
        { 0xBC, 320, 240, {1, 0, 0, 0, 0} },
        // QQVGA
        { 0xB8, 160, 120, {2, 1, 0, 0, 0} },
        // QQQVGA
        { 0xBD,  80,  60, {3, 2, 1, 0, 0} },
        // QQQQVGA
        { 0xB7,  40,  30, {4, 3, 2, 1, 0} }
      };

      enum CameraUpdateMode
      {
        CAMERA_UPDATE_CONTINUOUS = 0,
        CAMERA_UPDATE_SINGLE
      };

      void MatCameraInit();
      void FrontCameraInit();

      // Intrinsic calibration:
      // A struct for holding intrinsic camera calibration parameters
      typedef struct {
        f32 focalLength_x, focalLength_y, fov_ver;
        f32 center_x, center_y;
        f32 skew;
        u16 nrows, ncols;
        f32 distortionCoeffs[NUM_RADIAL_DISTORTION_COEFFS];
      } CameraInfo;

      const CameraInfo* GetHeadCamInfo();
      const CameraInfo* GetMatCamInfo() ;

      // Set the camera capture resolution with CAMERA_MODE_XXXXX_HEADER.
      void       SetHeadCamMode(const u8 frameResHeader);
      CameraMode GetHeadCamMode(void);

      // Starts camera frame synchronization
      void CameraStartFrame(CameraID cameraID, u8* frame, CameraMode mode,
          CameraUpdateMode updateMode, u16 exposure, bool enableLight);

      // Get the number of lines received so far for the specified camera
      u32 CameraGetReceivedLines(CameraID cameraID);

      // Returns whether or not the specfied camera has received a full frame
      bool CameraIsEndOfFrame(CameraID cameraID);

      // TODO: At some point, isEOF should be set automatically by the HAL,
      // but currently, the consumer has to set it
      void CameraSetIsEndOfFrame(CameraID cameraID, bool isEOF);

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
      //

      const u32 LED_CHANNEL_COUNT = 8;

      // Set the intensity for each LED channel in the range [0, 255]
      void LEDSet(u8 leds[LED_CHANNEL_COUNT]);

// #pragma mark --- Radio ---
      /////////////////////////////////////////////////////////////////////
      // RADIO
      //
      enum RadioState
      {
        RADIO_STATE_ADVERTISING = 0,
        RADIO_STATE_CONNECTED
      };

      const u32 RADIO_BUFFER_SIZE = 100;

      bool RadioIsConnected();

      u32 RadioGetNumBytesAvailable(void);

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
      struct BirthCertificate
      {
        u32 esn;
        u32 modelNumber;
        u32 lotCode;
        u32 birthday;
        u32 hwVersion;
      };

      const BirthCertificate& GetBirthCertificate();

      // Interrupts
      void IRQDisable();
      void IRQEnable();

	  // TODO: remove when interrupts don't cause problems
	  void DisableCamera(CameraID cameraID);

      // Put a byte into a send buffer to be sent by LongExecution()
      // (Using same prototype as putc / USBPutChar for printf.)
      int USBBufferChar(int c);

      // Send the contents of the USB message buffer.
      void USBSendPrintBuffer(void);

#ifdef SIMULATOR
      // Called by SendFooter() to terminate a message when in simulation,
      // otherwise a no-op.
      void USBFlush();
#endif

#if USE_OFFBOARD_VISION
      const u8 USB_MESSAGE_HEADER = 0xDD;

      // Bytes to add to USB frame header to tell the offboard processor
      // what to do with the frame
      const u8 USB_VISION_COMMAND_DETECTBLOCKS    = 0xAB;
      const u8 USB_VISION_COMMAND_SETTRACKMARKER  = 0xBC;
      const u8 USB_VISION_COMMAND_TRACK           = 0xCD;
      const u8 USB_VISION_COMMAND_ROBOTSTATE      = 0xDE;
      const u8 USB_VISION_COMMAND_MATLOCALIZATION = 0xEF;
      const u8 USB_VISION_COMMAND_DISPLAY_IMAGE   = 0xF0;
      const u8 USB_VISION_COMMAND_SAVE_BUFFERED_DATA = 0x01;

      const u8 USB_VISION_COMMAND_HEAD_CALIBRATION = 0xC1;
      const u8 USB_VISION_COMMAND_MAT_CALIBRATION  = 0xC2;

      enum BufferedDataType
      {
        BUFFERED_DATA_MISC = 0,
        BUFFERED_DATA_IMAGE = 1
      };

      // Send header/footer around each message/packet/image
      void USBSendHeader(const u8 command);
      void USBSendFooter(const u8 command);

      // Send a command message (from messageProtocol.h)
      // The msgID will determine the size
      void USBSendMessage(const void* msg, const Messages::ID msgID);

      // Send a frame at the current frame resolution (last set by
      // a call to SetUSBFrameResolution)
      void USBSendFrame(const u8*        frame,
                        const TimeStamp_t  timestamp,
                        const CameraMode inputResolution,
                        const CameraMode sendResolution,
                        const u8         commandByte);

      // Send an arbitrary packet of data
      void USBSendPacket(const u8 packetType, const void* data, const u32 numBytes);

      // Registur a message name with its ID (e.g. for Matlab, which doesn't
      // read messageProtocol.h directly)
      const u8 USB_DEFINE_MESSAGE_ID = 0xD0;
      void SendMessageID(const char* name, const u8 msgID);

#endif // if USE_OFFBOARD_VISION

      // Definition of the data structures being transferred between SYSCON and
      // the vision processor
      enum SPISource
      {
        SPI_SOURCE_HEAD = 'H',
        SPI_SOURCE_BODY = 'B'
      };
      
      struct GlobalCommon
      {
        SPISource source;
        u8 RESERVED[3];
      };
      
      struct GlobalDataToHead
      {
        GlobalCommon common;
        
        u8 RESERVED[60];  // Pad out to 64 bytes
      };
      
      // TODO: get static_assert to work so we can verify sizeof
      
      struct GlobalDataToBody
      {
        GlobalCommon common;
        s16 motorPWM[MOTOR_COUNT];
        
        u8 reserved[52];  // Pad out to 64 bytes
      };

    } // namespace HAL
  } // namespace Cozmo
} // namespace Anki

#endif // ANKI_COZMO_ROBOT_HARDWAREINTERFACE_H

