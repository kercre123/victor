// -----------------------------------------------------------------------------
// Copyright (C) 2012 Movidius Ltd. All rights reserved
//
// Company          : Movidius
// Author           : Rick Richmond (richard.richmond@movidius.com)
// Description      : SIPP Accelerator HW base filter model
// -----------------------------------------------------------------------------

#ifndef __SIPP_BASE_H__
#define __SIPP_BASE_H__

#include "criticalSection.h"
#include "moviThreadsUtils.h"
#include "sippIrq.h"
#include "sippBuffer.h"
#include "sippCommon.h"
#include "fp16.h"

#include <string>
#include <iostream>

#ifdef SYSTEMC
#define SC_INCLUDE_DYNAMIC_PROCESSES
#include <systemc.h>
using namespace sc_core;
using namespace sc_dt;
#endif

typedef struct {
	int width;
	int height;
	int width_o;
	int height_o;
	int kernel_lines;
	int max_pad;
} BaseParameters;

class SippBaseFilt {
public:
	// Scanline buffers
	Buffer in;
	Buffer out;

	SippBaseFilt(SippIrq *pObj = 0) :
		in          ("input", Read),
		out         ("output", Write),
		pIrqObj     (pObj),
		name        ("Undefined"),
		id          (0),
		width       (0),
		height      (0),
		width_o     (0),
		height_o    (0),
		line_idx    (0),
		line_idx_o  (0),
		frame_count (0),
		reg         (0),
		en          (false),
		verbose     (false) {
	}

	~SippBaseFilt() {
	}

	// Interrupt status update callback. Callback is to SetStatusBit() method
	// of SippIrq object pointed to by pIrqObj
	void SetIrqBit (int irq, int bit) {
		SippIrq *theIrq = pIrqObj;
		theIrq->SetStatusBit(irq, bit);
	}
	void InputBufferLevelDecIrq () {
		SetIrqBit(0, id);
	}
	void OutputBufferLevelIncIrq () {
		SetIrqBit(1, id);
	}
	void EndOfFrameIrq () {
		SetIrqBit(2, id);
	}

	void Enable () {
		if (verbose)
			std::cout << "Starting " << name << " (filter id = " << id << ")" << std::endl;
		en = true;
	}
	void Disable () {
		en = false;
	}
	bool IsEnabled () {
		return en;
	}

	void SetPIrqObj (SippIrq *p) {
		pIrqObj = p;
	}
	void SetName (std::string name) {
		this->name = name;
	}
	void SetId (int id) {
		this->id = id;
	}

	void SetVerbose () {
		verbose = true;
	}
	void UnsetVerbose () {
		verbose = false;
	}
	bool Verbose () {
		return verbose;
	}

	void SetKernelLinesNo (int val) {
		kernel_lines = val;
		base_params[reg].max_pad = kernel_lines >> 1;
	}
	int GetKernelLinesNo() {
		return kernel_lines;
	}

	void SetLineIdx (int val) {
		line_idx = val & SIPP_IMGDIM_MASK;
	}
	int GetLineIdx () {
		return line_idx;
	}

	void SetOutputLineIdx (int val) {
		line_idx_o = val & SIPP_IMGDIM_MASK;
	}
	int GetOutputLineIdx () {
		return line_idx_o;
	}

	// Configure global slice related parameters
	static void SetGlobalSliceSize (int val) {
		g_slice_size = val;
	}
	static int GetGlobalSliceSize () {
		return g_slice_size;
	}
	static void SetGlobalFirstSlice (int val) {
		g_first_slice = val;
	}
	static int GetGlobalFirstSlice () {
		return g_first_slice;
	}
	static void SetGlobalLastSlice (int val) {
		g_last_slice = val;
	}
	static int GetGlobalLastSlice () {
		return g_last_slice;
	}

	// Set/Get methods regarding the currently utilized register's index
	void SetUtilizedRegisters (int reg = 0) {
		this->reg = reg;
		in.SetUtilizedRegisters(reg);
		out.SetUtilizedRegisters(reg);
	}
	int GetUtilizedRegisters () {
		return reg;
	}

	bool EndOfFrame () {
		return (GetLineIdx() == 0);
	}

	// Grouped Set/Get methods for emulation of packed APB register access
	void SetImgDimensions (int val, int reg = 0) {
		base_params[reg].width = val & SIPP_IMGDIM_MASK;
		base_params[reg].height = (val >> SIPP_IMGDIM_SIZE) & SIPP_IMGDIM_MASK;
	}
	int GetImgDimensions (int reg) {
		int ret = 0;
		ret |= base_params[reg].width;
		ret |= base_params[reg].height << SIPP_IMGDIM_SIZE;
		return ret;
	}
	int GetImgDimensions () {
		int ret = 0;
		ret |= base_params[reg].width;
		ret |= base_params[reg].height << SIPP_IMGDIM_SIZE;
		return ret;
	}
	virtual void SetOutImgDimensions (int val, int reg = 0) {
		base_params[reg].width_o = val & SIPP_IMGDIM_MASK;
		base_params[reg].height_o = (val >> SIPP_IMGDIM_SIZE) & SIPP_IMGDIM_MASK;
	}
	int GetOutImgDimensions (int reg) {
		int ret = 0;
		ret |= base_params[reg].width_o;
		ret |= base_params[reg].height_o << SIPP_IMGDIM_SIZE;
		return ret;
	}
	int GetOutImgDimensions () {
		int ret = 0;
		ret |= base_params[reg].width_o;
		ret |= base_params[reg].height_o << SIPP_IMGDIM_SIZE;
		return ret;
	}

