/*
** Crypto implementation based on public domain code.  Original author:  Doug Whiting, Hifn, 2005
**
** The code was optimized by Nathan Monson to use minimal space on Cortext-M0
** The optimized version is Copyright (c) 2011 Nathan Monson and released to Anki without restrictions or royalty
**
** This is the only file meant to be included in the ARM boot loader
** For usage, see the description of cryLoad() in crypto.h, and read crypto.txt
*/
#include "key.h"
#include "crypto.h"

// This shortcut just turns off all crypto and signature checks, using memcpy instead
// This is useful in conjunction with -nocrypto for finding faults without encryption in the way
// #define NO_CRYPTO
#ifdef NO_CRYPTO
inline static void opensafeSetupNonce(CryptoContext *ctx, c32p nonce)
{ return;   }                       // Nothing to setup
inline static void opensafeDecryptPage(CryptoContext *ctx, c32p srcPtr, u32p dstPtr, u32b msgLen)
{ memcpy(dstPtr, srcPtr, msgLen); } // Straight copy
inline static int opensafeCheckMac(CryptoContext *ctx, u32p mac)
{ return 0; }                       // Always passes 
#else

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

/* Complete half block round */
static void Crypto_H(u32p Z, u32b xx, u32b xy)          
{
    Z[0] +=(Z[3]^(xx));         Z[3] = ROTL32(Z[3],ROT_3b);
    Z[1] += Z[4];               Z[4] = ROTL32(Z[4],ROT_4b);
    Z[2] ^= Z[0];               Z[0] = ROTL32(Z[0],ROT_0a);
    Z[3] ^= Z[1];               Z[1] = ROTL32(Z[1],ROT_1a);
    Z[4] += Z[2];               Z[2] = ROTL32(Z[2],ROT_2a);
    Z[0] ^=(Z[3]+(xy));         Z[3] = ROTL32(Z[3],ROT_3a); 
    Z[1] ^= Z[4];               Z[4] = ROTL32(Z[4],ROT_4a); 
    Z[2] += Z[0];               Z[0] = ROTL32(Z[0],ROT_0b); 
    Z[3] += Z[1];               Z[1] = ROTL32(Z[1],ROT_1b); 
    Z[4] ^= Z[2];               Z[2] = ROTL32(Z[2],ROT_2b);
}

#ifdef BOOTLOADER  
extern u32b KEYXOR[8];
#else
const static u32b KEYXOR[8] = KEY_X0;
#endif
static u32b key[8];

/* Crypto per-nonce setup */
void opensafeSetupNonce(CryptoContext *ctx, c32p nonce)
{
    u32b i,n;

    // Initialize key prior to decryption  
    for (i=0; i<8; i++)
      key[i] = KEYXOR[i];
#ifdef BOOTLOADER  
    // Personalize the key for this chip using MCU UID
    // Comment out this code if you need to build a bootloader for JTAG test
    for (i=0; i<3; i++)
      key[i] += ((u32b*)0x1FFFF7AC)[i];
#endif

    /* initialize subkeys and Z values */
    for (i=0;i<4;i++)
    {
        n = (nonce[i]);
        ctx->ks.X_1[i  ] = key[i+4] +    n ;
        ctx->ks.X_1[i+4] = key[i  ] + (i-n);
        ctx->cs.Z  [i  ] = key[i+3] ^ (nonce[i]) ;
    }
    ctx->ks.X_1[1]   += CRYPTO_X1_BUMP;       /* X' adjustment for i==1 mod 4 */
    ctx->ks.X_1[5]   += CRYPTO_X1_BUMP;
    ctx->cs.Z[4]      = key[7];

    for (i=0;i<8;i++)
    {   /* customized version of loop for zero initialization */
        Crypto_H(ctx->cs.Z,0,key[i]);

        // NDM: Hoist constant addition of j=0-7 into X_1 constants
        ctx->ks.X_1[i] += i;

        Crypto_H(ctx->cs.Z,0,ctx->ks.X_1[i]);

        ctx->cs.oldZ[i&3] = ctx->cs.Z[OLD_Z_REG]; /* save the "old" value */
    }
    ctx->cs.i = i;
}

// Decrypt encrypted data of the specified length, in-place (replacing the encrypted data)
// For performance/code size, j and i are kept synchronized through block (eliminating need for j)
void opensafeDecrypt(CryptoContext *ctx, u32p srcPtr, u32b msgLen)
{
    u32b i,plainText,keyStream;

  	i = ctx->cs.i;
    while (msgLen > 0)
    {
        Crypto_H(ctx->cs.Z,0,key[i&7]);
        keyStream = ctx->cs.Z[4] + ctx->cs.oldZ[i&3];
        plainText   = *srcPtr ^ keyStream;
        *srcPtr     = plainText;        
        Crypto_H(ctx->cs.Z,plainText,ctx->ks.X_1[i&7]+(i & (~7)));
        ctx->cs.oldZ[i&3] = ctx->cs.Z[OLD_Z_REG];

        msgLen -= 4;
		    i++;
		    srcPtr++;
    }
	  ctx->cs.i = i;
}

// Returns 0 on success, non-zero on bad MAC
int opensafeCheckMac(CryptoContext *ctx, u32p mac)
{
    const u32b key[8] = KEY_X0;           // Const array containing key material
    u32b i,j,k,tmp[MAC_INIT_CNT+MAC_WORD_CNT];
    u32p tmpmac = &tmp[MAC_INIT_CNT];

    i = ctx->cs.i;
    ctx->cs.Z[0] ^= MAC_MAGIC_XOR;

    for (k=0;k<MAC_INIT_CNT+MAC_WORD_CNT;k++,i++)
    {
        j = i & 7;
        Crypto_H(ctx->cs.Z,0,key[j]);
        tmp[k] = ctx->cs.Z[4] + ctx->cs.oldZ[i&3];
        Crypto_H(ctx->cs.Z,0,ctx->ks.X_1[j]+i-j);	// NDM - this is valid because msgLen % 4 is always 0
        ctx->cs.oldZ[i&3] = ctx->cs.Z[OLD_Z_REG]; /* save the "old" value */
    }
    
    // Return non-zero unless all 128 bits match exactly - don't loop, timing attacks will get you
    return (mac[0] ^ tmpmac[0]) | (mac[1] ^ tmpmac[1]) | (mac[2] ^ tmpmac[2]) | (mac[3] ^ tmpmac[3]);
}
#endif

// Decrypt one entire block of data, return non-zero on error
int opensafeDecryptBlock(safe_header* head)
{
  char* srcPtr = (char*)(head + 1);     // Skip past the header
  u32b mac[4];    
  CryptoContext ctx;

  // Make sure this matches our hardware version
  if (head->taga != SAFE_PLATFORM_TAG_A || head->tagb != SAFE_PLATFORM_TAG_B)
    return 1;
  
  // Keep a backup copy of the MAC for later comparison
  mac[0] = head->mac[0];  mac[1] = head->mac[1];
  mac[2] = head->mac[2];  mac[3] = head->mac[3];

  // Seed decryption with the nonce and decode the remainder of the first page (less header size)
  opensafeSetupNonce(&ctx, (c32p)head);
  opensafeDecrypt(&ctx, (u32p)srcPtr, SAFE_PAYLOAD_SIZE);

  return opensafeCheckMac(&ctx, mac);
}
