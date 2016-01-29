#include <string.h>

#include "anki/cozmo/robot/hal.h"
#include "spine.h"
#include "cube.h"

#define __BRYON_ROBOT

// This is a temporary mechanic until we get a message for the phone
static const uint32_t PROP_WHITELIST[] = {
#ifdef __BRYON_ROBOT
  0x000000E7,
  0x0000010B,
  0x80000030, // 
#else
#endif
  0
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
      if (!PROP_WHITELIST[index]) {
        index = 0;
      }
    
      msg.opcode = ASSIGN_PROP;
      msg.AssignProp.slot = index;
      msg.AssignProp.prop_id = PROP_WHITELIST[index++];
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
