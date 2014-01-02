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

#include "sippBicubic.h"

#include <string>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

// Need to set rounding mode to match HW
#ifndef WIN32
#include <fenv.h>
#pragma STDC FENV_ACCESS ON
#endif

#define LOG_OFFSET   0x0  
#define LOG_SAMPLES  0x10
// 32bit machine epsilon
//#define M_EPS        ((float)1.19e-7)
#define M_ONE_MINUS_EPS (0.999f)

// Need to set rounding mode to match HW
#ifndef WIN32
#include <fenv.h>
#pragma STDC FENV_ACCESS ON
#endif


void BicubicFilt::SetUpAndRun(uint8_t * list) {
	static int count = 0;
	static tBicubicReg  regSet;

	regMap = &regSet; 
	regMap->BIC_LINKED_LIST_ADDR = (uint32_t)list;
	regMap->BIC_CTRL = (1<<BICUBIC_RUN_SHIFT)&BICUBIC_RUN_MASK;

	// excution conditions
	while ( (regMap->BIC_LINKED_LIST_ADDR != 0) && (((regMap->BIC_CTRL&BICUBIC_RUN_MASK)>>BICUBIC_RUN_SHIFT) !=0) )
	{
		CommandUnpack((uint8_t *)regMap->BIC_LINKED_LIST_ADDR, regMap);
		Run();
	}
	regMap->BIC_CTRL &= regMap->BIC_CTRL&(~BICUBIC_RUN_MASK);
}

void BicubicFilt::TryRun(void) {
	static int count = 0;

	// Can't run if not enabled!
	if (regMap == 0)
		return;

	// execution conditions
    while ( (regMap->BIC_LINKED_LIST_ADDR != 0) && (((regMap->BIC_CTRL&BICUBIC_RUN_MASK)>>BICUBIC_RUN_SHIFT) !=0) ) 
    { 
        //uint8_t * list = (uint8_t *)regMap->BIC_LINKED_LIST_ADDR; 
        CommandUnpack((uint8_t *)regMap->BIC_LINKED_LIST_ADDR, regMap); 
        Run(); 
    } 
    
    if ( (((regMap->BIC_CTRL&BICUBIC_RUN_MASK)>>BICUBIC_RUN_SHIFT) !=0) ) 
    { 
        Run(); 
    }  
    regMap->BIC_CTRL &= (~BICUBIC_RUN_MASK); 
}	

