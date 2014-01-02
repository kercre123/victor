// -----------------------------------------------------------------------------
// Copyright (C) 2012 Movidius Ltd. All rights reserved
//
// Company          : Movidius
// Author           : Rick Richmond (richard.richmond@movidius.com)
// Description      : SIPP Accelerator HW buffer model
//
// Note: all the accessor methods are overloaded - either with or without an
// integer parameter. The former ones are used in direct access via the APB
// slave interface (e.g. one might want to retrieve a value programmed in the
// shadow register regardless of whether it is or not currently in use).
// The latter ones serve the purpose of accessing the registers that are
// specified by the mask programmed in SIPP_SHADOW_SELECT_ADR.
// -----------------------------------------------------------------------------

#ifndef __SIPP_BUFFER_H__
#define __SIPP_BUFFER_H__


#include "moviThreadsUtils.h" // For Mutex class

#include <string>
#include <math.h>
using std::string;

#define SIPP_NL_SIZE  10   // Number of lines in buffer width (up to 1k)
#define SIPP_SC_SIZE  1    // Synchronous mode control
#define SIPP_SL_SIZE  10   // Start level in the input buffer
#define SIPP_OF_SIZE  3    // Offset (0 - 7 bytes) in the output buffer
#define SIPP_NP_SIZE  4    // Number of planes width (up to 16)
#define SIPP_FO_SIZE  3    // Format in number of bytes (1, 2 or 4)
#define SIPP_LS_SIZE  25   // Line stride width (up to 32MB - 1)
#define SIPP_PS_SIZE  25   // Plane stride width (up to 32MB - 1)
#define SIPP_IR_SIZE  4    // Interrupt rate specifier width (2**0 to 2**15)
#define SIPP_IT_SIZE  2    // Interrupt type
#define SIPP_SS_SIZE  4    // Start slice
#define SIPP_CS_SIZE  16   // Line chunk size (up to 64KB - 1)
#define SIPP_SB_SIZE  1    // Start bit (used to run filters in synchronous mode)
#define SIPP_KL_SIZE  4    // Number of lines in kernel width (up to 15)

#define SIPP_NL_MASK  0x3ff
#define SIPP_SC_MASK  0x1
#define SIPP_SL_MASK  0x3ff
#define SIPP_OF_MASK  0x7
#define SIPP_NP_MASK  0xf
#define SIPP_FO_MASK  0x7
#define SIPP_LS_MASK  0x1ffffff
#define SIPP_PS_MASK  0x1ffffff
#define SIPP_IR_MASK  0xf
#define SIPP_IT_MASK  0x3
#define SIPP_SS_MASK  0xf
#define SIPP_CS_MASK  0xfff8
#define SIPP_SB_MASK  0x1
#define SIPP_KL_MASK  0xf

// Offsets from bit 0 in packed APB buffer configuration registers
#define SIPP_NL_OFFSET 0
#define SIPP_SC_OFFSET 10
#define SIPP_SL_OFFSET 12
#define SIPP_OF_OFFSET 12
#define SIPP_NP_OFFSET 24
#define SIPP_FO_OFFSET 28

// Offsets from bit 0 in packed APB interrupt configuration registers
#define SIPP_IR_OFFSET 0
#define SIPP_IT_OFFSET 4
#define SIPP_SS_OFFSET 8
#define SIPP_CS_OFFSET 16

// Input/output buffers' format
#define SIPP_FORMAT_8BIT   0x1
#define SIPP_FORMAT_16BIT  0x2
#define SIPP_FORMAT_32BIT  0x4


enum eBufferAccess {
	Read,  // i.e. input buffer
	Write, // i.e. output buffer
	ReadOnly};

typedef struct {
	void *buffer;
	int base;
	int buffer_lines;
	int offset;
	int nplanes;
	int format; 
	int line_stride;
	int plane_stride;
	int irq_type;
	int start_slice;
	int chunk_size;
} BufferParameters;

class Buffer {
public:
	// Scanline buffer
	void *buffer;

    void* cmxPtr;
    void* ddrPtr;

	Buffer(string name, eBufferAccess buff = ReadOnly);
	~Buffer() {
	}

	// Set buffer name
	void SetName (std::string val) {
		name = val;
	}
	// Set buffer access
	void SetAccess (eBufferAccess val) {
		buff = val;
	}
	// Set/Get methods regarding the currently utilized register's index
	void SetUtilizedRegisters (int reg = 0) {
		this->reg = reg;
	}
	int GetUtilizedRegisters () {
		return reg;
	}
	// Copies the buffer specific programmable parameters from either default or shadow registers
	void SelectBufferParameters () {
		buffer       = buff_params[reg].buffer;
		base         = buff_params[reg].base;
		buffer_lines = buff_params[reg].buffer_lines; 
		offset       = buff_params[reg].offset;
		nplanes      = buff_params[reg].nplanes;
		format       = buff_params[reg].format;
		line_stride  = buff_params[reg].line_stride;
		plane_stride = buff_params[reg].plane_stride;
		irq_type     = buff_params[reg].irq_type;
		start_slice  = buff_params[reg].start_slice;
		chunk_size   = buff_params[reg].chunk_size;
	}

