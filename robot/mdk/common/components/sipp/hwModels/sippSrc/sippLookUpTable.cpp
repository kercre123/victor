// -----------------------------------------------------------------------------
// Copyright (C) 2012 Movidius Ltd. All rights reserved
//
// Company          : Movidius
// Author           : Razvan Delibasa (razvan.delibasa@movidius.com)
// Description      : SIPP Accelerator HW model - Look up table
//
//
// -----------------------------------------------------------------------------

#include "sippLookUpTable.h"
#include "sippDebug.h"

#include <stdlib.h>
#ifdef WIN32
#include <search.h>
#else
#include <cmath>
#endif
#include <iostream>
#include <iomanip>
#include <string>
#include <cstring>
#include <climits>
#include <math.h>


LookUpTable::LookUpTable(SippIrq *pObj, int id, int k, std::string name) :
	kernel_size (k),
	loader("input", Read) {

		// pObj, name and id are protected members of SippBaseFilter
		SetPIrqObj (pObj);
		SetName (name);
		SetId (id);
		SetKernelLinesNo (kernel_size);

		perform_interp = true;
		crt_output_sp = 0;
}

void LookUpTable::SetInterpolationModeEnable(int val, int reg) {
	lut_params[reg].interp_mode_en = val;
}

void LookUpTable::SetChannelModeEnable(int val, int reg) {
	lut_params[reg].channel_mode_en = val;

	if(lut_params[reg].channel_mode_en)
		interp_offset = 2 << 2;
	else
		interp_offset = 2;
}

void LookUpTable::SetIntegerWidth(int val, int reg) {
	if(GetInterpolationModeEnable() == DISABLED) {
		if(val < 8 || val > 16) {
			std::cerr << "Invalid integer width: must be 8 <= int_width <= 16." << std::endl;
			abort();
		}
		lut_params[reg].int_width = val;
	}
}

void LookUpTable::SetNumberOfLUTs(int val, int reg) {
	lut_params[reg].luts_no = val;
}

void LookUpTable::SetNumberOfChannels(int val, int reg) {
	lut_params[reg].channels_no = val;
}

void LookUpTable::SetLutLoadEnable(int val) {
	lut_load_en = val;
}

void LookUpTable::SetApbAccessEnable(int val) {
	apb_access_en = val;
}

void LookUpTable::SetRegions7_0(int val, int reg) {
	int rng_size;
	for(int r = 0; r < 8; r++, val >>= 4) {
		rng_size = val & 0xf;
		if(rng_size < 0 || rng_size > 10) {
			std::cerr << "Invalid range size: must be 0 <= n <= 10." << std::endl;
			abort();
		}
		lut_params[reg].regions[r] = rng_size;
	}
}

void LookUpTable::SetRegions15_8(int val, int reg) {
	int rng_size;
	for(int r = 8; r < 16; r++, val >>= 4) {
		rng_size = val & 0xf;
		if(rng_size < 0 || rng_size > 10) {
			std::cerr << "Invalid range size: must be 0 <= n <= 10." << std::endl;
			abort();
		}
		lut_params[reg].regions[r] = rng_size;
	}
}

void LookUpTable::SetLutValue(int val) {
	if(GetInterpolationModeEnable() == DISABLED)
		lut_setintval = val;
	else
		lut_setfpval.setPackedValue((uint16_t)val);
}

void LookUpTable::SetRamAddress(int val) {
	ram_addr = val;
}

void LookUpTable::SetRamInstance(int val) {
	ram_inst = val;
}

void LookUpTable::SetLutRW(int val) {
	rw = val;

	if(!apb_access_en) {
		std::cerr << "APB access is not enabled." << std::endl;
	} else {
		int lut_addr = loader.GetBase() + (((ram_addr << 3) + ram_inst) << 1);
		if(rw == Read) {
			if(GetInterpolationModeEnable() == DISABLED) {
				lut_getintval = *(uint16_t *)lut_addr;
			} else {
				lut_getfpval  = *(fp16     *)lut_addr;
			}
		} else {
			if(GetInterpolationModeEnable() == DISABLED) {
				*(uint16_t *)lut_addr = lut_setintval;
			} else {
				*(fp16     *)lut_addr = lut_setfpval;
			}
		}
	}
}

