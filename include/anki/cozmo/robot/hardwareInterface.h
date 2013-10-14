
/**
 * File: hardwareInterface.h
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

#include "anki/common/types.h"

namespace Anki {
  namespace Cozmo {
    namespace HardwareInterface {

      //
      // Parameters / Constants
      //
      const u8 NUM_RADIAL_DISTORTION_COEFFS = 5;
      
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
      
      // Wheel motors
      void SetLeftWheelAngularVelocity(f32 rad_per_sec);
      void SetRightWheelAngularVelocity(f32 rad_per_sec);
      
      void SetWheelAngularVelocity(f32 left_rad_per_sec,
                                   f32 right_rad_per_sec);
      
      f32 GetLeftWheelPosition();
      f32 GetRightWheelPosition();
      void  GetWheelPositions(f32 &left_rad, f32 &right_rad);
      
      f32 GetLeftWheelSpeed();
      f32 GetRightWheelSpeed();
      
      // Head pitch
      void  SetHeadPitch(f32 pitch_rad);
      f32 GetHeadPitch();
      
      // Lift position
      void  SetLiftPitch(f32 pitch_rad);
      f32 GetLiftPitch();
      
      // Gripper control
      void ManageGripper(); // needed?
      void EngageGripper();
      void DisengageGripper();
      bool IsGripperEngaged();
      
      // Cameras
      // TODO: Add functions for adjusting ROI of cameras?
      const u8* GetHeadImage();
      const u8* GetMatImage();
      const FrameGrabber GetHeadFrameGrabber();
      const FrameGrabber GetMatFrameGrabber();
      
      const CameraInfo* GetHeadCamInfo();
      const CameraInfo* GetMatCamInfo() ;
      
      // Communications
      void ManageRecvBuffer();
      void SendMessage(const void* data, s32 size);
      s32  RecvMessage(void* data);
      bool IsConnected();
      
      // Misc
      bool IsInitialized();
      void UpdateDisplay();
      
      // Take a step (needed for webots, possibly a no-op for real robot?)
      ReturnCode Step(void);
      
    } // namespace HardwareInterface
  } // namespace Cozmo
} // namespace Anki

#endif // ANKI_COZMO_ROBOT_HARDWAREINTERFACE_H