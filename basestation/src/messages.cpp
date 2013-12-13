#include "anki/cozmo/messages.h"


namespace Anki {
  namespace Cozmo {
    namespace Messages {
      
      // TODO: Fill this in!
     
     
     
      // 4. Fill in the message information lookup table:
      
      // TODO: Would be nice to use NUM_MSG_IDS instead of hard-coded 256 here.
      TableEntry LookupTable[256] = {
        {0,0,0}, // Empty entry for NO_MESSAGE_ID
#undef  MESSAGE_DEFINITION_MODE
#define MESSAGE_DEFINITION_MODE MESSAGE_TABLE_DEFINITION_MODE
#include "anki/cozmo/MessageDefinitions.h"
        {0, 0, 0} // Final dummy entry without comma at end
      };
      
      
      // THese are dummy placeholders to avoid linker errors for now
      
void ProcessClearPathMessage(unsigned char const*) {}

void ProcessSetMotionMessage(unsigned char const*) {}

void ProcessRobotAvailableMessage(unsigned char const*) {}

void ProcessMatMarkerObservedMessage(unsigned char const*) {}

void ProcessRobotAddedToWorldMessage(unsigned char const*) {}

void ProcessSetPathSegmentArcMessage(unsigned char const*) {}

void ProcessDockingErrorSignalMessage(unsigned char const*) {}

void ProcessSetPathSegmentLineMessage(unsigned char const*) {}

void ProcessBlockMarkerObservedMessage(unsigned char const*) {}

void ProcessTemplateInitializedMessage(unsigned char const*) {}

void ProcessTotalBlocksDetectedMessage(unsigned char const*) {}

void ProcessMatCameraCalibrationMessage(unsigned char const*) {}

void ProcessAbsLocalizationUpdateMessage(unsigned char const*) {}

void ProcessHeadCameraCalibrationMessage(unsigned char const*) {}

      
    } // namespace Messages
  } // namespace Cozmo
} // namespace Anki