int LookUpTable::SelectRangeFP16(fp16 val) {
	return (val.getPackedValue() >> 10) & 0x1f;
}

int LookUpTable::SelectRangeINT(int val) {
	switch(int_width) {
	case 8  : 
		return (val >>  4) & 0xf;
	case 9  : 		    
		return (val >>  5) & 0xf;
	case 10 : 		    
		return (val >>  6) & 0xf;
	case 11 : 		    
		return (val >>  7) & 0xf;
	case 12 :		    
		return (val >>  8) & 0xf;
	case 13 : 		    
		return (val >>  9) & 0xf;
	case 14 : 
		return (val >> 10) & 0xf;
	case 15 : 
		return (val >> 11) & 0xf;
	case 16 : 
		return (val >> 12) & 0xf;
	default :
		// The sole purpose of this is to eliminate a warning since the 
		// correct value of the integer width is handled when set
		return -1;
	}
}

int LookUpTable::ComputeRawOffsetFP16(fp16 val) {
	return val.getPackedValue() & 0x3ff;
}

int LookUpTable::ComputeRawOffsetINT(int val) {
	switch(int_width) {
	case 8  : 
		return (val & 0xf  ) << 6;
	case 9  : 		       
		return (val & 0x1f ) << 5;
	case 10 : 		       
		return (val & 0x3f ) << 4;
	case 11 : 			   
		return (val & 0x7f ) << 3;
	case 12 : 		       
		return (val & 0xff ) << 2;
	case 13 : 		    
		return (val & 0x1ff) << 1;
	case 14 : 
		return (val & 0x3ff) << 0;
	case 15 : 		    
		return (val & 0x7fe) >> 1;
	case 16 : 		    
		return (val & 0xffc) >> 2;
	default :
		// The sole purpose of this is to eliminate a warning since the 
		// correct value of the integer width is handled when set
		return -1;
	}
}

int LookUpTable::ComputeAddrOffsetFP16(fp16 val) {
	int range_size_ind = SelectRangeFP16(val);
	int raw_offset = ComputeRawOffsetFP16(val);

	switch(regions[range_size_ind]) {
	case 0  :
		return 0;
	case 1  :
		return (raw_offset & 0x200) >> 9;
	case 2  :
		return (raw_offset & 0x300) >> 8;
	case 3  :
		return (raw_offset & 0x380) >> 7;
	case 4  :
		return (raw_offset & 0x3c0) >> 6;
	case 5  :
		return (raw_offset & 0x3e0) >> 5;
	case 6  :
		return (raw_offset & 0x3f0) >> 4;
	case 7  :
		return (raw_offset & 0x3f8) >> 3;
	case 8  :
		return (raw_offset & 0x3fc) >> 2;
	case 9  :
		return (raw_offset & 0x3fe) >> 1;
	case 10 :
		return (raw_offset & 0x3ff) >> 0;
	default :
		// The sole purpose of this is to eliminate a warning since the correct 
		// value of the programmable range size is handled when set
		return -1;
	}
}

int LookUpTable::ComputeAddrOffsetINT(int val) {
	int range_size_ind = SelectRangeINT(val);
	int raw_offset = ComputeRawOffsetINT(val);

	switch(regions[range_size_ind]) {
	case 0  :
		return 0;
	case 1  :
		return (raw_offset & 0x200) >> 9;
	case 2  :
		return (raw_offset & 0x300) >> 8;
	case 3  :
		return (raw_offset & 0x380) >> 7;
	case 4  :
		return (raw_offset & 0x3c0) >> 6;
	case 5  :
		return (raw_offset & 0x3e0) >> 5;
	case 6  :
		return (raw_offset & 0x3f0) >> 4;
	case 7  :
		return (raw_offset & 0x3f8) >> 3;
	case 8  :
		return (raw_offset & 0x3fc) >> 2;
	case 9  :
		return (raw_offset & 0x3fe) >> 1;
	case 10 :
		return (raw_offset & 0x3ff) >> 0;
	default :
		// The sole purpose of this is to eliminate a warning since the correct 
		// value of the programmable range size is handled when set
		return -1;
	}
}

