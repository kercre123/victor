#ifndef ANKI_COZMO_ROBOT_VISIONSYSTEM_H
#define ANKI_COZMO_ROBOT_VISIONSYSTEM_H

#include "anki/common/types.h"


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
      bool IsInitialized();
      
      void Destroy();
      
      ReturnCode Update(u8* memoryBuffer);

      // Select a block type to look for to dock with.  Use 0 to disable.
      // Next time the vision system sees a block of this type while looking
      // for blocks, it will initialize a template tracker and switch to
      // docking mode.
      // TODO: Something smarter about seeing the block in the expected place as well?
      ReturnCode SetDockingBlock(const u16 blockTypeToDockWith = 0);
      
      // NOTE: the following are provided as public API because they messaging
      //       system may call them.
      
      // Check to see if this is the blockType the vision system is looking for,
      // i.e., the one set by a prior call to SetDockingBlock() above.
      void CheckForDockingBlock(const u16 blockType);
      
      // Switch to docking mode if isTemplateInitialized is true, otherwise
      // stay in current mode.
      void SetDockingMode(const bool isTemplateInitalized);
      
      // Update internal state based on whether tracking succeeded
      void UpdateTrackingStatus(const bool didTrackingSucceed);
      
    } // namespace VisionSystem
  } // namespace Cozmo
} // namespace Anki

#endif // ANKI_COZMO_ROBOT_VISIONSYSTEM_H
