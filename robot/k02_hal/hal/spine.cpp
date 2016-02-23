#include <string.h>

#include "anki/cozmo/robot/hal.h"
#include "spine.h"
#include "cube.h"

namespace Anki {
namespace Cozmo {
namespace HAL {
  static const int QUEUE_DEPTH = 8;
  
  static SpineProtocol spinebuffer[QUEUE_DEPTH];
  static volatile int spine_enter = 0;
  static volatile int spine_exit = 0;

  void Spine::Dequeue(SpineProtocol& msg) {
    if (spine_enter == spine_exit) {
      Cube::SpineIdle(msg);
      return ;
    }
    
    memcpy(&msg, &spinebuffer[spine_enter], sizeof(SpineProtocol));
    spine_enter = (spine_enter+1) % QUEUE_DEPTH;
  }
  
  void Spine::Enqueue(SpineProtocol& msg) {
    int exit = (spine_exit+1) % QUEUE_DEPTH;
    if (spine_enter == exit) {
      return ;
    }

    memcpy(&spinebuffer[spine_exit], &msg, sizeof(SpineProtocol));
    spine_exit = exit;
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
    
      case REQUEST_PROPS:
        Cube::SendPropIds();
        break ;

      // No operation and body only messages
      case NO_OPERATION:
      case ASSIGN_PROP:
      case SET_PROP_STATE:
      case SET_BACKPACK_STATE:
      case ENTER_RECOVERY:
        break ;
    }

    // Prevent same messagr from getting processed twice (if the spine desyncs)
    msg.opcode = NO_OPERATION;
  }

}
}
}
