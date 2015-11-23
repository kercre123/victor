#ifndef TIMER_H
#define TIMER_H

// Each count is 1/2^23 seconds, so 8,388.608 counts equals one millisecond
// The counter is quantized to 256 counts, so it updates only every 30.51uS
// Wraps every 512 seconds - use unsigned overflow math to hide wrapping
#define COUNT_PER_MS (8389)

// Initialize the RTC peripheral
void TimerInit();

// Get the counter - with each tick being ~120 ns (see COUNT_PER_MS)
// XXX: Mike observed the counter jumping back, sometimes.  Is it still happening?
#define GetCounter() ((u32)(NRF_RTC1->COUNTER << 8))

// Wait in a loop for the specified amount of microseconds
void MicroWait(uint32_t microseconds);

#endif
