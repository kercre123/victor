#include <string.h>
#include "hal.h"
#include "portable.h"


#define GAMMA_CORRECT(x)           ((x)*(x) >> 8)

/*
struct charlieChannel
{
  ledColor color;
  char anode;
  char cathode;
};

struct charlieLeds
{
  struct charlieChannel channels[13];
  const char ledPins[4];
};
*/

typedef enum color
{
  red,
  green,
  blue,
  black
};

const u8 ledPins[4] = {1<<2, 1<<3, 1<<4, 1<<5};
const u8 ledAnode[13] = {0, 0, 0, 1, 1, 1, 2, 2, 2, 3, 3, 3, 0};
const u8 ledCathode[13] = {1, 2, 3, 0, 2, 3, 0, 1, 3, 0, 1, 2, 0};

volatile u8 xdata ledValues[13];

volatile u8 gCurrentLed = 0;

void SetLedValue(char led, char value)
{
  ledValues[led] = value;
}

void SetLedValues(char *newValues)
{
  memcpy(ledValues, newValues, sizeof(ledValues));
} 

void SetLedValuesByDelta()
{
  
}  


void InitTimer2()
{  
  T2PS = 0; // prescaler 1/12 of ckCpu
  T2R1 = 0;
  T2R0 = 0; // reload disabled
  T2I1 = 0;
  T2I0 = 1; // Start timer
  T2PS = 0;
  
  ET2 = 1; // enable timer 2 interrupt
  EA = 1; // enable global interrupts
}

void StartTimer2()
{
  // Turn on current light
  PIN_OUT(P0DIR, ledPins[ledAnode[gCurrentLed]] | ledPins[ledCathode[gCurrentLed]]);
  GPIO_SET(P0, ledPins[ledAnode[gCurrentLed]]);
  GPIO_RESET(P0, ledPins[ledCathode[gCurrentLed]]);
  
  T2I0 = 1;
  ET2 = 1;
}

void StopTimer2()
{
  T2I0 = 0; // turn off timer2 interrupt
  ET2 = 0; // turn off timer
  // Turn off all lights
  PIN_IN(P0DIR, ledPins[0] | ledPins[1] | ledPins[2] | ledPins[3]);   
  /*
  TL2 = 0xFF - (waitTime & 0xFF); // XXX do I need to save these values?
  TH2 = 0xFF;
  */
}


void LightsOff()
{
  PIN_IN(P0DIR, ledPins[0] | ledPins[1] | ledPins[2] | ledPins[3]);
}

void LightOn(char i)
{
  LightsOff(); // first turn everything off
  PIN_OUT(P0DIR, ledPins[ledAnode[i]] | ledPins[ledCathode[i]]);
  GPIO_SET(P0, ledPins[ledAnode[i]]);
  GPIO_RESET(P0, ledPins[ledCathode[i]]);
}

T2_ISR()
{
  static unsigned int waitTime;
  
  // turn everyone off
  PIN_IN(P0DIR, ledPins[0] | ledPins[1] | ledPins[2] | ledPins[3]); 
  
  // Turn on an LED
  do
  {
    if(gCurrentLed<12)
    {
      gCurrentLed++;
    }
    else
    {
      gCurrentLed=0;
    }
  }while(ledValues[gCurrentLed] == 0 && gCurrentLed != 12); // need a way to break out
  
  waitTime = (unsigned int)(ledValues[gCurrentLed]);
  waitTime = (waitTime*waitTime)>>8;
  
 
//  if(gCurrentLed != 2)
//  {
  // Turn on LED
  PIN_OUT(P0DIR, ledPins[ledAnode[gCurrentLed]] | ledPins[ledCathode[gCurrentLed]]);
  GPIO_SET(P0, ledPins[ledAnode[gCurrentLed]]);
  GPIO_RESET(P0, ledPins[ledCathode[gCurrentLed]]);
//  }
  TL2 = 0xFF - (waitTime & 0xFF);
  TH2 = 0xFF;
  
  TF2 = 0; // reset timer 2 flag  
}