int LookUpTable::ComputeIN(fp16 val) {
	int range_size_ind = SelectRangeFP16(val);
	int raw_offset = ComputeRawOffsetFP16(val);
	
	switch(regions[range_size_ind]) {
	case 0  :
		return (raw_offset & 0x3ff) << 0;
	case 1  :
		return (raw_offset & 0x1ff) << 1;
	case 2  :
		return (raw_offset & 0x0ff) << 2;
	case 3  :
		return (raw_offset & 0x07f) << 3;
	case 4  :
		return (raw_offset & 0x03f) << 4;
	case 5  :
		return (raw_offset & 0x01f) << 5;
	case 6  :
		return (raw_offset & 0x00f) << 6;
	case 7  :
		return (raw_offset & 0x007) << 7;
	case 8  :
		return (raw_offset & 0x003) << 8;
	case 9  :
		return (raw_offset & 0x001) << 9;
	case 10 :
		return (raw_offset & 0x000)     ;
	default :
		// The sole purpose of this is to eliminate a warning since the correct 
		// value of the programmable range size is handled when set
		return -1;
	}
}

size_t LookUpTable::ComputeLutAddrFP16(fp16 val) {
	int range_size_ind = SelectRangeFP16(val);
	int range_offset = 0;
	int plane_offset = (crt_plane % (luts_no + 1)) * GetLutSize();
	fp16 *base = (fp16 *)loader.GetBase();
	int first_value_offset;

	for(int r = 0; r < range_size_ind; r++)
		range_offset += 1 << regions[r];

	if((first_value_offset = range_offset + ComputeAddrOffsetFP16(val)) == GetLutSize() - 1)
		perform_interp = false;
	else
		perform_interp = true;

	return (size_t)(base + first_value_offset + plane_offset);
}

size_t LookUpTable::ComputeLutAddrINT(int val) {
	int range_size_ind = SelectRangeINT(val);
	int range_offset = 0;
	int plane_offset = (crt_plane % (luts_no + 1)) * GetLutSize();
	uint16_t *base = (uint16_t *)loader.GetBase();

	for(int r = 0; r < range_size_ind; r++)
		range_offset += 1 << regions[r];

	return (size_t)(base + range_offset + ComputeAddrOffsetINT(val) + plane_offset);
}

int LookUpTable::ComputeChannelAddr(int addr) {
	int base = loader.GetBase();
	int channel_offset = addr - base;
	int actual_addr = base + (((channel_offset << 1) + crt_channel) << 1);
	return actual_addr;
}

int LookUpTable::GetLutSize() {
	int size = 0;
	for(int r = 0; r < 16; r++)
		size += 1 << regions[r];
	return size;
}

int LookUpTable::GetLufSize() {
	return loader.GetNumPlanes() * GetLutSize();
}

void LookUpTable::SelectParameters(void) {
	SippBaseFilt::SelectParameters();
	loader.SelectBufferParameters();
	interp_mode_en  = lut_params[reg].interp_mode_en;
	channel_mode_en = lut_params[reg].channel_mode_en;
	int_width       = lut_params[reg].int_width;
	luts_no         = lut_params[reg].luts_no;
	channels_no     = lut_params[reg].channels_no;
	for(int r = 0; r < 16; r++)
		regions[r] = lut_params[reg].regions[r];
}

