#include "hal.h"
#include "bma222.h"

// Borrow some I2C functions from the bootloader
// XXX: This API will all need to be reworked for EP3F / SPI
#define Start ((void (code *) (void)) 0x3ACF)
#define Stop ((void (code *) (void)) 0x3A9F)
#define Read ((u8 (code *) (u8 ack)) 0x3AE8)
#define Write ((void (code *) (u8 b)) 0x3AB0)
#define DataRead ((u8 (code *) (u8 reg)) 0x3A89)
#define DataWrite ((void (code *) (u8 reg, u8 val)) 0x3ADA)

u8 idata _readings[33];

// This is the "simple" (EP1) tap detector
#define TAP_THRESH    20        // Equivalent to EP1 10 (since I shift less)
#define TAP_DEBOUNCE  45        // 45/500 = 90ms before tap is recognized
#define TAP_DURATION  5         // 5/100 = 10ms is max length of a tap (anything slower is rejected)
u8 _debounce, _taps;
s8 _last;
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
    
    if (!_debounce)
    {
      if (diff > TAP_THRESH)
      {
        _debounce = TAP_DEBOUNCE;
        _posFirst = 1;
      }
      else if (diff < -TAP_THRESH)
      {
        _debounce = TAP_DEBOUNCE;
        _posFirst = 0;
      }
    }
    else 
    {
      if (_debounce > (TAP_DEBOUNCE-TAP_DURATION) && 
          ((diff >  TAP_THRESH && !_posFirst) || 
           (diff < -TAP_THRESH &&  _posFirst)))
      {
        _taps++;
        _debounce = TAP_DEBOUNCE-TAP_DURATION;
      }
      _debounce--;
    }
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
  
  if (IsCharger())
    return;

  Start(); //start condition
  Write((I2C_ADDR << 1) | I2C_WRITE_BIT); //slave address send
  Write(FIFO_DATA); //word address send
  Stop();
  Start();
  Write((I2C_ADDR << 1) | I2C_READ_BIT);
  for (i = 0; i < 33; i++)
  {
    if (!(Read(I2C_ACK) & 1))   // Bail out early if FIFO runs dry
      break;
    *p = Read(I2C_ACK);
    p++;
  }
  Read(I2C_NACK);
  Stop();
  
  // DebugPrint('x', _readings, i);   // See raw accelerometer data
  SimpleTap(i);
}

// Initialize accelerometer for run mode
void AccelInit()
{
  if (IsCharger())
    return;   // Skip startup, DO NOT init P1.x (they're grounded on EP3)
  
  _taps = 0;
  
  // Wake up accelerometer and 1.8ms fot it to boot (see datasheet)
  I2CInit();  
  DataRead(BGW_CHIPID);
  DataWrite(BGW_SOFTRESET,BGW_SOFTRESET_MAGIC);
  TimerStart(1800);
  while (TimerMSB())
  {}
  
  // Set up accelerometer to stream data
  DataWrite(PMU_RANGE, RANGE_2G);
  DataWrite(PMU_BW, BW_250);
  DataWrite(FIFO_CONFIG_1, FIFO_STREAM);
}
