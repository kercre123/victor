#ifndef __MOTORLED_H
#define __MOTORLED_H

#include "portable.h"

void MotorMV(int millivolts, bool reverse_nForward = false ); // Set motor speed and direction between 0 and 5000 millivolts
int GrabADC(int channel); //slow oversampled ADC
int QuickADC(int channel); //quick & dirty
void ReadEncoder(bool light, int usDelay, int& a, int& b, bool skipa = false);

//Backpack LEDs
int  LEDCnt(void);      //report # of available leds
void LEDOn(u8 led);     //turns on single led (all others -> off). led={0..LEDCnt()-1}
int  LEDGetHighSideMv(u8 led); //PU LED high side and measure led anode voltage [mV]
int  LEDGetExpectedMv(u8 led); //expected value from LEDGetHighSideMv(), if circuit is working correctly.

//Backpack Button
bool BPBtnGet(int *out_mv = NULL); //return button state (true=pressed). Returns optional ADC sample.
int  BPBtnGetMv(void);  //return button input voltage [mV]

#endif
