#ifndef ANKI_COZMO_ROBOT_VISIONSYSTEM_H
#define ANKI_COZMO_ROBOT_VISIONSYSTEM_H

#include "anki/common/types.h"

#include "anki/vision/robot/fiducialMarkers.h"


namespace Anki {
  namespace Cozmo {
    namespace VisionSystem {
      
      //
      // Methods
      //
      
      ReturnCode Init(void);
      bool IsInitialized();
      
      void Destroy();
      
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
      
    } // namespace VisionSystem
    
  } // namespace Cozmo
} // namespace Anki

#endif // ANKI_COZMO_ROBOT_VISIONSYSTEM_H
