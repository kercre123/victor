/**
 * This is a JPEG encoder meant to run in realtime on the Cortex-M4
 * The header-generating code was found in a public domain JavaScript JPEG encoder
 *
 * Call JPEGStart() to begin a new frame
 * Call JPEGCompress() to process 8 rows of YUYV data
 * Call JPEGEnd() to complete the frame
 */
#ifdef ROBOT_HARDWARE
#include "lib/stm32f4xx.h"
#include "anki/cozmo/robot/hal.h"
#include "hal/portable.h"
#else
#include <stdio.h>
typedef unsigned char u8;
#endif

int JPEGStart(u8* out, int width, int height, int quality);
int JPEGCompress(u8* out, u8* in);
int JPEGEnd(u8* out);

//#define PROFILE
#ifdef PROFILE
static unsigned short timeDCT, timeQuant, timeHuff;
#define START(x)  do { __disable_irq(); x -= TIM6->CNT; } while(0)
#define STOP(x)   do { __enable_irq();  x += TIM6->CNT; } while(0)
#else
#define START(x)
#define STOP(x)
#endif

//#define DEBUG(...) printf (__VA_ARGS__)
#define DEBUG(...)

// Built to handle YUYV inputs
const int PIXWIDTH = 2;

// JPEG zig-zag mapping, indexed by DCT position, returning bitstream position
static const u8 ZIG_ZAG[] =
{  0, 1, 5, 6,14,15,27,28,
	 2, 4, 7,13,16,26,29,42,
	 3, 8,12,17,25,30,41,43,
	 9,11,18,24,31,40,44,53,
	10,19,23,32,39,45,52,54,
	20,22,33,38,46,51,55,60,
	21,34,37,47,50,56,59,61,
	35,36,48,49,57,58,62,63 };
  
// Standard JPEG Y huffman tables for DC/AC
static const unsigned short HTDC[256][2] = { {0,2},{2,3},{3,3},{4,3},{5,3},{6,3},{14,4},{30,5},{62,6},{126,7},{254,8},{510,9}};
static const unsigned short HTAC[256][2] = { 
  {10,4},{0,2},{1,2},{4,3},{11,4},{26,5},{120,7},{248,8},{1014,10},{65410,16},{65411,16},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},
  {12,4},{27,5},{121,7},{502,9},{2038,11},{65412,16},{65413,16},{65414,16},{65415,16},{65416,16},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},
  {28,5},{249,8},{1015,10},{4084,12},{65417,16},{65418,16},{65419,16},{65420,16},{65421,16},{65422,16},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},
  {58,6},{503,9},{4085,12},{65423,16},{65424,16},{65425,16},{65426,16},{65427,16},{65428,16},{65429,16},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},
  {59,6},{1016,10},{65430,16},{65431,16},{65432,16},{65433,16},{65434,16},{65435,16},{65436,16},{65437,16},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},
  {122,7},{2039,11},{65438,16},{65439,16},{65440,16},{65441,16},{65442,16},{65443,16},{65444,16},{65445,16},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},
  {123,7},{4086,12},{65446,16},{65447,16},{65448,16},{65449,16},{65450,16},{65451,16},{65452,16},{65453,16},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},
  {250,8},{4087,12},{65454,16},{65455,16},{65456,16},{65457,16},{65458,16},{65459,16},{65460,16},{65461,16},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},
  {504,9},{32704,15},{65462,16},{65463,16},{65464,16},{65465,16},{65466,16},{65467,16},{65468,16},{65469,16},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},
  {505,9},{65470,16},{65471,16},{65472,16},{65473,16},{65474,16},{65475,16},{65476,16},{65477,16},{65478,16},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},
  {506,9},{65479,16},{65480,16},{65481,16},{65482,16},{65483,16},{65484,16},{65485,16},{65486,16},{65487,16},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},
  {1017,10},{65488,16},{65489,16},{65490,16},{65491,16},{65492,16},{65493,16},{65494,16},{65495,16},{65496,16},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},
  {1018,10},{65497,16},{65498,16},{65499,16},{65500,16},{65501,16},{65502,16},{65503,16},{65504,16},{65505,16},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},
  {2040,11},{65506,16},{65507,16},{65508,16},{65509,16},{65510,16},{65511,16},{65512,16},{65513,16},{65514,16},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},
  {65515,16},{65516,16},{65517,16},{65518,16},{65519,16},{65520,16},{65521,16},{65522,16},{65523,16},{65524,16},{0,0},{0,0},{0,0},{0,0},{0,0},
  {2041,11},{65525,16},{65526,16},{65527,16},{65528,16},{65529,16},{65530,16},{65531,16},{65532,16},{65533,16},{65534,16},{0,0},{0,0},{0,0},{0,0},{0,0}
};