void BicubicFilt::Run(void) 
{
	uint32_t  border    = (uint32_t)regMap->BIC_BORDER_PIXEL;
	uint32_t  samples   = (uint32_t)regMap->BIC_LINE_LENGTH;
	uint32_t  width     = (uint32_t)regMap->BIC_INPUT_LINE_WIDTH;
	uint32_t  height    = (uint32_t)regMap->BIC_INPUT_HEIGHT;
	uint32_t  pixel_type = (uint32_t)((regMap->BIC_CTRL&BICUBIC_PIX_FORMAT_MASK)>>BICUBIC_PIX_FORMAT_SHIFT);
	uint32_t  bay = (uint32_t)((regMap->BIC_CTRL&BICUBIC_BAYER_ENABLE_MASK)>>BICUBIC_BAYER_ENABLE_SHIFT);
	uint32_t  cb  = (uint32_t)((regMap->BIC_CTRL&BICUBIC_CLAMP_MASK)>>BICUBIC_CLAMP_SHIFT);
	uint32_t  bilinear = (uint32_t)((regMap->BIC_CTRL&BICUBIC_BILINEAR_MASK)>>BICUBIC_BILINEAR_SHIFT);
	float   *coord_list = (float *)regMap->BIC_XY_PIXEL_LIST_BASE;
	float    mx, my, x, y;
	fp16     a11[4], a12[4], a13[4], a14[4], a21[4], a22[4], a23[4], a24[4], a31[4], a32[4], a33[4], a34[4], a41[4], a42[4], a43[4], a44[4], out[4];
	fp16     b1, b2, b3, b4, t1, t2, t3, t4, x1, x2, x3, x4, w1, w2, w3, w4, clampLo, clampHi;
	fp16     t1_0_5, t2_0_5, t3_0_5;
	uint32_t dummy, i, pl, planes, outsider;
	int      offset, ox, oy, x_odd_pixels, y_odd_pixels;
	FILE    *fout;

	// Set rounding mode towards zero
	unsigned int rnd;
#ifndef WIN32
	rnd = fegetround();
	fesetround(FE_TOWARDZERO);
#else
	_controlfp_s(&rnd, _RC_CHOP, _MCW_RC);
#endif

	clampLo.setPackedValue((uint16_t)regMap->BIC_CLAMP_MIN);
	clampHi.setPackedValue((uint16_t)regMap->BIC_CLAMP_MAX);

	if(!(fout = fopen("kernel_log.txt", "w"))) 
	{
		perror("Couldn't open output file!");
		abort();
	}

	for (i=0; i<samples; i++)
	{
		mx = *coord_list;
		coord_list++;
		my = *coord_list;
		coord_list++;

		// clamp and detrmine sample position
		outsider = 0;
		if (mx <= 0.0f)
		{
			outsider++;
			mx = 0.0f;
		}
		if (mx >= (width+1))
		{
			outsider++;
			mx = (float)(width) + M_ONE_MINUS_EPS;
		}
		if (my <= 0.0f)
		{
			outsider++;
			my = 0.0f;
		}
		if (my >= (height+1))
		{
			outsider++;
			my = (float)(height) + M_ONE_MINUS_EPS;
		}

		if (bay != 0)
		{
			switch ((regMap->BIC_CTRL&BICUBIC_BAYER_X_MODE_MASK)>>BICUBIC_BAYER_X_MODE_SHIFT)
			{
			case BICUBIC_BAYER_MODE_NORMAL:
				x_odd_pixels = i&1;
				break;
			case BICUBIC_BAYER_MODE_INVERT:
				x_odd_pixels = (i+1)&1;
				break;
			case BICUBIC_BAYER_MODE_ODD:
				x_odd_pixels = 1;
				break;
			case BICUBIC_BAYER_MODE_EVEN:
				x_odd_pixels = 0;
				break;
			}
			y_odd_pixels = (regMap->BIC_CTRL&BICUBIC_BAYER_Y_ODD_MASK)>>BICUBIC_BAYER_Y_ODD_SHIFT;

			x = mx - (float)x_odd_pixels;
			x = x - 2.0f*floor(x/2.0f);
			mx = mx - x*0.5f;
			y = my - (float)y_odd_pixels;
			y = y - 2.0f*floor(y/2.0f);
			my = my - y*0.5f;         
		}


		ox = (int)mx;
		oy = (int)my;
		mx = mx - (float)ox;
		my = my - (float)oy;

		// default output to border pixel value
	    switch (pixel_type)
		{
			case BICUBIC_PIX_FORMAT_RGBX8888:
				out[0] = U8F_TO_FP16((border>>0)&0xFF);
				out[1] = U8F_TO_FP16((border>>8)&0xFF);
				out[2] = U8F_TO_FP16((border>>16)&0xFF);
				out[3] = U8F_TO_FP16((border>>24)&0xFF);
				break;
			case BICUBIC_PIX_FORMAT_U8F:
				out[0] = U8F_TO_FP16(border);
				break;
			case BICUBIC_PIX_FORMAT_FP16:
				out[0].setPackedValue(border);
				break;
			case BICUBIC_PIX_FORMAT_U16F:
				out[0] = U16F_TO_FP16(border);
				break;
		}

		// handling is done when center is inside or border clamping is enabled
		if ( (outsider == 0) || ((regMap->BIC_CTRL&BICUBIC_BORDER_MODE_MASK) == 0) )
		{
			// collect samples
			switch (pixel_type)
			{
			case BICUBIC_PIX_FORMAT_RGBX8888:
			{
				uint32_t *src_addr = (uint32_t *)regMap->BIC_INPUT_BASE_ADDR;

				planes = 4;

				offset = BorderLogic((ox-(bay+1)),   (oy-(bay+1)), 2);
				if (offset >= 0)
				{
					dummy = src_addr[offset];
					a11[0] = U8F_TO_FP16((dummy>>0)&0xFF);
					a11[1] = U8F_TO_FP16((dummy>>8)&0xFF);
					a11[2] = U8F_TO_FP16((dummy>>16)&0xFF);
					a11[3] = U8F_TO_FP16((dummy>>24)&0xFF);
				} else
				{
					a11[0] = U8F_TO_FP16((border>>0)&0xFF);
					a11[1] = U8F_TO_FP16((border>>8)&0xFF);
					a11[2] = U8F_TO_FP16((border>>16)&0xFF);
					a11[3] = U8F_TO_FP16((border>>24)&0xFF);
				}
				offset = BorderLogic((ox+0),         (oy-(bay+1)), 2);
				if (offset >= 0)
				{
					dummy = src_addr[offset];
					a12[0] = U8F_TO_FP16((dummy>>0)&0xFF);
					a12[1] = U8F_TO_FP16((dummy>>8)&0xFF);
					a12[2] = U8F_TO_FP16((dummy>>16)&0xFF);
					a12[3] = U8F_TO_FP16((dummy>>24)&0xFF);
				} else
				{
					a12[0] = U8F_TO_FP16((border>>0)&0xFF);
					a12[1] = U8F_TO_FP16((border>>8)&0xFF);
					a12[2] = U8F_TO_FP16((border>>16)&0xFF);
					a12[3] = U8F_TO_FP16((border>>24)&0xFF);
				}
				offset = BorderLogic((ox+(bay+1)),   (oy-(bay+1)), 2);
				if (offset >= 0)
				{
					dummy = src_addr[offset];
					a13[0] = U8F_TO_FP16((dummy>>0)&0xFF);
					a13[1] = U8F_TO_FP16((dummy>>8)&0xFF);
					a13[2] = U8F_TO_FP16((dummy>>16)&0xFF);
					a13[3] = U8F_TO_FP16((dummy>>24)&0xFF);
				} else
				{
					a13[0] = U8F_TO_FP16((border>>0)&0xFF);
					a13[1] = U8F_TO_FP16((border>>8)&0xFF);
					a13[2] = U8F_TO_FP16((border>>16)&0xFF);
					a13[3] = U8F_TO_FP16((border>>24)&0xFF);
				}
				offset = BorderLogic((ox+2*(bay+1)), (oy-(bay+1)), 2);
				if (offset >= 0)
				{
					dummy = src_addr[offset];
					a14[0] = U8F_TO_FP16((dummy>>0)&0xFF);
					a14[1] = U8F_TO_FP16((dummy>>8)&0xFF);
					a14[2] = U8F_TO_FP16((dummy>>16)&0xFF);
					a14[3] = U8F_TO_FP16((dummy>>24)&0xFF);
				} else
				{
					a14[0] = U8F_TO_FP16((border>>0)&0xFF);
					a14[1] = U8F_TO_FP16((border>>8)&0xFF);
					a14[2] = U8F_TO_FP16((border>>16)&0xFF);
					a14[3] = U8F_TO_FP16((border>>24)&0xFF);
				}
				offset = BorderLogic((ox-(bay+1)),   (oy+0), 2);
				if (offset >= 0)
				{
					dummy = src_addr[offset];
					a21[0] = U8F_TO_FP16((dummy>>0)&0xFF);
					a21[1] = U8F_TO_FP16((dummy>>8)&0xFF);
					a21[2] = U8F_TO_FP16((dummy>>16)&0xFF);
					a21[3] = U8F_TO_FP16((dummy>>24)&0xFF);
				} else
				{
					a21[0] = U8F_TO_FP16((border>>0)&0xFF);
					a21[1] = U8F_TO_FP16((border>>8)&0xFF);
					a21[2] = U8F_TO_FP16((border>>16)&0xFF);
					a21[3] = U8F_TO_FP16((border>>24)&0xFF);
				}
				offset = BorderLogic((ox+0),         (oy+0), 2);
				if (offset >= 0)
				{
					dummy = src_addr[offset];
					a22[0] = U8F_TO_FP16((dummy>>0)&0xFF);
					a22[1] = U8F_TO_FP16((dummy>>8)&0xFF);
					a22[2] = U8F_TO_FP16((dummy>>16)&0xFF);
					a22[3] = U8F_TO_FP16((dummy>>24)&0xFF);
				} else
				{
					a22[0] = U8F_TO_FP16((border>>0)&0xFF);
					a22[1] = U8F_TO_FP16((border>>8)&0xFF);
					a22[2] = U8F_TO_FP16((border>>16)&0xFF);
					a22[3] = U8F_TO_FP16((border>>24)&0xFF);
				}
				offset = BorderLogic((ox+(bay+1)),   (oy+0), 2);
				if (offset >= 0)
				{
					dummy = src_addr[offset];
					a23[0] = U8F_TO_FP16((dummy>>0)&0xFF);
					a23[1] = U8F_TO_FP16((dummy>>8)&0xFF);
					a23[2] = U8F_TO_FP16((dummy>>16)&0xFF);
					a23[3] = U8F_TO_FP16((dummy>>24)&0xFF);
				} else
				{
					a23[0] = U8F_TO_FP16((border>>0)&0xFF);
					a23[1] = U8F_TO_FP16((border>>8)&0xFF);
					a23[2] = U8F_TO_FP16((border>>16)&0xFF);
					a23[3] = U8F_TO_FP16((border>>24)&0xFF);
				}
				offset = BorderLogic((ox+2*(bay+1)), (oy+0), 2);
				if (offset >= 0)
				{
					dummy = src_addr[offset];
					a24[0] = U8F_TO_FP16((dummy>>0)&0xFF);
					a24[1] = U8F_TO_FP16((dummy>>8)&0xFF);
					a24[2] = U8F_TO_FP16((dummy>>16)&0xFF);
					a24[3] = U8F_TO_FP16((dummy>>24)&0xFF);
				} else
				{
					a24[0] = U8F_TO_FP16((border>>0)&0xFF);
					a24[1] = U8F_TO_FP16((border>>8)&0xFF);
					a24[2] = U8F_TO_FP16((border>>16)&0xFF);
					a24[3] = U8F_TO_FP16((border>>24)&0xFF);
				}
				offset = BorderLogic((ox-(bay+1)),   (oy+(bay+1)), 2);
				if (offset >= 0)
				{
					dummy = src_addr[offset];
					a31[0] = U8F_TO_FP16((dummy>>0)&0xFF);
					a31[1] = U8F_TO_FP16((dummy>>8)&0xFF);
					a31[2] = U8F_TO_FP16((dummy>>16)&0xFF);
					a31[3] = U8F_TO_FP16((dummy>>24)&0xFF);
				} else
				{
					a31[0] = U8F_TO_FP16((border>>0)&0xFF);
					a31[1] = U8F_TO_FP16((border>>8)&0xFF);
					a31[2] = U8F_TO_FP16((border>>16)&0xFF);
					a31[3] = U8F_TO_FP16((border>>24)&0xFF);
				}
				offset = BorderLogic((ox+0),         (oy+(bay+1)), 2);
				if (offset >= 0)
				{
					dummy = src_addr[offset];
					a32[0] = U8F_TO_FP16((dummy>>0)&0xFF);
					a32[1] = U8F_TO_FP16((dummy>>8)&0xFF);
					a32[2] = U8F_TO_FP16((dummy>>16)&0xFF);
					a32[3] = U8F_TO_FP16((dummy>>24)&0xFF);
				} else
				{
					a32[0] = U8F_TO_FP16((border>>0)&0xFF);
					a32[1] = U8F_TO_FP16((border>>8)&0xFF);
					a32[2] = U8F_TO_FP16((border>>16)&0xFF);
					a32[3] = U8F_TO_FP16((border>>24)&0xFF);
				}
				offset = BorderLogic((ox+(bay+1)),   (oy+(bay+1)), 2);
				if (offset >= 0)
				{
					dummy = src_addr[offset];
					a33[0] = U8F_TO_FP16((dummy>>0)&0xFF);
					a33[1] = U8F_TO_FP16((dummy>>8)&0xFF);
					a33[2] = U8F_TO_FP16((dummy>>16)&0xFF);
					a33[3] = U8F_TO_FP16((dummy>>24)&0xFF);
				} else
				{
					a33[0] = U8F_TO_FP16((border>>0)&0xFF);
					a33[1] = U8F_TO_FP16((border>>8)&0xFF);
					a33[2] = U8F_TO_FP16((border>>16)&0xFF);
					a33[3] = U8F_TO_FP16((border>>24)&0xFF);
				}
				offset = BorderLogic((ox+2*(bay+1)), (oy+(bay+1)), 2);
				if (offset >= 0)
				{
					dummy = src_addr[offset];
					a34[0] = U8F_TO_FP16((dummy>>0)&0xFF);
					a34[1] = U8F_TO_FP16((dummy>>8)&0xFF);
					a34[2] = U8F_TO_FP16((dummy>>16)&0xFF);
					a34[3] = U8F_TO_FP16((dummy>>24)&0xFF);
				} else
				{
					a34[0] = U8F_TO_FP16((border>>0)&0xFF);
					a34[1] = U8F_TO_FP16((border>>8)&0xFF);
					a34[2] = U8F_TO_FP16((border>>16)&0xFF);
					a34[3] = U8F_TO_FP16((border>>24)&0xFF);
				}
				offset = BorderLogic((ox-(bay+1)),   (oy+2*(bay+1)), 2);
				if (offset >= 0)
				{
					dummy = src_addr[offset];
					a41[0] = U8F_TO_FP16((dummy>>0)&0xFF);
					a41[1] = U8F_TO_FP16((dummy>>8)&0xFF);
					a41[2] = U8F_TO_FP16((dummy>>16)&0xFF);
					a41[3] = U8F_TO_FP16((dummy>>24)&0xFF);
				} else
				{
					a41[0] = U8F_TO_FP16((border>>0)&0xFF);
					a41[1] = U8F_TO_FP16((border>>8)&0xFF);
					a41[2] = U8F_TO_FP16((border>>16)&0xFF);
					a41[3] = U8F_TO_FP16((border>>24)&0xFF);
				}
				offset = BorderLogic((ox+0),         (oy+2*(bay+1)), 2);
				if (offset >= 0)
				{
					dummy = src_addr[offset];
					a42[0] = U8F_TO_FP16((dummy>>0)&0xFF);
					a42[1] = U8F_TO_FP16((dummy>>8)&0xFF);
					a42[2] = U8F_TO_FP16((dummy>>16)&0xFF);
					a42[3] = U8F_TO_FP16((dummy>>24)&0xFF);
				} else
				{
					a42[0] = U8F_TO_FP16((border>>0)&0xFF);
					a42[1] = U8F_TO_FP16((border>>8)&0xFF);
					a42[2] = U8F_TO_FP16((border>>16)&0xFF);
					a42[3] = U8F_TO_FP16((border>>24)&0xFF);
				}
				offset = BorderLogic((ox+(bay+1)),   (oy+2*(bay+1)), 2);
				if (offset >= 0)
				{
					dummy = src_addr[offset];
					a43[0] = U8F_TO_FP16((dummy>>0)&0xFF);
					a43[1] = U8F_TO_FP16((dummy>>8)&0xFF);
					a43[2] = U8F_TO_FP16((dummy>>16)&0xFF);
					a43[3] = U8F_TO_FP16((dummy>>24)&0xFF);
				} else
				{
					a43[0] = U8F_TO_FP16((border>>0)&0xFF);
					a43[1] = U8F_TO_FP16((border>>8)&0xFF);
					a43[2] = U8F_TO_FP16((border>>16)&0xFF);
					a43[3] = U8F_TO_FP16((border>>24)&0xFF);
				}
				offset = BorderLogic((ox+2*(bay+1)), (oy+2*(bay+1)), 2);
				if (offset >= 0)
				{
					dummy = src_addr[offset];
					a44[0] = U8F_TO_FP16((dummy>>0)&0xFF);
					a44[1] = U8F_TO_FP16((dummy>>8)&0xFF);
					a44[2] = U8F_TO_FP16((dummy>>16)&0xFF);
					a44[3] = U8F_TO_FP16((dummy>>24)&0xFF);
				} else
				{
					a44[0] = U8F_TO_FP16((border>>0)&0xFF);
					a44[1] = U8F_TO_FP16((border>>8)&0xFF);
					a44[2] = U8F_TO_FP16((border>>16)&0xFF);
					a44[3] = U8F_TO_FP16((border>>24)&0xFF);
				}
			}
			break;
			case BICUBIC_PIX_FORMAT_U8F:
			{
				uint8_t *src_addr = (uint8_t *)regMap->BIC_INPUT_BASE_ADDR;

				planes = 1;
				offset = BorderLogic((ox-(bay+1)),   (oy-(bay+1)), 0);
				if (offset >= 0) a11[0] = U8F_TO_FP16(src_addr[offset]);
				else a11[0] = U8F_TO_FP16(border);
				offset = BorderLogic((ox+0),         (oy-(bay+1)), 0);
				if (offset >= 0) a12[0] = U8F_TO_FP16(src_addr[offset]);
				else a12[0] = U8F_TO_FP16(border);
				offset = BorderLogic((ox+(bay+1)),   (oy-(bay+1)), 0);
				if (offset >= 0) a13[0] = U8F_TO_FP16(src_addr[offset]);
				else a13[0] = U8F_TO_FP16(border);
				offset = BorderLogic((ox+2*(bay+1)), (oy-(bay+1)), 0);
				if (offset >= 0) a14[0] = U8F_TO_FP16(src_addr[offset]);
				else a14[0] = U8F_TO_FP16(border);
				offset = BorderLogic((ox-(bay+1)),   (oy+0), 0);
				if (offset >= 0) a21[0] = U8F_TO_FP16(src_addr[offset]);
				else a21[0] = U8F_TO_FP16(border);
				offset = BorderLogic((ox+0),         (oy+0), 0);
				if (offset >= 0) a22[0] = U8F_TO_FP16(src_addr[offset]);
				else a22[0] = U8F_TO_FP16(border);
				offset = BorderLogic((ox+(bay+1)),   (oy+0), 0);
				if (offset >= 0) a23[0] = U8F_TO_FP16(src_addr[offset]);
				else a23[0] = U8F_TO_FP16(border);
				offset = BorderLogic((ox+2*(bay+1)), (oy+0), 0);
				if (offset >= 0) a24[0] = U8F_TO_FP16(src_addr[offset]);
				else a24[0] = U8F_TO_FP16(border);
				offset = BorderLogic((ox-(bay+1)),   (oy+(bay+1)), 0);
				if (offset >= 0) a31[0] = U8F_TO_FP16(src_addr[offset]);
				else a31[0] = U8F_TO_FP16(border);
				offset = BorderLogic((ox+0),         (oy+(bay+1)), 0);
				if (offset >= 0) a32[0] = U8F_TO_FP16(src_addr[offset]);
				else a32[0] = U8F_TO_FP16(border);
				offset = BorderLogic((ox+(bay+1)),   (oy+(bay+1)), 0);
				if (offset >= 0) a33[0] = U8F_TO_FP16(src_addr[offset]);
				else a33[0] = U8F_TO_FP16(border);
				offset = BorderLogic((ox+2*(bay+1)), (oy+(bay+1)), 0);
				if (offset >= 0) a34[0] = U8F_TO_FP16(src_addr[offset]);
				else a34[0] = U8F_TO_FP16(border);
				offset = BorderLogic((ox-(bay+1)),   (oy+2*(bay+1)), 0);
				if (offset >= 0) a41[0] = U8F_TO_FP16(src_addr[offset]);
				else a41[0] = U8F_TO_FP16(border);
				offset = BorderLogic((ox+0),         (oy+2*(bay+1)), 0);
				if (offset >= 0) a42[0] = U8F_TO_FP16(src_addr[offset]);
				else a42[0] = U8F_TO_FP16(border);
				offset = BorderLogic((ox+(bay+1)),   (oy+2*(bay+1)), 0);
				if (offset >= 0) a43[0] = U8F_TO_FP16(src_addr[offset]);
				else a43[0] = U8F_TO_FP16(border);
				offset = BorderLogic((ox+2*(bay+1)), (oy+2*(bay+1)), 0);
				if (offset >= 0) a44[0] = U8F_TO_FP16(src_addr[offset]);
				else a44[0] = U8F_TO_FP16(border);
			}
			break;
			case BICUBIC_PIX_FORMAT_FP16:
			{
				fp16 *src_addr = (fp16 *)regMap->BIC_INPUT_BASE_ADDR;

				planes = 1;
				offset = BorderLogic((ox-(bay+1)),   (oy-(bay+1)), 1);
				if (offset >= 0) a11[0] = src_addr[offset];
				else a11[0].setPackedValue(border);
				offset = BorderLogic((ox+0),         (oy-(bay+1)), 1);
				if (offset >= 0) a12[0] = src_addr[offset];
				else a12[0].setPackedValue(border);
				offset = BorderLogic((ox+(bay+1)),   (oy-(bay+1)), 1);
				if (offset >= 0) a13[0] = src_addr[offset];
				else a13[0].setPackedValue(border);
				offset = BorderLogic((ox+2*(bay+1)), (oy-(bay+1)), 1);
				if (offset >= 0) a14[0] = src_addr[offset];
				else a14[0].setPackedValue(border);
				offset = BorderLogic((ox-(bay+1)),   (oy+0), 1);
				if (offset >= 0) a21[0] = src_addr[offset];
				else a21[0].setPackedValue(border);
				offset = BorderLogic((ox+0),         (oy+0), 1);
				if (offset >= 0) a22[0] = src_addr[offset];
				else a22[0].setPackedValue(border);
				offset = BorderLogic((ox+(bay+1)),   (oy+0), 1);
				if (offset >= 0) a23[0] = src_addr[offset];
				else a23[0].setPackedValue(border);
				offset = BorderLogic((ox+2*(bay+1)), (oy+0), 1);
				if (offset >= 0) a24[0] = src_addr[offset];
				else a24[0].setPackedValue(border);
				offset = BorderLogic((ox-(bay+1)),   (oy+(bay+1)), 1);
				if (offset >= 0) a31[0] = src_addr[offset];
				else a31[0].setPackedValue(border);
				offset = BorderLogic((ox+0),         (oy+(bay+1)), 1);
				if (offset >= 0) a32[0] = src_addr[offset];
				else a32[0].setPackedValue(border);
				offset = BorderLogic((ox+(bay+1)),   (oy+(bay+1)), 1);
				if (offset >= 0) a33[0] = src_addr[offset];
				else a33[0].setPackedValue(border);
				offset = BorderLogic((ox+2*(bay+1)), (oy+(bay+1)), 1);
				if (offset >= 0) a34[0] = src_addr[offset];
				else a34[0].setPackedValue(border);
				offset = BorderLogic((ox-(bay+1)),   (oy+2*(bay+1)), 1);
				if (offset >= 0) a41[0] = src_addr[offset];
				else a41[0].setPackedValue(border);
				offset = BorderLogic((ox+0),         (oy+2*(bay+1)), 1);
				if (offset >= 0) a42[0] = src_addr[offset];
				else a42[0].setPackedValue(border);
				offset = BorderLogic((ox+(bay+1)),   (oy+2*(bay+1)), 1);
				if (offset >= 0) a43[0] = src_addr[offset];
				else a43[0].setPackedValue(border);
				offset = BorderLogic((ox+2*(bay+1)), (oy+2*(bay+1)), 1);
				if (offset >= 0) a44[0] = src_addr[offset];
				else a44[0].setPackedValue(border);
			}
			break;
			case BICUBIC_PIX_FORMAT_U16F:
			{
				uint16_t *src_addr = (uint16_t *)regMap->BIC_INPUT_BASE_ADDR;

				planes = 1;
				offset = BorderLogic((ox-(bay+1)),   (oy-(bay+1)), 1);
				if (offset >= 0) a11[0] = U16F_TO_FP16(src_addr[offset]);
				else a11[0] = U16F_TO_FP16(border);
				offset = BorderLogic((ox+0),         (oy-(bay+1)), 1);
				if (offset >= 0) a12[0] = U16F_TO_FP16(src_addr[offset]);
				else a12[0] = U16F_TO_FP16(border);
				offset = BorderLogic((ox+(bay+1)),   (oy-(bay+1)), 1);
				if (offset >= 0) a13[0] = U16F_TO_FP16(src_addr[offset]);
				else a13[0] = U16F_TO_FP16(border);
				offset = BorderLogic((ox+2*(bay+1)), (oy-(bay+1)), 1);
				if (offset >= 0) a14[0] = U16F_TO_FP16(src_addr[offset]);
				else a14[0] = U16F_TO_FP16(border);
				offset = BorderLogic((ox-(bay+1)),   (oy+0), 1);
				if (offset >= 0) a21[0] = U16F_TO_FP16(src_addr[offset]);
				else a21[0] = U16F_TO_FP16(border);
				offset = BorderLogic((ox+0),         (oy+0), 1);
				if (offset >= 0) a22[0] = U16F_TO_FP16(src_addr[offset]);
				else a22[0] = U16F_TO_FP16(border);
				offset = BorderLogic((ox+(bay+1)),   (oy+0), 1);
				if (offset >= 0) a23[0] = U16F_TO_FP16(src_addr[offset]);
				else a23[0] = U16F_TO_FP16(border);
				offset = BorderLogic((ox+2*(bay+1)), (oy+0), 1);
				if (offset >= 0) a24[0] = U16F_TO_FP16(src_addr[offset]);
				else a24[0] = U16F_TO_FP16(border);
				offset = BorderLogic((ox-(bay+1)),   (oy+(bay+1)), 1);
				if (offset >= 0) a31[0] = U16F_TO_FP16(src_addr[offset]);
				else a31[0] = U16F_TO_FP16(border);
				offset = BorderLogic((ox+0),         (oy+(bay+1)), 1);
				if (offset >= 0) a32[0] = U16F_TO_FP16(src_addr[offset]);
				else a32[0] = U16F_TO_FP16(border);
				offset = BorderLogic((ox+(bay+1)),   (oy+(bay+1)), 1);
				if (offset >= 0) a33[0] = U16F_TO_FP16(src_addr[offset]);
				else a33[0] = U16F_TO_FP16(border);
				offset = BorderLogic((ox+2*(bay+1)), (oy+(bay+1)), 1);
				if (offset >= 0) a34[0] = U16F_TO_FP16(src_addr[offset]);
				else a34[0] = U16F_TO_FP16(border);
				offset = BorderLogic((ox-(bay+1)),   (oy+2*(bay+1)), 1);
				if (offset >= 0) a41[0] = U16F_TO_FP16(src_addr[offset]);
				else a41[0] = U16F_TO_FP16(border);
				offset = BorderLogic((ox+0),         (oy+2*(bay+1)), 1);
				if (offset >= 0) a42[0] = U16F_TO_FP16(src_addr[offset]);
				else a42[0] = U16F_TO_FP16(border);
				offset = BorderLogic((ox+(bay+1)),   (oy+2*(bay+1)), 1);
				if (offset >= 0) a43[0] = U16F_TO_FP16(src_addr[offset]);
				else a43[0] = U16F_TO_FP16(border);
				offset = BorderLogic((ox+2*(bay+1)), (oy+2*(bay+1)), 1);
				if (offset >= 0) a44[0] = U16F_TO_FP16(src_addr[offset]);
				else a44[0] = U16F_TO_FP16(border);
			}
			break;
			}
			// bicubic kernel
			for (pl=0; pl<planes; pl++)
			{
				t1 = (fp16)mx;
				t2 = (fp16)(mx*mx);
				t3 = (fp16)(mx*mx*mx);   
				// coefficients emulate sumx
				if (bilinear != 0)
				{
					w1 = (fp16)0.0f;
					w2 = (fp16)1.0f-t1;
					w3 = t1;
					w4 = (fp16)0.0f;
				} else
				{
					//w1 = (fp16)(((double)t1*(double)-0.5f) + (double)t2 + ((double)t3*(double)-0.5f));
					//w2 = (fp16)(((double)1.0f) + ((double)t2*(double)-2.5f) + ((double)t3*(double)1.5f));
					//w3 = (fp16)(((double)t1*(double)0.5f) + ((double)t2*(double)2.0f) + ((double)t3*(double)-1.5f));
					//w4 = (fp16)(((double)t2*(double)-0.5f) + ((double)t3*(double)0.5f));

					t1_0_5 = (fp16)(t1*(fp16)+0.5f);
					t2_0_5 = (fp16)(t2*(fp16)+0.5f);
					t3_0_5 = (fp16)(t3*(fp16)+0.5f);
					w1 = (fp16)(-(double)t1_0_5        + (double)t2                                - (double)t3_0_5);
					w2 = (fp16)((double)1.0f           - (double)(t2*(fp16)2.0f) - (double)t2_0_5  + (double)t3 + (double)t3_0_5);
					w3 = (fp16)((double)t1_0_5         + (double)(t2*(fp16)2.0f)                   - (double)t3 - (double)t3_0_5);
					w4 = (fp16)(                       - (double)t2_0_5                            + (double)t3_0_5);        

				}
				// multiplier &adder stage emulate sumx
				x1 = a11[pl]*w1; x2 = a12[pl]*w2; x3 = a13[pl]*w3; x4 = a14[pl]*w4;
				b1 = (fp16)((double)x1 + (double)x2 + (double)x3 + (double)x4);
				x1 = a21[pl]*w1; x2 = a22[pl]*w2; x3 = a23[pl]*w3; x4 = a24[pl]*w4;
				b2 = (fp16)((double)x1 + (double)x2 + (double)x3 + (double)x4);
				x1 = a31[pl]*w1; x2 = a32[pl]*w2; x3 = a33[pl]*w3; x4 = a34[pl]*w4;
				b3 = (fp16)((double)x1 + (double)x2 + (double)x3 + (double)x4);
				x1 = a41[pl]*w1; x2 = a42[pl]*w2; x3 = a43[pl]*w3; x4 = a44[pl]*w4;
				b4 = (fp16)((double)x1 + (double)x2 + (double)x3 + (double)x4);
				// commented original matlab copy
				//b1 = ((fp16)-0.5f*t1 + t2 + (fp16)-0.5f*t3)*a11[pl] + ((fp16)1.0f + (fp16)-2.5f*t2 + (fp16)1.5f*t3)*a12[pl] + ((fp16)0.5f*t1 + (fp16)2.0f*t2 + (fp16)-1.5f*t3)*a13[pl] + ((fp16)-0.5f*t2 +(fp16)0.5f*t3)*a14[pl];
				//b2 = ((fp16)-0.5f*t1 + t2 + (fp16)-0.5f*t3)*a21[pl] + ((fp16)1.0f + (fp16)-2.5f*t2 + (fp16)1.5f*t3)*a22[pl] + ((fp16)0.5f*t1 + (fp16)2.0f*t2 + (fp16)-1.5f*t3)*a23[pl] + ((fp16)-0.5f*t2 +(fp16)0.5f*t3)*a24[pl];
				//b3 = ((fp16)-0.5f*t1 + t2 + (fp16)-0.5f*t3)*a31[pl] + ((fp16)1.0f + (fp16)-2.5f*t2 + (fp16)1.5f*t3)*a32[pl] + ((fp16)0.5f*t1 + (fp16)2.0f*t2 + (fp16)-1.5f*t3)*a33[pl] + ((fp16)-0.5f*t2 +(fp16)0.5f*t3)*a34[pl];
				//b4 = ((fp16)-0.5f*t1 + t2 + (fp16)-0.5f*t3)*a41[pl] + ((fp16)1.0f + (fp16)-2.5f*t2 + (fp16)1.5f*t3)*a42[pl] + ((fp16)0.5f*t1 + (fp16)2.0f*t2 + (fp16)-1.5f*t3)*a43[pl] + ((fp16)-0.5f*t2 +(fp16)0.5f*t3)*a44[pl];

				t1 = (fp16)my;
				t2 = (fp16)(my*my);
				t3 = (fp16)(my*my*my);

				// coefficients emulate sumx
				if (bilinear != 0)
				{
					w1 = (fp16)0.0f;
					w2 = (fp16)1.0f-t1;
					w3 = t1;
					w4 = (fp16)0.0f;
				} else
				{
					// w1 = (fp16)(((double)t1*(double)-0.5f) + (double)t2 + ((double)t3*(double)-0.5f));
					// w2 = (fp16)(((double)1.0f) + ((double)t2*(double)-2.5f) + ((double)t3*(double)1.5f));
					// w3 = (fp16)(((double)t1*(double)0.5f) + ((double)t2*(double)2.0f) + ((double)t3*(double)-1.5f));
					// w4 = (fp16)(((double)t2*(double)-0.5f) + ((double)t3*(double)0.5f));

					t1_0_5 = (fp16)(t1*(fp16)+0.5f);
					t2_0_5 = (fp16)(t2*(fp16)+0.5f);
					t3_0_5 = (fp16)(t3*(fp16)+0.5f);
					w1 = (fp16)(-(double)t1_0_5        + (double)t2                                - (double)t3_0_5);
					w2 = (fp16)((double)1.0f           - (double)(t2*(fp16)2.0f) - (double)t2_0_5  + (double)t3 + (double)t3_0_5);
					w3 = (fp16)((double)t1_0_5         + (double)(t2*(fp16)2.0f)                   - (double)t3 - (double)t3_0_5);
					w4 = (fp16)(                       - (double)t2_0_5                            + (double)t3_0_5);
				}
				// multiplier stage
				x1 = b1*w1; x2 = b2*w2; x3 = b3*w3; x4 = b4*w4;
				// adder stage emulate sumx
				out[pl] = (fp16)((double)x1 + (double)x2 + (double)x3 + (double)x4);  
				// commented original matlab copy
				//out[pl] = ((fp16)-0.5f*t1 + t2 + (fp16)-0.5f*t3)*b1 + ((fp16)1.0f + (fp16)-2.5f*t2 + (fp16)1.5f*t3)*b2 + ((fp16)0.5f*t1 + (fp16)2.0f*t2 + (fp16)-1.5f*t3)*b3 + ((fp16)-0.5f*t2 +(fp16)0.5f*t3)*b4;

				// clamp values
				if (cb != 0)
				{
					if (out[pl] < clampLo) out[pl] = clampLo;
					if (out[pl] > clampHi) out[pl] = clampHi;
				}
			}

			if ( (i >= LOG_OFFSET) && (i<(LOG_OFFSET+LOG_SAMPLES)) )
			{
				
				for (pl=0; pl<planes; pl++)
				{
					fprintf(fout, "Plane %d\n", pl);
					fprintf(fout, "a11: 0x%X, a12: 0x%X, a13: 0x%X, a14: 0x%X \n", a11[pl].getPackedValue(), a12[pl].getPackedValue(), a13[pl].getPackedValue(), a14[pl].getPackedValue());
					fprintf(fout, "a21: 0x%X, a22: 0x%X, a23: 0x%X, a24: 0x%X \n", a21[pl].getPackedValue(), a22[pl].getPackedValue(), a23[pl].getPackedValue(), a24[pl].getPackedValue());
					fprintf(fout, "a31: 0x%X, a32: 0x%X, a33: 0x%X, a34: 0x%X \n", a31[pl].getPackedValue(), a32[pl].getPackedValue(), a33[pl].getPackedValue(), a34[pl].getPackedValue());
					fprintf(fout, "a41: 0x%X, a42: 0x%X, a43: 0x%X, a44: 0x%X \n", a41[pl].getPackedValue(), a42[pl].getPackedValue(), a43[pl].getPackedValue(), a44[pl].getPackedValue());
					fprintf(fout, "out: 0x%X \n", out[pl].getPackedValue());
				}
			}
		} 

		switch (pixel_type)
		{ 
		case BICUBIC_PIX_FORMAT_RGBX8888:
			{
			uint32_t *dst_addr = (uint32_t *)regMap->BIC_OUTPUT_BASE_ADDR;

			// account for hw f16.u8f conversion
			if (out[0] < 0.0f) out[0] = 0.0f;
			if (out[0] > 1.0f) out[0] = 1.0f;
			if (out[1] < 0.0f) out[1] = 0.0f;
			if (out[1] > 1.0f) out[1] = 1.0f;
			if (out[2] < 0.0f) out[2] = 0.0f;
			if (out[2] > 1.0f) out[2] = 1.0f;
			if (out[3] < 0.0f) out[3] = 0.0f;
			if (out[3] > 1.0f) out[3] = 1.0f;

			offset = (i/(int)regMap->BIC_OUTPUT_CHUNK_WIDTH)*(((int)regMap->BIC_OUTPUT_CHUNK_STRIDE>>2)-(int)regMap->BIC_OUTPUT_CHUNK_WIDTH) + i;
			if ( (outsider != 0) && ((regMap->BIC_CTRL&BICUBIC_BORDER_MODE_MASK) != 0) )
			{
			  dst_addr[offset] = border;
			} else 
			{
			  dst_addr[offset] = FP16_TO_U8F(out[0]) + (FP16_TO_U8F(out[1])<<8) + (FP16_TO_U8F(out[2])<<16) + (FP16_TO_U8F(out[3])<<24);
			}
			}
			break;
		case BICUBIC_PIX_FORMAT_U8F:
			{
			uint8_t *dst_addr = (uint8_t *)regMap->BIC_OUTPUT_BASE_ADDR;

			// account for hw f16.u8f conversion
			if (out[0] < 0.0f) out[0] = 0.0f;
			if (out[0] > 1.0f) out[0] = 1.0f;

			offset = (i/(int)regMap->BIC_OUTPUT_CHUNK_WIDTH)*(((int)regMap->BIC_OUTPUT_CHUNK_STRIDE>>0)-(int)regMap->BIC_OUTPUT_CHUNK_WIDTH) + i;
			if ( (outsider != 0) && ((regMap->BIC_CTRL&BICUBIC_BORDER_MODE_MASK) != 0) )
			{
			  dst_addr[offset] = (border&0xFF);
			} else 
			{
			  dst_addr[offset] = FP16_TO_U8F(out[0]); 
			}
			}
			break;
		case BICUBIC_PIX_FORMAT_FP16:
			{
			fp16 *dst_addr = (fp16 *)regMap->BIC_OUTPUT_BASE_ADDR;

			offset = (i/(int)regMap->BIC_OUTPUT_CHUNK_WIDTH)*(((int)regMap->BIC_OUTPUT_CHUNK_STRIDE>>1)-(int)regMap->BIC_OUTPUT_CHUNK_WIDTH) + i;
			if ( (outsider != 0) && ((regMap->BIC_CTRL&BICUBIC_BORDER_MODE_MASK) != 0) )
			{
			  dst_addr[offset].setPackedValue(border);
			} else 
			{
			  dst_addr[offset] = out[0]; 
			}
			}
			break;
		case BICUBIC_PIX_FORMAT_U16F:
			{
			uint16_t *dst_addr = (uint16_t *)regMap->BIC_OUTPUT_BASE_ADDR;

			// account for hw f16.u16f conversion
			if (out[0] < 0.0f) out[0] = 0.0f;
			if (out[0] > 1.0f) out[0] = 1.0f;

			offset = (i/(int)regMap->BIC_OUTPUT_CHUNK_WIDTH)*(((int)regMap->BIC_OUTPUT_CHUNK_STRIDE>>1)-(int)regMap->BIC_OUTPUT_CHUNK_WIDTH) + i;
			if ( (outsider != 0) && ((regMap->BIC_CTRL&BICUBIC_BORDER_MODE_MASK) != 0) )
			{
			  dst_addr[offset] = (border&0xFFFF);
			} else 
			{
			  dst_addr[offset] = FP16_TO_U16F(out[0]); 
			}
			}
			break;
		}
	}

	// Restore the rounding mode
#ifndef WIN32
	fesetround(rnd);
#else
	_controlfp_s(0, rnd, _MCW_RC);
#endif

	fclose(fout);
}

