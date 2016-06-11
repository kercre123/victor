#ifndef __LIGHTS_H
#define __LIGHTS_H

namespace Lights {
  void init(void);
  void set(int channel, int colors, uint8_t brightness);
  void stop(void);

  void setChannel(int channel);
  void update();
}

#endif