	// Increment/decrement input/output buffer fill level functions
	// ============================================================
	// Buffer fill levels determine whether the filter may run or not!
	//
	// Calling IncInputBufferFillLevel() would run the filter if, after incrementing
	// input buffer fill level there are sufficient lines in the input buffer to run
	// and the output buffer is not full.
	//
	// Calling DecOutputBufferFillLevel() would run the filter if the output buffer was
	// full prior to decrementing its fill level and there are sufficient lines in the
	// input buffer.
	virtual int IncInputBufferFillLevel (void);
	virtual int DecOutputBufferFillLevel (void);

	void SetCmxBasePointer (void* cmxMemPtr);
	void SetDdrBasePointer (void* ddrMemPtr);

	// VIRTUAL TryRun() function called by IncInputBufferFillLevel() and
	// DecOutputBufferFillLevel()
	virtual void TryRun(void);

protected:
	// Pointer to SippIrq object for interrupt status update callback
	SippIrq *pIrqObj;

	std::string name;

	BaseParameters base_params[2];
	int id;
	int width;
	int height;
	int width_o;
	int height_o;
	int kernel_lines;
	int line_idx;
	int line_idx_o;
	int frame_count;

	// Vertical padding related
	int max_pad;
	int min_fill;

	// Global slice size
	static int g_slice_size;

	// Global first and last slice
	static int g_first_slice;
	static int g_last_slice;

	// Current plane start slice
	int  input_plane_start_slice;
	int output_plane_start_slice;

	// Default/shadow registers selection
	int reg;

	// Critical section object
	CriticalSection cs;

	bool en;
	bool verbose;

	// Copies the programmable parameters from either default or shadow registers
	void SelectParameters(void);

	// Filter start, frame line index and buffer update logic
	bool CanRun(int);
	bool CanRunNoPad(int);
	void UpdateBuffers(void);
	void UpdateBuffersNoPad(int);
	void UpdateBuffersSync(void);
	void UpdateBuffersSyncNoPad(int);
	void UpdateOutputBuffer(void);
	void UpdateOutputBufferSync(void);
	void UpdateOutputBufferNoWrap(void);
	void UpdateOutputBufferNoPadNoWrap(int);
	void UpdateInputBuffer(void);
	void UpdateInputBufferNoPad(int);
	void UpdateInputBufferSync(void);
	void UpdateInputBufferSyncNoPad(int);
	void UpdateInputBufferNoWrap(void);
	void UpdateInputBufferNoPadNoWrap(int);
	int IncLineIdx(void);
	int IncLineIdxNoPad(int);
	int IncOutputLineIdx(void);
	int IncOutputLineIdx(int);

	// Sets output buffer line for filter run
	void setOutputLine (uint8_t  **, int);
	void setOutputLine (uint16_t **, int);
	void setOutputLine (uint32_t **, int);
	void setOutputLine (fp16     **, int);
	void setOutputLine (float    **, int);
	void setOutputLines(uint8_t  **, int);
	void setOutputLines(uint16_t **, int);
	void setOutputLines(fp16     **, int);

	// Jump to the next slice in a circular manner
	int NextInputSlice (int offset = 0);
	int NextOutputSlice(int offset = 0);
	void ComputeInputStartSlice (int);
	void ComputeOutputStartSlice(int);

	// Packing/splitting methods (used in slice chunking address mode)
	void PackInputLines(uint8_t  **, int, uint8_t  **);
	void PackInputLines(uint16_t **, int, uint16_t **);
	void PackInputLines(uint32_t **, int, uint32_t **);
	void PackInputLines(fp16     **, int, fp16     **);
	void SplitOutputLine(uint8_t  *, int, uint8_t  *);
	void SplitOutputLine(uint16_t *, int, uint16_t *);
	void SplitOutputLine(uint32_t *, int, uint32_t *);
	void SplitOutputLine(fp16     *, int, fp16     *);
	void SplitOutputLine(float    *, int, float    *);

	// Gets input kernel lines by conditionally calling following methods - suitable for uint8_t kernels
	void getKernelLines(uint8_t **, int, int);
	// Gets input kernel lines by conditionally calling following methods - suitable for uint16_t kernels
	void getKernelLines(uint16_t **, int, int);
	// Gets input kernel lines by conditionally calling following methods - suitable for uint32_t kernels
	void getKernelLines(uint32_t **, int, int);
	// Gets input kernel lines by conditionally calling following methods - suitable for FP16 kernels
	void getKernelLines(fp16 **, int, int);

