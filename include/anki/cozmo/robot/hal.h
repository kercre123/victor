
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

#ifdef __cplusplus
extern "C" {
#endif
#include <stdio.h>
#ifdef __cplusplus
}
#endif
#ifndef SIMULATOR
#undef printf
#define printf(...) _xprintf(Anki::Cozmo::HAL::USBPutChar, 0, __VA_ARGS__)
//#define PRINT(...) _xprintf(Anki::Cozmo::HAL::UARTPutChar, 0, __VA_ARGS__)
#define PRINT(...) explicitPrintf(0, __VA_ARGS__)

// Prints once every num_calls_between_prints times you call it
#define PERIODIC_PRINT(num_calls_between_prints, ...)  \
{ \
  static u16 cnt = num_calls_between_prints; \
  if (cnt++ >= num_calls_between_prints) { \
    explicitPrintf(0, __VA_ARGS__); \
    cnt = 0; \
  } \
}

#elif defined(SIMULATOR)
#define PRINT(...) fprintf(stdout, __VA_ARGS__)

#define PERIODIC_PRINT(num_calls_between_prints, ...)  \
{ \
  static u16 cnt = num_calls_between_prints; \
  if (cnt++ >= num_calls_between_prints) { \
    fprintf(stdout, __VA_ARGS__); \
    cnt = 0; \
  } \
}

#endif  // SIMULATOR

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
      const u8  NUM_RADIAL_DISTORTION_COEFFS = 5;
      const f32 MOTOR_PWM_MAXVAL = 2400.f;
      const f32 MOTOR_MAX_POWER = 1.0f;
      
      ///////////////////
      // TODO: The following are constants for a naive linear approximation of power to speed,
      // which is definitely a non-linear relationship. Eventually, we should figure out the true
      // relationship on the robot so that the simulator can approximate it.
      
      // The max angular speed the head can move when max power is commanded.
      const f32 MAX_HEAD_SPEED = 2*PI; // rad/s
      
      // The max angular speed the head can move when max power is commanded.
      const f32 MAX_LIFT_SPEED = PI/2; // rad/s
      
      const f32 MAX_WHEEL_SPEED = 300; //mm/s
      //////////////////////
      
      
      //
      // Typedefs
      //
      
      // Define a function pointer type for returning a frame like so:
      //   img = fcn();
      typedef const u8* (*FrameGrabber)(void);
      
      // A struct for holding camera parameters
      typedef struct {
        f32 focalLength_x, focalLength_y, fov_ver;
        f32 center_x, center_y;
        f32 skew;
        u16 nrows, ncols;
        f32 distortionCoeffs[NUM_RADIAL_DISTORTION_COEFFS];
      } CameraInfo;
      
      //
      // Hardware Interface Methods:
      //
      
      ReturnCode Init(void);
      void Destroy(void);

      // Gripper control
      bool IsGripperEngaged();
      
      // Cameras
      // TODO: Add functions for adjusting ROI of cameras?
      void MatCameraInit();
      void FrontCameraInit();

      const FrameGrabber GetHeadFrameGrabber();
      const FrameGrabber GetMatFrameGrabber();
      
      const CameraInfo* GetHeadCamInfo();
      const CameraInfo* GetMatCamInfo() ;
      
      // Communications
      bool IsConnected();
      
      // Misc
      bool IsInitialized();
      void UpdateDisplay();

      // Get the number of microseconds since boot
      u32 GetMicroCounter(void);
      void MicroWait(u32 microseconds);

      s32 GetRobotID(void);

      // Take a step (needed for webots, possibly a no-op for real robot?)
      ReturnCode Step(void);

      // Ground truth (no-op if not in simulation?)
      void GetGroundTruthPose(f32 &x, f32 &y, f32& rad);

      // .....................


      // Audio
      const u32 AUDIO_SAMPLE_SIZE = 480;

      // Play an audio sample at 24 kHz. Returns true if it was played.
      bool AudioPlay(s16 buffer[AUDIO_SAMPLE_SIZE]);

      // Flash memory
      const u32 FLASH_PAGE_SIZE = 4 * 1024;
      void FlashWrite(u32 page, u8 data[FLASH_PAGE_SIZE]);
      void FlashRead(u32 page, u8 data[FLASH_PAGE_SIZE]);

      // USB / UART
      // Send a variable length buffer
      void USBSendBuffer(u8* buffer, u32 size);

      // Get a character from the serial buffer.
      // Timeout is in microseconds.
      // Returns < 0 if no character available within timeout.
      s32 USBGetChar(u32 timeout = 0);

      // Send a byte.
      // Prototype matches putc for printf.
      int USBPutChar(int c);

      // Motors
      enum MotorID
      {
        MOTOR_LEFT_WHEEL = 0,
        MOTOR_RIGHT_WHEEL,
        MOTOR_LIFT,
        MOTOR_GRIP,
        MOTOR_HEAD,
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

      // Cameras
      enum CameraID
      {
        CAMERA_FRONT = 0,
        CAMERA_MAT,
        CAMERA_COUNT
      };

      enum CameraMode
      {
        CAMERA_MODE_VGA = 0,
        CAMERA_MODE_QVGA,
        CAMERA_MODE_QQVGA,
        CAMERA_MODE_COUNT,

        CAMERA_MODE_NONE = CAMERA_MODE_COUNT
      };

      enum CameraUpdateMode
      {
        CAMERA_UPDATE_CONTINUOUS = 0,
        CAMERA_UPDATE_SINGLE
      };

      // Starts camera frame synchronization
      void CameraStartFrame(CameraID cameraID, u8* frame, CameraMode mode,
          CameraUpdateMode updateMode, u16 exposure, bool enableLight);

      // Get the number of lines received so far for the specified camera
      u32 CameraGetReceivedLines(CameraID cameraID);

      // Returns whether or not the specfied camera has received a full frame
      bool CameraIsEndOfFrame(CameraID cameraID);

      // Battery
      // Get the battery percent between [0, 100]
      u8 BatteryGetPercent();

      // Return whether or not the battery is charging
      bool BatteryIsCharging();

      // Return whether or not the robot is connected to a charger
      bool BatteryIsOnCharger();

      // UI LEDs
      const u32 LED_CHANNEL_COUNT = 8;

      // Set the intensity for each LED channel in the range [0, 255]
      void LEDSet(u8 leds[LED_CHANNEL_COUNT]);

      // Radio
      enum RadioState
      {
        RADIO_STATE_ADVERTISING = 0,
        RADIO_STATE_CONNECTED
      };

      const u32 RADIO_BUFFER_SIZE = 100;
      
      // Returns number of bytes received from the basestation
      u32 RadioFromBase(u8 buffer[RADIO_BUFFER_SIZE]);
     
      // Returns true if the buffer has been sent to the basestation
      bool RadioToBase(u8* buffer, u32 size);

      // Power management
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

    } // namespace HAL
  } // namespace Cozmo
} // namespace Anki

#endif // ANKI_COZMO_ROBOT_HARDWAREINTERFACE_H

