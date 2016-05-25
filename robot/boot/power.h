#ifndef __POWER_H
#define __POWER_H

namespace Anki {
  namespace Cozmo {
		namespace HAL {
			namespace Power {
				void init(void);
        void enableEspressif(bool fixture);
        void disableEspressif(void);
			}
		}
  }
}

#endif
