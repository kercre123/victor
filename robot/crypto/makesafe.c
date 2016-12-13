/*
** This is a command-line image packager that encrypts to .safe format
** Run without arguments for usage information
*/
#define _CRT_SECURE_NO_WARNINGS
#include "cube.h"
#include "crypto.h"
#include "key.h"
#include "stdio.h"
#include "stdlib.h"
#include "memory.h"
#include "assert.h"

// This version number is reserved for developer builds
#define DEV_VERSION 0xd3b
#define MIN_REAL_VERSION 0x2020   // Minimum real version

int m_guid = 0;
int TestCrunch(char* dest, unsigned short* src, int srclen, int guid);

// Need a timestamp to form a 32-bit GUID - in Linux, gettimeofday works, in Windows we have to be more creative
#ifdef _WIN32
#include "windows.h"
static int getGUID(void)
{
  FILETIME ft;
  GetSystemTimeAsFileTime(&ft);	// Get time in 100ns units since 1900
  return ((ft.dwHighDateTime-30250000) << 12) | ((ft.dwLowDateTime >> 20) & 4095);   // One "tick" per 0.1s
}
#else
#include "sys/time.h"
static int getGUID(void)
{
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return tv.tv_sec;
}
#endif

// Local state used to process input
static char m_stageImage[TARGET_IMAGE_SIZE];    // Staging view of memory
static char m_safeImage[TARGET_IMAGE_SIZE*2];    // Image of safe file (double size, for scramble mode)
static char m_verifyImage[TARGET_IMAGE_SIZE];   // Verification view of memory
static char m_blockMap[TARGET_MAX_BLOCK];       // Map of used blocks for staging area
static char m_otaImage[TARGET_IMAGE_SIZE*5/4];	// OTA version of the image - can grow 9/8 the size of original

// Print usage - see main() for parser
void usage(char* msg, char* arg)
{
  if (msg)
    printf(msg, arg);
  printf("\n\n");
  printf("makesafe -prod [-option ...] output.safe input.bin\n");
  printf("makesafe -ota [-option ...] output.safe input.bin\n");
  printf("  Create a prod image in .safe format, or a .safe plus an .ota file (-ota)\n");
  printf("  Valid options include:\n");
  printf("    -nocrypto  create a clear file for use with #define NO_CRYPTO in opensafe.c\n");
  printf("    -scramble  for testing, scramble and duplicate the order of blocks\n\n");
  printf("makesafe -cube [-option ...] output.safe input.bin\n");
  printf("  Create a cube image in .safe format - the .bin must be zero-based\n\n");
  printf("makesafe -opensafe [-cube] input.safe output.bin\n");
  printf("  Decode the .safe file into a binary image of main memory\n");
  exit(1);
}
typedef enum {
  OPT_PKG = 1,      // Packaging is what -prod/-ota do
  OPT_UNSAFE = 2,   // -decode
  OPT_OTA = 4,      // ota flag - if absent, this is a prod build
  OPT_NOCRYPTO = 8,
  OPT_SCRAMBLE = 16,
  OPT_GUID = 32,
  OPT_CUBE = 64
} cmdline;

// Return bits of the safe image when testing opensafe()
void* nandReadPage(int nandAddress)
{
  return m_safeImage + nandAddress;
}

// Fail with an error message
void bye(char* msg)
{
  printf("%s\n\n", msg);
  exit(2);
}

// Get the current version number from whatever.build
int getVersion(char* filename)
{
  // Add .build to the filename
  int version = DEV_VERSION;
	char buildname[256], builddat[256];
  if (NULL == strchr(filename, '.'))
    bye("Need a .safe extension in the dest filename");
	strncpy(buildname, filename, 256);
	strcpy(strchr(buildname, '.'), ".build");

  // Read it
  FILE* fp = fopen(buildname, "r");
  if (NULL != fp) {
    memset(builddat, 0, sizeof(builddat));
    fread(builddat, 1, sizeof(builddat), fp);
    sscanf(builddat+6, "%04X", &version);
    fclose(fp);
  }

  // Increment it as if it is a build number, and do not allow LSB to be <0x20 or >0x7e
  if (version >= MIN_REAL_VERSION) {
    version++;
    if ((version & 255) < 0x20 || (version & 255) > 0x7e)
      version = ((version + 256) & (~255)) + 0x20;   // Increment MSB, start LSB at 0x20
  }

  // Write it
  fp = fopen(buildname, "w");
  if (NULL == fp)
    bye("Unable to write .build file");
  fprintf(fp, "Build %04x autocommit by build server\n", version);
  fclose(fp);

  return version;
}

