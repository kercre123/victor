#ifndef __RNG_H
#define __RNG_H

namespace Random {
  void init();
  void start();
  void get(void* data, int length);
}

#endif