void LookUpTable::SetUpAndRun(void) {
	static int count = 0;

	// Can't run if not enabled!
	if (!IsEnabled())
		return;

	uint8_t  ***split_in_ptr_u8  = 0;
	uint8_t  ***      in_ptr_u8  = 0;
	uint16_t ***split_in_ptr_u16 = 0;
	uint16_t ***      in_ptr_u16 = 0;
	fp16     ***split_in_ptr_f   = 0;
	fp16     ***      in_ptr_f   = 0;
	uint8_t  **      out_ptr_u8  = 0;
	uint16_t **      out_ptr_u16 = 0;
	fp16     **      out_ptr_f   = 0;

	void ***in_ptr;
	void **out_ptr;

	// Select either default or shadow registers
	SelectParameters();

	// Assign first and second dimensions of pointers to
	// input planes and split input lines, respectively,
	// and allocate pointers to hold packed incoming data
	if(GetInterpolationModeEnable() == DISABLED) {
		if(in.GetFormat() == SIPP_FORMAT_8BIT) {
			split_in_ptr_u8  = new uint8_t  **[channels_no + 1];
			      in_ptr_u8  = new uint8_t  **[channels_no + 1];
			for(int ch = 0; ch <= channels_no; ch++) {
				split_in_ptr_u8 [ch] = new uint8_t  *[kernel_size];
				      in_ptr_u8 [ch] = new uint8_t  *[kernel_size];
				for(int ln = 0; ln < kernel_size; ln++)
					in_ptr_u8 [ch][ln] = new uint8_t [width];
			}
			in_ptr = (void ***)in_ptr_u8;
		} else {
			split_in_ptr_u16 = new uint16_t **[channels_no + 1];
			      in_ptr_u16 = new uint16_t **[channels_no + 1];
			for(int ch = 0; ch <= channels_no; ch++) {
				split_in_ptr_u16[ch] = new uint16_t *[kernel_size];
				      in_ptr_u16[ch] = new uint16_t *[kernel_size];
				for(int ln = 0; ln < kernel_size; ln++)
					in_ptr_u16[ch][ln] = new uint16_t[width];
			}
			in_ptr = (void ***)in_ptr_u16;
		}
	} else {
		split_in_ptr_f = new fp16 **[channels_no + 1];
		      in_ptr_f = new fp16 **[channels_no + 1];
		for(int ch = 0; ch <= channels_no; ch++) {
			split_in_ptr_f[ch] = new fp16 *[kernel_size];
			      in_ptr_f[ch] = new fp16 *[kernel_size];
			for(int ln = 0; ln < kernel_size; ln++)
				in_ptr_f[ch][ln] = new fp16[width];
		}
		in_ptr = (void ***)in_ptr_f;
	}

	// Assign first dimension of pointers to output planes
	// and prepare filter output buffer line pointers
	if(GetInterpolationModeEnable() == DISABLED) {
		if(out.GetFormat() == SIPP_FORMAT_8BIT) {
			out_ptr_u8  = new uint8_t  *[out.GetNumPlanes() + 1];
			setOutputLines(out_ptr_u8, out.GetNumPlanes());
			out_ptr = (void **)out_ptr_u8;
		} else {
			out_ptr_u16 = new uint16_t *[out.GetNumPlanes() + 1];
			setOutputLines(out_ptr_u16, out.GetNumPlanes());
			out_ptr = (void **)out_ptr_u16;
		}
	} else {
		if(out.GetFormat() == SIPP_FORMAT_8BIT) {
			out_ptr_u8  = new uint8_t  *[out.GetNumPlanes() + 1];
			setOutputLines(out_ptr_u8, out.GetNumPlanes());
			out_ptr = (void **)out_ptr_u8;
		} else {
			out_ptr_f   = new fp16     *[out.GetNumPlanes() + 1];
			setOutputLines(out_ptr_f , out.GetNumPlanes());
			out_ptr = (void **)out_ptr_f;
		}
	}
   
	if (Verbose())
		std::cout << name << " run " << std::setbase(10) << count++ << std::endl;

	// Filter (channels_no + 1) lines simultaneously
	for (crt_plane = 0; crt_plane <= in.GetNumPlanes(); crt_plane++) {
		// Compute the planar offset
		int offset = crt_plane * (channels_no + 1);

		// Prepare filter buffer line pointers then run
		if(GetInterpolationModeEnable() == DISABLED) {
			if(in.GetFormat() == SIPP_FORMAT_8BIT)
				for(int ch = 0; ch <= channels_no; ch++) {
					getKernelLines(split_in_ptr_u8 [ch], kernel_size, offset + ch);
					PackInputLines(split_in_ptr_u8 [ch], kernel_size, in_ptr_u8 [ch]);
				}
			else
				for(int ch = 0; ch <= channels_no; ch++) {
					getKernelLines(split_in_ptr_u16[ch], kernel_size, offset + ch);
					PackInputLines(split_in_ptr_u16[ch], kernel_size, in_ptr_u16[ch]);
				}
		} else {
			for(int ch = 0; ch <= channels_no; ch++) {
				getKernelLines(split_in_ptr_f[ch], kernel_size, offset + ch);
				PackInputLines(split_in_ptr_f[ch], kernel_size, in_ptr_f[ch]);
			}
		}
		
		Run(in_ptr, out_ptr);
		crt_output_sp += channels_no + 1;
	}
	// Clear start bit, update buffer fill levels, input frame 
	// line index and reset the plane starting point
	in.ClrStartBit();
	UpdateBuffersSync();
	IncLineIdx();
	crt_output_sp = 0;

	// Trigger end of frame interrupt
	if (EndOfFrame()) {
		EndOfFrameIrq();
	}

	if(GetInterpolationModeEnable() == DISABLED) {
		if(in.GetFormat() == SIPP_FORMAT_8BIT) {
			for(int c = 0; c <= channels_no; c++) {
				delete[] split_in_ptr_u8 [c];
				delete[]       in_ptr_u8 [c];
			}
			delete[] split_in_ptr_u8 ;
			delete[]       in_ptr_u8 ;
		} else {
			for(int c = 0; c <= channels_no; c++) {
				delete[] split_in_ptr_u16[c];
				delete[]       in_ptr_u16[c];
			}
			delete[] split_in_ptr_u16;
			delete[]       in_ptr_u16;
		}
		if(out.GetFormat() == SIPP_FORMAT_8BIT)
			delete[] out_ptr_u8;
		else
			delete[] out_ptr_u16;
	} else {
		for(int c = 0; c <= channels_no; c++) {
			delete[] split_in_ptr_f[c];
			delete[]       in_ptr_f[c];
		}
		delete[] split_in_ptr_f;
		delete[]       in_ptr_f;
		if(out.GetFormat() == SIPP_FORMAT_8BIT)
			delete[] out_ptr_u8;
		else
			delete[] out_ptr_f;
	}
}

