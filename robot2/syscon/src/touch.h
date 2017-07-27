#ifndef __TOUCH_H
#define __TOUCH_H

namespace Touch {
	void init(void);
  void transmit(uint16_t* data);
	void tick(void);
}

#endif
