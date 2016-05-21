#ifndef __TIMER_H
#define __TIMER_H

uint32_t GetMicroCounter(void);
void TimerInit(void);
void MicroWait(uint32_t us);

#endif