int BicubicFilt::BorderLogic(int x, int y, int shift) 
{
	int offset, offside, top;

	offside = 0;
	if (x<0.0f)
	{
		x=0;
		offside++;
	}
	if (x>=(int)regMap->BIC_INPUT_LINE_WIDTH)
	{
		x=(int)regMap->BIC_INPUT_LINE_WIDTH-1;
		offside++;
	}
	if (y<0.0f)
	{
		y=0;
		offside++;
	}
	if (y>=(int)regMap->BIC_INPUT_HEIGHT)
	{
		y=(int)regMap->BIC_INPUT_HEIGHT-1;
		offside++;
	}
	if ( ((regMap->BIC_CTRL&BICUBIC_BORDER_MODE_MASK) != 0) && (offside != 0) )
	{
		offset = -1;
	} else
	{
		top = (regMap->BIC_INPUT_TOP_ADDR-regMap->BIC_INPUT_BASE_ADDR)>>shift;

		// first calculate start of row
		offset = (y-(int)regMap->BIC_INPUT_OFFSET)*((int)regMap->BIC_INPUT_LINE_STRIDE>>shift);
		// apply circular buffer wrap around
		if (offset < 0) offset = top + offset;
		if (offset >= top) offset = offset - top;
		// complete offset by adding column offset
		offset += (x/(int)regMap->BIC_INPUT_CHUNK_WIDTH)*(((int)regMap->BIC_INPUT_CHUNK_STRIDE>>shift)-(int)regMap->BIC_INPUT_CHUNK_WIDTH)+x;
	}

	return offset;
}

