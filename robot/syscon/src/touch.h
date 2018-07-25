#ifndef __TOUCH_H
#define __TOUCH_H

namespace Touch {
	void init(void);
  void transmit(uint16_t* data);
	void tick(void);
  
  void inline Pause(void) { TIM16->DIER = 0; }
  void inline Resume(void) { TIM16->DIER = TIM_DIER_UIE; }
}

#endif
