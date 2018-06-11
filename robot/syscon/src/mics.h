#ifndef __MICS_H
#define __MICS_H


namespace Mics {
  void init(void);
  void start(void);
  void stop(void);
  void reduce(bool reduce);
  void transmit(int16_t* payload);
  void errorCode(uint16_t* data);
}

#endif
