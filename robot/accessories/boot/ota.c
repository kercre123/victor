// OTA files are encrypted with 256-byte block XXTEA as an HMAC to ensure data integrity
// We can't afford to OTP a single wrong byte - it would permanently damage the accessory
#include "hal.h"

// This key must match key.h in makesafe (the signing tool)
u8 code bekey[] = { 0xdd,0xed,0x88,0x17,0xdc,0x28,0xe6,0x0f,0x41,0x98,0x76,0xfa,0xdc,0xfd,0x3c,0xfa };

// C51-efficient shortcuts to access arrays using byte pointers
#define pram32(x) *((u32 pdata *)(x))
#define pram16(x) *((u16 pdata *)(x))
#define pram8(x) *((u8 pdata *)(x))
#define key32(x) *(u32 code *)(bekey+(x))

// Decrypt the contents of _pram (caller sets which page) - if valid, burn to OTP
// This takes a long time - up to 250ms - so make sure watchdog is pet and accelerometer is off
void OTABurn()
{
  #define DELTA 0x9e3779b9  // XXTEA magic
  #define ROUNDS 6          // 6 + 52/CRYPT_WORDS - or just 6 since CRYPT_WORDS=64
  
  u8 rounds = ROUNDS, p = 0, e;
  u32 sum = 0;
  
  u8 xdata *dest;
  u8 len, src, temp;

  // This is an interlock to prevent stray jumps from skipping the XXTEA step
  FSR = FSR_WEN;      // Allow writes
  
  // Decrypt XXTEA block (we use "encrypt" below since it's symmetrical, and faster/smaller)
  do {
    sum += DELTA;
    e = sum & 255;
    do {
      u32 y = pram32(p+4);
      u32 z = pram32(p-4);
      u32 t = ((z ^ key32((p^e)&12)) + (sum^y)) ^ ((z>>5^y<<2) + (y>>3^z<<4));
      pram32(p) += t;
      p += 4;
    } while (p);
  } while (--rounds);
  
  // Ensure the first 8 bytes (our pseudo-HMAC) are all zero
  src = 0;
  do {
    while (_pram[src])
      ;   // If HMAC is invalid, freeze here - the watchdog will reboot us
    src++;
  } while (lessthanpow2(src, 8));
  
  // If we get here, begin the permanent unrepairable burning.. gulp..
  do {
    // Read the chunk header, then copy the chunk
    dest = (u8 xdata *) pram16(src);
    src += 2;
    len = pram8(src);
    src++;
    
    // A zero-length chunk indicates a successful completion, so return to the caller
    if (!len)
    {
      FSR = 0;            // Disallow writes - the only way out!
      return;
    }
    
    // Burn the chunk, byte by byte
    do {
      temp = pram8(src);  // Fetch source byte
      src++;
      
      // LED interrupts happen here - but they don't use pram, so it won't break anything
      // But burning wrecks LED/RTC timing by freezing 800uS per byte, breaking OTA
      PCON = PCON_PMW;    // Map code space over to xdata
      *dest = temp;       // BURN IT
      PCON = 0;           // Restore data space (to fetch next byte safely)
      
      dest++;
    } while (--len);
  } while (1);
}
