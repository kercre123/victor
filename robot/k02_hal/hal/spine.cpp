#include <string.h>

#include "anki/cozmo/robot/hal.h"
#include "spine.h"
#include "cube.h"

namespace Anki {
namespace Cozmo {
namespace HAL {

  // This function is super stupid for now
  void Spine::Dequeue(SpineProtocol& msg) {
    Cube::SpineIdle(msg);
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
