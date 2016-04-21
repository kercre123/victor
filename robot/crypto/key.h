/**
 * THIS IS THE COZMO FIXTURE KEY.. If you have a new purpose, go use keygen.exe to make a new one.
**/
#ifndef _KEY_H
#define _KEY_H

#define KEY    { 0xfa67929e,0xa63619d8,0x0fd775a9,0x242b7676,0xc086dbf9,0xbc73f730,0xb2217a81,0x45f9deda }
#define KEY_X0 { 0x8c2230c6,0xcdc085b0,0xc59c9984,0x79686371,0x2aa1fe3c,0x6cfc36ae,0xb229b19e,0x98d8420b }

// Here's the 128-bit XXTEA key used for cubes, chargers, and other Cozmo accessories
#define XX_KEY		{ 0xdded8817,         0xdc28e60f,         0x419876fa,         0xdcfd3cfa          }
// Big-endian version used for 8051 - since Keil C51 uses big-endian (sometimes)
#define XX_BEKEY	{ 0xdd,0xed,0x88,0x17,0xdc,0x28,0xe6,0x0f,0x41,0x98,0x76,0xfa,0xdc,0xfd,0x3c,0xfa }

#endif  // _KEY_H
