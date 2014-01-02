// -----------------------------------------------------------------------------
// Copyright (C) 2012 Movidius Ltd. All rights reserved
//
// Company          : Movidius
// Author           : Rick Richmond (richard.richmond@movidius.com)
// Description      : SIPP Accelerator HW buffer model
// -----------------------------------------------------------------------------

#include <iostream>
#include <iomanip>
#include <cstdlib>
#include <cstdio>

#include "fp16.h"
#include "sippBuffer.h"

using namespace std;

Buffer::Buffer(string name, eBufferAccess buff) :
	buffer          (0),
	base            (0),
	buffer_lines    (0),
	start_level     (0),
	offset          (0),
	nplanes         (0),
	format          (0),
	line_stride     (0),
	plane_stride    (0),
	irq_rate        (0),
	irq_type        (0),
	chunk_size      (0),
	fill_level      (0),
	buffer_idx      (0),
	reg             (0) {
		this->name = name;
		this->buff = buff;
}

void Buffer::BufferAlloc(void *ptr, int reg) {
	// Should only be called once, memory should actually be allocated externally, 
	// here we just point at it via base
	if (buff_params[reg].base == 0) {
		cerr <<  "Error: BufferAlloc(): attempted " << name << " buffer allocation to NULL base address!" << endl;
		exit(1);
	} else {
#ifdef MOVI_SIM_SIPP
    if (((unsigned int)ptr >= (unsigned int)0x70000000) && ((unsigned int)ptr < ((unsigned int)0x70000000 + 16 * 0x20000)))
    {
        buff_params[reg].base = (int)((unsigned char*)cmxPtr + ((unsigned int)ptr & ((0x1 << 21) - 1)));
        buff_params[reg].buffer = (fp16 *)((unsigned char*)cmxPtr + ((unsigned int)ptr & ((0x1 << 21) - 1)));
    }
    else if ((unsigned int)ptr >= (unsigned int)0x80000000)
    {
        buff_params[reg].base = (int)((unsigned char*)ddrPtr + ((unsigned int)ptr & 0xfffffff));
        buff_params[reg].buffer = (fp16 *)((unsigned char*)ddrPtr + ((unsigned int)ptr & 0xfffffff));
    }
    printf("Info: BufferAlloc(): %s buffer @0x%x \n", name.c_str(), buff_params[reg].buffer);
#else
		buff_params[reg].buffer = ptr;
		cout << setbase (16);
		cout <<  "Info: BufferAlloc(): " << name << " buffer @0x" << (size_t)buff_params[reg].buffer << endl;
#endif
    }
	cout << setbase (10);
}

void Buffer::SetConfig (int val, int reg) {
	SetBuffLines(val >> SIPP_NL_OFFSET, reg);
	if (buff == Read) SetSyncEnable(val >> SIPP_SC_OFFSET);
	if (buff == Read) SetStartLevel(val >> SIPP_SL_OFFSET);
	if (buff == Write) SetOffset(val >> SIPP_OF_OFFSET, reg);
	SetNumPlanes(val >> SIPP_NP_OFFSET, reg);
	SetFormat(val >> SIPP_FO_OFFSET, reg);
}

int Buffer::GetConfig (int reg) {
	int ret = 0;
	ret |= buff_params[reg].buffer_lines << SIPP_NL_OFFSET;
	if (buff == Read)  ret |= sync_enable << SIPP_SC_OFFSET;
	if (buff == Read)  ret |= start_level << SIPP_SL_OFFSET;
	if (buff == Write) ret |= buff_params[reg].offset << SIPP_OF_OFFSET;
	ret |= buff_params[reg].nplanes << SIPP_NP_OFFSET;
	ret |= buff_params[reg].format << SIPP_FO_OFFSET;
	return ret;
}

int Buffer::GetConfig (void) {
	int ret = 0;
	ret |= buff_params[reg].buffer_lines << SIPP_NL_OFFSET;
	if (buff == Read)  ret |= sync_enable << SIPP_SC_OFFSET;
	if (buff == Read)  ret |= start_level << SIPP_SL_OFFSET;
	if (buff == Write) ret |= buff_params[reg].offset << SIPP_OF_OFFSET;
	ret |= buff_params[reg].nplanes << SIPP_NP_OFFSET;
	ret |= buff_params[reg].format << SIPP_FO_OFFSET;
	return ret;
}

