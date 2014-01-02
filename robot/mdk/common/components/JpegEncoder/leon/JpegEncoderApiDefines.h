///
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved
///            For License Warranty see: common/license.txt
///
/// @brief     Definitions and types needed by the JPEG encoding library
///

#ifndef _JPEG_ENCODER_API_DEFINES_H_
#define _JPEG_ENCODER_API_DEFINES_H_

// 1: Defines
// ----------------------------------------------------------------------------
// Data
#ifndef JPEG_DATA_SECTION
#define JPEG_DATA_SECTION(x) x
#endif

#ifndef JPEG_JPEGGUARD_DATA_SECTION
#define JPEG_JPEGGUARD_DATA_SECTION(x) x
#endif

#ifndef JPEG_CONST_DATA_SECTION
#define JPEG_CONST_DATA_SECTION(x) x
#endif

// Code
#ifndef JPEG_CODE_SECTION
#define JPEG_CODE_SECTION(x) x
#endif


#define SVU(x) x
#define IRF(x) x

#ifndef JPEGe_SHAVE_NO
#define JPEGe_SHAVE_NO 0
#endif

#define JPGSVE_sym_aux(svu_number,symbolname) JpegEncoder##svu_number##_##symbolname
#define JPGSVE_sym(svu_number,symbolname) JPGSVE_sym_aux(svu_number,symbolname)

// 2: Typedefs (types, enums, structs)
// ----------------------------------------------------------------------------
typedef struct
{
	unsigned char *Y;
	unsigned char *U;
	unsigned char *V;
	u32 width;
	u32 height;
}jpegFrameSpec;

typedef struct {
	unsigned char length;
	unsigned short value;
} bitstring;

typedef struct  {
  unsigned short marker, length;
  unsigned char  QTYinfo,  Ytable[64];
  unsigned char QTCbinfo, Cbtable[64];
}DQTinfotype;

typedef  struct  {
  unsigned short marker, length;
  unsigned char  HTYDCinfo,  YDC_nrcodes[16],  YDC_values[ 12];
  unsigned char  HTYACinfo,  YAC_nrcodes[16],  YAC_values[162];
  unsigned char HTCbDCinfo, CbDC_nrcodes[16], CbDC_values[ 12];
  unsigned char HTCbACinfo, CbAC_nrcodes[16], CbAC_values[162];
} DHTinfotype;

typedef struct  {
  unsigned short marker, length;
  unsigned char nrofcomponents;
  unsigned char IdY,  HTY;
  unsigned char IdCb, HTCb;
  unsigned char IdCr, HTCr;
  unsigned char Ss, Se, Bf;
} SOSinfotype;

typedef struct infotype {
  unsigned short marker, length;
  unsigned char precision;
  unsigned short height, width;
  unsigned char nrofcomponents;
  unsigned char  IdY,  HVY,  QTY;
  unsigned char IdCb, HVCb, QTCb;
  unsigned char IdCr, HVCr, QTCr;
}SOF0infotype;

typedef struct  infoT{
  unsigned short DRI, Lr, Ri;
}RSTinfotype;

typedef struct  {
  unsigned short marker, length;
  unsigned char JFIFsignature[5];
  unsigned char versionhi, versionlo;
  unsigned char xyunits;
  unsigned short xdensity, ydensity;
  unsigned char thumbnwidth, thumbnheight;
}APP0infotype;


// 3: Local const declarations     NB: ONLY const declarations go here
// ----------------------------------------------------------------------------

#endif
