#ifndef ANKI_COZMO_ROBOT_VISIONSYSTEM_H
#define ANKI_COZMO_ROBOT_VISIONSYSTEM_H

/*
 class VisionSystem
 {
 public:
 */

#include "anki/common/types.h"

#include "anki/common/robot/geometry_declarations.h"

#include "anki/cozmo/MessageProtocol.h"

#include "anki/cozmo/robot/hal.h"

// If enabled, will use Matlab as the vision system for processing images
#if defined(SIMULATOR) && ANKICORETECH_USE_MATLAB
#define USE_MATLAB_FOR_HEAD_CAMERA
#define USE_MATLAB_FOR_MAT_CAMERA
#endif

namespace Anki {
  namespace Cozmo {
   
    namespace VisionSystem {
    
      //
      // Parameters / Constants:
      //
      
      const HAL::CameraMode DETECTION_RESOLUTION = HAL::CAMERA_MODE_QVGA;
      const HAL::CameraMode TRACKING_RESOLUTION  = HAL::CAMERA_MODE_QQQVGA;
      
      const HAL::CameraMode MAT_LOCALIZATION_RESOLUTION = HAL::CAMERA_MODE_QVGA;
      const HAL::CameraMode MAT_ODOMETRY_RESOLUTION     = HAL::CAMERA_MODE_QQQQVGA;
     
      
      //
      // Methods
      //
      
      ReturnCode Init(void);
      
      void Destroy();
      
      ReturnCode Update(u8* memoryBuffer);

      // Select a block type to look for to dock with.  Use 0 to disable.
      ReturnCode SetDockingBlock(const u16 blockTypeToDockWith = 0);
      
      // Check to see if this is blockType the vision system is looking for
      // (Note this is public API b/c messaging system may call it.)
      void CheckForDockingBlock(const u16 blockType);
      
      // Switch to docking mode if isTemplateInitialized is true, otherwise
      // stay in current mode.
      // (Note this is public API b/c messaging system may call it.)
      void SetDockingMode(const bool isTemplateInitalized);
      
      // Update internal state based on whether tracking succeeded
      // (Note this is public API b/c messaging system may call it.)
      void UpdateTrackingStatus(const bool didTrackingSucceed);
      
      bool IsInitialized();
      
    } // namespace VisionSystem
  } // namespace Cozmo
} // namespace Anki

#endif // ANKI_COZMO_ROBOT_VISIONSYSTEM_H