// Standard JPEG UV huffman tables for DC/AC
static const unsigned short UVDC_HT[256][2] = { {0,2},{1,2},{2,2},{6,3},{14,4},{30,5},{62,6},{126,7},{254,8},{510,9},{1022,10},{2046,11}};
static const unsigned short UVAC_HT[256][2] = { 
  {0,2},{1,2},{4,3},{10,4},{24,5},{25,5},{56,6},{120,7},{500,9},{1014,10},{4084,12},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},
  {11,4},{57,6},{246,8},{501,9},{2038,11},{4085,12},{65416,16},{65417,16},{65418,16},{65419,16},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},
  {26,5},{247,8},{1015,10},{4086,12},{32706,15},{65420,16},{65421,16},{65422,16},{65423,16},{65424,16},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},
  {27,5},{248,8},{1016,10},{4087,12},{65425,16},{65426,16},{65427,16},{65428,16},{65429,16},{65430,16},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},
  {58,6},{502,9},{65431,16},{65432,16},{65433,16},{65434,16},{65435,16},{65436,16},{65437,16},{65438,16},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},
  {59,6},{1017,10},{65439,16},{65440,16},{65441,16},{65442,16},{65443,16},{65444,16},{65445,16},{65446,16},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},
  {121,7},{2039,11},{65447,16},{65448,16},{65449,16},{65450,16},{65451,16},{65452,16},{65453,16},{65454,16},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},
  {122,7},{2040,11},{65455,16},{65456,16},{65457,16},{65458,16},{65459,16},{65460,16},{65461,16},{65462,16},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},
  {249,8},{65463,16},{65464,16},{65465,16},{65466,16},{65467,16},{65468,16},{65469,16},{65470,16},{65471,16},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},
  {503,9},{65472,16},{65473,16},{65474,16},{65475,16},{65476,16},{65477,16},{65478,16},{65479,16},{65480,16},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},
  {504,9},{65481,16},{65482,16},{65483,16},{65484,16},{65485,16},{65486,16},{65487,16},{65488,16},{65489,16},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},
  {505,9},{65490,16},{65491,16},{65492,16},{65493,16},{65494,16},{65495,16},{65496,16},{65497,16},{65498,16},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},
  {506,9},{65499,16},{65500,16},{65501,16},{65502,16},{65503,16},{65504,16},{65505,16},{65506,16},{65507,16},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},
  {2041,11},{65508,16},{65509,16},{65510,16},{65511,16},{65512,16},{65513,16},{65514,16},{65515,16},{65516,16},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},
  {16352,14},{65517,16},{65518,16},{65519,16},{65520,16},{65521,16},{65522,16},{65523,16},{65524,16},{65525,16},{0,0},{0,0},{0,0},{0,0},{0,0},
  {1018,10},{32707,15},{65526,16},{65527,16},{65528,16},{65529,16},{65530,16},{65531,16},{65532,16},{65533,16},{65534,16},{0,0},{0,0},{0,0},{0,0},{0,0}
};

// Special codes for "end of macroblock" and "16 zeros"
const unsigned short EOB[2] = { HTAC[0x00][0], HTAC[0x00][1] };
const unsigned short ZERO16[2] = { HTAC[0xF0][0], HTAC[0xF0][1] };

// Since we only process one row at a time, this is the state we must keep row-to-row
static int m_width;   // Width of the image
static int m_lastDC;  // Last DC value
static u8* m_out;     // Pointer to the output byte buffer
static int m_bitBuf, m_bitCnt;      // Current (unflushed) bit buffer and count of bits

static float m_quantY[64], m_quantUV[64];  // Quantization tables, including AAN scalers

// Write bits to the bit buffer - remember to flush excess bits at the end
static void inline writeBits(const unsigned short bits[2]) 
{
  // JPEG encoding is big-endian - so bits are placed starting at MSB and moving toward LSB
	m_bitCnt -= bits[1];
	m_bitBuf |= bits[0] << m_bitCnt;

	//DEBUG("|%x/%d|", bits[0], bits[1]);
	//for (int i = bits[1]-1; i >= 0; i--)
	//	DEBUG("%d", 1 & (bits[0] >> i));

	while (m_bitCnt <= 24)
  {
		u8 c = m_bitBuf >> 24;
		*m_out++ = c;
		if (c == 255)      // JPEG-style byte-stuffing
			*m_out++ = 0;
		m_bitBuf <<= 8;
		m_bitCnt += 8;
	}
}

