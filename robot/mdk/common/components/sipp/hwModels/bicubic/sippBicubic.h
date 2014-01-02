// -----------------------------------------------------------------------------
// Copyright (C) 2012 Movidius Ltd. All rights reserved
//
// Company          : Movidius
// Author           : Attila Banyai (attila.banyai@movidius.com)
// Description      : SIPP Accelerator HW model - Bicubic filter
//
// Bicubic resampling filter. Limited version in the sense that it takes
// xy coordinates of the transform from external list.
// -----------------------------------------------------------------------------

#ifndef __SIPP_BICUBIC_H__
#define __SIPP_BICUBIC_H__

#ifdef _WIN32
#include <cstdint>
#else
#include <stdint.h>
#include <unistd.h>
#endif
#include "fp16.h"

#ifndef BICUBIC_PIX_FORMAT_RGBX8888  
#define BICUBIC_PIX_FORMAT_RGBX8888  0
#endif

#ifndef BICUBIC_PIX_FORMAT_U8F  
#define BICUBIC_PIX_FORMAT_U8F       1
#endif

#ifndef BICUBIC_PIX_FORMAT_FP16
#define BICUBIC_PIX_FORMAT_FP16      2
#endif

#ifndef BICUBIC_PIX_FORMAT_U16F
#define BICUBIC_PIX_FORMAT_U16F      3
#endif

#ifndef BICUBIC_BAYER_MODE_NORMAL  
#define BICUBIC_BAYER_MODE_NORMAL    0
#endif

#ifndef BICUBIC_BAYER_MODE_INVERT  
#define BICUBIC_BAYER_MODE_INVERT    1
#endif

#ifndef BICUBIC_BAYER_MODE_ODD  
#define BICUBIC_BAYER_MODE_ODD       2
#endif

#ifndef BICUBIC_BAYER_MODE_EVEN  
#define BICUBIC_BAYER_MODE_EVEN      3
#endif

// CTRL register bitfield defines
#ifndef BICUBIC_RUN_SHIFT
#define BICUBIC_RUN_SHIFT            0
#endif

#ifndef BICUBIC_RUN_MASK
#define BICUBIC_RUN_MASK             0x01
#endif

#ifndef BICUBIC_BYPASS_SHIFT
#define BICUBIC_BYPASS_SHIFT         1
#endif

#ifndef BICUBIC_BYPASS_MASK
#define BICUBIC_BYPASS_MASK          0x02
#endif

#ifndef BICUBIC_IRQ_SHIFT
#define BICUBIC_IRQ_SHIFT            2
#endif

#ifndef BICUBIC_IRQ_MASK
#define BICUBIC_IRQ_MASK             0x04
#endif

#ifndef BICUBIC_SMOKETEST_SHIFT
#define BICUBIC_SMOKETEST_SHIFT      3
#endif

#ifndef BICUBIC_SMOKETEST_MASK
#define BICUBIC_SMOKETEST_MASK       0x08
#endif

#ifndef BICUBIC_RUNSTATUS_SHIFT
#define BICUBIC_RUNSTATUS_SHIFT      4
#endif

#ifndef BICUBIC_RUNSTATUS_MASK
#define BICUBIC_RUNSTATUS_MASK       0x10
#endif

#ifndef BICUBIC_CACHE_REQ_SHIFT
#define BICUBIC_CACHE_REQ_SHIFT      5
#endif

#ifndef BICUBIC_CACHE_REQ_MASK
#define BICUBIC_CACHE_REQ_MASK       0x20
#endif

#ifndef BICUBIC_CACHE_WEN_SHIFT
#define BICUBIC_CACHE_WEN_SHIFT      6
#endif

#ifndef BICUBIC_CACHE_WEN_MASK
#define BICUBIC_CACHE_WEN_MASK       0x40
#endif

#ifndef BICUBIC_BILINEAR_SHIFT
#define BICUBIC_BILINEAR_SHIFT       7
#endif

#ifndef BICUBIC_BILINEAR_MASK
#define BICUBIC_BILINEAR_MASK        0x80
#endif

#ifndef BICUBIC_PIX_FORMAT_SHIFT
#define BICUBIC_PIX_FORMAT_SHIFT     8
#endif