	// Padding at top, bottom, vertical middle execution - suitable for uint8_t kernels
	void PadAtTop(uint8_t **, int, int, int);
	void PadAtBottom(uint8_t **, int, int, int);
	void MiddleExecutionV(uint8_t **, int, int);

	// Padding at top, bottom, vertical middle execution - suitable for uint16_t kernels
	void PadAtTop(uint16_t **, int, int, int);
	void PadAtBottom(uint16_t **, int, int, int);
	void MiddleExecutionV(uint16_t **, int, int);

	// Padding at top, bottom, vertical middle execution - suitable for uint32_t kernels
	void PadAtTop(uint32_t **, int, int, int);
	void PadAtBottom(uint32_t **, int, int, int);
	void MiddleExecutionV(uint32_t **, int, int);

	// Padding at top, bottom, vertical middle execution - suitable for FP16 kernels
	void PadAtTop(fp16 **, int, int, int);
	void PadAtBottom(fp16 **, int, int, int);
	void MiddleExecutionV(fp16 **, int, int);


	// Padding at left, right, horizontal middle execution for a 2D uint8_t filter
	void PadAtLeft(uint8_t **, uint8_t *, int, int, int);
	void PadAtRight(uint8_t **, uint8_t *, int, int, int);
	void MiddleExecutionH(uint8_t **, uint8_t *, int, int);

	// Padding at left, right, horizontal middle execution for Luma denoise
	void PadAtLeft(uint8_t **, uint16_t *, int, int, int);
	void PadAtRight(uint8_t **, uint16_t *, int, int, int);
	void MiddleExecutionH(uint8_t **, uint16_t *, int, int);

	// Padding at left, right, horizontal middle execution for a 2D uint16_t filter
	void PadAtLeft(uint16_t **, uint16_t *, int, int, int);
	void PadAtRight(uint16_t **, uint16_t *, int, int, int);
	void MiddleExecutionH(uint16_t **, uint16_t *, int, int);

	// Padding at left, right, horizontal middle execution for a 2D fp16 filter
	void PadAtLeft(fp16 **, fp16 *, int, int, int);
	void PadAtRight(fp16 **, fp16 *, int, int, int);
	void MiddleExecutionH(fp16 **, fp16 *, int, int);

	// Padding at left, right, horizontal middle execution for the output of an 1D filter cascaded into another one(uint8_t)
	void PadAtLeft(uint8_t *, uint8_t *, int, int, int);
	void PadAtRight(uint8_t *, uint8_t *, int, int, int);
	void MiddleExecutionH(uint8_t *, uint8_t *, int, int);

	// Padding at left, right, horizontal middle execution for the output of an 1D filter cascaded into another one(float)
	void PadAtLeft(fp16 *, fp16 *, int, int, int);
	void PadAtRight(fp16 *, fp16 *, int, int, int);
	void MiddleExecutionH(fp16 *, fp16 *, int, int);


	// Builds the kernel directly from the injected input lines (2D uint8_t filters' case)
	void buildKernel (uint8_t **, uint8_t *, int, int);

	// Builds the kernel directly from the injected input lines (Luma denoise)
	void buildKernel (uint8_t **, uint16_t *, int, int);

	// Builds the kernel directly from the injected input lines (2D uint16_t filters' case)
	void buildKernel (uint16_t **, uint16_t *, int, int);

	// Builds the kernel directly from the injected input lines (2D fp16 filters' case)
	void buildKernel (fp16 **, fp16 *, int, int);

	// Builds the kernel based on the output from the pass of an 1D filter(uint8_t case)
	void buildKernel (uint8_t *, uint8_t *, int, int);

	// Builds the kernel based on the output from the pass of an 1D filter(float case)
	void buildKernel (fp16 *, fp16 *, int, int);


	// Convert uint8_t kernel to fp16 kernel, normalizing data in range [0, 255] into [0, 1.0]
	void convertKernel(uint8_t *, fp16 *, int);

	// Convert fp16 kernel to u12f kernel
	void convertKernel(fp16 *, uint16_t *, int);

	// Convert uint8_t kernel to uint16_t kernel
	void convertKernel(uint8_t *, uint16_t *, int);

	// Linear interpolation
	float Interpolate(float, float, float, float, float);

	// Clamp within specified range
	float ClampWR(float, float, float);
	int   ClampWR(int, int, int);

	// Get the MSB of 1
	int GetMsb(int x);

	// PURE VIRTUAL Run() function
	//
	// Typically produces one line of filter output
	//
	// First parameter  - Array of pointers to input lines (for multi-line filter kernels)
	// Second parameter - Pointer to output line
	virtual void *Run(void **, void *) = 0;
};

#endif // _SIPP_BASE_H__
