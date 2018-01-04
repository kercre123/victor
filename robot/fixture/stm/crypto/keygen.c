/*
** Crypto implementation based on public domain code.  Original author:  Doug Whiting, Hifn, 2005
**
** The code was optimized by Nathan Monson to work efficiently with ARM
** The optimized version is Copyright (c) 2011 Nathan Monson and released to Anki without restrictions or royalty
*/
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>

#include "crypto.h"

/**
 * Keygen REQUIRES a secure source of random numbers - don't cheat with rand()!
 */
#ifdef _WIN32
#include <windows.h>
#include <wincrypt.h> 
void getRandom(void* dest, int count)
{
  HCRYPTPROV hProvider = 0;
  if (CryptAcquireContext(&hProvider, 0, 0, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT) &&
      0 != hProvider &&
      CryptGenRandom(hProvider, count, dest))
    return;
  exit(2);  // Windows sucks
}
#else
void getRandom(void* dest, int count)
{
  FILE* fp = fopen("/dev/urandom", "rb");
  assert(count == fread(dest, 1, count, fp));
  fclose(fp);
}
#endif

enum    /* Crypto algorithm internal constants */
    {
    OLD_Z_REG       =    4,                 /* which var used for "old" state   */
    ZERO_INIT_CNT   =    8,                 /* how many words of initial mixing */
    MAC_INIT_CNT    =    8,                 /* how many words of pre-MAC mixing */
    MAC_WORD_CNT    =  CRYPTO_MAC_SIZE/32,  /* how many words of MAC output     */
    
    ROT_0a          =    9,     /* rotation constants for Crypto block function */
    ROT_1a          =   10,
    ROT_2a          =   17,
    ROT_3a          =   30,
    ROT_4a          =   13,

    ROT_0b          =   20,
    ROT_1b          =   11,
    ROT_2b          =    5,
    ROT_3b          =   15,
    ROT_4b          =   25
    };

#define MAC_MAGIC_XOR   0x912D94F1          /* magic constant for MAC */

/* 1st half of a half block */
#define Crypto_H0(Z,xx)                                             \
            Z[0] +=(Z[3]^(xx));         Z[3] = ROTL32(Z[3],ROT_3b); \
            Z[1] += Z[4];               Z[4] = ROTL32(Z[4],ROT_4b); \
            Z[2] ^= Z[0];               Z[0] = ROTL32(Z[0],ROT_0a); \
            Z[3] ^= Z[1];               Z[1] = ROTL32(Z[1],ROT_1a); \
            Z[4] += Z[2];               Z[2] = ROTL32(Z[2],ROT_2a); 
                        
/* 2nd half of a half block */
#define Crypto_H1(Z,xx)                                             \
            Z[0] ^=(Z[3]+(xx));         Z[3] = ROTL32(Z[3],ROT_3a); \
            Z[1] ^= Z[4];               Z[4] = ROTL32(Z[4],ROT_4a); \
            Z[2] += Z[0];               Z[0] = ROTL32(Z[0],ROT_0b); \
            Z[3] += Z[1];               Z[1] = ROTL32(Z[1],ROT_1b); \
            Z[4] ^= Z[2];               Z[2] = ROTL32(Z[2],ROT_2b);

int  _debugCipher_ = 0;             /* global: enable debug i/o */

/*  some useful macros for detailed debug output */
#ifdef ALLOW_DEBUG_IO
#include <stdio.h>
void DebugByteOut(const void *p,u32b cnt)
    { u32b j; for (j=0;j<cnt;j++) printf("%s%02X",(j%16)?" ":"\n  ",((u08p)p)[j]); }

void DebugWordOut(const void *p,u32b cnt,const char *suffix)
    { u32b j; for (j=0;j<cnt;j++) printf("%08X%s",((u32b *)p)[j],(j==cnt-1)?suffix:" "); }

#define DebugStr(s)                 if (_debugCipher_) printf(s)
#define DebugStrN(s,N)              if (_debugCipher_) printf(s,N)
#define DebugWords(fmtStr,v,wCnt,p)       if (_debugCipher_)            \
                {  printf(fmtStr,v,wCnt);   DebugWordOut(p,wCnt," "); }
