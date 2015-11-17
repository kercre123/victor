//#include <string.h>
#include "hal.h"
#include "portable.h"


#define GAMMA_CORRECT(x)           ((x)*(x) >> 8)

#define NUM_LEDS      17
#define DARK_BYTE     NUM_LEDS-1
#define NUM_LED_PINS

static const u8 code ledPins[NUM_LED_PINS] = {1<<1, 1<<6, 1<<5, 1<<7, 1<<0, 1<<1};
static volatile u8 ledValues[NUM_LEDS] = {0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0,0, 128};
static volatile u8 gCurrentLed = 0;

struct s_led {
  u8 anode : 3;
  u8 anode_port : 1;  
  u8 cathode : 3;
  u8 cathode_port : 1;  
};

const struct s_led code s_leds[NUM_LEDS] = 
{
  {0, 0, 4, 1}, // R1
  {0, 0, 1, 0}, // G1
  {0, 0, 2, 0}, // B1
  
  {1, 0, 4, 1}, // R2
  {1, 0, 0, 0}, // G2
  {1, 0, 3, 0}, // B2
  
  {2, 0, 5, 1}, // R3
  {2, 0, 0, 0}, // G3
  {2, 0, 3, 0}, // B3
  
  {3, 0, 5, 1}, // R4
  {3, 0, 2, 0}, // G4
  {3, 0, 1, 0}, // B4
  
  {4, 1, 0, 0}, // IR1
  {4, 1, 1, 0}, // IR2 
  
  {5, 1, 2, 0}, // IR3
  {5, 1, 3, 0}, // IR4
  
  {0, 0, 0, 0}  // Dark
};



void ValidateDarkByte()
{
  if(ledValues[DARK_BYTE] < 16) // dark byte must be at least 16 to avoid errors
  ledValues[DARK_BYTE] = 16;
}

void SetLedValue(char led, char value)
{
  ledValues[led] = value;
  ValidateDarkByte();
}

void SetLedValues(char *newValues)
{
  simple_memcpy(ledValues, newValues, sizeof(ledValues));
  ValidateDarkByte();
} 

void SetLedValuesByDelta()
{
  // TODO //
}  

void InitTimer2()
{  
  #ifndef USE_UART
  T2PS = 0; // prescaler 1/12 of ckCpu
  T2R1 = 0;
  T2R0 = 0; // reload disabled
  T2I1 = 0;
  T2I0 = 1; // Start timer
  T2PS = 0;
  
  ET2 = 1; // enable timer 2 interrupt
  EA = 1; // enable global interrupts
  #endif
}

void StartTimer2()
{
  #ifndef USE_UART
  // Turn on current light
  LightOn(gCurrentLed);
  T2I0 = 1;
  ET2 = 1;
  #endif
}

void StopTimer2()
{
  #ifndef USE_UART
  T2I0 = 0; // turn off timer2 interrupt
  ET2 = 0; // turn off timer
  LightsOff();
  #endif
}


void LightsOff() // hardcoded for speed (TODO: figure out if this is necessary)
{
  PIN_IN(P0DIR, ledPins[0] | ledPins[1] | ledPins[2] | ledPins[3]);
  PIN_IN(P1DIR, ledPins[4] | ledPins[5]);
}

void LightOn(char i)
{
  LightsOff(); // first turn everything off
  PIN_OUT_MP(s_leds[i].anode_port, ledPins[s_leds[i].anode]);
  PIN_OUT_MP(s_leds[i].cathode_port, ledPins[s_leds[i].cathode]);
  GPIO_SET_MP(s_leds[i].anode_port, ledPins[s_leds[i].anode]);
  GPIO_RESET_MP(s_leds[i].cathode_port, ledPins[s_leds[i].cathode]);
}

T2_ISR()
{
  static unsigned int waitTime;
  
  // turn everyone off
  ET2 = 0; // disable timer 2 interrupt
  PIN_IN(P0DIR, ledPins[0] | ledPins[1] | ledPins[2] | ledPins[3]); 
  
  // Turn on an LED
  do
  {
    if(gCurrentLed<12) // XXX NUM_LEDS
    {
      gCurrentLed++;
    }
    else
    {
      gCurrentLed=0;
    }
  }while(ledValues[gCurrentLed] < 16 && gCurrentLed != 12); // need a way to break out // XXX NUM_LEDS
  
  waitTime = (unsigned int)(ledValues[gCurrentLed]);
  waitTime = (waitTime*waitTime)>>8;
  
  // Turn on LED
  PIN_OUT_MP(s_leds[gCurrentLed].anode_port, ledPins[s_leds[gCurrentLed].anode]);
  PIN_OUT_MP(s_leds[gCurrentLed].cathode_port, ledPins[s_leds[gCurrentLed].cathode]);
  GPIO_SET_MP(s_leds[gCurrentLed].anode_port, ledPins[s_leds[gCurrentLed].anode]);
  GPIO_RESET_MP(s_leds[gCurrentLed].cathode_port, ledPins[s_leds[gCurrentLed].cathode]);

  if (waitTime<32)
    waitTime = 32;
  TL2 = 0xFF - (waitTime & 0xFF);
  TH2 = 0xFF;
  
  TF2 = 0; // reset timer 2 flag  
  ET2 = 1; // enable timer 2 interrupt
}
