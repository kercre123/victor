#include <string.h>

#include "anki/cozmo/robot/hal.h"
#include "spine.h"
#include "cube.h"

#define __KEVIN_ROBOT

// This is a temporary mechanic until we get a message for the phone
static const uint32_t PROP_WHITELIST[MAX_CUBES] = {
#ifdef __BRYON_ROBOT
  0x00000072,
  0x00000091,
  0x0000003C,
  0x000000E7,
#elseif __KEVIN_ROBOT
  0x000000DA,
  0x000000A9,
  0x00000020,
  0x000000C3,
#endif
};

namespace Anki {
namespace Cozmo {
namespace HAL {

  // This function is super stupid for now
  void Spine::Dequeue(SpineProtocol& msg) {
    static int index = 0;
    static int prop = 0;

    if (prop < MAX_CUBES) {
      msg.opcode = SET_PROP_STATE;
      msg.SetPropState.slot = prop;
      memcpy((void*)&msg.SetPropState.colors, &g_LedStatus[prop], sizeof(msg.SetPropState.colors));
    } else {
      msg.opcode = ASSIGN_PROP;
      msg.AssignProp.slot = index;
      msg.AssignProp.prop_id = PROP_WHITELIST[index];
      
      if (++index >= MAX_CUBES) {
        index = 0;
      }
    }
    
    if (++prop > MAX_CUBES) {
      prop = 0;
    }
  }

  void Spine::Manage(SpineProtocol& msg) {
    switch(msg.opcode) {
      case GET_PROP_STATE:
        Anki::Cozmo::HAL::GetPropState(
          msg.GetPropState.slot, 
          msg.GetPropState.x, msg.GetPropState.y, msg.GetPropState.z, 
          msg.GetPropState.shockCount);
        break ;
      
      case PROP_DISCOVERED:
        Anki::Cozmo::HAL::DiscoverProp(msg.PropDiscovered.prop_id);
        break ;
    }
    
    // Prevent same messagr from getting processed twice (if the spine desyncs)
    msg.opcode = NO_OPERATION;
  }

}
}
}
