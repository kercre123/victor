#ifndef __MOTORLED_H
#define __MOTORLED_H

// Set motor speed between 0 and 5000 millivolts
void MotorMV(int millivolts);

// Drive one of 16 LEDs on the backpack, then measure voltage on ADC
int LEDTest(u8 led);

int GrabADC(int channel);
int QuickADC(int channel);
void LEDOn(u8 led);
int LEDTest(u8 led);
void ReadEncoder(bool light, int usDelay, int& a, int& b, bool skipa = false);

#endif