void Buffer::SetIrqConfig (int val, int reg) {
	if (buff == Read) SetIrqRate(val >> SIPP_IR_OFFSET);
	SetIrqType(val >> SIPP_IT_OFFSET, reg);
	SetStartSlice(val >> SIPP_SS_OFFSET, reg);
	SetChunkSize(val >> SIPP_CS_OFFSET, reg);
}

int Buffer::GetIrqConfig (int reg) {
	int ret = 0;
	if (buff == Read) ret |= irq_rate << SIPP_IR_OFFSET;
	ret |= buff_params[reg].irq_type << SIPP_IT_OFFSET;
	ret |= buff_params[reg].start_slice << SIPP_SS_OFFSET;
	ret |= buff_params[reg].chunk_size << SIPP_CS_OFFSET;
	return ret;
}

int Buffer::GetIrqConfig (void) {
	int ret = 0;
	if (buff == Read) ret |= irq_rate << SIPP_IR_OFFSET;
	ret |= buff_params[reg].irq_type << SIPP_IT_OFFSET;
	ret |= buff_params[reg].start_slice << SIPP_SS_OFFSET;
	ret |= buff_params[reg].chunk_size << SIPP_CS_OFFSET;
	return ret;
}

int Buffer::IncFillLevel(void) {
	int ret;
	//fill_level_m.Lock();
	if (++fill_level > buff_params[reg].buffer_lines) {
		cerr <<  "Error: IncFillLevel(): " << name << " buffer fill_level overflow!" << endl;
		exit(1);
	}
	ret = fill_level;
	//fill_level_m.Unlock();
	return ret;
}

int Buffer::DecFillLevel(void) {
	int ret;
	//fill_level_m.Lock();
	if (--fill_level < 0) {
		cerr <<  "Error: DecFillLevel(): " << name << " buffer fill_level underflow!" << endl;
		exit(1);
	}
	ret = fill_level;
	//fill_level_m.Unlock();
	return ret;
}

int Buffer::SetFillLevel(int val) {
	//fill_level_m.Lock();
	fill_level = val & SIPP_NL_MASK;
	//fill_level_m.Unlock();
	return val;
}

int Buffer::GetFillLevel(void) {
	int ret;
	//fill_level_m.Lock();
	ret = fill_level;
	//fill_level_m.Unlock();
	return ret;
}

int Buffer::IncBufferIdx(void) {
	if (++buffer_idx > (buff_params[reg].buffer_lines - 1))
		buffer_idx = 0;
	return buffer_idx;
}

int Buffer::IncBufferIdxNoWrap(int height) {
	if (++buffer_idx > (height - 1))
		buffer_idx = 0;
	return buffer_idx;
}

void Buffer::SetBufferIdx(int val) {
	buffer_idx = val;
	if (buffer_idx > (buff_params[reg].buffer_lines - 1)) {
		cerr <<  "Error: SetBufferIdx(): " << name << " buffer buffer_idx may not exceed buffer_lines - 1!" << endl;
		exit(1);
	}
}

void Buffer::SetBufferIdxNoWrap(int val, int height) {
	buffer_idx = val;
	if (buffer_idx > (height - 1)) {
		cerr <<  "Error: SetBufferIdxNoWrap(): " << name << " buffer buffer_idx may not exceed height - 1!" << endl;
		exit(1);
	}
}

int Buffer::GetBufferIdx(int offset) {
	int idx_offset = buffer_idx + offset;

	// Buffer index is circular, handle wrap-around at both ends
	if (idx_offset > (buffer_lines - 1))
		return idx_offset - buffer_lines;
	else if (idx_offset < 0)
		return idx_offset + buffer_lines;

	return idx_offset;
}

int Buffer::GetBufferIdxNoWrap(int height, int offset) {
	if (offset < 0 || offset > (height - 1)) {
		cerr <<  "Error: GetBufferIdxNoWrap(): " << name << " buffer is not circular!" << endl;
		exit(1);
	} else {
		int idx_offset = buffer_idx + offset;

		return idx_offset;
	}
}

