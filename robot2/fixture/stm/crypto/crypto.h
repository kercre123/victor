#ifndef _CRYPTO_H_
#define _CRYPTO_H_
/*
** Crypto implementation based on public domain code.  Original author:  Doug Whiting, Hifn, 2005
**
** The code was optimized by Nathan Monson to work efficiently with ARM
** The optimized version is Copyright (c) 2011 Nathan Monson and released to Anki without restrictions or royalty
*/

#define OTA_PACKET_SIZE		20		// 20 byte packets

// This is for Drive HW version 5.x, or DriveHW5
// Each breaking hardware revision must be increment this tag
#define SAFE_PLATFORM_TAG_A  0x76697244
#define SAFE_PLATFORM_TAG_B  0x35574865

// This is for HW version 5.4+
#define OTA_PLATFORM_TAG     0x54eeffc0

#ifndef FIXTURE
#   define TARGET_IMAGE_SIZE	 (56*1024)	 // 56KB is all the flash we have (minus bootloader)
#   define TARGET_IMAGE_BASE	 0x08002000	 // Base address of image
#else
#   define TARGET_IMAGE_SIZE     (640 * 1024)
#   define TARGET_IMAGE_BASE     0x08060000
#endif

#define SAFE_PAYLOAD_SIZE    2048	     // Must be a power of 2!	Does not need to match flash block size.
#define SAFE_HEADER_SIZE     32          // Can't be changed - you're stuck with it    
#define SAFE_BLOCK_SIZE      (SAFE_PAYLOAD_SIZE + SAFE_HEADER_SIZE)

#define TARGET_MAX_BLOCK    (TARGET_IMAGE_SIZE/SAFE_PAYLOAD_SIZE)    // Round down!
#define TARGET_MAX_BYTE     (TARGET_MAX_BLOCK*SAFE_PAYLOAD_SIZE)

// Don't blame me for these weird types, they come from the crypto libraries
typedef unsigned char   u08b;
typedef unsigned short  u16b;
typedef unsigned int    u32b;
typedef const u32b    * c32p; 
typedef       u32b    * u32p;
typedef       u08b    * u08p;

// The safe_header structure starts each and every SAFE_BLOCK
// It must be exactly 32 bytes, because there is no secure way to place anything but a 16-byte nonce and 16-byte mac here
// All parameters in the nonce (first 16-bytes) are cryptographically secured by the mac while still being cleartext readable
typedef struct {
  u32b taga;        // Set to SAFE_PLATFORM_TAG - prevents the boot loader from loading the wrong platform's binaries
  u32b tagb;        // Could be used for future expansion - this is the only available space
  u32b guid;        // MUST NEVER REPEAT!  Low 16-bits are build number, high 16-bits are effectively random
  u32b blockFlags;  // Flags for this block, of the form BLOCK_FLAG_XXX - unique for each block
  u32b mac[4];      // 16-byte message authentication code, indicating this block has not been tampered with
} safe_header;

// The ota_header structure appears just once in the OTA output
// It must be exactly 32 bytes, because there is no secure way to place anything but a 16-byte nonce and 16-byte mac here
typedef struct {
  u32b tag;         // Set to OTA_PLATFORM_TAG - prevents the boot loader from loading incompatible binaries
  u16b srclen;      // Source (compressed/encrypted) length in bytes - excluding header
  u16b destlen;     // Destination length in 16-bit words
  u32b guid;        // MUST NEVER REPEAT!  Unique identifier for this image - usually set to gettimeofday() - in hopes we won't repeat
  u16b millis;      // Milliseconds per minute - reduces chance of collision
  u08b who;         // Indicates which station/PC signed the header
  u08b version;     // Major version number - incremented on releases
  u32b mac[4];      // 16-byte message authentication code, indicating this block has not been tampered with
} ota_header;

// Block flag bits
#define BLOCK_FLAG_BLOCK		0x1FF             // Mask for this block's address - 1MB ought to be enough for anyone!
#define BLOCK_FLAG_LAST			0x4000            // Set on the last block
#define BLOCK_FLAG_SHORT_LAST	0x8000            // Set on the last block for files <= 128KB (old fixtures)

/** Crypto stuff follows **/
#define CRYPTO_NONCE_SIZE   128     /* size of nonce        in bits */
#define CRYPTO_MAC_SIZE     128     /* size of full mac tag in bits */
#define CRYPTO_KEY_SIZE     256     /* size of full key     in bits */
#define CRYPTO_X1_BUMP      128     /* computed as keySize/2 + 256*(macSize % CRYPTO_MAC_SIZE) */

typedef struct          /* keep all cipher state                    */
{       
    struct  /* key schedule (changes once for each setupNonce)      */
    {
        u32b    X_1[8];
    } ks;   /* ks --> "key schedule" */

    struct  /* internal cipher state (varies during block) */
    {
        u32b    oldZ[4];    /* previous four Z_4 values for output  */
        u32b    Z[5];       /* 5 internal state words (160 bits)    */
        u32b    i;          /* block number                         */
    } cs;   /* cs --> "cipher state" */
} CryptoContext;

/* Crypto function prototypes, given definition of CryptoContext    */
void CryptoSetupNonce  (CryptoContext *ctx,const u08p noncePtr);
void CryptoEncryptBytes(CryptoContext *ctx,const u08p plaintext,u08p ciphertext,u32b msgLen);
void CryptoDecryptBytes(CryptoContext *ctx,const u08p ciphertext,u08p plaintext,u32b msgLen);
void CryptoFinalize    (CryptoContext *ctx,u08p mac);

int opensafeDecryptBlock(safe_header* head);
int opensafeCheckMac(CryptoContext *ctx, u32p mac);
void opensafeDecrypt(CryptoContext *ctx, u32p srcPtr, u32b msgLen);
void opensafeSetupNonce(CryptoContext *ctx, c32p nonce);

// This is recognized as a ROL/ROR by ARM GCC
#ifndef ROTL32
#define ROTL32(x,n) ((u32b)(((x) << (n)) ^ ((x) >> (32-(n)))))
#endif

#ifdef _WIN32
#define inline __inline
#endif

#endif /* ifndef _CRYPTO_H_ */
