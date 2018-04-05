#ifndef __LIGHTS_H
#define __LIGHTS_H

namespace Lights {
  void init(void);
  void tick(void);
  void disable(void);
  void receive(const uint8_t* values);
}

#endif
