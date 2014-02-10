#ifndef VIZ_STRUCTS_H
#define VIZ_STRUCTS_H

#include "anki/common/constantsAndMacros.h"
#include "anki/common/types.h"
namespace Anki {
  namespace Cozmo {

    
    // Port to connect to for sending visualization messages
    const u32 VIZ_SERVER_PORT = 5252;

    const u32 ALL_PATH_IDs = u32_MAX;
    const u32 ALL_OBJECT_IDs = u32_MAX;

    const u32 DEFAULT_COLOR_ID = u32_MAX;
    
    // Define viz messages
//#ifdef COZMO_ROBOT
    
#ifdef COZMO_BASESTATION
#define COZMO_ROBOT
#undef COZMO_BASESTATION
#define THIS_IS_ACTUALLY_BASESTATION
#endif
    
    
    // 1. Initial include just defines the definition modes for use below
#include "anki/cozmo/VizMsgDefs.h"

    
    // 2. Define all the message structs:
#define MESSAGE_DEFINITION_MODE MESSAGE_STRUCT_DEFINITION_MODE
#include "anki/cozmo/VizMsgDefs.h"
    
    // 3. Create the enumerated message IDs:
    typedef enum {
      NO_VIZ_MESSAGE_ID = 0,
#undef MESSAGE_DEFINITION_MODE
#define MESSAGE_DEFINITION_MODE MESSAGE_ENUM_DEFINITION_MODE
#include "anki/cozmo/VizMsgDefs.h"
      NUM_VIZ_MSG_IDS // Final entry without comma at end
    } VizMsgID;

    
#ifndef THIS_IS_ACTUALLY_BASESTATION
    // 4. Fill in the message information lookup table:
    typedef struct {
      u8 priority;
      u8 size;
      void (*dispatchFcn)(const u8* buffer);
    } TableEntry;
    
    const size_t NUM_TABLE_ENTRIES = NUM_VIZ_MSG_IDS + 1;
    TableEntry LookupTable_[NUM_TABLE_ENTRIES] = {
      {0, 0, 0}, // Empty entry for NO_MESSAGE_ID
#undef  MESSAGE_DEFINITION_MODE
#define MESSAGE_DEFINITION_MODE MESSAGE_TABLE_DEFINITION_MODE
#include "anki/cozmo/VizMsgDefs.h"
      {0, 0, 0} // Final dummy entry without comma at end
    };
#endif //ifndef THIS_IS_ACTUALLY_BASESTATION
    
    
#ifdef THIS_IS_ACTUALLY_BASESTATION
#define COZMO_BASESTATION
#undef COZMO_ROBOT
#undef THIS_IS_ACTUALLY_BASESTATION
#endif
    
    
/*
#elif defined(COZMO_BASESTATION)
    
#include "anki/cozmo/basestation/messages.h"
    
    // 1. Initial include just defines the definition modes for use below
#undef MESSAGE_DEFINITION_MODE
#include "anki/cozmo/VizMsgDefs.h"
    
    // 2. Define all the message classes:
#define MESSAGE_DEFINITION_MODE MESSAGE_CLASS_DEFINITION_MODE
#include "anki/cozmo/VizMsgDefs.h"

#else
#error VizStructs.h - Neither COZMO_ROBOT or COZMO_BASESTATION is defined!
#endif
*/
    
  } // namespace Cozmo
} // namespace Anki


#endif // VIZ_STRUCTS_H