// Test/hack the header in a binary file
void hackHeader(char* target, safe_header* safehead)
{
#ifndef FIXTURE
	// See hal/flashparams.h and startup_stm32f0xx.s file in vehicle for more details
	int* header = (int*)(target);
	if (header[4] != 0x494b4e41)	// 'ANKI'
		bye("Source .bin file is missing required header");
  header[0] = 0xffffffff;       // Set "invalid" flag to -1
	header[6] = safehead->guid;
#else
    int* header = (int*)(target + 28);
    header[0] = safehead->guid;
#endif
}

// Write out one encrypted block at block number 'i'
void safeWriteBlock(FILE *fp, CryptoContext* ctx, safe_header* head, int i, int options)
{
  if (m_blockMap[i])
  {
    int srcAddr = i * SAFE_PAYLOAD_SIZE;
    char* srcPtr = m_stageImage + srcAddr;

    // If no decryption, memcpy, else decrypt
    if (options & OPT_NOCRYPTO)
    {
      memcpy(head + 1, srcPtr, SAFE_PAYLOAD_SIZE);
    } else {
	    head->blockFlags = i;
	    if (!m_blockMap[i+1])
		    head->blockFlags |= BLOCK_FLAG_LAST;
	    if (!m_blockMap[i+1] || i == 63)	// Set for backward compatibility with old 128KB fixtures
		    head->blockFlags |= BLOCK_FLAG_SHORT_LAST;
      CryptoSetupNonce(ctx, (u08p)head);          // The nonce is the first 16-bytes of the header
      CryptoEncryptBytes(ctx, (u08p)srcPtr, (u08p)(head + 1), SAFE_PAYLOAD_SIZE);
      CryptoFinalize(ctx, (u08p)head->mac); // Fill in MAC
    }
    fwrite(head, 1, SAFE_BLOCK_SIZE, fp);
  }
}

// Based on m_blockMap, write out the m_stageImage as a .safe file
void safeWrite(char* filename, int options)
{
  u08b block[SAFE_BLOCK_SIZE];
  safe_header* head = (safe_header*)block;
  FILE *fp;
  int i = 0, blocks = 0, scramble = 0;
  CryptoContext ctx;

  assert(32 == SAFE_HEADER_SIZE && 32 == sizeof(safe_header));  // Must be 32 due to nonce/mac limits
  assert(BLOCK_FLAG_BLOCK >= TARGET_MAX_BLOCK);  // Make sure our block addresses will fit in the header

  // Set up per-image headers
  head->taga = SAFE_PLATFORM_TAG_A;
  head->tagb = SAFE_PLATFORM_TAG_B;
  head->guid = m_guid;

  // Hack the header to match our newly generated GUID
  hackHeader(m_stageImage, head);

  // Open file for write
  fp = fopen(filename, "wb");
  printf("Writing %s...", filename);
  if (NULL == fp)
    bye("could not create file");

  // Write out each block
  for (i = 0; i < TARGET_MAX_BLOCK; i++)
  {
    safeWriteBlock(fp, &ctx, head, i, options);
    if (options & OPT_SCRAMBLE)
      safeWriteBlock(fp, &ctx, head, (TARGET_MAX_BLOCK-1) - i, options);
	blocks += m_blockMap[i];
  }
  fclose(fp);

  printf("wrote %d bytes, %d blocks, version 0x%04x, GUID 0x%08x\n", blocks * SAFE_BLOCK_SIZE, blocks, head->guid & 0xffff, head->guid);
}

