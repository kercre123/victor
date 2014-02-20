#ifndef ANKI_COZMO_ROBOT_VISIONSYSTEM_H
#define ANKI_COZMO_ROBOT_VISIONSYSTEM_H

#include "anki/common/types.h"


namespace Anki {
  namespace Cozmo {
    namespace VisionSystem {
    
      // TODO: This is potentially dangerous since people can use a code without
      //       realizing it is an array.  Change to a full-blown class?
      //typedef u8 MarkerCode[VISION_MARKER_CODE_LENGTH];
      
      class MarkerCode
      {
      public:
        static const u8 CODE_LENGTH_IN_BYTES = VISION_MARKER_CODE_LENGTH*sizeof(u8);
        
        MarkerCode();
        MarkerCode(const u8* bits);
        
        const u8* GetBits() const;
        
        void Set(const MarkerCode& other);
        //void Set(const u8* bits);
        void Unset();
        
        bool IsSet() const;
        
        bool operator==(const u8* otherBits) const;
        bool operator==(const MarkerCode& other) const;
        
      protected:
        bool isSet_;
        u8 bits_[VISION_MARKER_CODE_LENGTH];
      };
      
      
      //
      // Parameters / Constants:
      //
      
      const HAL::CameraMode DETECTION_RESOLUTION = HAL::CAMERA_MODE_QVGA;
      const HAL::CameraMode TRACKING_RESOLUTION  = HAL::CAMERA_MODE_QQQVGA;
      
      //
      // Methods
      //
      
      ReturnCode Init(void);
      bool IsInitialized();
      
      void Destroy();
      
      ReturnCode Update(void);
      
      void StopTracking();

      // Select a block type to look for to dock with.  Use 0 to disable.
      // Next time the vision system sees a block of this type while looking
      // for blocks, it will initialize a template tracker and switch to
      // docking mode.
      // TODO: Something smarter about seeing the block in the expected place as well?
      ReturnCode SetMarkerToTrack(const MarkerCode& codeToTrack);
      
      // NOTE: the following are provided as public API because they messaging
      //       system may call them.
      
      // Check to see if this is the blockType the vision system is looking for,
      // i.e., the one set by a prior call to SetDockingBlock() above.
      void CheckForTrackingMarker(const u8* bits);
      
      // Switch to docking mode if isTemplateInitialized is true, otherwise
      // stay in current mode.
      void SetTrackingMode(const bool isTemplateInitalized);
      
      // Update internal state based on whether tracking succeeded
      void UpdateTrackingStatus(const bool didTrackingSucceed);
      
    } // namespace VisionSystem
    
  } // namespace Cozmo
} // namespace Anki

#endif // ANKI_COZMO_ROBOT_VISIONSYSTEM_H