// Turn a coefficient into a bit count and bit value 
// XXX: This should be a lookup table!
static inline void encodeCoeff(int val, unsigned short bits[2]) 
{
	int tmp1 = val < 0 ? -val : val;
	val = val < 0 ? val-1 : val;
	bits[1] = 1;
	while(tmp1 >>= 1)
		++bits[1];
	bits[0] = val & ((1<<bits[1])-1);
}

// Write a lengthy block to the bit buffer, as in header construction
static void putBlock(const u8* p, int len)
{
  while (len--)
    *m_out++ = *p++;
}
static inline void putByte(u8 c)
{
  *m_out++ = c;
}
  
// This single-precision implementation of the AAN 1-D DCT comes from the IJG source
static inline void fpDCT(
  float i0, float i1, float i2, float i3, float i4, float i5, float i6, float i7,
  float &d0, float &d1, float &d2, float &d3, float &d4, float &d5, float &d6, float &d7)
{
	float tmp0 = i0 + i7;
	float tmp7 = i0 - i7;
	float tmp1 = i1 + i6;
	float tmp6 = i1 - i6;
	float tmp2 = i2 + i5;
	float tmp5 = i2 - i5;
	float tmp3 = i3 + i4;
	float tmp4 = i3 - i4;

	// Even part
	float tmp10 = tmp0 + tmp3;	// phase 2
	float tmp13 = tmp0 - tmp3;
	float tmp11 = tmp1 + tmp2;
	float tmp12 = tmp1 - tmp2;

	d0 = tmp10 + tmp11; 		// phase 3
	d4 = tmp10 - tmp11;

	float z1 = (tmp12 + tmp13) * 0.707106781f; // c4
	d2 = tmp13 + z1; 		// phase 5
	d6 = tmp13 - z1;

	// Odd part
	tmp10 = tmp4 + tmp5; 		// phase 2
	tmp11 = tmp5 + tmp6;
	tmp12 = tmp6 + tmp7;

	// The rotator is modified from fig 4-8 to avoid extra negations.
	float z5 = (tmp10 - tmp12) * 0.382683433f; // c6
	float z2 = tmp10 * 0.541196100f + z5; // c2-c6
	float z4 = tmp12 * 1.306562965f + z5; // c2+c6
	float z3 = tmp11 * 0.707106781f; // c4

	float z11 = tmp7 + z3;		// phase 5
	float z13 = tmp7 - z3;

	d5 = z13 + z2;			// phase 6
	d3 = z13 - z2;
	d1 = z11 + z4;
	d7 = z11 - z4;
} 

