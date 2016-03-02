#ifndef __RNG_H
#define __RNG_H

namespace Crypto {
  void init();
  void start();
  void random(void* data, int length);
}

#endif
