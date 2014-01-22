#include "anki/cozmo/basestation/messages.h"


namespace Anki {
  namespace Cozmo {
    namespace Messages {
      
      // TODO: Fill this in!
     
     
      namespace {
        
        // 4. Fill in the message information lookup table:
        typedef struct {
          u8 priority;
          u8 size;
          void (*dispatchFcn)(const u8* buffer);
        } TableEntry;
        
        // TODO: Would be nice to use NUM_MSG_IDS instead of hard-coded 256 here.
        TableEntry LookupTable_[256] = {
          {0,0,0}, // Empty entry for NO_MESSAGE_ID
#undef  MESSAGE_DEFINITION_MODE
#define MESSAGE_DEFINITION_MODE MESSAGE_TABLE_DEFINITION_MODE
#include "anki/cozmo/MessageDefinitions.h"
          {0, 0, 0} // Final dummy entry without comma at end
        };
        
      } // private namespace
      
      u8 GetSize(const ID msgID)
      {
        return LookupTable_[msgID].size;
      }
      
      
      // THese are dummy placeholders to avoid linker errors for now
      
void ProcessClearPathMessage(const ClearPath&) {}

void ProcessSetMotionMessage(const SetMotion&) {}

void ProcessRobotAvailableMessage(const RobotAvailable&) {}

void ProcessMatMarkerObservedMessage(const MatMarkerObserved&) {}

void ProcessRobotAddedToWorldMessage(const RobotAddedToWorld&) {}

void ProcessSetPathSegmentArcMessage(const SetPathSegmentArc&) {}

void ProcessDockingErrorSignalMessage(const DockingErrorSignal&) {}

void ProcessSetPathSegmentLineMessage(const SetPathSegmentLine&) {}

void ProcessBlockMarkerObservedMessage(const BlockMarkerObserved&) {}

void ProcessTemplateInitializedMessage(const TemplateInitialized&) {}

void ProcessTotalBlocksDetectedMessage(const TotalBlocksDetected&) {}

void ProcessMatCameraCalibrationMessage(const MatCameraCalibration&) {}

void ProcessAbsLocalizationUpdateMessage(const AbsLocalizationUpdate&) {}

void ProcessHeadCameraCalibrationMessage(const HeadCameraCalibration&) {}

      
    } // namespace Messages
  } // namespace Cozmo
} // namespace Anki
