#include "hal.h"
#include "bma222.h"

// Borrow some SPI accel.c functions from the bootloader
#define DataRead ((u8 (code *) (u8 addr)) 0x3B26)
#define DataWrite ((void (code *) (u8 addr, u8 dataByte)) 0x3AFC)
#define Read ((u8 (code *) (void)) 0x3DCC)
#define Write ((u8 (code *) (u8 b)) 0x3B09)

#define MAX_READINGS 33   // 3 AXES * 11 readings keeps up with ~10/tick
u8 idata _readings[MAX_READINGS];

// This is the "simple" (EP1) tap detector
#define TAP_THRESH    20        // Equivalent to EP1 10 (since I shift less)
#define TAP_DEBOUNCE  45        // 45/500 = 90ms before tap is recognized
#define TAP_DURATION  5         // 5/100 = 10ms is max length of a tap (anything slower is rejected)
u8 _debounce, _taps, _tapTime;
s8 _last, _tapPos, _tapNeg;
bit _posFirst;
void SimpleTap(u8 readings)
{
  s8 diff, current;
  s8 idata *p = _readings+2;  // Z axis only
 
  // Scan any new readings, updating the state
  while (readings)
  {
    current = *p >> 1;        // Sign-extend and decrease range (to allow MAX-MIN diff)
    diff = current - _last;
    _last = current;
    p += 3;                   // Z axis only
    readings -= 3;
    
    // If not debouncing...
    if (!_debounce)
    {
      // Look for something big enough to start the debounce
      if (diff > TAP_THRESH)
        _posFirst = 1;
      else if (diff < -TAP_THRESH)
        _posFirst = 0;
      else
        continue;   // Nothing, just continue looking
      
      _debounce = TAP_DEBOUNCE;
      _tapPos = _tapNeg = 0;      // New potential tap, reset min/max
    }
    // If debouncing, look for a tap
    else 
    {
      if (_debounce > (TAP_DEBOUNCE-TAP_DURATION) && 
          ((diff >  TAP_THRESH && !_posFirst) || 
           (diff < -TAP_THRESH &&  _posFirst)))
      {
        _taps++;
        _debounce = TAP_DEBOUNCE-TAP_DURATION;
        _radioOut[5] = _tapTime;  // As soon as we call it a tap, save timestamp
      }
      _debounce--;
    }
    
    // If we get here, we are in a potential tap, track min/max absolute acceleration
    if (_tapNeg < diff)
      _tapNeg = diff;
    if (_tapPos > diff)
      _tapPos = diff;
    _radioOut[6] = _tapPos;
    _radioOut[7] = _tapNeg;
  }
  
  // Set up outbound message
  _radioOut[0] = A2R_SIMPLE_TAP;
  _radioOut[1] = _readings[1];
  _radioOut[2] = _readings[2];
  _radioOut[3] = _readings[3];
  _radioOut[4] = _taps;
  
  _radioOut[8] = ADCDATH;   // Battery volts, where 255=3.6V
}

// Drain the entire accelerometer FIFO into an outbound packet
void AccelRead()
{
  u8 i, count;

  // Chargers return all zero
  if (IsCharger())
  {
    for (i = 0; i < HAND_LEN; i++)
      _radioOut[i] = 0;
    return;
  }

  // Set up SPI for reading the FIFO
  ADCCON1 = ADC_START(VBAT_ADC_CHAN);  // Measure battery in background
  SPIInit();
  CSB = 0;

  // Bit-bang Write(SPI_READ | FIFO_DATA) at ~2MHz
  SCK = 0;          SCK = 1;
  SCK = 0; SDI = 0; SCK = 1;
  SCK = 0; SDI = 1; SCK = 1;
  SCK = 0;          SCK = 1;
  SCK = 0;          SCK = 1;
  SCK = 0;          SCK = 1;
  SCK = 0;          SCK = 1;
  SCK = 0;          SCK = 1;
  
  // Now drive hardware SPI at max (safe) speed - 4MHz
  SPIMCON0 = SPIEN + SPI4M + SPIPOL+SPIPHASE; // Mode 3: Idle high, sample on rising edge
  SPIMDAT = 0;            // Get SPI moving - 0 blinks IR LED
  LEDDIR = DEFDIR;        // DebugCube needs this

  // Read exactly 2*MAX_READINGS bytes from SPI (store MSB only)
  // Bosch FIFO has weird problems if you read non-divisible-by 6
  // The loop is tangled by a slightly-ahead SPI pipeline (SPIMDAT=0) for performance
  count = i = 0;
  while (1)
  {
    SPIMDAT = 0;        // Pipeline next read
    i++;
 
    while (!(SPIMSTAT & RXREADY)) // Wait for byte ready
      ; 
    if (SPIMDAT & 1)    // If FIFO isn't empty, set count
      count = i;
    if (i == MAX_READINGS)
      break;            // Quit loop at last reading (before pipelining the next)
    
    SPIMDAT = 0;        // Pipeline next read
//  while (!(SPIMSTAT & RXREADY)) // Wait for byte ready - 4MHz doesn't need this one
//    ;
    _readings[i-1] = SPIMDAT;     // Grab byte
  }
  while (!(SPIMSTAT & RXREADY)) // Wait for byte ready
    ;
  _readings[MAX_READINGS-1] = SPIMDAT;  // Grab last byte
  
  CSB = 1;
  SPIMCON0 = 0;

  DebugPrint('x', _readings, count);    // See raw accelerometer data
  SimpleTap(count);
}

// Register initialization for accelerometer
// This is largely based on Bosch MIS-AN003: "FIFO bug workarounds"
code u8 ACCEL_INIT[] =
{
  BGW_SOFTRESET, BGW_SOFTRESET_MAGIC,   // Exit SUSPEND mode
  
  // Enter STANDBY mode
  PMU_LOW_POWER, PMU_LOWPOWER_MODE,     // 0x12 first
  PMU_LPW, PMU_SUSPEND,                 // Then 0x11
  
  BGW_SPI3_WDT, 1,                      // 3 wire mode
  PMU_RANGE, RANGE_2G,
  PMU_BW, BW_250,
  
  // Throw out old FIFO data, reset errors, XYZ mode
  FIFO_CONFIG_1, FIFO_STREAM|FIFO_WORKAROUND, // Undocumented FIFO_WORKAROUND
  
  // From Bosch MIS-AN003: FIFO bug workarounds
  0x35, 0xCA,   // Undocumented sequence to turn off temperature sensor
  0x35, 0xCA,
  0x4F, 0x00,
  0x35, 0x0A,

  // Enter NORMAL mode
  PMU_LPW, PMU_NORMAL,
  0
};

// Initialize accelerometer for run mode
void AccelInit()
{
  u8 i = 0;
  
  if (IsCharger())
    return;   // Skip startup
  
  _taps = 0;

  // Run accelerometer setup script to stream data
  do {
    // Write registers, must be 2uS apart (measured at 9uS)
    DataWrite(ACCEL_INIT[i], ACCEL_INIT[i+1]);
    
    // Must wait 1.8ms after each reset
    if (ACCEL_INIT[i] == BGW_SOFTRESET)
    {
      TimerStart(1800);
      while (TimerMSB())
      {}
    }
    i += 2;
  } while (ACCEL_INIT[i]);
}