void LookUpTable::TryRun(void) {
	static int count = 0;

	// Can't run if not enabled!
	if (!IsEnabled())
		return;

	uint8_t  ***split_in_ptr_u8  = 0;
	uint8_t  ***      in_ptr_u8  = 0;
	uint16_t ***split_in_ptr_u16 = 0;
	uint16_t ***      in_ptr_u16 = 0;
	fp16     ***split_in_ptr_f   = 0;
	fp16     ***      in_ptr_f   = 0;
	uint8_t  **      out_ptr_u8  = 0;
	uint16_t **      out_ptr_u16 = 0;
	fp16     **      out_ptr_f   = 0;

	void ***in_ptr;
	void **out_ptr;

	// Try to enter critical section...
	if (!cs.TryEnter())
		return;

	// Select either default or shadow registers
	SelectParameters();

	// Assign first and second dimensions of pointers to
	// input planes and split input lines, respectively,
	// and allocate pointers to hold packed incoming data
	if(GetInterpolationModeEnable() == DISABLED) {
		if(in.GetFormat() == SIPP_FORMAT_8BIT) {
			split_in_ptr_u8  = new uint8_t  **[channels_no + 1];
			      in_ptr_u8  = new uint8_t  **[channels_no + 1];
			for(int ch = 0; ch <= channels_no; ch++) {
				split_in_ptr_u8 [ch] = new uint8_t  *[kernel_size];
				      in_ptr_u8 [ch] = new uint8_t  *[kernel_size];
				for(int ln = 0; ln < kernel_size; ln++)
					in_ptr_u8 [ch][ln] = new uint8_t [width];
			}
			in_ptr = (void ***)in_ptr_u8;
		} else {
			split_in_ptr_u16 = new uint16_t **[channels_no + 1];
			      in_ptr_u16 = new uint16_t **[channels_no + 1];
			for(int ch = 0; ch <= channels_no; ch++) {
				split_in_ptr_u16[ch] = new uint16_t *[kernel_size];
				      in_ptr_u16[ch] = new uint16_t *[kernel_size];
				for(int ln = 0; ln < kernel_size; ln++)
					in_ptr_u16[ch][ln] = new uint16_t[width];
			}
			in_ptr = (void ***)in_ptr_u16;
		}
	} else {
		split_in_ptr_f = new fp16 **[channels_no + 1];
		      in_ptr_f = new fp16 **[channels_no + 1];
		for(int ch = 0; ch <= channels_no; ch++) {
			split_in_ptr_f[ch] = new fp16 *[kernel_size];
			      in_ptr_f[ch] = new fp16 *[kernel_size];
			for(int ln = 0; ln < kernel_size; ln++)
				in_ptr_f[ch][ln] = new fp16[width];
		}
		in_ptr = (void ***)in_ptr_f;
	}

	// Assign first dimension of pointers to output planes
	if(GetInterpolationModeEnable() == DISABLED) {
		if(out.GetFormat() == SIPP_FORMAT_8BIT) {
			out_ptr_u8  = new uint8_t  *[out.GetNumPlanes() + 1];
			out_ptr = (void **)out_ptr_u8;
		} else {
			out_ptr_u16 = new uint16_t *[out.GetNumPlanes() + 1];
			out_ptr = (void **)out_ptr_u16;
		}
	} else {
		if(out.GetFormat() == SIPP_FORMAT_8BIT) {
			out_ptr_u8  = new uint8_t  *[out.GetNumPlanes() + 1];
			out_ptr = (void **)out_ptr_u8;
		} else {
			out_ptr_f  = new fp16      *[out.GetNumPlanes() + 1];
			out_ptr = (void **)out_ptr_f;
		}
	}

	// Following code in critical section since TryRun() may be invoked by different
	// threads of execution, depending on whether IncInputBufferFillLevel() or 
	// DecOutputBufferFillLevel() was called...
	while (CanRun(kernel_size)) {
		if (Verbose())
			std::cout << name << " run " << std::setbase(10) << count++ << std::endl;

		// Prepare filter output buffer line pointers
		if(GetInterpolationModeEnable() == DISABLED)
			if(out.GetFormat() == SIPP_FORMAT_8BIT)
				setOutputLines(out_ptr_u8 , out.GetNumPlanes());
			else
				setOutputLines(out_ptr_u16, out.GetNumPlanes());
		else
			if(out.GetFormat() == SIPP_FORMAT_8BIT)
				setOutputLines(out_ptr_u8 , out.GetNumPlanes());
			else
				setOutputLines(out_ptr_f  , out.GetNumPlanes());

		// Filter (channels_no + 1) lines simultaneously
		for (crt_plane = 0; crt_plane <= in.GetNumPlanes(); crt_plane++) {
			// Compute the planar offset
			int offset = crt_plane * (channels_no + 1);

			// Prepare filter buffer line pointers then run
			if(GetInterpolationModeEnable() == DISABLED) {
				if(in.GetFormat() == SIPP_FORMAT_8BIT)
					for(int ch = 0; ch <= channels_no; ch++) {
						getKernelLines(split_in_ptr_u8 [ch], kernel_size, offset + ch);
						PackInputLines(split_in_ptr_u8 [ch], kernel_size, in_ptr_u8 [ch]);
					}
				else
					for(int ch = 0; ch <= channels_no; ch++) {
						getKernelLines(split_in_ptr_u16[ch], kernel_size, offset + ch);
						PackInputLines(split_in_ptr_u16[ch], kernel_size, in_ptr_u16[ch]);
					}
			} else {
				for(int ch = 0; ch <= channels_no; ch++) {
					getKernelLines(split_in_ptr_f[ch], kernel_size, offset + ch);
					PackInputLines(split_in_ptr_f[ch], kernel_size, in_ptr_f[ch]);
				}
			}

			Run(in_ptr, out_ptr);
			crt_output_sp += channels_no + 1;
		}
		// Update buffer fill levels, input frame line 
		// index and reset the plane starting point
		UpdateBuffers();
		IncLineIdx();
		crt_output_sp = 0;

		// At the end of the frame
		if (EndOfFrame()) {
			// Trigger end of frame interrupt
			EndOfFrameIrq();

			// Disable the filter unless the
			// input buffer is circular
			if(in.GetBuffLines() == 0) {
				Disable();
				break;
			}
		}
	}
	cs.Leave();

	if(GetInterpolationModeEnable() == DISABLED) {
		if(in.GetFormat() == SIPP_FORMAT_8BIT) {
			for(int c = 0; c <= channels_no; c++) {
				delete[] split_in_ptr_u8 [c];
				delete[]       in_ptr_u8 [c];
			}
			delete[] split_in_ptr_u8 ;
			delete[]       in_ptr_u8 ;
		} else {
			for(int c = 0; c <= channels_no; c++) {
				delete[] split_in_ptr_u16[c];
				delete[]       in_ptr_u16[c];
			}
			delete[] split_in_ptr_u16;
			delete[]       in_ptr_u16;
		}
		if(out.GetFormat() == SIPP_FORMAT_8BIT)
			delete[] out_ptr_u8;
		else
			delete[] out_ptr_u16;
	} else {
		for(int c = 0; c <= channels_no; c++) {
			delete[] split_in_ptr_f[c];
			delete[]       in_ptr_f[c];
		}
		delete[] split_in_ptr_f;
		delete[]       in_ptr_f;
		if(out.GetFormat() == SIPP_FORMAT_8BIT)
			delete[] out_ptr_u8;
		else
			delete[] out_ptr_f;
	}
}

