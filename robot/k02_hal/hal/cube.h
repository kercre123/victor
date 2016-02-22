#ifndef __CUBE_H
#define __CUBE_H

namespace Anki {
  namespace Cozmo {
    namespace HAL {
      namespace Cube {
        void Update(void);
        void SpineIdle(SpineProtocol& msg);
        void SendPropIds(void);
      }
    }
  }
}

#endif