void BicubicFilt::CommandPack(tBicubicReg * reg, uint8_t * link)
{
	// parse command structure
	link[0]  = (uint8_t)(reg->BIC_LINKED_LIST_ADDR>>0);
	link[1]  = (uint8_t)(reg->BIC_LINKED_LIST_ADDR>>8);
	link[2]  = (uint8_t)(reg->BIC_LINKED_LIST_ADDR>>16);
	link[3]  = (uint8_t)(reg->BIC_LINKED_LIST_ADDR>>24);
	link[4]  = (uint8_t)(reg->BIC_ID>>0);
	link[5]  = 0;
	link[6]  = (uint8_t)(reg->BIC_CTRL>>0);
	link[7]  = (uint8_t)(reg->BIC_CTRL>>8);
	link[8]  = (uint8_t)(reg->BIC_XY_PIXEL_LIST_BASE>>0);
	link[9]  = (uint8_t)(reg->BIC_XY_PIXEL_LIST_BASE>>8);
	link[10] = (uint8_t)(reg->BIC_XY_PIXEL_LIST_BASE>>16);
	link[11] = (uint8_t)(reg->BIC_XY_PIXEL_LIST_BASE>>24);
	link[12] = (uint8_t)(reg->BIC_LINE_LENGTH>>0);
	link[13] = (uint8_t)(reg->BIC_LINE_LENGTH>>8);
	link[14] = (uint8_t)(reg->BIC_LINE_LENGTH>>16);
	link[15] = 0;
	link[16] = (uint8_t)(reg->BIC_BORDER_PIXEL>>0);
	link[17] = (uint8_t)(reg->BIC_BORDER_PIXEL>>8);
	link[18] = (uint8_t)(reg->BIC_BORDER_PIXEL>>16);
	link[19] = (uint8_t)(reg->BIC_BORDER_PIXEL>>24);
	link[20] = (uint8_t)(reg->BIC_CLAMP_MIN>>0);
	link[21] = (uint8_t)(reg->BIC_CLAMP_MIN>>8);
	link[22] = (uint8_t)(reg->BIC_CLAMP_MAX>>0);
	link[23] = (uint8_t)(reg->BIC_CLAMP_MAX>>8);
	link[24] = (uint8_t)(reg->BIC_INPUT_BASE_ADDR>>0);
	link[25] = (uint8_t)(reg->BIC_INPUT_BASE_ADDR>>8);
	link[26] = (uint8_t)(reg->BIC_INPUT_BASE_ADDR>>16);
	link[27] = (uint8_t)(reg->BIC_INPUT_BASE_ADDR>>24);
	link[28] = (uint8_t)(reg->BIC_INPUT_TOP_ADDR>>0);
	link[29] = (uint8_t)(reg->BIC_INPUT_TOP_ADDR>>8);
	link[30] = (uint8_t)(reg->BIC_INPUT_TOP_ADDR>>16);
	link[31] = (uint8_t)(reg->BIC_INPUT_TOP_ADDR>>24);
	link[32] = (uint8_t)(reg->BIC_INPUT_CHUNK_WIDTH>>0);
	link[33] = (uint8_t)(reg->BIC_INPUT_CHUNK_WIDTH>>8);
	link[34] = (uint8_t)(reg->BIC_INPUT_CHUNK_WIDTH>>16);
	link[35] = 0;
	link[36] = (uint8_t)(reg->BIC_INPUT_CHUNK_STRIDE>>0);
	link[37] = (uint8_t)(reg->BIC_INPUT_CHUNK_STRIDE>>8);
	link[38] = (uint8_t)(reg->BIC_INPUT_CHUNK_STRIDE>>16);
	link[39] = 0;
	link[40] = (uint8_t)(reg->BIC_INPUT_LINE_WIDTH>>0);
	link[41] = (uint8_t)(reg->BIC_INPUT_LINE_WIDTH>>8);
	link[42] = (uint8_t)(reg->BIC_INPUT_LINE_WIDTH>>16);
	link[43] = 0;
	link[44] = (uint8_t)(reg->BIC_INPUT_LINE_STRIDE>>0);
	link[45] = (uint8_t)(reg->BIC_INPUT_LINE_STRIDE>>8);
	link[46] = (uint8_t)(reg->BIC_INPUT_LINE_STRIDE>>16);
	link[47] = 0;
	link[48] = (uint8_t)(reg->BIC_INPUT_OFFSET>>0);
	link[49] = (uint8_t)(reg->BIC_INPUT_OFFSET>>8);
	link[50] = (uint8_t)(reg->BIC_INPUT_HEIGHT>>0);
	link[51] = (uint8_t)(reg->BIC_INPUT_HEIGHT>>8);
	link[52] = (uint8_t)(reg->BIC_OUTPUT_BASE_ADDR>>0);
	link[53] = (uint8_t)(reg->BIC_OUTPUT_BASE_ADDR>>8);
	link[54] = (uint8_t)(reg->BIC_OUTPUT_BASE_ADDR>>16);
	link[55] = (uint8_t)(reg->BIC_OUTPUT_BASE_ADDR>>24);
	link[56] = (uint8_t)(reg->BIC_OUTPUT_CHUNK_WIDTH>>0);
	link[57] = (uint8_t)(reg->BIC_OUTPUT_CHUNK_WIDTH>>8);
	link[58] = (uint8_t)(reg->BIC_OUTPUT_CHUNK_WIDTH>>16);
	link[59] = 0;
	link[60] = (uint8_t)(reg->BIC_OUTPUT_CHUNK_STRIDE>>0);
	link[61] = (uint8_t)(reg->BIC_OUTPUT_CHUNK_STRIDE>>8);
	link[62] = (uint8_t)(reg->BIC_OUTPUT_CHUNK_STRIDE>>16);
	link[63] = 0;
}