void LookUpTable::Run(void ***in_ptr, void **out_ptr) {
	uint8_t  ***      src_u8  = 0;
	uint16_t ***      src_u16 = 0;
	fp16     ***      src_f   = 0;
	uint8_t  **       dst_u8  = 0;
	uint8_t  **packed_dst_u8  = 0;
	uint16_t **       dst_u16 = 0;
	uint16_t **packed_dst_u16 = 0;
	fp16     **       dst_f   = 0;
	fp16     **packed_dst_f   = 0;

	// Allocating and setting input/output pointers
	// and allocating temporary packed planar output buffers
	if(GetInterpolationModeEnable() == DISABLED) {
		if(in.GetFormat() == SIPP_FORMAT_8BIT) {
			src_u8  = new uint8_t  **[channels_no + 1];
			for(int c = 0; c <= channels_no; c++) {
				src_u8[c]  = new uint8_t  *[kernel_size];
				for (int i = 0; i < kernel_size; i++)
					src_u8[c][i]  = (uint8_t  *)in_ptr[c][i];
			}
		} else {
			src_u16 = new uint16_t **[channels_no + 1];
			for(int c = 0; c <= channels_no; c++) {
				src_u16[c] = new uint16_t *[kernel_size];
				for (int i = 0; i < kernel_size; i++)
					src_u16[c][i] = (uint16_t *)in_ptr[c][i];
			}
		}
		if(out.GetFormat() == SIPP_FORMAT_8BIT) {
			       dst_u8  = new uint8_t  *[channels_no + 1];
			packed_dst_u8  = new uint8_t  *[channels_no + 1];
			for(int lpl = 0, pl = crt_output_sp; pl <= crt_output_sp + channels_no; pl++) {
				       dst_u8 [lpl  ]  = (uint8_t  *)out_ptr[pl];
				packed_dst_u8 [lpl++]  = new uint8_t [width];
			}
		} else {
			       dst_u16 = new uint16_t *[channels_no + 1];
			packed_dst_u16 = new uint16_t *[channels_no + 1];
			for(int lpl = 0, pl = crt_output_sp; pl <= crt_output_sp + channels_no; pl++) {
				       dst_u16[lpl  ]  = (uint16_t *)out_ptr[pl];
				packed_dst_u16[lpl++]  = new uint16_t[width];
			}
		}
	} else {
		src_f = new fp16 **[channels_no + 1];
		for(int c = 0; c <= channels_no; c++) {
			src_f[c] = new fp16 *[kernel_size];
			for (int i = 0; i < kernel_size; i++)
				src_f[c][i] = (fp16 *)in_ptr[c][i];
		}
		if(out.GetFormat() == SIPP_FORMAT_8BIT) {
			       dst_u8 = new uint8_t *[channels_no + 1];
			packed_dst_u8 = new uint8_t *[channels_no + 1];
			for(int lpl = 0, pl = crt_output_sp; pl <= crt_output_sp + channels_no; pl++) {
				       dst_u8[lpl  ]  = (uint8_t *)out_ptr[pl];
				packed_dst_u8[lpl++]  = new uint8_t[width];
			}
		} else {
			       dst_f  = new fp16    *[channels_no + 1];
			packed_dst_f  = new fp16    *[channels_no + 1];
			for(int lpl = 0, pl = crt_output_sp; pl <= crt_output_sp + channels_no; pl++) {
				       dst_f [lpl  ]  = (fp16    *)out_ptr[pl];
				packed_dst_f [lpl++]  = new fp16   [width];
			}
		}
	}

	int x = 0;
	int lut_addr;

	while (x < width) {
		for(crt_channel = 0; crt_channel <= channels_no; crt_channel++) {
			// Get the LUT value and store it based on the LUT mode
			if(GetInterpolationModeEnable() == DISABLED) {
				if(in.GetFormat() == SIPP_FORMAT_8BIT)
					lut_addr = ComputeLutAddrINT(src_u8 [crt_channel][0][x]);
				else
					lut_addr = ComputeLutAddrINT(src_u16[crt_channel][0][x]);

				// Adjust the address with respect to the current channel
				if(GetChannelModeEnable() == ENABLED)
					lut_addr = ComputeChannelAddr(lut_addr);

				// Fetch the 16bit LUT value
				uint16_t lut_value_i = *(uint16_t *)lut_addr;

				// Store the LUT value based on the output buffer's format
				if(out.GetFormat() == SIPP_FORMAT_8BIT)
					packed_dst_u8 [crt_channel][x] = (uint8_t)lut_value_i;
				else
					packed_dst_u16[crt_channel][x] =          lut_value_i;
			} else {
				lut_addr = ComputeLutAddrFP16(src_f[crt_channel][0][x]);

				// Adjust the address with respect to the current channel
				if(GetChannelModeEnable() == ENABLED)
					lut_addr = ComputeChannelAddr(lut_addr);

				// Fetch the first 16bit LUT value
				fp16 lut_value_f = *(fp16 *)(lut_addr);

				// Fetch the second 16bit LUT value and perform 
				// interpolation unless at the end of the LUT
				if(perform_interp) {
					fp16 next_value = *(fp16 *)(lut_addr + interp_offset);

					// Determine interpolation numerator
					int interp_numerator = ComputeIN(src_f[crt_channel][0][x]);

					// Interpolate between current and next value
					lut_value_f = Interpolate(interp_numerator / (float)(1 << 10), 0.0f, 1.0f, lut_value_f, next_value);
				}
				if(out.GetFormat() == SIPP_FORMAT_8BIT)
					packed_dst_u8[crt_channel][x] = FP16_TO_U8F(lut_value_f);
				else
					packed_dst_f [crt_channel][x] =             lut_value_f;
			}
		}
		x++;
	}

	if(GetInterpolationModeEnable() == DISABLED) {
		if(out.GetFormat() == SIPP_FORMAT_8BIT) {
			for(crt_channel = 0; crt_channel <= channels_no; crt_channel++)
				SplitOutputLine(packed_dst_u8 [crt_channel], width, dst_u8 [crt_channel]);
			delete[]        dst_u8 ;
			delete[] packed_dst_u8 ;
		} else {
			for(crt_channel = 0; crt_channel <= channels_no; crt_channel++)
				SplitOutputLine(packed_dst_u16[crt_channel], width, dst_u16[crt_channel]);
			delete[]        dst_u16;
			delete[] packed_dst_u16;
		}

		if(in.GetFormat() == SIPP_FORMAT_8BIT) {
			for(int c = 0; c <= channels_no; c++)
				delete[] src_u8 [c];
			delete[] src_u8;
		} else {
			for(int c = 0; c <= channels_no; c++)
				delete[] src_u16[c];
			delete[] src_u16;
		}
	} else {
		if(out.GetFormat() == SIPP_FORMAT_8BIT) {
			for(crt_channel = 0; crt_channel <= channels_no; crt_channel++)
				SplitOutputLine(packed_dst_u8[crt_channel], width, dst_u8[crt_channel]);
			delete[]        dst_u8;
			delete[] packed_dst_u8;
		} else {
			for(crt_channel = 0; crt_channel <= channels_no; crt_channel++)
				SplitOutputLine(packed_dst_f [crt_channel], width, dst_f [crt_channel]);
			delete[]        dst_f ;
			delete[] packed_dst_f ;
		}

		for(int c = 0; c <= channels_no; c++)
			delete[] src_f[c];
		delete[] src_f;
	}
}

// Not used in this particular filter as it has his own version of Run
void *LookUpTable::Run(void **in_ptr, void *out_ptr)
{
	return NULL;
}