#ifndef BICUBIC_PIX_FORMAT_MASK
#define BICUBIC_PIX_FORMAT_MASK      0x300
#endif

#ifndef BICUBIC_CLAMP_SHIFT
#define BICUBIC_CLAMP_SHIFT          10
#endif

#ifndef BICUBIC_CLAMP_MASK
#define BICUBIC_CLAMP_MASK           0x400
#endif

#ifndef BICUBIC_BORDER_MODE_SHIFT
#define BICUBIC_BORDER_MODE_SHIFT    11
#endif

#ifndef BICUBIC_BORDER_MODE_MASK
#define BICUBIC_BORDER_MODE_MASK     0x800
#endif

#ifndef BICUBIC_BAYER_ENABLE_SHIFT
#define BICUBIC_BAYER_ENABLE_SHIFT   12
#endif

#ifndef BICUBIC_BAYER_ENABLE_MASK
#define BICUBIC_BAYER_ENABLE_MASK    0x1000
#endif

#ifndef BICUBIC_BAYER_Y_ODD_SHIFT
#define BICUBIC_BAYER_Y_ODD_SHIFT    13
#endif

#ifndef BICUBIC_BAYER_Y_ODD_MASK
#define BICUBIC_BAYER_Y_ODD_MASK     0x2000
#endif

#ifndef BICUBIC_BAYER_X_MODE_SHIFT
#define BICUBIC_BAYER_X_MODE_SHIFT   14
#endif

#ifndef BICUBIC_BAYER_X_MODE_MASK
#define BICUBIC_BAYER_X_MODE_MASK    0xC000
#endif

// CTRL register bitfield defines
//#ifndef BICUBIC_RUN_SHIFT
//#define BICUBIC_RUN_SHIFT            0
//#endif
//
//#ifndef BICUBIC_RUN_MASK
//#define BICUBIC_RUN_MASK             0x01
//#endif
//
//#ifndef BICUBIC_BYPASS_SHIFT
//#define BICUBIC_BYPASS_SHIFT         1 
//#endif
//
//#ifndef BICUBIC_BYPASS_MASK
//#define BICUBIC_BYPASS_MASK          0x02
//#endif
//
//#ifndef BICUBIC_CLAMP_SHIFT
//#define BICUBIC_CLAMP_SHIFT          2 
//#endif
//
//#ifndef BICUBIC_CLAMP_MASK
//#define BICUBIC_CLAMP_MASK           0x04
//#endif
//
//#ifndef BICUBIC_PIX_FORMAT_SHIFT
//#define BICUBIC_PIX_FORMAT_SHIFT     3
//#endif
//
//#ifndef BICUBIC_PIX_FORMAT_MASK
//#define BICUBIC_PIX_FORMAT_MASK      0x18
//#endif
//
//#ifndef BICUBIC_PIX_FORMAT_RGBX8888  
//#define BICUBIC_PIX_FORMAT_RGBX8888  0
//#endif
//
//#ifndef BICUBIC_PIX_FORMAT_FP8  
//#define BICUBIC_PIX_FORMAT_FP8       1
//#endif
//
//#ifndef BICUBIC_PIX_FORMAT_FP16
//#define BICUBIC_PIX_FORMAT_FP16      2
//#endif
//
//#ifndef BICUBIC_BAYER_MODE_SHIFT
//#define BICUBIC_BAYER_MODE_SHIFT     5 
//#endif
//
//#ifndef BICUBIC_BAYER_MODE_MASK
//#define BICUBIC_BAYER_MODE_MASK      0x20
//#endif
//
//#ifndef BICUBIC_BAYER_SAMPLE_SHIFT
//#define BICUBIC_BAYER_SAMPLE_SHIFT   6 
//#endif
//
//#ifndef BICUBIC_BAYER_SAMPLE_MASK
//#define BICUBIC_BAYER_SAMPLE_MASK    0xC0
//#endif
//
//#ifndef BICUBIC_IRQ_SHIFT
//#define BICUBIC_IRQ_SHIFT            8 
//#endif
//
//#ifndef BICUBIC_IRQ_MASK
//#define BICUBIC_IRQ_MASK             0x100
//#endif
//
//#ifndef BICUBIC_BORDER_MODE_SHIFT
//#define BICUBIC_BORDER_MODE_SHIFT    9 
//#endif
//
//#ifndef BICUBIC_BORDER_MODE_MASK
//#define BICUBIC_BORDER_MODE_MASK     0x200
//#endif