// Write a cube image
void cubeWrite(char* filename, int options)
{
  // Check patch for valid start location, get hardware version from last byte of filename
  int start = (m_stageImage[1] << 8) | (u8)m_stageImage[2];
  int hwVer = filename[strlen(filename)-1] - '0';
  printf("Start address 0x%04x (%02x%02x), hardware version %x\n", start, m_stageImage[start]&0xff, m_stageImage[start+1]&0xff, hwVer);
  if (m_stageImage[0] != 0x2 || (start & 0xff))
    bye("FAIL:  Cube patch must be linked to start on a 256 byte boundary");
  if (hwVer < 4 || hwVer > 9)
    bye("Accessory filename must end in a valid hardware version\n");

  // Patch must start with mov sp, ... instruction - denoting proper startup.a51 file
  // Also, you can't just hack this here - the bootloader is looking for this combination
  if ((m_stageImage[start]&0xff) != 0x75 || (m_stageImage[start+1]&0xff) != 0x81)
    bye("FAIL:  Cube patch startup.a51 invalid");
  // Wipe out the start address so it doesn't become part of the patch
  m_stageImage[0] = m_stageImage[1] = m_stageImage[2] = -1;

  // XXX: Implement a patching mechanism for interrupt vectors (when you need it)
  for (int i = 0; i < 128; i++)
    if (0xff != (u8)m_stageImage[i])
      bye("FAIL:  Interrupts don't work in cube patches yet - add patch logic to cubeWrite in makesafe");

  // Figure out the "version" bitfield - set according to which 1KB blocks have content
  int version = 0;
  for (int i = CUBE_LEN; i >= 0; i--)
    if (0xff != (u8)m_stageImage[i])
      version |= 1<<(i>>10);
  // Tack on -p%d to the filename
  for (int i = 0; i < 16; i++)
    if (version & (0x8000 >> i))
      sprintf(filename+strlen(filename), "-p%d", i);
  strcat(filename, ".safe");
  version = 0xffff ^ version;

  // Advertising version always gets written last (since we write bytes in order)
  m_stageImage[0x3ff6] = version;         // See accessories/boot/hal/advertise.c for details
  m_stageImage[0x3ff7] = version >> 8;

  // Leave space for header before encrypting
  int len = 16;
  len += cubeEncrypt((u8*)m_stageImage, (u8*)m_safeImage + len, !!(options & OPT_NOCRYPTO));
  
  // Now add the header
  m_safeImage[0] = 'C'; m_safeImage[1] = 'u'; m_safeImage[2] = 'b'; m_safeImage[3] = 'e';
  m_safeImage[4] = (len-16);  m_safeImage[5] = (len-16)>>8;
  m_safeImage[6] = version;   m_safeImage[7] = version>>8;
  m_safeImage[8] = start>>8;
  m_safeImage[9] = hwVer;         // HW version 3=EP2, 4=EP3, 6=Pilot

  FILE *fp = fopen(filename, "wb");
  printf("Writing %s...", filename);
  if (NULL == fp || len != fwrite(m_safeImage, 1, len, fp))
    bye("could not create file");
  fclose(fp);

  printf("wrote %d bytes, %d blocks, patch level 0x%04X\n", len, len >> 8, version);
}

// Write the requested image
void writeBin(char* filename, char* image, int len)
{
  FILE *fp = fopen(filename, "wb");
  printf("Writing %s...", filename);
  if (NULL == fp)
    bye("could not create file");
  fwrite(image, 1, len, fp);
  fclose(fp);
  printf("wrote %d bytes, based at address 0x%08x\n", len, TARGET_IMAGE_BASE);
}

// Diff pre and post buffers
void diff(char* pre, char* post, int len)
{
  int i, errors = 0, firsterr = 0;
  printf("Comparing presafed to opensafed image...");

  for (i = 0; i < len; i++)
    if (pre[i] != post[i])
    {
      errors++;
      if (!firsterr)
        firsterr = i;
    }

  if (errors)
    printf("FAILED\n  %d errors found starting at offset 0x%x\n", errors, firsterr);
  else
    printf("OK\n");
}

// Mark block as in-use for the given address
static inline void markBlock(int addr)
{
  if (addr >= TARGET_MAX_BYTE)
    bye("attempted to write past end of image");
  m_blockMap[addr / SAFE_PAYLOAD_SIZE] = 1;
}

// Load a file into a specified memory region (dest[start]-dest[end])
// Returns the actual end address
int load(char* filename, char* dest, int start, int end)
{
  FILE* fp;
  int i, len;

  // Read into memory buffer, do not exceed limits
  printf("Reading %s...", filename);
  fp = fopen(filename, "rb");
  if (NULL == fp)
    bye("not found!");
  len = fread(dest+start, 1, end-start, fp);
  if (len >= (end-start))
    bye("file exceeds max length, adjust crypto.h or reduce file size");
  end = start + len;
  fclose(fp);

  // If we're writing a stage image, mark all loaded blocks in use
  if (dest == m_stageImage)
  {
	  printf("%d bytes read\n", len);
    for (i = start; i < end; i++)
      markBlock(i);
  } else {
	  printf("%d bytes read\n", len);
  }

  return end;
}

// Decode a safe image
void opensafe(char* srcAddress, char* destAddress)
{
	int retval;
	while (0 ==	(retval = opensafeDecryptBlock((safe_header*)srcAddress)))
	{
    memcpy(destAddress, srcAddress + sizeof(safe_header), SAFE_PAYLOAD_SIZE);
		srcAddress += SAFE_BLOCK_SIZE;
		destAddress += SAFE_PAYLOAD_SIZE;
	}
	if (retval != 1)	// End of frame
		printf("Decrypt failed HMAC with result %08x\n", retval);
}

