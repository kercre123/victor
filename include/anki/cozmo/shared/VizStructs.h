#ifndef VIZ_STRUCTS_H
#define VIZ_STRUCTS_H

#include "anki/common/constantsAndMacros.h"
#include "anki/common/types.h"
namespace Anki {
  namespace Cozmo {

    
    // Port to connect to for sending visualization messages
    const u32 VIZ_SERVER_PORT = 5252;
    
    // Only cozmo_viz_controller (aka CozmoVizDisplay) should connect to the
    // cozmo_physics plugin. Basestation or robot can draw via cozmo_viz_controller.
    const u32 PHYSICS_PLUGIN_SERVER_PORT = 5253;

    const u32 ALL_PATH_IDs   = u32_MAX;
    const u32 ALL_QUAD_IDs   = u32_MAX;
    const u32 ALL_QUAD_TYPEs = u32_MAX;
    const u32 ALL_OBJECT_IDs = u32_MAX;
    const u32 OBJECT_ID_RANGE = ALL_OBJECT_IDs - 1;
    
    enum VizObjectType {
      VIZ_OBJECT_ROBOT = 0,
      VIZ_OBJECT_CUBOID,
      VIZ_OBJECT_RAMP,
      VIZ_OBJECT_CHARGER,      
      VIZ_OBJECT_PREDOCKPOSE,
      VIZ_OBJECT_HUMAN_HEAD,
      
      NUM_VIZ_OBJECT_TYPES
    };
    
    // Base IDs for each VizObject type
    const u32 VizObjectBaseID[NUM_VIZ_OBJECT_TYPES+1] = {
      0,    // VIZ_OJECT_ROBOT
      1000, // VIZ_OBJECT_CUBOID
      2000, // VIZ_OBJECT_RAMP
      3000, // VIZ_OBJECT_CHARGER
      4000, // VIZ_OJECT_PREDOCKPOSE
      5000, // VIZ_OBJECT_HUMAN_HEAD
      u32_MAX - 100 // Last valid object ID allowed
    };
    
    
    enum VizQuadType {
      VIZ_QUAD_GENERIC_2D = 0,
      VIZ_QUAD_GENERIC_3D,
      VIZ_QUAD_MAT_MARKER,
      VIZ_QUAD_PLANNER_OBSTACLE,
      VIZ_QUAD_PLANNER_OBSTACLE_REPLAN,
      VIZ_QUAD_ROBOT_BOUNDING_BOX,
      VIZ_QUAD_POSE_MARKER,
      
      NUM_VIZ_QUAD_TYPES
    };
    
    /*
    // Base IDs for each VizQuad type
    const u32 VizQuadBaseID[NUM_VIZ_QUAD_TYPES+1] = {
      0,    // VIZ_QUAD_MAT_MARKER
      1000, // VIZ_QUAD_PLANNER_OBSTACLE
      
      u32_MAX - 100 // Lass valid quad ID allowed
    };
    */

    enum VizRobotMarkerType {
      VIZ_ROBOT_MARKER_SMALL_TRIANGLE,
      VIZ_ROBOT_MARKER_BIG_TRIANGLE
    };
    
    // Define viz messages
//#ifdef COZMO_ROBOT
    
#ifdef COZMO_BASESTATION
#define COZMO_ROBOT
#undef COZMO_BASESTATION
#define THIS_IS_ACTUALLY_BASESTATION
#endif
    
    
    // 1. Initial include just defines the definition modes for use below
#include "anki/cozmo/shared/VizMsgDefs.h"

    
    // 2. Define all the message structs:
#define MESSAGE_DEFINITION_MODE MESSAGE_STRUCT_DEFINITION_MODE
#include "anki/cozmo/shared/VizMsgDefs.h"
    
    // 3. Create the enumerated message IDs:
    typedef enum {
      NO_VIZ_MESSAGE_ID = 0,
#undef MESSAGE_DEFINITION_MODE
#define MESSAGE_DEFINITION_MODE MESSAGE_ENUM_DEFINITION_MODE
#include "anki/cozmo/shared/VizMsgDefs.h"
      NUM_VIZ_MSG_IDS // Final entry without comma at end
    } VizMsgID;

    
    
    // 4. Create {priority,size} table
    typedef struct {
      u8 priority;
      u32 size;
    } VizMsgTableEntry;
    
    const size_t NUM_VIZ_MSG_TABLE_ENTRIES = Anki::Cozmo::NUM_VIZ_MSG_IDS + 1;
    const VizMsgTableEntry VizMsgLookupTable_[NUM_VIZ_MSG_TABLE_ENTRIES] = {
      {0, 0}, // Empty entry for NO_MESSAGE_ID
#undef  MESSAGE_DEFINITION_MODE
#define MESSAGE_DEFINITION_MODE MESSAGE_SIZE_TABLE_DEFINITION_MODE
#include "anki/cozmo/shared/VizMsgDefs.h"
      {0, 0} // Final dummy entry without comma at end
    };
    
    
    
#ifdef THIS_IS_ACTUALLY_BASESTATION
#define COZMO_BASESTATION
#undef COZMO_ROBOT
#undef THIS_IS_ACTUALLY_BASESTATION
#endif
    
    
/*
#elif defined(COZMO_BASESTATION)
    
#include "anki/cozmo/basestation/comms/robot/robotMessages.h"
    
    // 1. Initial include just defines the definition modes for use below
#undef MESSAGE_DEFINITION_MODE
#include "anki/cozmo/shared/VizMsgDefs.h"
    
    // 2. Define all the message classes:
#define MESSAGE_DEFINITION_MODE MESSAGE_CLASS_DEFINITION_MODE
#include "anki/cozmo/shared/VizMsgDefs.h"

#else
#error VizStructs.h - Neither COZMO_ROBOT or COZMO_BASESTATION is defined!
#endif
*/
    
  } // namespace Cozmo
} // namespace Anki


#endif // VIZ_STRUCTS_H