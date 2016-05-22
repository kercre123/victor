/**
 * The K02 is a slow CPU with little RAM, an especially bad I-cache, and terrible branch performance
 * So here's a JPEG encoder hacked and unrolled to fit on a K02 - sorry about the gotos
 * 
 * To save RAM and CPU, this encoder is hardwired to 640x(anything), Q50, monochrome
 * The bitstream is JPEG compliant if you add a JFIF header and pad 0xFF to 0xFF 0x00
 * 
 * JPEGGetRowBuf() to get the row buffer address - DMA format is 640 pixels, 2560 bytes: YxUxYxVx
 * JPEGCompress(true) to compress one line of video found in the DMA row buffer
 *  Note:  Flush buffers by calling JPEGCompress(false) after end of frame - EOF is at 0 bytes returned
 */
#include <string.h>
#include "anki/cozmo/robot/hal.h"
#include "anki/common/robot/config.h"
#include "hal/portable.h"
#include "hal/spi.h"
#include "MK02F12810.h"
#include "uart.h"

#include "anki/cozmo/robot/drop.h"
extern DropToWiFi* spi_write_buff;  // To save RAM, we write directly into spi_write_buff

//#define PROFILE   // If you want to UART-print how many cycles you're using
    
namespace Anki
{
  namespace Cozmo
  {
    namespace HAL
    {

const int DROP_LIMIT = DROP_TO_WIFI_MAX_PAYLOAD;    // Max size of JPEG part of drop - used for rate-limiting
      
const int BLOCKW = 8, BLOCKH = 8; // A JPEG block is 8x8 pixels
const int WIDTH = 640;            // Frame width is 640x
const int UVSUB = BLOCKW;         // Subsample U/V by 8x in both dimensions
const int DCT_SIZE = 76;

const int BLOCKS_LINE = WIDTH / BLOCKW / BLOCKH;    // Number of blocks we can compress per line
#define BLOCK_SKIP 5     // Set to 5 for QVGA, 0 for VGA
const int SLICEW = (BLOCKS_LINE-BLOCK_SKIP)*BLOCKW;  // A slice is 1/8th of a line (since slices are 8 lines high)

// This is for JPEG state that must be laid out in a specific order due to DMA/CPU limitations
// There is no need to add to this structure unless you're doing DMA stuff - just use member variables
CAMRAM struct JPEGState {
// This section must be 768 bytes - see .sct file if you need to change that
  uint32_t bitBuf;     // Leftover bits from last encoder run
  int8_t   bitCnt;     // Number of bits still free in bitBuf
  uint8_t  lastDC;     // Last DC value from last encoder run
  int16_t  reserved;
  
  int8_t   dct[DCT_SIZE * BLOCKS_LINE];  // Holds DCT results for safe access in 40% time
  
// This section must start at the first byte of the DMA bank in RAM and total less than 8192 bytes
  
  // Row is actually WIDTH*4 bytes, but it overlaps coeff[] since the two are never used at once
  uint8_t  row[WIDTH];              // YxUx YxVx DMA format - 4 bytes/pixel
  uint8_t  coeff[3*WIDTH];          // Column DCT output - used only during 60% time mid-DCT
  
  uint8_t  slice[WIDTH*BLOCKH];    // 2D rolling buffer holding 8 lines of Y (grayscale) data
  uint16_t uvsum[2*WIDTH/UVSUB];   // Buffer accumulating intermediate UV data while subsamping
  uint8_t  uvdata[2*WIDTH/UVSUB];  // Single buffer holding 1 line of subsampled UV (color) data
} state;

#ifdef PROFILE
static int timeDMA, timeCol, timeRow, timeEmit;
#define START(x)  do { x += SysTick->VAL; } while(0)
#define STOP(x)   do { x -= SysTick->VAL; } while(0)
#else
#define START(x)
#define STOP(x)
#endif

// Because optimized 2D DCTs are huge I-cache busting things, use a 1D DCT on columns, then rows
// Here's a fast SIMD-friendly patent-free 1D DCT:  The Bink 2.2 integer DCT (aka variant B2)
// This is a scaled DCT, so the quantizers in rowB2() also descale the results to JPEG standard
#define BINK() \
  int a6=d2-d5, a0=d0+d7, a2=d2+d5, a4=d0-d7;           \
  int a1=d1+d6, a3=d3+d4, a5=d1-d6, a7=d3-d4;           \
  int b0=a0+a3, b1=a1+a2, b2=a0-a3, b3=a1-a2;           \
  d0=b0+b1; d4=b0-b1; d2=b2+(b2>>2)+(b3>>1); d6=(b2>>1)-b3-(b3>>2); \
  int b4=(a7>>2)+a4+(a4>>2)-(a4>>4), b7=(a4>>2)-a7-(a7>>2)+(a7>>4); \
  int b5=a5+a6-(a6>>2)-(a6>>4), b6=a6-a5+(a5>>2)+(a5>>4);           \
  int c4=b4+b5, c5=b4-b5, c6=b6+b7, c7=b6-b7;           \
  d1=c4; d5=c5+c7; d3=c5-c7; d7=c6;

// This performs column DCTs on all pixel data in the slice buffer and writes the cofficients to state.coeff
static void __attribute__((noinline)) simcolB2x(uint8_t* image)
{
  const int pitch = SLICEW;
  // We do columns 0-3, then 4-7 to stay behind DMA - which is still crunching Y data out of state.coeff
  for (int h = 0; h < 2; h++)             // First or second half
    for (int b = BLOCK_SKIP; b < BLOCKS_LINE; b++) // Block number
      for (int c = 0; c < 4; c++)         // Column
      {
        uint8_t* p = image + b*BLOCKW + h*4 + c;
        int d0 = *p;
        int d1 = *(p += pitch);
        int d2 = *(p += pitch);
        int d3 = *(p += pitch);
        int d4 = *(p += pitch);
        int d5 = *(p += pitch);
        int d6 = *(p += pitch);
        int d7 = *(p += pitch);

        BINK();
        
        // Store 4 * 8 halfwords per 76 bytes
        int16_t* co = (int16_t*)(state.coeff + c*2 + (h*BLOCKS_LINE+b)*DCT_SIZE);
        *co = d0;
        *(co += 4) = d1;  // Actually 8 bytes apart - and we'll store all 8 eventually
        *(co += 4) = d2;
        *(co += 4) = d3;
        *(co += 4) = d4;
        *(co += 4) = d5;
        *(co += 4) = d6;
        *(co += 4) = d7;
      }
}
static void __attribute__((noinline)) simcolB2y(uint8_t* image)
{
  const int pitch = SLICEW*BLOCKH;
  // We do columns 0-3, then 4-7 to stay behind DMA - which is still crunching Y data out of state.coeff
  for (int h = 0; h < 2; h++)             // First or second half
    for (int b = BLOCK_SKIP; b < BLOCKS_LINE; b++) // Block number
      for (int c = 0; c < 4; c++)         // Column
      {
        uint8_t* p = image + b*BLOCKW + h*4 + c;
        int d0 = *p;
        int d1 = *(p += pitch);
        int d2 = *(p += pitch);
        int d3 = *(p += pitch);
        int d4 = *(p += pitch);
        int d5 = *(p += pitch);
        int d6 = *(p += pitch);
        int d7 = *(p += pitch);

        BINK();
        
        // Store 4 * 8 halfwords per 76 bytes
        int16_t* co = (int16_t*)(state.coeff + c*2 + (h*BLOCKS_LINE+b)*DCT_SIZE);
        *co = d0;
        *(co += 4) = d1;  // Actually 8 bytes apart - and we'll store all 8 eventually
        *(co += 4) = d2;
        *(co += 4) = d3;
        *(co += 4) = d4;
        *(co += 4) = d5;
        *(co += 4) = d6;
        *(co += 4) = d7;
      }
}
// This performs all the row DCTs on state.coeff and writes Zig-Zag Quantized results into state.dct
// To save costly table look-ups, Zig-Zag index and Q50 Quantizers (ZZQ) are inlined
// To prevent I-cache thrashing, the same ZZQs are used 10 times in a row (strip-mine order)
static void unrowB2()
{
  // These macros load the coefficients and run the outer loop for each row
  #define ROW(r) \
    dct = state.dct + BLOCK_SKIP*DCT_SIZE; \
    do { \
    int d0=CTa(0,r),d1=CTa(1,r),d2=CTa(2,r),d3=CTa(3,r), d4=CTb(0,r),d5=CTb(1,r),d6=CTb(2,r),d7=CTb(3,r);      \
    BINK();
//#define ENDROW } while ((dct += DCT_SIZE) < state.dct + BLOCKS_LINE*DCT_SIZE)// Sane version
  #define ENDROW } while ((intptr_t)(dct += DCT_SIZE) & 1024)                  // "Clever" version
    
  // Coefficients are loaded relative to dct to save a register and a lot of spilling
  #define CTa(x,r) *(int16_t*)(state.coeff + (dct-state.dct) + r*8 + x*2)
  #define CTb(x,r) *(int16_t*)(state.coeff + (dct-state.dct) + r*8 + x*2 + BLOCKS_LINE*DCT_SIZE)
  // Quantize a value and store it in the correct zig-zag location
  #define ZZQ(i,v,q) dct[i] = __uhsax(1, v*q);
  #define ZHQ(i,v,q) dct[i] = __uhsax(1, v*(q >> 16));
  #define ZLQ(i,v,q) dct[i] = __uhsax(1, v*(q & 65535));
  
  // JPEG uses signed pixels, we use unsigned - this removes the offset
  #define DC_START 64     // 128 (our offset) * 64 pixels * sqrt(8) (DCT scale) / 16 (Q50 quant)

  s8* dct;
  
  // Unrolled loops with JPEG-standard zig-zag and Q50 quantizers (16.16 reciprocal Q50 * row/col Bink scalers)
  ROW(0) ZZQ( 0,d0,0x400) ZZQ( 1,d1,0x6c8) ZZQ( 5,d2,0x6b9) ZZQ( 6,d3,0x34c) ZZQ(14,d4,0x2ab) ZZQ(15,d5,0x152) ZZQ(27,d6,0x151) ZZQ(28,d7,0x139) ENDROW;
  ROW(1) ZZQ( 2,d0,0x637) ZZQ( 4,d1,0x73f) ZZQ( 7,d2,0x599) ZZQ(13,d3,0x33c) ZZQ(16,d4,0x2de) ZZQ(26,d5,0x10f) ZZQ(29,d6,0x14e) ZZQ(42,d7,0x195) ENDROW;
  ROW(2) ZZQ( 3,d0,0x4cd) ZZQ( 8,d1,0x607) ZZQ(12,d2,0x46a) ZZQ(17,d3,0x24f) ZZQ(25,d4,0x1ae) ZZQ(30,d5,0x0f9) ZZQ(41,d6,0x106) ZZQ(43,d7,0x166) ENDROW;
  ROW(3) ZZQ( 9,d0,0x3c4) ZZQ(11,d1,0x39e) ZZQ(18,d2,0x285) ZZQ(24,d3,0x180) ZZQ(31,d4,0x109) ZZQ(40,d5,0x080) ZZQ(44,d6,0x0b1) ZZQ(53,d7,0x0fe) ENDROW;
  ROW(4) ZZQ(10,d0,0x38e) ZZQ(19,d1,0x364) ZZQ(23,d2,0x1d1) ZZQ(32,d3,0x0f1) ZZQ(39,d4,0x0f1) ZZQ(45,d5,0x07c) ZZQ(52,d6,0x0a7) ZZQ(54,d7,0x0f8) ENDROW;
  ROW(5) ZZQ(20,d0,0x233) ZZQ(22,d1,0x1c2) ZZQ(33,d2,0x102) ZZQ(38,d3,0x0ae) ZZQ(46,d4,0x0a7) ZZQ(51,d5,0x06b) ZZQ(55,d6,0x07e) ZZQ(60,d7,0x0ab) ENDROW;
  ROW(6) ZZQ(21,d0,0x15f) ZZQ(34,d1,0x139) ZZQ(37,d2,0x0e8) ZZQ(47,d3,0x0a3) ZZQ(50,d4,0x0a7) ZZQ(56,d5,0x075) ZZQ(59,d6,0x097) ZZQ(61,d7,0x0c7) ENDROW;
  ROW(7) ZZQ(35,d0,0x109) ZZQ(36,d1,0x0f2) ZZQ(48,d2,0x0d3) ZZQ(49,d3,0x0a1) ZZQ(57,d4,0x0aa) ZZQ(58,d5,0x09d) ZZQ(62,d6,0x0c3) ENDROW;
}
  
// This uses DMA to de-interleave Y data to the left side of the buffer
// This makes it much faster for the CPU to work with and frees space for coefficient storage
static void DMACrunch()
{
  // TODO: DMA not working yet
  // Note: Compiler already optimizes this 
  uint8_t *src = state.row, *dst = (state.row) - 8;
  for (int i = 0; i < SLICEW*BLOCKH; i++)
    *src++ = *(dst+=8);
}

const int EOB_SIZE = 4;   // EOB is 0xA, 4-bits long
// Write a JPEG EOF - first flush bitBuf, then write EOB, then write EOI
static inline uint8_t* writeEOF(uint8_t* out)
{
  int bitsFree = state.bitCnt;            // Always 0-31 - bitBuf is never empty
  uint32_t bits = state.bitBuf << bitsFree;
  bits |= (0xAFFFFFFFU) >> (32-bitsFree); // Place EOB at end of bits
  *(uint32_t*)out = __REV(bits);               // Flush what we had
  if (bitsFree < EOB_SIZE)      // If EOB was cropped, add the rest of it
    out[4] = (0xAFF) >> (EOB_SIZE-bitsFree);
  out += (7 + EOB_SIZE + 32-bitsFree) >> 3;   // Round pointer up (+7) to nearest byte including EOB (+4)
  return out;
}

// Shortened Y Huffman Tables, consisting of 1 DC table and 16 AC tables, each 8 entries apart
// Each entry contains the bit count in the lowest byte, and an LSB-justified huffman pattern in bits 8-31 -minus- the AC/DC value
const uint32_t YHT[17*8] = {
// DC table:  Columns go 255 127 63 31 15 7 3 2 1 0
0x02be0012,0x00af0010,0x002b800e,0x000ac00c,0x0005500b,0x0002a00a,0x00014c09,0x0000a408,
0x00002806,
// AC tables: Row 0 is 0 zeros, row 15 is 15 zeros, columns go (unused) 127 63 31 15 7 3 2 1
           0x007c000f,0x001e000d,0x0003400a,0x0000b008,0x00002006,0x00000404,0x00000003,
0x00000000,0x7fc28017,0x3fe10016,0x00fec010,0x001f600d,0x0003c80a,0x00006c07,0x00001805,
0x00000000,0x7fc58017,0x3fe28016,0x1ff12015,0x00ff4010,0x001fb80d,0x0003e40a,0x00003806,
0x00000000,0x7fc90017,0x3fe44016,0x1ff20015,0x0ff8f014,0x007fa80f,0x0007dc0b,0x00007407,
0x00000000,0x7fcd0017,0x3fe64016,0x1ff30015,0x0ff97014,0x07fcb013,0x000fe00c,0x00007607,
0x00000000,0x7fd10017,0x3fe84016,0x1ff40015,0x0ff9f014,0x07fcf013,0x001fdc0d,0x0000f408,
0x00000000,0x7fd50017,0x3fea4016,0x1ff50015,0x0ffa7014,0x07fd3013,0x003fd80e,0x0000f608,
0x00000000,0x7fd90017,0x3fec4016,0x1ff60015,0x0ffaf014,0x07fd7013,0x003fdc0e,0x0001f409,
0x00000000,0x7fdd0017,0x3fee4016,0x1ff70015,0x0ffb7014,0x07fdb013,0x01ff0011,0x0003f00a,
0x00000000,0x7fe18017,0x3ff08016,0x1ff82015,0x0ffc0014,0x07fdf813,0x03fef812,0x0003f20a,
0x00000000,0x7fe60017,0x3ff2c016,0x1ff94015,0x0ffc9014,0x07fe4013,0x03ff1c12,0x0003f40a,
0x00000000,0x7fea8017,0x3ff50016,0x1ffa6015,0x0ffd2014,0x07fe8813,0x03ff4012,0x0007f20b,
0x00000000,0x7fef0017,0x3ff74016,0x1ffb8015,0x0ffdb014,0x07fed013,0x03ff6412,0x0007f40b,
0x00000000,0x7ff38017,0x3ff98016,0x1ffca015,0x0ffe4014,0x07ff1813,0x03ff8812,0x000ff00c,
0x00000000,0x7ff88017,0x3ffc0016,0x1ffde015,0x0ffee014,0x07ff6813,0x03ffb012,0x01ffd611,
0x00000000,0x7ffd8017,0x3ffe8016,0x1fff2015,0x0fff8014,0x07ffb813,0x03ffd812,0x01ffea11,
};

// The 'emit' stage runs in 40% time, and uses up a variable amount of CPU
// The input is our (still-full-size) frequency-domain image, but quantized to zeros and small integers
// To compress it, we skip runs of zeros as fast as we can, then shrink small integers with Huffman coding
// To save time and eliminate a special case with EOBs, I skip the least-important 63rd AC coefficient
static uint8_t* emit(__packed uint8_t* outp)
{
  // This is a C version of the assembly routine to aid testing/development
  // gotos and variable (register) reuse are meant to duplicate the structure of that routine
  int val, len, tmp;  // Three temporary registers with helpful(ish) names
  
  intptr_t hufftable;  // Pointer to current huffman table - 1 DC + 16 AC table
  intptr_t qp;         // Pointer to next coefficient

  int bitBuf, bitCnt; // Current bit buffer and number of bits still "free" in buffer
  
  uint32_t* out = (uint32_t*)outp;
  //uint32_t* outchop = out + DROP_LIMIT/4;  // End of first buffer
  uint32_t* outchop = out + (DROP_LIMIT/4 - ((BLOCKS_LINE-BLOCK_SKIP)-1)*2);  // Point where rate limiting must happen
  
  int8_t* dct = state.dct + BLOCK_SKIP*DCT_SIZE;
  int lastDC;     // DC delta (saves ten bytes of RAM to do this here, and we have regs)
  
  // Store any tricky constants in registers, since loading them is too slow
  intptr_t actable = ((intptr_t)YHT) + 4*(8-24);     // +8*4 to skip DC table, -24*4 since we encode 8 (of 32) CLZ bits
  const uint32_t allones = ~0;            // Dynamic shifts on ARM must use registers
  
  // Load bit encoder state
  bitBuf = state.bitBuf;
  bitCnt = state.bitCnt;
  lastDC = state.lastDC;

  // Start coding the first DC term
  goto startdc;

// Skip any zeros in val and adjust the AC hufftable to encode the number of zeros skipped
countzeros:
  tmp = __REV(val);
  len = __CLZ(tmp);        // Find out which byte is non-zero (LSB-indexed)
  qp += (len >> 3);        // Point at the non-zero byte
  val = *(int8_t*)qp++;        // Re-read the byte (faster than shifting)

  hufftable += qp << 5;    // Compute zero-count, *32 to index into correct AC table
  //printf("skip %d ", (hufftable - actable) >> 5);
  
// Huffman-compress and write the value currently in 'val'
writeval:
  //printf("%04d\n", val);
  // Compute the value's bit-length manually to save table space (just 1 code per bit-length)
  // Future optimization:  Can save ~40 cycles/block using 832 byte complete AC table 0
  len = __CLZ(val);
  if ((tmp = -val) > 0) {    
    len = __CLZ(tmp);     // If val is negative, re-do CLZ on positive version
    val = allones >> len; // Make ones-complement mask the same length as val
    val ^= tmp;           // JPEG standard uses ones-complement of positive version
  }

  // Look up huffman code and length, then merge them into val
  len = ((int*)hufftable)[len]; 
  val += (len >> 8);
  len &= 255;
  
  // Make room in bitbuffer for the huffman code - or head to flushbits if out of space
  if ((bitCnt -= len) < 0)
    goto flushbits;   // 16% taken
  
writebits:
  // Shift the huffman code into the bitbuffer
  bitBuf <<= len;
  bitBuf += val;

  // If the next AC is non-zero, shortcut past the zero counting and just write the value
  hufftable = actable; // Point to first AC table (zerocount = 0)
  if (0 != (val = *(int8_t*)qp++))
    goto writeval;    // 75% taken - major savings on detailed blocks where we need it most
    
  // Else, we found a zero, so remember first zero location and keep scanning for a non-zero
  hufftable -= (qp << 5);  // *32 since AC tables are 32 bytes apart
  
  // Take advantage of M4 unaligned loads (adds 0-6 cycles) to fast-scan for non-zeros
  if (0 != (val = *(uint32_t*)qp))
    goto countzeros;
  if (0 != (val = *(uint32_t*)(qp += 4))) 
    goto countzeros;
  if (0 != (val = *(uint32_t*)(qp += 4))) 
    goto countzeros;
  
  // After 13 zeros, stop scanning - saves time, eliminates ZERO16 special-case, causes no degradation
  // This is also how we terminate the DCT scan - with 13 zeros as a sentinel between blocks

chopblock:
  // Multi-DC loop goes here
  dct += DCT_SIZE;
  
  // XXX: This weird register-saving hack needs to be even weirder on ARM..
  if ((intptr_t)dct >= (intptr_t)state.row)
    goto stop;

  outchop += 2;   // Give a bit more room for the next block
  
// Load the DC value and encode it
startdc:
  qp = (intptr_t)dct;
  hufftable = actable - 32; // DC table is just before the AC tables (which are 32 bytes apart)
  tmp = *(int8_t*)qp++;
  val = tmp - lastDC;       // DC values are delta coded
  lastDC = tmp;
  goto writeval;

// Write out 32-bits - +14 cycles 18% of the time, or 2.5 cycles/code overhead
flushbits:
  tmp = bitCnt + len;
  bitBuf <<= tmp;               // Left-align to last free bit

  tmp = -bitCnt;
  tmp = val >> tmp;
  bitBuf |= tmp;                // Fill in first half of val

  tmp = __REV(bitBuf);
  *out++ = tmp;                 // Flush in byte-reversed order (JPEG spec)
  
  // Clever trick warning:  bitCnt ensures second half of val is (later) cropped
  bitCnt += 32;                 // Make room for more bits (minus rest of val)    
  
  // Check that we haven't run out of space
  if (out < outchop)
    goto writebits;
/*  
  // If we ran out buffer 1, fill buffer 2 first
  if ((intptr_t)out < (intptr_t)state.dct) {
    out = (uint32_t*)state.dct;
    outchop = out + (DROP_LIMIT/4 - ((BLOCKS_LINE-BLOCK_SKIP)-1)*2);  // Point where rate limiting must happen
    goto writebits;
  }
*/
  // We're really out of space - save the code, then end the block
  bitBuf <<= len;
  bitBuf += val;
  
  goto chopblock;
  
stop:
  // Store bit buffer state
  state.bitBuf = bitBuf;
  state.bitCnt = bitCnt;
  state.lastDC = lastDC;

  return (uint8_t*)out;
}

// Read DMA lines from dmabuff_ and write JPEG-compressed data to the current drop (spi_write_buff)
// 'line' should be the line number of the current frame and 'height' is the total number of lines
// To allow time for EOF, there must be a vblank of 1 or more lines (where line >= height)
// Note there is an 8-line latency, because JPEG works in 8x8 blocks
void JPEGCompress(int line, int height)
{
  static uint32_t frameNumber;
  // Current state of the encoder for this frame
  static uint16_t readline = 0;       // Pointer to next line to compress (lags by 8 lines)
  static uint16_t xpitch = SLICEW;    // To put lines in and pull blocks out, slice pitch changes every 8 lines
  
  int ypitch = xpitch ^ (SLICEW^(SLICEW*BLOCKH));   // ypitch is always orthogonal to xpitch
  uint8_t *out = spi_write_buff->payload;           // Pointer to destination

  // QVGA subsampling code - TODO: Make switchable
  int subsample = !(line & 1);
  height >>= 1;
  line >>= 1;
  
  // Do we have anything ready to compress sitting in the pipeline?
  int docompress = readline < height && (readline > line || line >= BLOCKH);
  int eof = 0, buflen = 0;
  
  if (line < 8) frameNumber = CameraGetFrameNumber();
  
  if (subsample)
  {
    // Start DMA-moving Y data out of the way - needs 320 cycles before coeff[0] is free, 1280 total
    START(timeDMA);
    DMACrunch();
    STOP(timeDMA);

    // TODO COLOR: In camera.cpp ISR start color DMACrunch, 160+160 U+V bytes to 640 dct bytes: 0u0v
    // First write color into output, then sadd16 -> uvsum, or packed uvdata (on rows 6 and 7) using 320 cycles
      
    // If we have a slice left in the buffer, start column DCT
    START(timeCol);
    if (docompress)
      if (ypitch == SLICEW)
        simcolB2x(state.slice + (-BLOCK_SKIP*BLOCKW) + xpitch*(readline&(BLOCKH-1)));
      else
        simcolB2y(state.slice + (-BLOCK_SKIP*BLOCKW) + xpitch*(readline&(BLOCKH-1)));
    else
      ; // TODO: No encode to slow us down, wait on crunch DMA to complete before row copy
    STOP(timeCol);

    // DMA copy the latest line into the slice buffer (even if it's not ready)
    // TODO:  Can we chain these or do they go between row DCTs?  Either way is fast enough..
    START(timeDMA);
    uint8_t* slicep = state.slice + xpitch*(line&(BLOCKH-1));
    if (line < height)
      for (int i = 0; i < SLICEW*BLOCKH; i += SLICEW)
      {
        uint32_t *src = (uint32_t*)slicep, *dst = (uint32_t*)(state.row + i);
        //for (int i = 0; i < SLICEW/4; i++)    // XXX - Keil turns it into memmove which is too slow!
        src[0] = dst[0]; src[1] = dst[1]; src[2] = dst[2]; src[3] = dst[3]; src[4] = dst[4];
        src[5] = dst[5]; src[6] = dst[6]; src[7] = dst[7]; src[8] = dst[8]; src[9] = dst[9];
        slicep += ypitch;
      }
    STOP(timeDMA);

    // QVGA:  We only have data on every other line
    SPI::FinalizeDrop(0, 0, 0);
  } else {
    // Continue with row DCT, then emit bits
    if (docompress)
    {
      START(timeRow);
      unrowB2();
      STOP(timeRow);
      START(timeEmit);
      buflen = emit(out) - out;
      if (buflen > DROP_LIMIT)
        buflen = DROP_LIMIT;
      STOP(timeEmit);
      readline++;         // Point at next line
      
    } else {
      // One line past the end of the image, send the EOF
      if (readline == height) {
        buflen = writeEOF(out) - out;
        eof = 1;
        readline = 0;
      #ifdef PROFILE
        UART::DebugPrintProfile(timeDMA);
        UART::DebugPrintProfile(timeCol);
        UART::DebugPrintProfile(timeRow);
        UART::DebugPrintProfile(timeEmit);
        UART::DebugPutc('\n');
        timeDMA = timeCol = timeRow = timeEmit = 0;
      #endif
      }
      
      // Since nothing to compress, reset encoder state for next time 
      state.bitCnt = 32-EOB_SIZE; // Just enough to cut off the first EOB
      state.lastDC = DC_START;
    }
        
    // On the last line of each block, reverse the slice pitch
    if (line < height && (BLOCKH-1) == (line & (BLOCKH-1)))
      xpitch = ypitch;
    
    // Write the data to the drop buffer
    SPI::FinalizeDrop(buflen, eof, frameNumber);
  }
}

    } // HAL
  }   // Cozmo
}     // Anki
