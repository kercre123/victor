/*
** This creates .safe files suitable for OTA upgrades on Cozmo cube/accessories
** Cubes use a weird 8051 CPU from Nordic that is one-time programmable, affecting many design decisions below
** Cubes don't need security, just a very robust "checksum" (to prevent unrecoverable OTP mistakes) and some obfuscation
** So I'm using XXTEA (a symmetric block cipher) in 256 byte blocks, with the last 8 bytes all 0 as a "checksum"
** The remaining 248 bytes contain 1 or more "chunks" - a 3 byte header (base+length) and a variable length payload to program
** It's up to the robot to program the accessories safely - never sending the next block until the last is ACKed
** Since version info is stored in the last chunk, an outdated version indicates a need to resend all blocks again
*/
#include "key.h"
#include "cube.h"
#include "string.h"
#include "stdlib.h"
#include "stdio.h"

#define MX (z>>5^y<<2) + (y>>3^z<<4) ^ (sum^y) + (k[p&3^e]^z);

long _KEY[] = XX_KEY;

// Official (public domain) XXTEA implementation
long btea(long* v, long n, long* k) {
  unsigned long z=v[n-1], y=v[0], sum=0, e, DELTA=0x9e3779b9;
  long p, q ;
  if (n > 1) {          /* Coding Part */
    q = 6 + 52/n;
    while (q-- > 0) {
      sum += DELTA;
      e = (sum >> 2) & 3;
      for (p=0; p<n-1; p++) y = v[p+1], z = v[p] += MX;
      y = v[0];
      z = v[n-1] += MX;
    }
    return 0 ; 
  } else if (n < -1) {  /* Decoding Part */
    n = -n;
    q = 6 + 52/n;
    sum = q*DELTA ;
    while (sum != 0) {
      e = (sum >> 2) & 3;
      for (p=n-1; p>0; p--) z = v[p-1], y = v[p] -= MX;
      z = v[n-1];
      y = v[0] -= MX;
      sum -= DELTA;
    }
    return 0;
  }
  return 1;
}

u8 _xx[XX_LEN];
// Keil requires byte swapped input/output
void swapForKeil()
{
  for (int i = 0; i < XX_LEN; i+=4)
    *(u32*)(_xx+i) = _byteswap_ulong(*(u32*)(_xx+i));
}

int cubeEncrypt(u8* src, u8* dest, bool nocrypt)
{
	int in = 0;
  int destp = 0;

	while (in < CUBE_LEN)
	{
    // Start up a new XXTEA block
    memset(_xx, 0, XX_LEN);
    int out = XX_HEADER;    // Skip first 8 bytes (zeros)
    while (out < XX_LEN-4 && in < CUBE_LEN)   // Make sure there's room for 4 byte minimum chunk
    {
      // Start by searching for a chunk of non-FF (we never start inside a chunk)
      while (0xff == src[in])
        if (++in > CUBE_LEN)
          break;
      // Then compute the length of the chunk in advance (ends at ff ff ff ff)
      int end = in;
      while (0xff != src[end] || 0xff != src[end+1] || 0xff != src[end+2] || 0xff != src[end+3])
        end++;
      int len = end-in, space = XX_LEN-3-out;   // Compute space remaining for data (-3 byte chunk header)
      if (len > space)
        len = space;
      // Add the chunk header and copy the payload over
      if (len > 0)
      {
        _xx[out++] = in >> 8;    // Base address: Keil C51 is big endian
        _xx[out++] = in;
        _xx[out++] = len;        // Payload length
        memcpy(_xx+out, src+in, len);
        in += len;
        out += len;
      }
    }
    // Encrypt it
    if (!nocrypt) {
      swapForKeil();           // Keil C51 uses big endian for 32-bit values
      btea((long*)_xx, -XX_LEN/4, _KEY);
      swapForKeil();           // Keil C51 uses big endian for 32-bit values
#ifdef TEST_CRYPT
      for (int i = 0; i < XX_LEN; i++)
        printf("0x%02x,", _xx[i]);
      printf("\n");
#endif
    }
    // Write out the block
    memcpy(dest+destp, _xx, XX_LEN);
    destp += XX_LEN;
	}

	return destp;
}

#define DELTA 0x9e3779b9
#define ROUNDS 6 					// 6 + 52/CRYPT_WORDS - we reach the minimum since CRYPT_WORDS > 52

u8 bekey[] = XX_BEKEY;
#define pram32(x) *((u32 *)(_xx + (x & 255)))
#define key32(x) _byteswap_ulong(*(u32*)(bekey+(x)))

// C51-optimized version of the code
// This takes advantage of 256 byte wrap-around on pointers and orders the code for best register use
void Decrypt()
{
	u8 rounds = ROUNDS, p = 0, e;
	u32 sum = 0;
	
	// XXTEA encrypt/decrypt are symmetrical - we're actually using Encrypt below since it's faster/smaller
	// Note:  The signing tool must byte-swap each word
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
}

// Decode a cube inage
void openCube(u8* src, u8* dest)
{
  int destp = 0;

  // Bail out when we run out of headers to parse
  while (1)
  {
    int in = XX_HEADER;
    memcpy(_xx, src+destp, XX_LEN);
    destp += XX_LEN;

    // Decrypt it
    swapForKeil();    // Keil C51 uses big endian for 32-bit values
    Decrypt();
    swapForKeil();    // Keil C51 uses big endian for 32-bit values
    //btea((long*)_xx, XX_LEN/4, _KEY);   // Alternate way to test it

    // Check header - bail out if we're past the end of the file
    if (0 != *(u32*)(_xx) || 0 != *(u32*)(_xx+4))
      break;

    // Parse the chunk until we find a length byte of 0
    do
    {
      u16 out = _xx[in] << 8; // Base address: Keil C51 is big endian
      in = (in + 1) & 255;
      out |= _xx[in];
      in = (in + 1) & 255;
      u8 len = _xx[in];
      in = (in + 1) & 255;
      if (!len)
        break;
      memcpy(dest+out, _xx+in, len);
      in = (in + len) & 255;
    } while (1);
  }
}