void BicubicFilt::CommandUnpack(uint8_t *link, tBicubicReg *reg)
{
	// parse command structure
	reg->BIC_LINKED_LIST_ADDR    = link[0]  + (link[1]<<8)  + (link[2]<<16)  + (link[3]<<24);
	reg->BIC_ID                  = link[4];
	reg->BIC_CTRL                = link[6]  + (link[7]<<8);
	reg->BIC_XY_PIXEL_LIST_BASE  = link[8]  + (link[9]<<8)  + (link[10]<<16) + (link[11]<<24);
	reg->BIC_LINE_LENGTH         = link[12] + (link[13]<<8) + (link[14]<<16);
	reg->BIC_BORDER_PIXEL        = link[16] + (link[17]<<8) + (link[18]<<16) + (link[19]<<24);
	reg->BIC_CLAMP_MIN           = link[20] + (link[21]<<8);
	reg->BIC_CLAMP_MAX           = link[22] + (link[23]<<8);
	reg->BIC_INPUT_BASE_ADDR     = link[24] + (link[25]<<8) + (link[26]<<16) + (link[27]<<24);
	reg->BIC_INPUT_TOP_ADDR      = link[28] + (link[29]<<8) + (link[30]<<16) + (link[31]<<24);
	reg->BIC_INPUT_CHUNK_WIDTH   = link[32] + (link[33]<<8) + (link[34]<<16);
	reg->BIC_INPUT_CHUNK_STRIDE  = link[36] + (link[37]<<8) + (link[38]<<16);
	reg->BIC_INPUT_LINE_WIDTH    = link[40] + (link[41]<<8) + (link[42]<<16);
	reg->BIC_INPUT_LINE_STRIDE   = link[44] + (link[45]<<8) + (link[46]<<16);
	reg->BIC_INPUT_OFFSET        = link[48] + (link[49]<<8);
	reg->BIC_INPUT_HEIGHT        = link[50] + (link[51]<<8);
	reg->BIC_OUTPUT_BASE_ADDR    = link[52] + (link[53]<<8) + (link[54]<<16) + (link[55]<<24);
	reg->BIC_OUTPUT_CHUNK_WIDTH  = link[56] + (link[57]<<8) + (link[58]<<16);
	reg->BIC_OUTPUT_CHUNK_STRIDE = link[60] + (link[61]<<8) + (link[62]<<16);
}

tBicubicReg * BicubicFilt::getReg()
{
    return regMap;
}

void BicubicFilt::SetReg(tBicubicReg *pReg)
{
    regMap = pReg;
}