// Compress a Y macroblock from pixels to bits - this is 99% of the work
static void doYBlock(u8 *image)
{
  float coeff[64];
	int quant[64];
  
  START(timeDCT);
  
	// DCT rows
	for (int i = 0; i < 64; i += 8)
  {
		fpDCT(image[0], image[1*PIXWIDTH], image[2*PIXWIDTH], image[3*PIXWIDTH], image[4*PIXWIDTH], image[5*PIXWIDTH], image[6*PIXWIDTH], image[7*PIXWIDTH],
      coeff[i], coeff[i+1], coeff[i+2], coeff[i+3], coeff[i+4], coeff[i+5], coeff[i+6], coeff[i+7]);
    image += m_width*PIXWIDTH;   // Next row
  }

	// DCT columns
	for (int i = 0; i < 8; i++)
		fpDCT(coeff[i], coeff[i+8], coeff[i+16], coeff[i+24], coeff[i+32], coeff[i+40], coeff[i+48], coeff[i+56],
          coeff[i], coeff[i+8], coeff[i+16], coeff[i+24], coeff[i+32], coeff[i+40], coeff[i+48], coeff[i+56]);
  
  STOP(timeDCT);
  START(timeQuant);

	// Quantize/descale/zigzag the coefficients
	for (int i = 0; i < 64; ++i)
  {
		float v = coeff[i]*m_quantY[i];
    // On M4 FPU conversions round toward zero, which is not as good as round toward nearest
    // XXX: It's possible to make the FPU round toward nearest by messing with flags and using VCVTR instruction..
		quant[ZIG_ZAG[i]] = ((int)(v)+128) >> 8;  // Rounding is based on 256.0x factor in m_quantY
    //quant[ZIG_ZAG[i]] = (int)(v < 0 ? v - 0.5f : v + 0.5f);
	}
  
  STOP(timeQuant);
  START(timeHuff);
  
	// Encode DC
	int diff = quant[0] - m_lastDC; 
	m_lastDC = quant[0];
	if (diff == 0)
  {
		writeBits(HTDC[0]);
	} else {
		unsigned short bits[2];
		encodeCoeff(diff, bits);
		writeBits(HTDC[bits[1]]);
		writeBits(bits);
	}
	DEBUG(" dc=%d ===============================================\n", diff);

	// Encode AC coefficients
	for (int i = 1; i < 64; i++)
  {
    // Count zeros
		int startpos = i;
		while (i < 64 && 0 == quant[i])
      i++;
    
    // If we have zeros to the end, write EOB
    if (i == 64)
    {
      writeBits(EOB);
      STOP(timeHuff);
      return;
    }
    
    // If we have 16 or more zeros, encode them specially
		int zeros = i - startpos;
		while (zeros >= 16)
    {
			writeBits(ZERO16);
      zeros -= 16;
			DEBUG(" zeros=16\n");	
    }
    
    // Find the length of the coefficient
    unsigned short bits[2];
		encodeCoeff(quant[i], bits);
    // Write the huffman prefix for zeros + coefficient length
		writeBits(HTAC[(zeros<<4) | bits[1]]);
		DEBUG(" zeros=%d, aclen=%d | ", zeros, bits[1]);
    // And finally the bare coefficient
		writeBits(bits);
		DEBUG(" ac=%d\n", quant[i]);
	}
  
  // If we have a non-zero at the very end, EOB is not written
  DEBUG(" STUPID NON-EOB\n");
  STOP(timeHuff);
}

// Compress a row of pixels
int JPEGCompress(u8* out, u8* in)
{  
  m_out = out;

  // Encode 8x8 macroblocks for one full row
  for(int x = 0; x < m_width; x += 8)
    doYBlock(in + x*PIXWIDTH);
	
  return m_out-out;
}

// End the JPEG, flushing the last bytes out
int JPEGEnd(u8* out)
{
  m_out = out;
  
  // Do the bit alignment of the EOI marker
  static const unsigned short fillBits[] = {0x7F, 7};
  writeBits(fillBits);

  // EOI
  putByte(0xFF);
  putByte(0xD9);

	return m_out-out;
}