// register map
typedef struct
{
	uint64_t BIC_CTRL;					// 16bit
	uint64_t BIC_ID;					// 8bit
	uint64_t BIC_LINKED_LIST_ADDR;		// 32bit
	uint64_t BIC_LINE_LENGTH;			// 24bit
	uint64_t BIC_INPUT_HEIGHT;			// 16bit
	uint64_t BIC_BORDER_PIXEL;			// 32bit
	uint64_t BIC_XY_PIXEL_LIST_BASE;	// 32bit
	uint64_t BIC_INPUT_BASE_ADDR;		// 32bit
	uint64_t BIC_INPUT_TOP_ADDR;		// 32bit
	uint64_t BIC_INPUT_CHUNK_STRIDE;	// 24bit
 	uint64_t BIC_INPUT_CHUNK_WIDTH;		// 24bit
 	uint64_t BIC_INPUT_LINE_STRIDE;     // 24bit
 	uint64_t BIC_INPUT_LINE_WIDTH;		// 24bit
	uint64_t BIC_INPUT_OFFSET;          // 16bit
	uint64_t BIC_OUTPUT_BASE_ADDR;		// 32bit
	uint64_t BIC_OUTPUT_CHUNK_STRIDE;	// 24bit
 	uint64_t BIC_OUTPUT_CHUNK_WIDTH;	// 24bit
	uint64_t BIC_RESERVED;              // account for gap in address space
	uint64_t BIC_CLAMP_MIN;				// 16bit
	uint64_t BIC_CLAMP_MAX;				// 16bit

 	uint64_t BIC_SMOKE_MX; 
 	uint64_t BIC_SMOKE_MY; 
 	uint64_t BIC_SMOKE_PIX11; 
 	uint64_t BIC_SMOKE_PIX21; 
 	uint64_t BIC_SMOKE_PIX31; 
 	uint64_t BIC_SMOKE_PIX41; 
 	uint64_t BIC_SMOKE_PIX12; 
 	uint64_t BIC_SMOKE_PIX22; 
 	uint64_t BIC_SMOKE_PIX32; 
 	uint64_t BIC_SMOKE_PIX42; 
 	uint64_t BIC_SMOKE_PIX13; 
 	uint64_t BIC_SMOKE_PIX23; 
 	uint64_t BIC_SMOKE_PIX33; 
 	uint64_t BIC_SMOKE_PIX43; 
 	uint64_t BIC_SMOKE_PIX14; 
 	uint64_t BIC_SMOKE_PIX24; 
 	uint64_t BIC_SMOKE_PIX34; 
 	uint64_t BIC_SMOKE_PIX44; 
 	uint64_t BIC_SMOKE_PIXEL; 
	uint64_t BIC_CACHE1_ADR;
	uint64_t BIC_CACHE1_WRITE_DATA;
	uint64_t BIC_CACHE1_READ_DATA;
	uint64_t BIC_RAM_CTRL;
	uint64_t BIC_CACHE_CTRL;
	uint64_t BIC_CACHE2_ADR;
	uint64_t BIC_CACHE2_WRITE_DATA;
	uint64_t BIC_CACHE2_READ_DATA;
} tBicubicReg;


class BicubicFilt {
public:
	BicubicFilt(tBicubicReg *pReg = 0) :
	  regMap (pReg) {
	}
	~BicubicFilt() {
	}
	void SetReg(tBicubicReg *pReg);
	tBicubicReg * getReg();

	// setup list element
	void CommandPack(tBicubicReg *, uint8_t *);
	virtual void CommandUnpack(uint8_t *, tBicubicReg *);
	// Set up pointers and run (synchronous mode)
	void SetUpAndRun(uint8_t *);
	// asynchronous model
	void TryRun(void);

protected:
	tBicubicReg *regMap;
	virtual void Run(void);
	int  BorderLogic(int, int, int);
};


#endif // __SIPP_BICUBIC_H__
