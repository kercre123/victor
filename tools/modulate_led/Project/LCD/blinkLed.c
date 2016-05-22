#include "main.h"
#include "math.h"

void BlinkLed(void)
{
  const int numFractionalBits = 8;
  
  const int onPercent = floorf(0.2f * powf(2,numFractionalBits) + 0.5f); // SQ15.8
  
  const int periodMicroseconds = 1000000;
  
  const int onMicroseconds = (periodMicroseconds * onPercent) >> numFractionalBits;
  const int offMicroseconds = periodMicroseconds - onMicroseconds;
  
  while(1) {
    GPIO_SetBits(GPIOA, GPIO_Pin_8);
    GPIO_SetBits(GPIOA, GPIO_Pin_9);
    GPIO_SetBits(GPIOA, GPIO_Pin_10);
    GPIO_SetBits(GPIOA, GPIO_Pin_11);
    GPIO_SetBits(GPIOA, GPIO_Pin_12);
    MicroWait(onMicroseconds);
    GPIO_ResetBits(GPIOA, GPIO_Pin_8);
    GPIO_ResetBits(GPIOA, GPIO_Pin_9);
    GPIO_ResetBits(GPIOA, GPIO_Pin_10);
    GPIO_ResetBits(GPIOA, GPIO_Pin_11);
    GPIO_ResetBits(GPIOA, GPIO_Pin_12);			
    MicroWait(offMicroseconds);
  } // } // while(1)
} // void BlinkLed()

