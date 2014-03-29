#ifndef ANKI_COZMO_ROBOT_VISIONSYSTEM_H
#define ANKI_COZMO_ROBOT_VISIONSYSTEM_H

#include "anki/common/types.h"

#include "anki/vision/robot/fiducialMarkers.h"

#include "anki/cozmo/robot/hal.h"
#include "anki/cozmo/robot/messages.h"

#include "visionParameters.h"

namespace Anki {
  namespace Cozmo {
    namespace VisionSystem {
      
      typedef struct {
        u8  headerByte; // used to specify a frame's resolution in a packet if transmitting
        u16 width, height;
        u8 downsamplePower[HAL::CAMERA_MODE_COUNT];
      } CameraModeInfo_t;
      
      // NOTE: To get the downsampling power to go from resoution "FROM" to
      //       resolution "TO", use:
      //       u8 power = CameraModeInfo[FROM].downsamplePower[TO];
      const CameraModeInfo_t CameraModeInfo[HAL::CAMERA_MODE_COUNT] =
      {
        // VGA
        { 0xBA, 640, 480, {0, 1, 2, 3, 4} },
        // QVGA
        { 0xBC, 320, 240, {0, 0, 1, 2, 3} },
        // QQVGA
        { 0xB8, 160, 120, {0, 0, 0, 1, 2} },
        // QQQVGA
        { 0xBD,  80,  60, {0, 0, 0, 0, 1} },
        // QQQQVGA
        { 0xB7,  40,  30, {0, 0, 0, 0, 0} }
      };
      
      
      //
      // Methods:
      //
      
      ReturnCode Init(void);
      bool IsInitialized();
      
      void Destroy();
      
      // Accessors
      const HAL::CameraInfo* GetCameraCalibration();
      f32 GetTrackingMarkerWidth();
      
      // NOTE: It is important the passed-in robot state message be passed by
      //   value and NOT by reference, since the vision system can be interrupted
      //   by main execution (which updates the state).
      ReturnCode Update(const Messages::RobotState robotState);
      
      void StopTracking();

      // Select a block type to look for to dock with.  Use NUM_MARKER_TYPES to disable.
      // Next time the vision system sees a block of this type while looking
      // for blocks, it will initialize a template tracker and switch to
      // docking mode.
      // TODO: Something smarter about seeing the block in the expected place as well?
      ReturnCode SetMarkerToTrack(const Vision::MarkerType& markerToTrack,
                                  const f32 markerWidth_mm);
      
      u32 DownsampleHelper(const Embedded::Array<u8>& imageIn,
                           Embedded::Array<u8>&       imageOut,
                           Embedded::MemoryStack      scratch);
      
    } // namespace VisionSystem
    
  } // namespace Cozmo
} // namespace Anki

#endif // ANKI_COZMO_ROBOT_VISIONSYSTEM_H
