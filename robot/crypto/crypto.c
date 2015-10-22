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

#include "key.h"
#include "crypto.h"

// Const array used to contain key material
const u32b CRYPTO_X0[8] = KEY_X0;

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

const   u32b MASK_TAB[4]    =   {0,0xFF,0xFFFF,0xFFFFFF};

/* Crypto per-nonce setup */
void CryptoSetupNonce(CryptoContext *ctx,const u08p noncePtr)
    {
    u32b i,j,n;
    c32p nonce = (c32p) noncePtr;
    CryptoContext pc;

    pc = *ctx;              /* make a local copy, for speed */

    /* initialize subkeys and Z values */
    for (i=0;i<4;i++)
        {
        n = (nonce[i]);
        pc.ks.X_1[i  ] = CRYPTO_X0[i+4] +    n ;
        pc.ks.X_1[i+4] = CRYPTO_X0[i  ] + (i-n);
        pc.cs.Z  [i  ] = CRYPTO_X0[i+3] ^ (nonce[i]) ;
        }
    pc.ks.X_1[1]   += CRYPTO_X1_BUMP;       /* X' adjustment for i==1 mod 4 */
    pc.ks.X_1[5]   += CRYPTO_X1_BUMP;
    pc.cs.Z[4]      = CRYPTO_X0[7];

    /* show the completed key schedule and init state (if debugging) */
    DebugStrN("SetupNonce: keySize  = %d bits.  ",CRYPTO_KEY_SIZE);
    DebugBytes("MAC tag = %d bits.\n  Nonce=",CRYPTO_MAC_SIZE,CRYPTO_NONCE_SIZE,noncePtr);
    DebugStr  ("\nWorking key schedule:");  
    DebugWords("\nX_i_0=",0,8,CRYPTO_X0); 
    DebugWords("\nX_i_1=",0,8,pc.ks.X_1);
    DebugStr  ("\n\n");
    DebugState(-1," ",pc.cs.Z,"\n",0,0);

    for (i=0;i<8;i++)
        {   /* customized version of loop for zero initialization */
        j = i & 7;

        Crypto_H0(pc.cs.Z,0);
        DebugState(i,"a",pc.cs.Z,"  OldZ       = %08X.     X_i_0 = %08X.",
                   pc.cs.oldZ[i&3],CRYPTO_X0[j]);

        Crypto_H1(pc.cs.Z,CRYPTO_X0[j]);
        DebugState(i,"b",pc.cs.Z,"                          plainText= %08X.",0,0);

        Crypto_H0(pc.cs.Z,0);
        DebugState(i,"c",pc.cs.Z,"               %08s      X_i_1 = %08X.","",pc.ks.X_1[j]);

        Crypto_H1(pc.cs.Z,pc.ks.X_1[j]+i);
        DebugState(i," ",pc.cs.Z,"               %08s","",0);
        DebugStr("\n");

        pc.cs.oldZ[i&3] = pc.cs.Z[OLD_Z_REG]; /* save the "old" value */
        }
    pc.cs.i = i;
    *ctx    = pc;       /* copy back the fully initialized context */
    }

void CryptoEncryptBytes(CryptoContext *ctx,const u08p pt,u08p ct,u32b msgLen)
    {
    u32b i,j,plainText,cipherText,keyStream;
    c32p srcPtr = (c32p) pt;
    u32p dstPtr = (u32p) ct;
    u32p p      = NULL;
    CryptoContext pc = *ctx;            /* make a local copy (for speed) */

    DebugStrN("EncryptBytes: %d bytes\n",msgLen);

    i = pc.cs.i;

    for (;msgLen;i++,srcPtr++,dstPtr++)
        {
        if (msgLen >= 4)
            msgLen -= 4;

        j = i & 7;

        Crypto_H0(pc.cs.Z,0);
        DebugState(i,"a",pc.cs.Z,"  OldZ       = %08X.     X_i_0 = %08X.",
                   pc.cs.oldZ[i&3],CRYPTO_X0[j]);

        Crypto_H1(pc.cs.Z,CRYPTO_X0[j]);
        keyStream = pc.cs.Z[4] + pc.cs.oldZ[i&3];

        plainText   = (*srcPtr);
        cipherText  = keyStream ^ plainText;
        *dstPtr     = (cipherText);

        DebugState(i,"b",pc.cs.Z,"  Keystream  = %08X.  plainText= %08X.",keyStream,plainText);

        Crypto_H0(pc.cs.Z,plainText);
        DebugState(i,"c",pc.cs.Z,"               %08s     X_i_1  = %08X.","",pc.ks.X_1[j]);

        Crypto_H1(pc.cs.Z,pc.ks.X_1[j]+i);
        DebugState(i," ",pc.cs.Z,"               %08s  cipherText= %08X.","",keyStream^plainText);
        DebugStr("\n");

        pc.cs.oldZ[i&3] = pc.cs.Z[OLD_Z_REG]; /* save the "old" value */
        }

    pc.cs.i = i;
    ctx->cs = pc.cs;                    /* copy back the modified state */
    }

#define PM32(dst,src,n) ((u32p)(dst))[n] = ((u32p)(src))[n]
#define PutMac(d,s,bits)    /* d = dst, s = src */  \
        switch (bits) {                 \
            case 128: PM32(d,s,3);      \
            case  96: PM32(d,s,2);      \
                      PM32(d,s,1);      \
                      PM32(d,s,0);      \
                      break;            \
            default:  memcpy(d,s,((bits)+7)/8); }   /* optimize "common" cases */

void CryptoFinalize(CryptoContext *ctx,u08p mac)
    {
    u32b i,j,k,tmp[MAC_INIT_CNT+MAC_WORD_CNT];
    CryptoContext pc = *ctx;            /* make a local copy (for speed) */

    DebugStr("FinalizeMAC:\n");

    i           = pc.cs.i;
    pc.cs.Z[0] ^= MAC_MAGIC_XOR;

    for (k=0;k<MAC_INIT_CNT+MAC_WORD_CNT;k++,i++)
        {
        j = i & 7;

        Crypto_H0(pc.cs.Z,0);
        DebugState(i,"a",pc.cs.Z,"  OldZ       = %08X.     X_i_0 = %08X.",
                   pc.cs.oldZ[i&3],CRYPTO_X0[j]);

        Crypto_H1(pc.cs.Z,CRYPTO_X0[j]);
        tmp[k] = pc.cs.Z[4] + pc.cs.oldZ[i&3];

        DebugState(i,"b",pc.cs.Z,"  Keystream  = %08X.  plainText= %08X.",keyStream,plainText);

        Crypto_H0(pc.cs.Z,0);	// NDM - this is valid because msgLen % 4 is always 0
        DebugState(i,"c",pc.cs.Z,"               %08s     X_i_1  = %08X.","",pc.ks.X_1[j]);

        Crypto_H1(pc.cs.Z,pc.ks.X_1[j]+i);
        DebugState(i," ",pc.cs.Z,"               %08s  cipherText= %08X.","",keyStream^plainText);
        DebugStr("\n");

        pc.cs.oldZ[i&3] = pc.cs.Z[OLD_Z_REG]; /* save the "old" value */
        }
    
    PutMac(mac,&tmp[MAC_INIT_CNT],CRYPTO_MAC_SIZE);

    /* no need to copy back any modified context -- we're done with this one! */
    }