	// Set/Get methods for private buffer configuration members
	void SetBase (int val, int reg = 0) {
		buff_params[reg].base = val;
		BufferAlloc((void *)val, reg);
	}
	int GetBase (int reg) {
		return buff_params[reg].base;    // Return base address int value for APB register emulation
	}
	int GetBase () {
		return buff_params[reg].base;    // Return base address int value for APB register emulation
	}
	void *GetPtr () {
		return buff_params[reg].buffer;  // Return pointer to buffer
	}
	void SetBuffLines (int val, int reg = 0) {
		buff_params[reg].buffer_lines = val & SIPP_NL_MASK;
	}
	int GetBuffLines (int reg) {
		return buff_params[reg].buffer_lines;
	}
	int GetBuffLines () {
		return buff_params[reg].buffer_lines;
	}
	void SetSyncEnable (int val) {
		sync_enable = val & SIPP_SC_MASK;
	}
	int GetSyncEnable () {
		return sync_enable;
	}
	void SetStartLevel (int val) {
		start_level = val & SIPP_SL_MASK;
	}
	int GetStartLevel () {
		return start_level;
	}
	void SetOffset (int val, int reg = 0) {
		buff_params[reg].offset = val & SIPP_OF_MASK;
	}
	int GetOffset (int reg) {
		return buff_params[reg].offset;
	}
	int GetOffset () {
		return buff_params[reg].offset;
	}
	void SetNumPlanes (int val, int reg = 0) {
		buff_params[reg].nplanes = val & SIPP_NP_MASK;
	}
	int GetNumPlanes (int reg) {
		return buff_params[reg].nplanes;
	}
	int GetNumPlanes () {
		return buff_params[reg].nplanes;
	}
	void SetFormat (int val, int reg = 0) {
		buff_params[reg].format = val & SIPP_FO_MASK;
	}
	int GetFormat (int reg) {
		return buff_params[reg].format;
	}
	int GetFormat () {
		return buff_params[reg].format;
	}
	void SetLineStride (int val, int reg = 0) {
		buff_params[reg].line_stride = val & SIPP_LS_MASK;
	}
	int GetLineStride (int reg) {
		return buff_params[reg].line_stride;
	}
	int GetLineStride () {
		return buff_params[reg].line_stride;
	}
	void SetPlaneStride (int val, int reg = 0) {
		buff_params[reg].plane_stride = val & SIPP_PS_MASK;
	}
	int GetPlaneStride (int reg) {
		return buff_params[reg].plane_stride;
	}
	int GetPlaneStride () {
		return buff_params[reg].plane_stride;
	}
	void SetIrqRate (int val) {
		irq_rate = val & SIPP_IR_MASK;
	}
	int GetIrqRate () {
		return irq_rate;
	}
	void SetIrqType (int val, int reg = 0) {
		buff_params[reg].irq_type = val & SIPP_IT_MASK;
	}
	int GetIrqType (int reg) {
		return buff_params[reg].irq_type;
	}
	int GetIrqType () {
		return buff_params[reg].irq_type;
	}
	void SetStartSlice (int val, int reg = 0) {
		buff_params[reg].start_slice = val & SIPP_SS_MASK;
	}
	int GetStartSlice (int reg) {
		return buff_params[reg].start_slice;
	}
	int GetStartSlice () {
		return buff_params[reg].start_slice;
	}
	void SetChunkSize (int val, int reg = 0) {
		buff_params[reg].chunk_size = val & SIPP_CS_MASK;
	}
	int GetChunkSize (int reg) {
		return buff_params[reg].chunk_size;
	}
	int GetChunkSize () {
		return buff_params[reg].chunk_size;
	}
	void SetStartBit (int val) {
		start_bit = val & SIPP_SB_MASK;
	}
	void ClrStartBit () {
		start_bit = 0;
	}

	// Grouped Set/Get methods for emulation of packed APB register access
	void SetConfig (int, int = 0);
	int GetConfig (int);
	int GetConfig ();
	void SetIrqConfig (int, int = 0);
	int GetIrqConfig (int);
	int GetIrqConfig ();

	// Buffer fill level/index methods all return fill level/index (the new one
	// if modified)
	int IncFillLevel();
	int DecFillLevel();
	int SetFillLevel(int);
	int GetFillLevel();
	int IncBufferIdx();
	int IncBufferIdxNoWrap(int);
	void SetBufferIdx(int);
	void SetBufferIdxNoWrap(int, int);
	int GetBufferIdx(int offset = 0);
	int GetBufferIdxNoWrap(int, int offset = 0);

private:
	std::string name;
	eBufferAccess buff;
	BufferParameters buff_params[2];
	int  base;
	int  buffer_lines;
	int  sync_enable;                  // Synchronous control enable
	int  start_level;
	int  offset;
	int  nplanes;                      // Number of planes - 1
	int  format; 
	int  line_stride;
	int  plane_stride;
	int  irq_rate;
	int  irq_type;
	int  start_slice;
	int  chunk_size;
	volatile int fill_level;
	int  buffer_idx;
	int  start_bit;

	// Default/shadow registers selection
	int reg;

	// Mutex for access to fill_level
	//Mutex fill_level_m;

	// Buffer allocation
	void BufferAlloc(void *, int);

	// TODO - model interrupt rate control
};

#endif // __SIPP_BUFFER_H__