#define DebugBytes(fmtStr,v,bitCnt,p) if (_debugCipher_)                \
                {  printf(fmtStr,v,bitCnt); DebugByteOut(p,bitCnt/8); }
#define DebugState(i,c,Z,s,v0,v1)   if (_debugCipher_)                  \
                {  printf("Z.%02d%s=",i-7,c); DebugWordOut(Z,5,".");    \
                   printf(s,v0,v1); printf("\n"); }
#else   /* do not generate any debug i/o code at all (for speed) */
#define DebugStr(s) 
#define DebugStrN(s,N)
#define DebugState(i,c,Z,s,v0,v1)
#define DebugWords(fmtStr,v,wCnt,p)
#define DebugBytes(fmtStr,v,bitCnt,p)
#endif

/* Crypto key schedule setup */
void CryptoSetupKey(u32p X_0,const u08p keyPtr,u32b keySize,u32b ivSize,u32b macSize)
    {
    u32b    i,j,k,Z[5],X[8];

    DebugStrN("SetupKey:  keySize = %d bits. ",keySize);
    DebugBytes("MAC tag = %d bits.\n  Raw Key = ",macSize,keySize,keyPtr);

    assert(CRYPTO_NONCE_SIZE==ivSize);  /* Crypto only supports "full" nonces       */
    assert( 0  == (keySize%8));         /* Crypto only supports byte-sized keys     */
    assert(256 >=  keySize);            /* Crypto only supports keys <= 256 bits    */

    for (i=0;i<(keySize+31)/32;i++)     /* copy key to X[], in correct endianness   */
        X[i] = (((c32p)keyPtr)[i]);
    for (   ;i<8;i++)                   /* handle zero padding at the word level    */
        X[i] = 0;           
    if (keySize % 32)                   /* handle zero padding of the bit  level    */
        X[keySize/32] &= (1 << (keySize%32)) - 1;

    DebugWords("\nKeyMixing:\nX.%d  =",8,8,X);
    /* Now process the padded "raw" key, using a Feistel network */
    for (i=0;i<8;i++)
        {
        k = 4*(i&1);
        for (j=0;j<4;j++)
            Z[j] = X[k+j];
        Z[4] = (keySize/8) + 64;
        Crypto_H0(Z,0);
        Crypto_H1(Z,0);
        Crypto_H0(Z,0);
        Crypto_H1(Z,0);
        k = (k+4) % 8;
        for (j=0;j<4;j++)
            X[k+j] ^= Z[j];
        DebugWords("\nX.%d  =",7-i,8,X);
        }
    for (i=0;i<8;i++)
        X_0[i] = X[i];
    }

// Print a number of 32-bit words
void print32(char* head, void* ptr, size_t size, char* tail)
{
  int* iptr = (int*)ptr;
  assert(0 == (size & 3));  // 32-bit words only
  if (size < 1)
    return;
  size >>= 2;
  printf("%s", head);
  while (--size)
    printf("0x%08x,", *iptr++);
  printf("0x%08x%s", *iptr++, tail);
}

/**
 * Main entry point - just create a random key and spit out the context
 */
int main(int argc, char **argv)
{
  u08b key[CRYPTO_KEY_SIZE/8];
  u32b ctx[8];

  printf("/**\n * NOCOMMIT XXX TBD DANGER!  DO NOT COMMIT SECRET KEYS!\n**/\n#ifndef _KEY_H\n#define _KEY_H\n\n");

  getRandom(key, sizeof(key));
  print32("#define KEY    { ", key, sizeof(key), " }\n");

  memset(ctx, 0, sizeof(ctx));
  CryptoSetupKey(ctx, key, CRYPTO_KEY_SIZE, CRYPTO_NONCE_SIZE, CRYPTO_MAC_SIZE);
  print32("#define KEY_X0 { ", ctx, sizeof(ctx), " }\n\n");

  printf("#endif  // _KEY_H\n");

  return 0;
}