// Start a new JPEG image at the specified width, height, and quality
// This regenerates the header
int JPEGStart(u8* out, int width, int height, int quality)
{  
  // Reset row encoder state to start of frame
  m_out = out;
  m_width = width;
  m_bitBuf = 0;
  m_bitCnt = 32;

  // For header format, see http://www.opennet.ru/docs/formats/jpeg.txt
	static const unsigned char std_dc_luminance_nrcodes[] = {0,0,1,5,1,1,1,1,1,1,0,0,0,0,0,0,0};
	static const unsigned char std_dc_luminance_values[] = {0,1,2,3,4,5,6,7,8,9,10,11};
	static const unsigned char std_ac_luminance_nrcodes[] = {0,0,2,1,3,3,2,4,3,5,5,4,4,0,0,1,0x7d};
	static const unsigned char std_ac_luminance_values[] = {
		0x01,0x02,0x03,0x00,0x04,0x11,0x05,0x12,0x21,0x31,0x41,0x06,0x13,0x51,0x61,0x07,0x22,0x71,0x14,0x32,0x81,0x91,0xa1,0x08,
		0x23,0x42,0xb1,0xc1,0x15,0x52,0xd1,0xf0,0x24,0x33,0x62,0x72,0x82,0x09,0x0a,0x16,0x17,0x18,0x19,0x1a,0x25,0x26,0x27,0x28,
		0x29,0x2a,0x34,0x35,0x36,0x37,0x38,0x39,0x3a,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0x4a,0x53,0x54,0x55,0x56,0x57,0x58,0x59,
		0x5a,0x63,0x64,0x65,0x66,0x67,0x68,0x69,0x6a,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7a,0x83,0x84,0x85,0x86,0x87,0x88,0x89,
		0x8a,0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99,0x9a,0xa2,0xa3,0xa4,0xa5,0xa6,0xa7,0xa8,0xa9,0xaa,0xb2,0xb3,0xb4,0xb5,0xb6,
		0xb7,0xb8,0xb9,0xba,0xc2,0xc3,0xc4,0xc5,0xc6,0xc7,0xc8,0xc9,0xca,0xd2,0xd3,0xd4,0xd5,0xd6,0xd7,0xd8,0xd9,0xda,0xe1,0xe2,
		0xe3,0xe4,0xe5,0xe6,0xe7,0xe8,0xe9,0xea,0xf1,0xf2,0xf3,0xf4,0xf5,0xf6,0xf7,0xf8,0xf9,0xfa
	};
	static const unsigned char std_dc_chrominance_nrcodes[] = {0,0,3,1,1,1,1,1,1,1,1,1,0,0,0,0,0};
	static const unsigned char std_dc_chrominance_values[] = {0,1,2,3,4,5,6,7,8,9,10,11};
	static const unsigned char std_ac_chrominance_nrcodes[] = {0,0,2,1,2,4,4,3,4,7,5,4,4,0,1,2,0x77};
	static const unsigned char std_ac_chrominance_values[] = {
		0x00,0x01,0x02,0x03,0x11,0x04,0x05,0x21,0x31,0x06,0x12,0x41,0x51,0x07,0x61,0x71,0x13,0x22,0x32,0x81,0x08,0x14,0x42,0x91,
		0xa1,0xb1,0xc1,0x09,0x23,0x33,0x52,0xf0,0x15,0x62,0x72,0xd1,0x0a,0x16,0x24,0x34,0xe1,0x25,0xf1,0x17,0x18,0x19,0x1a,0x26,
		0x27,0x28,0x29,0x2a,0x35,0x36,0x37,0x38,0x39,0x3a,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0x4a,0x53,0x54,0x55,0x56,0x57,0x58,
		0x59,0x5a,0x63,0x64,0x65,0x66,0x67,0x68,0x69,0x6a,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7a,0x82,0x83,0x84,0x85,0x86,0x87,
		0x88,0x89,0x8a,0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99,0x9a,0xa2,0xa3,0xa4,0xa5,0xa6,0xa7,0xa8,0xa9,0xaa,0xb2,0xb3,0xb4,
		0xb5,0xb6,0xb7,0xb8,0xb9,0xba,0xc2,0xc3,0xc4,0xc5,0xc6,0xc7,0xc8,0xc9,0xca,0xd2,0xd3,0xd4,0xd5,0xd6,0xd7,0xd8,0xd9,0xda,
		0xe2,0xe3,0xe4,0xe5,0xe6,0xe7,0xe8,0xe9,0xea,0xf2,0xf3,0xf4,0xf5,0xf6,0xf7,0xf8,0xf9,0xfa
	};

  // Quantization tables and AAN scalers
	static const int YQT[] = {16,11,10,16,24,40,51,61,12,12,14,19,26,58,60,55,14,13,16,24,40,57,69,56,14,17,22,29,51,87,80,62,18,22,37,56,68,109,103,77,24,35,55,64,81,104,113,92,49,64,78,87,103,121,120,101,72,92,95,98,112,100,103,99};
	static const int UVQT[] = {17,18,24,47,99,99,99,99,18,21,26,66,99,99,99,99,24,26,56,99,99,99,99,99,47,66,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99};
	static const float AAN_SCALERS[] = { 1.0f * 2.828427125f, 1.387039845f * 2.828427125f, 1.306562965f * 2.828427125f, 1.175875602f * 2.828427125f, 1.0f * 2.828427125f, 0.785694958f * 2.828427125f, 0.541196100f * 2.828427125f, 0.275899379f * 2.828427125f };

  // This is the IJG way of turning quality into quantization
	quality = quality ? quality : 90;
	quality = quality < 1 ? 1 : quality > 100 ? 100 : quality;
	quality = quality < 50 ? 5000 / quality : 200 - quality * 2;

	unsigned char YTable[64], UVTable[64];
	for(int i = 0; i < 64; ++i)
  {
		int yti = (YQT[i]*quality+50)/100;
		int uvti  = (UVQT[i]*quality+50)/100;
		YTable[ZIG_ZAG[i]] = yti < 1 ? 1 : yti > 255 ? 255 : yti;
		UVTable[ZIG_ZAG[i]] = uvti < 1 ? 1 : uvti > 255 ? 255 : uvti;
	}

  // Now scale the quantization tables for AAN
	for(int row = 0, k = 0; row < 8; ++row)
  {
		for(int col = 0; col < 8; ++col, ++k) 
    {
      // XXX: This 256.0 (instead of 1.0) is used by a cheesy integer rouding trick in m_quantY
			m_quantY[k]  = 256.0f / (YTable [ZIG_ZAG[k]] * AAN_SCALERS[row] * AAN_SCALERS[col]);
      DEBUG("%4.0f(%7.5f) ", (double)YTable[k], (double)m_quantY[k]);
			m_quantUV[k] = 256.0f / (UVTable[ZIG_ZAG[k]] * AAN_SCALERS[row] * AAN_SCALERS[col]);
		}
		DEBUG("\n");
  }

  // Write Headers
	static const unsigned char head0[] = { 
    0xFF,0xD8,0xFF,0xE0,0,2+14,  // SOI, APP0, APP0 len (2+14 bytes)
    'J','F','I','F',0,
    1,1,                        // Version 1.1
    0, 0,1, 0,1, 0,0,           // No DPI info, no thumbnail
    0xFF,0xDB,0,2 + 65 * 1,     // DQT (define quantization table) of length 2+(1+64)*(number of QTs)
    0,                          // First QT follows
  };
	putBlock(head0, sizeof(head0));
	putBlock(YTable, sizeof(YTable));   // First QT
	//putByte(1);                         // Second QT follows
	//putBlock(UVTable, sizeof(UVTable)); // Second QT
  
  const int dhtlen = 2 + 
    1 + sizeof(std_dc_luminance_nrcodes)-1 + sizeof(std_dc_luminance_values) +
    1 + sizeof(std_ac_luminance_nrcodes)-1 + sizeof(std_ac_luminance_values);
  const unsigned char head1[] = { 
    0xFF,0xC0,                 // SOF0 (start of frame)
    0, 8 + 3*1,                // Length is 8 + 3 * components
    8, height>>8,height&0xFF, width>>8,width&0xFF,    // 8-bit image, widthxheight
    1,                         // Number of components (1 = gray, 3 = YCbCr)
    1,0x11,0,                  //   Y -  1x1 sampling, DCT 0
    //2,0x11,1,                  //   Cb - 1x1 sampling, DCT 1
    //3,0x11,1,                  //   Cr - 1x1 sampling, DCT 1
    0xFF,0xC4,dhtlen >> 8,dhtlen & 255,       // DHT (define huffman table) based on dhtlen
    0 };                       // DC part of first HT follows 
	putBlock(head1, sizeof(head1));
	putBlock(std_dc_luminance_nrcodes+1, sizeof(std_dc_luminance_nrcodes)-1);
	putBlock(std_dc_luminance_values, sizeof(std_dc_luminance_values));
	putByte(0x10); // AC part of first HT follows
	putBlock(std_ac_luminance_nrcodes+1, sizeof(std_ac_luminance_nrcodes)-1);
	putBlock(std_ac_luminance_values, sizeof(std_ac_luminance_values));
	//putByte(1); // DC part of second HT follows
	//putBlock(std_dc_chrominance_nrcodes+1, sizeof(std_dc_chrominance_nrcodes)-1);
	//putBlock(std_dc_chrominance_values, sizeof(std_dc_chrominance_values));
	//putByte(0x11); // DC part of second HT follows
	//putBlock(std_ac_chrominance_nrcodes+1, sizeof(std_ac_chrominance_nrcodes)-1);
	//putBlock(std_ac_chrominance_values, sizeof(std_ac_chrominance_values));
	static const unsigned char head2[] = { 
    0xFF,0xDA,0,6 + 2*1,       // Start of scan, length = 6 + 2 * number of components
    1,                         // Number of components in scan
    1,0,                       // Y = Huffman table to use - AC/DC = 0
    //2,0x11,                    // Cb = Huffman table to use - AC/DC = 1
    //3,0x11,                    // Cr = Huffman table to use - AC/DC = 1
    0,0x3F,0 };                // I don't know
	putBlock(head2, sizeof(head2));
    
#ifdef PROFILE
  printf(" DCT=%5dus, Quant=%5dus, Huff=%5dus\n", timeDCT, timeQuant, timeHuff);
  timeDCT = timeQuant = timeHuff = 0;
#endif  
  
  // This removes a running DC bias caused by our use of unsigned pixels (JPEG specifies signed pixels)
  m_lastDC = 8*128 / YTable[0];  // 64 pixels are 128 counts brighter than expected, scaled down by DC quant
 
  return m_out-out;
}
