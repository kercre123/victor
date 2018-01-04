#ifndef __TIMER_H
#define __TIMER_H

#include <stdint.h>

#ifdef __cplusplus
namespace Timer
{
  void init(void);
  uint32_t get(void); //get the running counter [us]
  void wait(uint32_t us); //wait for # of uS
  void delayMs(uint32_t ms); //wait for # of mS
  uint32_t elapsedUs(uint32_t base); //calculate time passed [us] since 'base' (value from 'get' method)
}
#endif /* __cplusplus */

#endif //__TIMER_H

