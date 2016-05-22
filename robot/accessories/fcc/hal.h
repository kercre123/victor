// Function prototypes for shared hal functions
#ifndef HAL_H__
#define HAL_H__

#include "reg31512.h"
#include "portable.h"
#include "board.h"

// Temporary holding area for radio payloads (inbound and outbound)
extern u8 idata _radioPayload[32];    // Largest packet supported by chip is 32 bytes

void TransmitData();

// XXX: Light up an LED for test purposes
void LedTest(u8 num);

#endif /* HAL_H__ */
