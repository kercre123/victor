#include "hal.h"
#include "bma222.h"

// Borrow some SPI accel.c functions from the bootloader
#define DataRead ((u8 (code *) (u8 addr)) 0x3B26)
#define DataWrite ((void (code *) (u8 addr, u8 dataByte)) 0x3AFC)
#define Read ((u8 (code *) (void)) 0x3DCC)
#define Write ((u8 (code *) (u8 b)) 0x3B09)

u8 idata _readings[33];

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
}

// Drain the entire accelerometer FIFO into an outbound packet
void AccelRead()
{
  u8 i;
  u8 idata *p = _readings;
  
  // Chargers return all zero
  if (IsCharger())
  {
    for (i = 0; i < HAND_LEN; i++)
      _radioOut[i] = 0;
    return;
  }

  // Set up SPI for reading the FIFO
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
  ACC = SPIMDAT;          // Drain anything left in the FIFO
  ACC = SPIMDAT;
  SPIMDAT = 0;

  // Read out up to 66 bytes from SPI (store 33 of them - MSB only)
  for (i = 0; i < 33; i++)
  {
    while (!(SPIMSTAT & RXREADY)) // Wait for byte ready
      ;
    if (!(SPIMDAT & 1)) // Bail out early if FIFO runs dry
      break;
    SPIMDAT = 0;        // Pipeline next read

    while (!(SPIMSTAT & RXREADY)) // Wait for byte ready
      ;
    *p = SPIMDAT;       // Grab data
    p++;
    SPIMDAT = 0;        // Pipeline next read
  }
  // Must let final byte clock out, or BMA's FIFO gets pissed
  while (!(SPIMSTAT & TXEMPTY))
    ;
  CSB = 1;
  SPIMCON0 = 0;

  DebugPrint('x', _readings, i);   // See raw accelerometer data
  SimpleTap(i);
}

// Initialize accelerometer for run mode
void AccelInit()
{
  if (IsCharger())
    return;   // Skip startup
  
  _taps = 0;

  // Wake up accelerometer and 1.8ms for it to boot (see datasheet)
  DataWrite(BGW_SOFTRESET,BGW_SOFTRESET_MAGIC);
  TimerStart(1800);
  while (TimerMSB())
  {}
  
  // Set up accelerometer to stream data
  DataWrite(BGW_SPI3_WDT, 1);   // 3 wire mode
  DataWrite(PMU_RANGE, RANGE_2G);
  DataWrite(PMU_BW, BW_250);
  DataWrite(FIFO_CONFIG_1, FIFO_STREAM);
}
