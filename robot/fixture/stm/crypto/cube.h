/**
 * THIS IS THE COZMO FIXTURE KEY.. If you have a new purpose, go use keygen.exe to make a new one.
**/
#ifndef _CUBE_H
#define _CUBE_H

#define CUBE_LEN 16384
#define XX_LEN 256
#define XX_HEADER 8

typedef unsigned char         u8;
typedef unsigned int          u16;
typedef unsigned long         u32;

typedef signed char           s8;
typedef signed int            s16;
typedef signed long           s32;

int cubeEncrypt(u8* src, u8* dest, bool nocrypt);
void openCube(u8* src, u8* dest);

#endif