void otaWrite(char *filename, int len)
{
	// Add .ota to the filename
	char otaname[256];
  if (NULL == strchr(filename, '.'))
    bye("Need a .safe extension in the dest filename");
	strncpy(otaname, filename, 256);
	strcpy(strchr(otaname, '.'), ".ota");

  // Write the binary
	writeBin(otaname, m_otaImage, len);
}

// Parse arguments, init structures, perform tasks
int main(int argc, char **argv)
{
  // Option parsing:  Options are stripped, flags are set, leaving only files in argv/argc
  char **argin = &argv[1], **argout = &argv[1];
  int options = 0, filelen = 0, otalen = 0;
  while (*argin)
  {
    if (argin[0][0] != '-')
    {
      *argout++ = *argin;       // Copy non-options
    } else {
      argc--;                   // Consume options
      if (0 == strcmp("-prod", *argin))
        options |= OPT_PKG;
      else if (0 == strcmp("-ota", *argin))
        options |= OPT_PKG | OPT_OTA;
      else if (0 == strcmp("-opensafe", *argin))
        options |= OPT_UNSAFE;
	  else if (0 == strcmp("-cube", *argin))
        options |= OPT_PKG | OPT_CUBE;
      else if (0 == strcmp("-nocrypto", *argin))
        options |= OPT_NOCRYPTO;
      else if (0 == strcmp("-scramble", *argin))
        options |= OPT_SCRAMBLE;    
      else if (0 == strcmp("-guid", *argin) && argin[1]) {
        options |= OPT_GUID;
        argc--;   // Read parameter
        argin++;
        sscanf(*argin, "%x", &m_guid);
  	  } else
        usage("Unknown option %s", *argin);
    }
    argin++;
  }
  // Enforce rules for options
  if ((options & OPT_PKG) && (options & OPT_UNSAFE))
    usage("-prod/-ota/-cube/-opensafe are mutually exclusive", NULL);
  else if (!(options & OPT_PKG) && !(options & OPT_UNSAFE))
    usage("No -options specified", NULL);

  // Initialize structures
  memset(m_stageImage, (options & OPT_CUBE) ? 0xff : 0, sizeof(m_stageImage));
  memset(m_verifyImage, (options & OPT_CUBE) ? 0xff : 0, sizeof(m_verifyImage));
  memset(m_otaImage, 0, sizeof(m_otaImage));
  memset(m_safeImage, 0, sizeof(m_safeImage));
  memset(m_blockMap, 0, sizeof(m_blockMap));

  // Perform packaging
  char destname[256];
  memcpy(destname, argv[1], sizeof(destname));
  if (options & OPT_PKG)
  {
    if (argc != 3)
      usage("-prod/-cube/-ota need 2 arguments", NULL);
    filelen = load(argv[2], m_stageImage, 0, TARGET_IMAGE_SIZE);

	  if (!(options & OPT_GUID) && !(options & OPT_CUBE))
	    m_guid = (getGUID() << 16) + getVersion(argv[1]);   // Generate guid

    if (options & OPT_CUBE)
      cubeWrite(destname, options);
    else
	    safeWrite(argv[1], options);     // Write out an encrypted memory image based on m_stageImage

    // Add OTA if requested
    if (options & OPT_OTA) {
	    otalen = TestCrunch(m_otaImage, (unsigned short*)m_stageImage, filelen/2, m_guid);
	    otaWrite(argv[1], otalen);
    }
  }

  // Check args for opensafeing
  if ((options & OPT_UNSAFE) && argc != 3)
    usage("-opensafe needs 2 arguments", NULL);

  // Always attempt to decode the file (even if just to verify the optPackage)
  load(destname, m_safeImage, 0, TARGET_IMAGE_SIZE*2);
  printf("Checking the image...\n");
  if (options & OPT_CUBE)
    openCube((u8*)m_safeImage+16, (u8*)m_verifyImage);  // Skip header
  else
    opensafe(m_safeImage, m_verifyImage);

  // If we're decoding, write out the verify image for inspection, else just compare it
  if (options & OPT_UNSAFE) {
    // Wipe out the version info - this will be replaced when we resign it
    if (!(options & OPT_CUBE))
      m_verifyImage[0x18] = m_verifyImage[0x19] = m_verifyImage[0x1a] = m_verifyImage[0x1b] = 0;
    writeBin(argv[2], m_verifyImage, TARGET_IMAGE_SIZE);
  } else
    diff(m_stageImage, m_verifyImage, TARGET_IMAGE_SIZE);

  return 0;
}
