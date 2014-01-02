// -----------------------------------------------------------------------------
// Copyright (C) 2012 Movidius Ltd. All rights reserved
//
// Company          : Movidius
// Author           : Rick Richmond (richard.richmond@movidius.com)
// Description      : SIPP Accelerator HW model
// -----------------------------------------------------------------------------

#include "sippHw.h"

#include <iostream>
#include <iomanip>

using namespace std;

SippHW::SippHW (void (*pIrqCb)(void *)) :
	// NOTE irq object may be passed a reference to callback implementing an
	// interrupt service routine. The callback function should accept a pointer
	// to a SippHW object, so that it can do APB transactions. In the
	// constructor we do this by having the SippHW object pass a reference to
	// itself (returned by private method myself()) to the constructor for the
	// irq object.
	irq     (pIrqCb, myself()),
	// Filter objects passed reference to Irq object, second parameter is filter ID
	// which is used to set the correct IRQ status register bits for the filter.
	// The third parameter is kernel size (default value is filter specific) which 
        // must match HW for RTL verification
	raw     (&irq, SIPP_RAW_ID, RAW_KERNEL_SIZE, HIST_KERNEL_SIZE),
	lsc     (&irq, SIPP_LSC_ID, LSC_KERNEL_SIZE),
	dbyr    (&irq, SIPP_DBYR_ID, DBYR_AHD_KERNEL_SIZE, DBYR_BIL_KERNEL_SIZE),
	dbyrppm (&irq, SIPP_DBYR_PPM_ID, DBYR_PPM_KERNEL_SIZE),
	cdn     (&irq, SIPP_CHROMA_ID, CHROMA_V_KERNEL_SIZE, CHROMA_H0_KERNEL_SIZE, CHROMA_H1_KERNEL_SIZE, CHROMA_H2_KERNEL_SIZE, CHROMA_REF_KERNEL_SIZE),
	luma    (&irq, SIPP_LUMA_ID, LUMA_KERNEL_SIZE, LUMA_REF_KERNEL_SIZE),
	usm     (&irq, SIPP_SHARPEN_ID),
	upfirdn (&irq, SIPP_UPFIRDN_ID),
	med     (&irq, SIPP_MED_ID),
	lut     (&irq, SIPP_LUT_ID, LUT_KERNEL_SIZE),
	edge    (&irq, SIPP_EDGE_OP_ID, EDGE_OP_KERNEL_SIZE),
	conv    (&irq, SIPP_CONV_ID),
	harr    (&irq, SIPP_HARRIS_ID),
	cc      (&irq, SIPP_CC_ID, CC_LUMA_KERNEL_SIZE, CC_CHROMA_KERNEL_SIZE) {
}

// int val is a bit mask corresponding to the the filter object IDs
void SippHW::SetVerbose (int val) {
	if (val & (1 << SIPP_RAW_ID))      raw.SetVerbose();
	if (val & (1 << SIPP_LSC_ID))      lsc.SetVerbose();
	if (val & (1 << SIPP_DBYR_ID))     dbyr.SetVerbose();
	if (val & (1 << SIPP_DBYR_PPM_ID)) dbyrppm.SetVerbose();
	if (val & (1 << SIPP_CHROMA_ID))   cdn.SetVerbose();
	if (val & (1 << SIPP_LUMA_ID))     luma.SetVerbose();
	if (val & (1 << SIPP_SHARPEN_ID))  usm.SetVerbose();
	if (val & (1 << SIPP_UPFIRDN_ID))  upfirdn.SetVerbose();
	if (val & (1 << SIPP_MED_ID))      med.SetVerbose();
	if (val & (1 << SIPP_LUT_ID))      lut.SetVerbose();
	if (val & (1 << SIPP_EDGE_OP_ID))  edge.SetVerbose();
	if (val & (1 << SIPP_CONV_ID))     conv.SetVerbose();
	if (val & (1 << SIPP_HARRIS_ID))   harr.SetVerbose();
	if (val & (1 << SIPP_CC_ID))       cc.SetVerbose();
}

void SippHW::ApbWrite (int addr, int data, void *ptr) {
#ifdef MOVI_SIM_SIPP
    //printf("\n write address %x -> data %x\n", addr, data);
#endif
	switch (addr) {
    case SIPP_SOFTRST_ADR:
        // ToDo: Add filter reset. Function calls here
        break;
	case SIPP_CONTROL_ADR :
		if (data & SIPP_RAW_ID_MASK)      raw.Enable();
		if (data & SIPP_LSC_ID_MASK)      lsc.Enable();
		if (data & SIPP_DBYR_ID_MASK)     dbyr.Enable();
		if (data & SIPP_DBYR_PPM_ID_MASK) dbyrppm.Enable();
		if (data & SIPP_CHROMA_ID_MASK)   cdn.Enable();
		if (data & SIPP_LUMA_ID_MASK)     luma.Enable();
		if (data & SIPP_SHARPEN_ID_MASK)  usm.Enable();
		if (data & SIPP_UPFIRDN_ID_MASK)  upfirdn.Enable();
		if (data & SIPP_MED_ID_MASK)      med.Enable();
		if (data & SIPP_LUT_ID_MASK)      lut.Enable();
		if (data & SIPP_EDGE_OP_ID_MASK)  edge.Enable();
		if (data & SIPP_CONV_ID_MASK)     conv.Enable();
		if (data & SIPP_HARRIS_ID_MASK)   harr.Enable();
		if (data & SIPP_CC_ID_MASK)       cc.Enable();
		break;
	case SIPP_START_ADR :
		if (data & SIPP_RAW_ID_MASK)      {     raw.in.SetStartBit(data >> SIPP_START_BIT);     raw.SetUpAndRun(); }
		if (data & SIPP_LSC_ID_MASK)      {     lsc.in.SetStartBit(data >> SIPP_START_BIT);     lsc.SetUpAndRun(); }
		if (data & SIPP_DBYR_ID_MASK)     {    dbyr.in.SetStartBit(data >> SIPP_START_BIT);    dbyr.SetUpAndRun(); }
		if (data & SIPP_DBYR_PPM_ID_MASK) { dbyrppm.in.SetStartBit(data >> SIPP_START_BIT); dbyrppm.SetUpAndRun(); }
		if (data & SIPP_CHROMA_ID_MASK)   {     cdn.in.SetStartBit(data >> SIPP_START_BIT);     cdn.SetUpAndRun(); }
		if (data & SIPP_LUMA_ID_MASK)     {    luma.in.SetStartBit(data >> SIPP_START_BIT);    luma.SetUpAndRun(); }
		if (data & SIPP_SHARPEN_ID_MASK)  {     usm.in.SetStartBit(data >> SIPP_START_BIT);     usm.SetUpAndRun(); }
		if (data & SIPP_UPFIRDN_ID_MASK)  { upfirdn.in.SetStartBit(data >> SIPP_START_BIT); upfirdn.SetUpAndRun(); }
		if (data & SIPP_MED_ID_MASK)      {     med.in.SetStartBit(data >> SIPP_START_BIT);     med.SetUpAndRun(); }
		if (data & SIPP_LUT_ID_MASK)      {     lut.in.SetStartBit(data >> SIPP_START_BIT);     lut.SetUpAndRun(); }
		if (data & SIPP_EDGE_OP_ID_MASK)  {    edge.in.SetStartBit(data >> SIPP_START_BIT);    edge.SetUpAndRun(); }
		if (data & SIPP_CONV_ID_MASK)     {    conv.in.SetStartBit(data >> SIPP_START_BIT);    conv.SetUpAndRun(); }
		if (data & SIPP_HARRIS_ID_MASK)   {    harr.in.SetStartBit(data >> SIPP_START_BIT);    harr.SetUpAndRun(); }
		if (data & SIPP_CC_ID_MASK)       {      cc.in.SetStartBit(data >> SIPP_START_BIT);      cc.SetUpAndRun(); }
		break;
	case SIPP_INT0_STATUS_ADR :
		// READ ONLY
		break;
	case SIPP_INT0_ENABLE_ADR :
		irq.SetEnable(0, data);
		break;
	case SIPP_INT0_CLEAR_ADR :
		irq.ClrStatus(0, data);
		break;
	case SIPP_INT1_STATUS_ADR :
		// READ ONLY
		break;
	case SIPP_INT1_ENABLE_ADR :
		irq.SetEnable(1, data);
		break;
	case SIPP_INT1_CLEAR_ADR :
		irq.ClrStatus(1, data);
		break;
	case SIPP_INT2_STATUS_ADR :
		// READ ONLY
		break;
	case SIPP_INT2_ENABLE_ADR :
		irq.SetEnable(2, data);
		break;
	case SIPP_INT2_CLEAR_ADR :
		irq.ClrStatus(2, data);
		break;
	case SIPP_SLC_SIZE_ADR :
		SippBaseFilt::SetGlobalSliceSize ((data >>  0) & 0xfffff);
		SippBaseFilt::SetGlobalFirstSlice((data >> 24) & 0x0000f);
		SippBaseFilt::SetGlobalLastSlice ((data >> 28) & 0x0000f);
		break;
	case SIPP_SHADOW_SELECT_ADR :
		raw.SetUtilizedRegisters((data & SIPP_RAW_ID_MASK) >> SIPP_RAW_ID);
		lsc.SetUtilizedRegisters((data & SIPP_LSC_ID_MASK) >> SIPP_LSC_ID);
		dbyr.SetUtilizedRegisters((data & SIPP_DBYR_ID_MASK) >> SIPP_DBYR_ID);
		dbyrppm.SetUtilizedRegisters((data & SIPP_DBYR_PPM_ID_MASK) >> SIPP_DBYR_PPM_ID);
		cdn.SetUtilizedRegisters((data & SIPP_CHROMA_ID_MASK) >> SIPP_CHROMA_ID);
		luma.SetUtilizedRegisters((data & SIPP_LUMA_ID_MASK) >> SIPP_LUMA_ID);
		usm.SetUtilizedRegisters((data & SIPP_SHARPEN_ID_MASK) >> SIPP_SHARPEN_ID);
		upfirdn.SetUtilizedRegisters((data & SIPP_UPFIRDN_ID_MASK) >> SIPP_UPFIRDN_ID);
		med.SetUtilizedRegisters((data & SIPP_MED_ID_MASK) >> SIPP_MED_ID);
		lut.SetUtilizedRegisters((data & SIPP_LUT_ID_MASK) >> SIPP_LUT_ID);
		edge.SetUtilizedRegisters((data & SIPP_EDGE_OP_ID_MASK) >> SIPP_EDGE_OP_ID);
		conv.SetUtilizedRegisters((data & SIPP_CONV_ID_MASK) >> SIPP_CONV_ID);
		harr.SetUtilizedRegisters((data & SIPP_HARRIS_ID_MASK) >> SIPP_HARRIS_ID);
		break;

		/*
		* SIPP input buffers configuration/control
		*
		* Connect APB registers to filter buffer objects
		*/
		// IBUF[0] - RAW filter input buffer
	case SIPP_IBUF0_BASE_ADR :
		raw.in.SetBase(data, DEFAULT);
		break;
	case SIPP_IBUF0_BASE_SHADOW_ADR :
		raw.in.SetBase(data, SHADOW);
		break;
	case SIPP_IBUF0_CFG_ADR :
		raw.in.SetConfig(data, DEFAULT);
		break;
	case SIPP_IBUF0_CFG_SHADOW_ADR :
		raw.in.SetConfig(data, SHADOW);
		break;
	case SIPP_IBUF0_LS_ADR :
		raw.in.SetLineStride(data, DEFAULT);
		break;
	case SIPP_IBUF0_LS_SHADOW_ADR :
		raw.in.SetLineStride(data, SHADOW);
		break;
	case SIPP_IBUF0_PS_ADR :
		raw.in.SetPlaneStride(data, DEFAULT);
		break;
	case SIPP_IBUF0_PS_SHADOW_ADR :
		raw.in.SetPlaneStride(data, SHADOW);
		break;
	case SIPP_IBUF0_IR_ADR :
		raw.in.SetIrqConfig(data, DEFAULT);
		break;
	case SIPP_IBUF0_IR_SHADOW_ADR :
		raw.in.SetIrqConfig(data, SHADOW);
		break;
	case SIPP_IBUF0_FC_ADR :
		if (data & SIPP_INCDEC_BIT_MASK)
			raw.IncInputBufferFillLevel();
		if (data & SIPP_CTXUP_BIT_MASK)
			raw.in.SetFillLevel(data & SIPP_NL_MASK);
		break;
	case SIPP_ICTX0_ADR :
		if (data & SIPP_CTXUP_BIT_MASK) {
			raw.SetLineIdx(data & SIPP_IMGDIM_MASK);
			if(raw.in.GetBuffLines() == 0)
				raw.in.SetBufferIdxNoWrap((data >> SIPP_CBL_OFFSET) & SIPP_NL_MASK, raw.GetImgDimensions() >> SIPP_IMGDIM_SIZE);
			else
				raw.in.SetBufferIdx((data >> SIPP_CBL_OFFSET) & SIPP_NL_MASK);
		}
		if (data & SIPP_START_BIT_MASK) {
			raw.in.SetStartBit(data >> SIPP_START_BIT);
			if(raw.in.GetBuffLines() == 0)
				raw.TryRun();
			else
				raw.SetUpAndRun();
		}
		break;

		// IBUF[1] - Lens shading correction filter input buffer
	case SIPP_IBUF1_BASE_ADR :
		lsc.in.SetBase(data, DEFAULT);
		break;
	case SIPP_IBUF1_BASE_SHADOW_ADR :
		lsc.in.SetBase(data, SHADOW);
		break;
	case SIPP_IBUF1_CFG_ADR :
		lsc.in.SetConfig(data, DEFAULT);
		break;
	case SIPP_IBUF1_CFG_SHADOW_ADR :
		lsc.in.SetConfig(data, SHADOW);
		break;
	case SIPP_IBUF1_LS_ADR :
		lsc.in.SetLineStride(data, DEFAULT);
		break;
	case SIPP_IBUF1_LS_SHADOW_ADR :
		lsc.in.SetLineStride(data, SHADOW);
		break;
	case SIPP_IBUF1_PS_ADR :
		lsc.in.SetPlaneStride(data, DEFAULT);
		break;
	case SIPP_IBUF1_PS_SHADOW_ADR :
		lsc.in.SetPlaneStride(data, SHADOW);
		break;
	case SIPP_IBUF1_IR_ADR :
		lsc.in.SetIrqConfig(data, DEFAULT);
		break;
	case SIPP_IBUF1_IR_SHADOW_ADR :
		lsc.in.SetIrqConfig(data, SHADOW);
		break;
	case SIPP_IBUF1_FC_ADR :
		if (data & SIPP_INCDEC_BIT_MASK)
			lsc.IncInputBufferFillLevel();
		if (data & SIPP_CTXUP_BIT_MASK)
			lsc.in.SetFillLevel(data & SIPP_NL_MASK);
		break;
	case SIPP_ICTX1_ADR :
		if (data & SIPP_CTXUP_BIT_MASK) {
			lsc.SetLineIdx(data & SIPP_IMGDIM_MASK);
			if(lsc.in.GetBuffLines() == 0)
				lsc.in.SetBufferIdxNoWrap((data >> SIPP_CBL_OFFSET) & SIPP_NL_MASK, lsc.GetImgDimensions() >> SIPP_IMGDIM_SIZE);
			else
				lsc.in.SetBufferIdx((data >> SIPP_CBL_OFFSET) & SIPP_NL_MASK);
		}
		if (data & SIPP_START_BIT_MASK) {
			lsc.in.SetStartBit(data >> SIPP_START_BIT);
			if(lsc.in.GetBuffLines() == 0)
				lsc.TryRun();
			else
				lsc.SetUpAndRun();
		}
		break;

		// IBUF[2] - Debayer filter input buffer
	case SIPP_IBUF2_BASE_ADR :
		dbyr.in.SetBase(data, DEFAULT);
		break;
	case SIPP_IBUF2_BASE_SHADOW_ADR :
		dbyr.in.SetBase(data, SHADOW);
		break;
	case SIPP_IBUF2_CFG_ADR :
		dbyr.in.SetConfig(data, DEFAULT);
		break;
	case SIPP_IBUF2_CFG_SHADOW_ADR :
		dbyr.in.SetConfig(data, SHADOW);
		break;
	case SIPP_IBUF2_LS_ADR :
		dbyr.in.SetLineStride(data, DEFAULT);
		break;
	case SIPP_IBUF2_LS_SHADOW_ADR :
		dbyr.in.SetLineStride(data, SHADOW);
		break;
	case SIPP_IBUF2_PS_ADR :
		dbyr.in.SetPlaneStride(data, DEFAULT);
		break;
	case SIPP_IBUF2_PS_SHADOW_ADR :
		dbyr.in.SetPlaneStride(data, SHADOW);
		break;
	case SIPP_IBUF2_IR_ADR :
		dbyr.in.SetIrqConfig(data, DEFAULT);
		break;
	case SIPP_IBUF2_IR_SHADOW_ADR :
		dbyr.in.SetIrqConfig(data, SHADOW);
		break;
	case SIPP_IBUF2_FC_ADR :
		if (data & SIPP_INCDEC_BIT_MASK)
			dbyr.IncInputBufferFillLevel();
		if (data & SIPP_CTXUP_BIT_MASK)
			dbyr.in.SetFillLevel(data & SIPP_NL_MASK);
		break;
	case SIPP_ICTX2_ADR :
		if (data & SIPP_CTXUP_BIT_MASK) {
			dbyr.SetLineIdx(data & SIPP_IMGDIM_MASK);
			if(dbyr.in.GetBuffLines() == 0)
				dbyr.in.SetBufferIdxNoWrap((data >> SIPP_CBL_OFFSET) & SIPP_NL_MASK, dbyr.GetImgDimensions() >> SIPP_IMGDIM_SIZE);
			else
				dbyr.in.SetBufferIdx((data >> SIPP_CBL_OFFSET) & SIPP_NL_MASK);
		}
		if (data & SIPP_START_BIT_MASK) {
			dbyr.in.SetStartBit(data >> SIPP_START_BIT);
			if(dbyr.in.GetBuffLines() == 0)
				dbyr.TryRun();
			else
				dbyr.SetUpAndRun();
		}
		break;

		// IBUF[3] - Chroma denoise filter input buffer
	case SIPP_IBUF3_BASE_ADR :
		cdn.in.SetBase(data, DEFAULT);
		break;
	case SIPP_IBUF3_BASE_SHADOW_ADR :
		cdn.in.SetBase(data, SHADOW);
		break;
	case SIPP_IBUF3_CFG_ADR :
		cdn.in.SetConfig(data, DEFAULT);
		break;
	case SIPP_IBUF3_CFG_SHADOW_ADR :
		cdn.in.SetConfig(data, SHADOW);
		break;
	case SIPP_IBUF3_LS_ADR :
		cdn.in.SetLineStride(data, DEFAULT);
		break;
	case SIPP_IBUF3_LS_SHADOW_ADR :
		cdn.in.SetLineStride(data, SHADOW);
		break;
	case SIPP_IBUF3_PS_ADR :
		cdn.in.SetPlaneStride(data, DEFAULT);
		break;
	case SIPP_IBUF3_PS_SHADOW_ADR :
		cdn.in.SetPlaneStride(data, SHADOW);
		break;
	case SIPP_IBUF3_IR_ADR :
		cdn.in.SetIrqConfig(data, DEFAULT);
		break;
	case SIPP_IBUF3_IR_SHADOW_ADR :
		cdn.in.SetIrqConfig(data, SHADOW);
		break;
	case SIPP_IBUF3_FC_ADR :
		if (data & SIPP_INCDEC_BIT_MASK)
			cdn.IncInputBufferFillLevel();
		if (data & SIPP_CTXUP_BIT_MASK)
			cdn.in.SetFillLevel(data & SIPP_NL_MASK);
		break;
	case SIPP_ICTX3_ADR :
		if (data & SIPP_CTXUP_BIT_MASK) {
			cdn.SetLineIdx(data & SIPP_IMGDIM_MASK);
			if(cdn.in.GetBuffLines() == 0)
				cdn.in.SetBufferIdxNoWrap((data >> SIPP_CBL_OFFSET) & SIPP_NL_MASK, cdn.GetImgDimensions() >> SIPP_IMGDIM_SIZE);
			else
				cdn.in.SetBufferIdx((data >> SIPP_CBL_OFFSET) & SIPP_NL_MASK);
		}
		if (data & SIPP_START_BIT_MASK) {
			cdn.in.SetStartBit(data >> SIPP_START_BIT);
			if(cdn.in.GetBuffLines() == 0)
				cdn.TryRun();
			else
				cdn.SetUpAndRun();
		}
		break;

		// IBUF[4] - Luma denoise filter input buffer
	case SIPP_IBUF4_BASE_ADR :
		luma.in.SetBase(data, DEFAULT);
		break;
	case SIPP_IBUF4_BASE_SHADOW_ADR :
		luma.in.SetBase(data, SHADOW);
		break;
	case SIPP_IBUF4_CFG_ADR :
		luma.in.SetConfig(data, DEFAULT);
		break;
	case SIPP_IBUF4_CFG_SHADOW_ADR :
		luma.in.SetConfig(data, SHADOW);
		break;
	case SIPP_IBUF4_LS_ADR :
		luma.in.SetLineStride(data, DEFAULT);
		break;
	case SIPP_IBUF4_LS_SHADOW_ADR :
		luma.in.SetLineStride(data, SHADOW);
		break;
	case SIPP_IBUF4_PS_ADR :
		luma.in.SetPlaneStride(data, DEFAULT);
		break;
	case SIPP_IBUF4_PS_SHADOW_ADR :
		luma.in.SetPlaneStride(data, SHADOW);
		break;
	case SIPP_IBUF4_IR_ADR :
		luma.in.SetIrqConfig(data, DEFAULT);
		break;
	case SIPP_IBUF4_IR_SHADOW_ADR :
		luma.in.SetIrqConfig(data, SHADOW);
		break;
	case SIPP_IBUF4_FC_ADR :
		if (data & SIPP_INCDEC_BIT_MASK)
			luma.IncInputBufferFillLevel();
		if (data & SIPP_CTXUP_BIT_MASK)
			luma.in.SetFillLevel(data & SIPP_NL_MASK);
		break;
	case SIPP_ICTX4_ADR :
		if (data & SIPP_CTXUP_BIT_MASK) {
			luma.SetLineIdx(data & SIPP_IMGDIM_MASK);
			if(luma.in.GetBuffLines() == 0)
				luma.in.SetBufferIdxNoWrap((data >> SIPP_CBL_OFFSET) & SIPP_NL_MASK, luma.GetImgDimensions() >> SIPP_IMGDIM_SIZE);
			else
				luma.in.SetBufferIdx((data >> SIPP_CBL_OFFSET) & SIPP_NL_MASK);
		}
		if (data & SIPP_START_BIT_MASK) {
			luma.in.SetStartBit(data >> SIPP_START_BIT);
			if(luma.in.GetBuffLines() == 0)
				luma.TryRun();
			else
				luma.SetUpAndRun();
		}
		break;

		// IBUF[5] - Sharpening filter input buffer
	case SIPP_IBUF5_BASE_ADR :
		usm.in.SetBase(data, DEFAULT);
		break;
	case SIPP_IBUF5_BASE_SHADOW_ADR :
		usm.in.SetBase(data, SHADOW);
		break;
	case SIPP_IBUF5_CFG_ADR :
		usm.in.SetConfig(data, DEFAULT);
		break;
	case SIPP_IBUF5_CFG_SHADOW_ADR :
		usm.in.SetConfig(data, SHADOW);
		break;
	case SIPP_IBUF5_LS_ADR :
		usm.in.SetLineStride(data, DEFAULT);
		break;
	case SIPP_IBUF5_LS_SHADOW_ADR :
		usm.in.SetLineStride(data, SHADOW);
		break;
	case SIPP_IBUF5_PS_ADR :
		usm.in.SetPlaneStride(data, DEFAULT);
		break;
	case SIPP_IBUF5_PS_SHADOW_ADR :
		usm.in.SetPlaneStride(data, SHADOW);
		break;
	case SIPP_IBUF5_IR_ADR :
		usm.in.SetIrqConfig(data, DEFAULT);
		break;
	case SIPP_IBUF5_IR_SHADOW_ADR :
		usm.in.SetIrqConfig(data, SHADOW);
		break;
	case SIPP_IBUF5_FC_ADR :
		if (data & SIPP_INCDEC_BIT_MASK)
			usm.IncInputBufferFillLevel();
		if (data & SIPP_CTXUP_BIT_MASK)
			usm.in.SetFillLevel(data & SIPP_NL_MASK);
		break;
	case SIPP_ICTX5_ADR :
		if (data & SIPP_CTXUP_BIT_MASK) {
			usm.SetLineIdx(data & SIPP_IMGDIM_MASK);
			if(usm.in.GetBuffLines() == 0)
				usm.in.SetBufferIdxNoWrap((data >> SIPP_CBL_OFFSET) & SIPP_NL_MASK, usm.GetImgDimensions() >> SIPP_IMGDIM_SIZE);
			else
				usm.in.SetBufferIdx((data >> SIPP_CBL_OFFSET) & SIPP_NL_MASK);
		}
		if (data & SIPP_START_BIT_MASK) {
			usm.in.SetStartBit(data >> SIPP_START_BIT);
			if(usm.in.GetBuffLines() == 0)
				usm.TryRun();
			else
				usm.SetUpAndRun();
		}
		break;

		// IBUF[6] - Polyphase FIR filter input buffer
	case SIPP_IBUF6_BASE_ADR :
		upfirdn.in.SetBase(data, DEFAULT);
		break;
	case SIPP_IBUF6_BASE_SHADOW_ADR :
		upfirdn.in.SetBase(data, SHADOW);
		break;
	case SIPP_IBUF6_CFG_ADR :
		upfirdn.in.SetConfig(data, DEFAULT);
		break;
	case SIPP_IBUF6_CFG_SHADOW_ADR :
		upfirdn.in.SetConfig(data, SHADOW);
		break;
	case SIPP_IBUF6_LS_ADR :
		upfirdn.in.SetLineStride(data, DEFAULT);
		break;
	case SIPP_IBUF6_LS_SHADOW_ADR :
		upfirdn.in.SetLineStride(data, SHADOW);
		break;
	case SIPP_IBUF6_PS_ADR :
		upfirdn.in.SetPlaneStride(data, DEFAULT);
		break;
	case SIPP_IBUF6_PS_SHADOW_ADR :
		upfirdn.in.SetPlaneStride(data, SHADOW);
		break;
	case SIPP_IBUF6_IR_ADR :
		upfirdn.in.SetIrqConfig(data, DEFAULT);
		break;
	case SIPP_IBUF6_IR_SHADOW_ADR :
		upfirdn.in.SetIrqConfig(data, SHADOW);
		break;
	case SIPP_IBUF6_FC_ADR :
		if (data & SIPP_INCDEC_BIT_MASK)
			upfirdn.IncInputBufferFillLevel();
		if (data & SIPP_CTXUP_BIT_MASK)
			upfirdn.in.SetFillLevel(data & SIPP_NL_MASK);
		break;
	case SIPP_ICTX6_ADR :
		if (data & SIPP_CTXUP_BIT_MASK) {
			upfirdn.SetLineIdx(data & SIPP_IMGDIM_MASK);
			if(upfirdn.in.GetBuffLines() == 0)
				upfirdn.in.SetBufferIdxNoWrap((data >> SIPP_CBL_OFFSET) & SIPP_NL_MASK, upfirdn.GetImgDimensions() >> SIPP_IMGDIM_SIZE);
			else
				upfirdn.in.SetBufferIdx((data >> SIPP_CBL_OFFSET) & SIPP_NL_MASK);
		}
		if (data & SIPP_START_BIT_MASK) {
			upfirdn.in.SetStartBit(data >> SIPP_START_BIT);
			if(upfirdn.in.GetBuffLines() == 0)
				upfirdn.TryRun();
			else
				upfirdn.SetUpAndRun();
		}
		break;

		// IBUF[7] - Median filter input buffer
	case SIPP_IBUF7_BASE_ADR :
		med.in.SetBase(data, DEFAULT);
		break;
	case SIPP_IBUF7_BASE_SHADOW_ADR :
		med.in.SetBase(data, SHADOW);
		break;
	case SIPP_IBUF7_CFG_ADR :
		med.in.SetConfig(data, DEFAULT);
		break;
	case SIPP_IBUF7_CFG_SHADOW_ADR :
		med.in.SetConfig(data, SHADOW);
		break;
	case SIPP_IBUF7_LS_ADR :
		med.in.SetLineStride(data, DEFAULT);
		break;
	case SIPP_IBUF7_LS_SHADOW_ADR :
		med.in.SetLineStride(data, SHADOW);
		break;
	case SIPP_IBUF7_PS_ADR :
		med.in.SetPlaneStride(data, DEFAULT);
		break;
	case SIPP_IBUF7_PS_SHADOW_ADR :
		med.in.SetPlaneStride(data, SHADOW);
		break;
	case SIPP_IBUF7_IR_ADR :
		med.in.SetIrqConfig(data, DEFAULT);
		break;
	case SIPP_IBUF7_IR_SHADOW_ADR :
		med.in.SetIrqConfig(data, SHADOW);
		break;
	case SIPP_IBUF7_FC_ADR :
		if (data & SIPP_INCDEC_BIT_MASK)
			med.IncInputBufferFillLevel();
		if (data & SIPP_CTXUP_BIT_MASK)
			med.in.SetFillLevel(data & SIPP_NL_MASK);
		break;
	case SIPP_ICTX7_ADR :
		if (data & SIPP_CTXUP_BIT_MASK) {
			med.SetLineIdx(data & SIPP_IMGDIM_MASK);
			if(med.in.GetBuffLines())
				med.in.SetBufferIdxNoWrap((data >> SIPP_CBL_OFFSET) & SIPP_NL_MASK, med.GetImgDimensions() >> SIPP_IMGDIM_SIZE);
			else
				med.in.SetBufferIdx((data >> SIPP_CBL_OFFSET) & SIPP_NL_MASK);
		}
		if (data & SIPP_START_BIT_MASK) {
			med.in.SetStartBit(data >> SIPP_START_BIT);
			if(med.in.GetBuffLines() == 0)
				med.TryRun();
			else
				med.SetUpAndRun();
		}
		break;

		// IBUF[8] - LUT input buffer
	case SIPP_IBUF8_BASE_ADR :
		lut.in.SetBase(data, DEFAULT);
		break;
	case SIPP_IBUF8_BASE_SHADOW_ADR :
		lut.in.SetBase(data, SHADOW);
		break;
	case SIPP_IBUF8_CFG_ADR :
		lut.in.SetConfig(data, DEFAULT);
		break;
	case SIPP_IBUF8_CFG_SHADOW_ADR :
		lut.in.SetConfig(data, SHADOW);
		break;
	case SIPP_IBUF8_LS_ADR :
		lut.in.SetLineStride(data, DEFAULT);
		break;
	case SIPP_IBUF8_LS_SHADOW_ADR :
		lut.in.SetLineStride(data, SHADOW);
		break;
	case SIPP_IBUF8_PS_ADR :
		lut.in.SetPlaneStride(data, DEFAULT);
		break;
	case SIPP_IBUF8_PS_SHADOW_ADR :
		lut.in.SetPlaneStride(data, SHADOW);
		break;
	case SIPP_IBUF8_IR_ADR :
		lut.in.SetIrqConfig(data, DEFAULT);
		break;
	case SIPP_IBUF8_IR_SHADOW_ADR :
		lut.in.SetIrqConfig(data, SHADOW);
		break;
	case SIPP_IBUF8_FC_ADR :
		if (data & SIPP_INCDEC_BIT_MASK)
			lut.IncInputBufferFillLevel();
		if (data & SIPP_CTXUP_BIT_MASK)
			lut.in.SetFillLevel(data & SIPP_NL_MASK);
		break;
	case SIPP_ICTX8_ADR :
		if (data & SIPP_CTXUP_BIT_MASK) {
			lut.SetLineIdx(data & SIPP_IMGDIM_MASK);
			if(lut.in.GetBuffLines() == 0)
				lut.in.SetBufferIdxNoWrap((data >> SIPP_CBL_OFFSET) & SIPP_NL_MASK, lut.GetImgDimensions() >> SIPP_IMGDIM_SIZE);
			else
				lut.in.SetBufferIdx((data >> SIPP_CBL_OFFSET) & SIPP_NL_MASK);
		}
		if (data & SIPP_START_BIT_MASK) {
			lut.in.SetStartBit(data >> SIPP_START_BIT);
			if(lut.in.GetBuffLines() == 0)
				lut.TryRun();
			else
				lut.SetUpAndRun();
		}
		break;

		// IBUF[9] - Edge operator filter input buffer
	case SIPP_IBUF9_BASE_ADR :
		edge.in.SetBase(data, DEFAULT);
		break;
	case SIPP_IBUF9_BASE_SHADOW_ADR :
		edge.in.SetBase(data, SHADOW);
		break;
	case SIPP_IBUF9_CFG_ADR :
		edge.in.SetConfig(data, DEFAULT);
		break;
	case SIPP_IBUF9_CFG_SHADOW_ADR :
		edge.in.SetConfig(data, SHADOW);
		break;
	case SIPP_IBUF9_LS_ADR :
		edge.in.SetLineStride(data, DEFAULT);
		break;
	case SIPP_IBUF9_LS_SHADOW_ADR :
		edge.in.SetLineStride(data, SHADOW);
		break;
	case SIPP_IBUF9_PS_ADR :
		edge.in.SetPlaneStride(data, DEFAULT);
		break;
	case SIPP_IBUF9_PS_SHADOW_ADR :
		edge.in.SetPlaneStride(data, SHADOW);
		break;
	case SIPP_IBUF9_IR_ADR :
		edge.in.SetIrqConfig(data, DEFAULT);
		break;
	case SIPP_IBUF9_IR_SHADOW_ADR :
		edge.in.SetIrqConfig(data, SHADOW);
		break;
	case SIPP_IBUF9_FC_ADR :
		if (data & SIPP_INCDEC_BIT_MASK)
			edge.IncInputBufferFillLevel();
		if (data & SIPP_CTXUP_BIT_MASK)
			edge.in.SetFillLevel(data & SIPP_NL_MASK);
		break;
	case SIPP_ICTX9_ADR :
		if (data & SIPP_CTXUP_BIT_MASK) {
			edge.SetLineIdx(data & SIPP_IMGDIM_MASK);
			if(edge.in.GetBuffLines() == 0)
				edge.in.SetBufferIdxNoWrap((data >> SIPP_CBL_OFFSET) & SIPP_NL_MASK, edge.GetImgDimensions() >> SIPP_IMGDIM_SIZE);
			else
				edge.in.SetBufferIdx((data >> SIPP_CBL_OFFSET) & SIPP_NL_MASK);
		}
		if (data & SIPP_START_BIT_MASK) {
			edge.in.SetStartBit(data >> SIPP_START_BIT);
			if(edge.in.GetBuffLines() == 0)
				edge.TryRun();
			else
				edge.SetUpAndRun();
		}
		break;

		// IBUF[10] - Programmable convolution filter input buffer
	case SIPP_IBUF10_BASE_ADR :
		conv.in.SetBase(data, DEFAULT);
		break;
	case SIPP_IBUF10_BASE_SHADOW_ADR :
		conv.in.SetBase(data, SHADOW);
		break;
	case SIPP_IBUF10_CFG_ADR :
		conv.in.SetConfig(data, DEFAULT);
		break;
	case SIPP_IBUF10_CFG_SHADOW_ADR :
		conv.in.SetConfig(data, SHADOW);
		break;
	case SIPP_IBUF10_LS_ADR :
		conv.in.SetLineStride(data, DEFAULT);
		break;
	case SIPP_IBUF10_LS_SHADOW_ADR :
		conv.in.SetLineStride(data, SHADOW);
		break;
	case SIPP_IBUF10_PS_ADR :
		conv.in.SetPlaneStride(data, DEFAULT);
		break;
	case SIPP_IBUF10_PS_SHADOW_ADR :
		conv.in.SetPlaneStride(data, SHADOW);
		break;
	case SIPP_IBUF10_IR_ADR :
		conv.in.SetIrqConfig(data, DEFAULT);
		break;
	case SIPP_IBUF10_IR_SHADOW_ADR :
		conv.in.SetIrqConfig(data, SHADOW);
		break;
	case SIPP_IBUF10_FC_ADR :
		if (data & SIPP_INCDEC_BIT_MASK)
			conv.IncInputBufferFillLevel();
		if (data & SIPP_CTXUP_BIT_MASK)
			conv.in.SetFillLevel(data & SIPP_NL_MASK);
		break;
	case SIPP_ICTX10_ADR :
		if (data & SIPP_CTXUP_BIT_MASK) {
			conv.SetLineIdx(data & SIPP_IMGDIM_MASK);
			if(conv.in.GetBuffLines() == 0)
				conv.in.SetBufferIdxNoWrap((data >> SIPP_CBL_OFFSET) & SIPP_NL_MASK, conv.GetImgDimensions() >> SIPP_IMGDIM_SIZE);
			else
				conv.in.SetBufferIdx((data >> SIPP_CBL_OFFSET) & SIPP_NL_MASK);
		}
		if (data & SIPP_START_BIT_MASK) {
			conv.in.SetStartBit(data >> SIPP_START_BIT);
			if(conv.in.GetBuffLines() == 0)
				conv.TryRun();
			else
				conv.SetUpAndRun();
		}
		break;

		// IBUF[11] - Harris corners filter input buffer
	case SIPP_IBUF11_BASE_ADR :
		harr.in.SetBase(data, DEFAULT);
		break;
	case SIPP_IBUF11_BASE_SHADOW_ADR :
		harr.in.SetBase(data, SHADOW);
		break;
	case SIPP_IBUF11_CFG_ADR :
		harr.in.SetConfig(data, DEFAULT);
		break;
	case SIPP_IBUF11_CFG_SHADOW_ADR :
		harr.in.SetConfig(data, SHADOW);
		break;
	case SIPP_IBUF11_LS_ADR :
		harr.in.SetLineStride(data, DEFAULT);
		break;
	case SIPP_IBUF11_LS_SHADOW_ADR :
		harr.in.SetLineStride(data, SHADOW);
		break;
	case SIPP_IBUF11_PS_ADR :
		harr.in.SetPlaneStride(data, DEFAULT);
		break;
	case SIPP_IBUF11_PS_SHADOW_ADR :
		harr.in.SetPlaneStride(data, SHADOW);
		break;
	case SIPP_IBUF11_IR_ADR :
		harr.in.SetIrqConfig(data, DEFAULT);
		break;
	case SIPP_IBUF11_IR_SHADOW_ADR :
		harr.in.SetIrqConfig(data, SHADOW);
		break;
	case SIPP_IBUF11_FC_ADR :
		if (data & SIPP_INCDEC_BIT_MASK)
			harr.IncInputBufferFillLevel();
		if (data & SIPP_CTXUP_BIT_MASK)
			harr.in.SetFillLevel(data & SIPP_NL_MASK);
		break;
	case SIPP_ICTX11_ADR :
		if (data & SIPP_CTXUP_BIT_MASK) {
			harr.SetLineIdx(data & SIPP_IMGDIM_MASK);
			if(harr.in.GetBuffLines() == 0)
				harr.in.SetBufferIdxNoWrap((data >> SIPP_CBL_OFFSET) & SIPP_NL_MASK, harr.GetImgDimensions() >> SIPP_IMGDIM_SIZE);
			else
				harr.in.SetBufferIdx((data >> SIPP_CBL_OFFSET) & SIPP_NL_MASK);
		}
		if (data & SIPP_START_BIT_MASK) {
			harr.in.SetStartBit(data >> SIPP_START_BIT);
			if(harr.in.GetBuffLines() == 0)
				harr.TryRun();
			else
				harr.SetUpAndRun();
		}
		break;

		// IBUF[12] - Colour combination luma input buffer
	case SIPP_IBUF12_BASE_ADR :
		cc.in.SetBase(data, DEFAULT);
		break;
	case SIPP_IBUF12_CFG_ADR :
		cc.in.SetConfig(data, DEFAULT);
		break;
	case SIPP_IBUF12_LS_ADR :
		cc.in.SetLineStride(data, DEFAULT);
		break;
	case SIPP_IBUF12_PS_ADR :
		cc.in.SetPlaneStride(data, DEFAULT);
		break;
	case SIPP_IBUF12_IR_ADR :
		cc.in.SetIrqConfig(data, DEFAULT);
		break;
	case SIPP_IBUF12_FC_ADR :
		if (data & SIPP_INCDEC_BIT_MASK)
			cc.IncInputBufferFillLevel();
		if (data & SIPP_CTXUP_BIT_MASK)
			cc.in.SetFillLevel(data & SIPP_NL_MASK);
		break;
	case SIPP_ICTX12_ADR :
		if (data & SIPP_CTXUP_BIT_MASK) {
			cc.SetLineIdx(data & SIPP_IMGDIM_MASK);
			if(cc.in.GetBuffLines() == 0)
				cc.in.SetBufferIdxNoWrap((data >> SIPP_CBL_OFFSET) & SIPP_NL_MASK, cc.GetImgDimensions() >> SIPP_IMGDIM_SIZE);
			else
				cc.in.SetBufferIdx((data >> SIPP_CBL_OFFSET) & SIPP_NL_MASK);
		}
		if (data & SIPP_START_BIT_MASK) {
			cc.in.SetStartBit(data >> SIPP_START_BIT);
			if(cc.in.GetBuffLines() == 0)
				cc.TryRun();
			else
				cc.SetUpAndRun();
		}
		break;

		// TODO
		// IBUF[14] - MIPI Tx[0] input buffer
		// IBUF[15] - MIPI Tx[1] input buffer

		// IBUF[16] - Lens shading correction gain mesh buffer
	case SIPP_IBUF16_BASE_ADR :
		lsc.lsc_gmb.SetBase(data, DEFAULT);
		break;
	case SIPP_IBUF16_BASE_SHADOW_ADR :
		lsc.lsc_gmb.SetBase(data, SHADOW);
		break;
	case SIPP_IBUF16_CFG_ADR :
		lsc.lsc_gmb.SetConfig(data, DEFAULT);
		break;
	case SIPP_IBUF16_CFG_SHADOW_ADR :
		lsc.lsc_gmb.SetConfig(data, SHADOW);
		break;
	case SIPP_IBUF16_LS_ADR :
		lsc.lsc_gmb.SetLineStride(data, DEFAULT);
		break;
	case SIPP_IBUF16_LS_SHADOW_ADR :
		lsc.lsc_gmb.SetLineStride(data, SHADOW);
		break;
	case SIPP_IBUF16_PS_ADR :
		lsc.lsc_gmb.SetPlaneStride(data, DEFAULT);
		break;
	case SIPP_IBUF16_PS_SHADOW_ADR :
		lsc.lsc_gmb.SetPlaneStride(data, SHADOW);
		break;
	case SIPP_IBUF16_IR_ADR :
		lsc.lsc_gmb.SetIrqConfig(data, DEFAULT);
		break;
	case SIPP_IBUF16_IR_SHADOW_ADR :
		lsc.lsc_gmb.SetIrqConfig(data, SHADOW);
		break;
	case SIPP_IBUF16_FC_ADR :
		if (data & SIPP_CTXUP_BIT_MASK)
			lsc.lsc_gmb.SetFillLevel(data & SIPP_NL_MASK);
		break;
	case SIPP_ICTX16_ADR :
		if (data & SIPP_CTXUP_BIT_MASK)
			lsc.lsc_gmb.SetBufferIdx((data >> SIPP_CBL_OFFSET) & SIPP_NL_MASK);
		break;

		// IBUF[17] - Chroma denoise reference input buffer
	case SIPP_IBUF17_BASE_ADR :
		cdn.ref.SetBase(data, DEFAULT);
		break;
	case SIPP_IBUF17_BASE_SHADOW_ADR :
		cdn.ref.SetBase(data, SHADOW);
		break;
	case SIPP_IBUF17_CFG_ADR :
		cdn.ref.SetConfig(data, DEFAULT);
		break;
	case SIPP_IBUF17_CFG_SHADOW_ADR :
		cdn.ref.SetConfig(data, SHADOW);
		break;
	case SIPP_IBUF17_LS_ADR :
		cdn.ref.SetLineStride(data, DEFAULT);
		break;
	case SIPP_IBUF17_LS_SHADOW_ADR :
		cdn.ref.SetLineStride(data, SHADOW);
		break;
	case SIPP_IBUF17_PS_ADR :
		cdn.ref.SetPlaneStride(data, DEFAULT);
		break;
	case SIPP_IBUF17_PS_SHADOW_ADR :
		cdn.ref.SetPlaneStride(data, SHADOW);
		break;
	case SIPP_IBUF17_IR_ADR :
		cdn.ref.SetIrqConfig(data, DEFAULT);
		break;
	case SIPP_IBUF17_IR_SHADOW_ADR :
		cdn.ref.SetIrqConfig(data, SHADOW);
		break;
	case SIPP_IBUF17_FC_ADR :
		// Reference buffer fill level is incremented synchronously with input
		if (data & SIPP_CTXUP_BIT_MASK)
			cdn.ref.SetFillLevel(data & SIPP_NL_MASK);
		break;
	case SIPP_ICTX17_ADR :
		if (data & SIPP_CTXUP_BIT_MASK)
			if(cdn.ref.GetBuffLines() == 0)
				cdn.ref.SetBufferIdxNoWrap((data >> SIPP_CBL_OFFSET) & SIPP_NL_MASK, cdn.GetImgDimensions() >> SIPP_IMGDIM_SIZE);
			else
				cdn.ref.SetBufferIdx((data >> SIPP_CBL_OFFSET) & SIPP_NL_MASK);
		break;

		// IBUF[18] - Luma denoise reference input buffer
	case SIPP_IBUF18_BASE_ADR :
		luma.ref.SetBase(data, DEFAULT);
		break;
	case SIPP_IBUF18_BASE_SHADOW_ADR :
		luma.ref.SetBase(data, SHADOW);
		break;
	case SIPP_IBUF18_CFG_ADR :
		luma.ref.SetConfig(data, DEFAULT);
		break;
	case SIPP_IBUF18_CFG_SHADOW_ADR :
		luma.ref.SetConfig(data, SHADOW);
		break;
	case SIPP_IBUF18_LS_ADR :
		luma.ref.SetLineStride(data, DEFAULT);
		break;
	case SIPP_IBUF18_LS_SHADOW_ADR :
		luma.ref.SetLineStride(data, SHADOW);
		break;
	case SIPP_IBUF18_PS_ADR :
		luma.ref.SetPlaneStride(data, DEFAULT);
		break;
	case SIPP_IBUF18_PS_SHADOW_ADR :
		luma.ref.SetPlaneStride(data, SHADOW);
		break;
	case SIPP_IBUF18_IR_ADR :
		luma.ref.SetIrqConfig(data, DEFAULT);
		break;
	case SIPP_IBUF18_IR_SHADOW_ADR :
		luma.ref.SetIrqConfig(data, SHADOW);
		break;
	case SIPP_IBUF18_FC_ADR :
		// Reference buffer fill level is incremented synchronously with input
		if (data & SIPP_CTXUP_BIT_MASK)
			luma.ref.SetFillLevel(data & SIPP_NL_MASK);
		break;
	case SIPP_ICTX18_ADR :
		if (data & SIPP_CTXUP_BIT_MASK)
			if(luma.ref.GetBuffLines() == 0)
				luma.ref.SetBufferIdxNoWrap((data >> SIPP_CBL_OFFSET) & SIPP_NL_MASK, luma.GetImgDimensions() >> SIPP_IMGDIM_SIZE);
			else
				luma.ref.SetBufferIdx((data >> SIPP_CBL_OFFSET) & SIPP_NL_MASK);
		break;

		// IBUF[19] - LUT loader
	case SIPP_IBUF19_BASE_ADR :
		lut.loader.SetBase(data);
		break;
	case SIPP_IBUF19_CFG_ADR :
		lut.loader.SetConfig(data);
		break;

		// IBUF[20] - Debayer post-processing median filter input buffer
	case SIPP_IBUF20_BASE_ADR :
		dbyrppm.in.SetBase(data, DEFAULT);
		break;
	case SIPP_IBUF20_BASE_SHADOW_ADR :
		dbyrppm.in.SetBase(data, SHADOW);
		break;
	case SIPP_IBUF20_CFG_ADR :
		dbyrppm.in.SetConfig(data, DEFAULT);
		break;
	case SIPP_IBUF20_CFG_SHADOW_ADR :
		dbyrppm.in.SetConfig(data, SHADOW);
		break;
	case SIPP_IBUF20_LS_ADR :
		dbyrppm.in.SetLineStride(data, DEFAULT);
		break;
	case SIPP_IBUF20_LS_SHADOW_ADR :
		dbyrppm.in.SetLineStride(data, SHADOW);
		break;
	case SIPP_IBUF20_PS_ADR :
		dbyrppm.in.SetPlaneStride(data, DEFAULT);
		break;
	case SIPP_IBUF20_PS_SHADOW_ADR :
		dbyrppm.in.SetPlaneStride(data, SHADOW);
		break;
	case SIPP_IBUF20_IR_ADR :
		dbyrppm.in.SetIrqConfig(data, DEFAULT);
		break;
	case SIPP_IBUF20_IR_SHADOW_ADR :
		dbyrppm.in.SetIrqConfig(data, SHADOW);
		break;
	case SIPP_IBUF20_FC_ADR :
		if (data & SIPP_INCDEC_BIT_MASK)
			dbyrppm.IncInputBufferFillLevel();
		if (data & SIPP_CTXUP_BIT_MASK)
			dbyrppm.in.SetFillLevel(data & SIPP_NL_MASK);
		break;
	case SIPP_ICTX20_ADR :
		if (data & SIPP_CTXUP_BIT_MASK) {
			dbyrppm.SetLineIdx(data & SIPP_IMGDIM_MASK);
			if(dbyrppm.in.GetBuffLines() == 0)
				dbyrppm.in.SetBufferIdxNoWrap((data >> SIPP_CBL_OFFSET) & SIPP_NL_MASK, dbyrppm.GetImgDimensions() >> SIPP_IMGDIM_SIZE);
			else
				dbyrppm.in.SetBufferIdx((data >> SIPP_CBL_OFFSET) & SIPP_NL_MASK);
		}
		if (data & SIPP_START_BIT_MASK) {
			dbyrppm.in.SetStartBit(data >> SIPP_START_BIT);
			if(dbyrppm.in.GetBuffLines() == 0)
				dbyrppm.TryRun();
			else
				dbyrppm.SetUpAndRun();
		}
		break;

		// IBUF[21] - Colour combination chroma input buffer
	case SIPP_IBUF21_BASE_ADR :
		cc.chr.SetBase(data, DEFAULT);
		break;
	case SIPP_IBUF21_CFG_ADR :
		cc.chr.SetConfig(data, DEFAULT);
		break;
	case SIPP_IBUF21_LS_ADR :
		cc.chr.SetLineStride(data, DEFAULT);
		break;
	case SIPP_IBUF21_PS_ADR :
		cc.chr.SetPlaneStride(data, DEFAULT);
		break;
	case SIPP_IBUF21_IR_ADR :
		cc.chr.SetIrqConfig(data, DEFAULT);
		break;
	case SIPP_IBUF21_FC_ADR :
		// Chroma buffer fill level is incremented synchronously with luma
		if (data & SIPP_CTXUP_BIT_MASK)
			cc.chr.SetFillLevel(data & SIPP_NL_MASK);
		break;
	case SIPP_ICTX21_ADR :
		if (data & SIPP_CTXUP_BIT_MASK)
			if(cc.chr.GetBuffLines() == 0)
				cc.chr.SetBufferIdxNoWrap((data >> SIPP_CBL_OFFSET) & SIPP_NL_MASK, (cc.GetImgDimensions() >> SIPP_IMGDIM_SIZE) >> 1);
			else
				cc.chr.SetBufferIdx((data >> SIPP_CBL_OFFSET) & SIPP_NL_MASK);
		break;

  
		/*
		* SIPP output buffers configuration/control
		*
		* Connect APB registers to filter output buffer objects
		*/
		// OBUF[0] - RAW filter output buffer
	case SIPP_OBUF0_BASE_ADR :
		raw.out.SetBase(data, DEFAULT);
		break;
	case SIPP_OBUF0_BASE_SHADOW_ADR :
		raw.out.SetBase(data, SHADOW);
		break;
	case SIPP_OBUF0_CFG_ADR :
		raw.out.SetConfig(data, DEFAULT);
		break;
	case SIPP_OBUF0_CFG_SHADOW_ADR :
		raw.out.SetConfig(data, SHADOW);
		break;
	case SIPP_OBUF0_LS_ADR :
		raw.out.SetLineStride(data, DEFAULT);
		break;
	case SIPP_OBUF0_LS_SHADOW_ADR :
		raw.out.SetLineStride(data, SHADOW);
		break;
	case SIPP_OBUF0_PS_ADR :
		raw.out.SetPlaneStride(data, DEFAULT);
		break;
	case SIPP_OBUF0_PS_SHADOW_ADR :
		raw.out.SetPlaneStride(data, SHADOW);
		break;
	case SIPP_OBUF0_IR_ADR :
		raw.out.SetIrqConfig(data, DEFAULT);
		break;
	case SIPP_OBUF0_IR_SHADOW_ADR :
		raw.out.SetIrqConfig(data, SHADOW);
		break;
	case SIPP_OBUF0_FC_ADR :
		if (data & SIPP_INCDEC_BIT_MASK)
			raw.DecOutputBufferFillLevel();
		if (data & SIPP_CTXUP_BIT_MASK)
			raw.out.SetFillLevel(data & SIPP_NL_MASK);
		break;
	case SIPP_OCTX0_ADR :
		if (data & SIPP_CTXUP_BIT_MASK) {
			raw.SetOutputLineIdx(data & SIPP_IMGDIM_MASK);
			if(raw.out.GetBuffLines() == 0)
				raw.out.SetBufferIdxNoWrap((data >> SIPP_CBL_OFFSET) & SIPP_NL_MASK, raw.GetImgDimensions() >> SIPP_IMGDIM_SIZE);
			else
				raw.out.SetBufferIdx((data >> SIPP_CBL_OFFSET) & SIPP_NL_MASK);
		}
		break;

		// OBUF[1] - Lens shading correction filter output buffer
	case SIPP_OBUF1_BASE_ADR :
		lsc.out.SetBase(data, DEFAULT);
		break;
	case SIPP_OBUF1_BASE_SHADOW_ADR :
		lsc.out.SetBase(data, SHADOW);
		break;
	case SIPP_OBUF1_CFG_ADR :
		lsc.out.SetConfig(data, DEFAULT);
		break;
	case SIPP_OBUF1_CFG_SHADOW_ADR :
		lsc.out.SetConfig(data, SHADOW);
		break;
	case SIPP_OBUF1_LS_ADR :
		lsc.out.SetLineStride(data, DEFAULT);
		break;
	case SIPP_OBUF1_LS_SHADOW_ADR :
		lsc.out.SetLineStride(data, SHADOW);
		break;
	case SIPP_OBUF1_PS_ADR :
		lsc.out.SetPlaneStride(data, DEFAULT);
		break;
	case SIPP_OBUF1_PS_SHADOW_ADR :
		lsc.out.SetPlaneStride(data, SHADOW);
		break;
	case SIPP_OBUF1_IR_ADR :
		lsc.out.SetIrqConfig(data, DEFAULT);
		break;
	case SIPP_OBUF1_IR_SHADOW_ADR :
		lsc.out.SetIrqConfig(data, SHADOW);
		break;
	case SIPP_OBUF1_FC_ADR :
		if (data & SIPP_INCDEC_BIT_MASK)
			lsc.DecOutputBufferFillLevel();
		if (data & SIPP_CTXUP_BIT_MASK) 
			lsc.out.SetFillLevel(data & SIPP_NL_MASK);
		break;
	case SIPP_OCTX1_ADR :
		if (data & SIPP_CTXUP_BIT_MASK) {
			lsc.SetOutputLineIdx(data & SIPP_IMGDIM_MASK);
			if(lsc.out.GetBuffLines() == 0)
				lsc.out.SetBufferIdxNoWrap((data >> SIPP_CBL_OFFSET) & SIPP_NL_MASK, lsc.GetImgDimensions() >> SIPP_IMGDIM_SIZE);
			else
				lsc.out.SetBufferIdx((data >> SIPP_CBL_OFFSET) & SIPP_NL_MASK);
		}
		break;

		// OBUF[2] - Debayer filter output buffer
	case SIPP_OBUF2_BASE_ADR :
		dbyr.out.SetBase(data, DEFAULT);
		break;
	case SIPP_OBUF2_BASE_SHADOW_ADR :
		dbyr.out.SetBase(data, SHADOW);
		break;
	case SIPP_OBUF2_CFG_ADR :
		dbyr.out.SetConfig(data, DEFAULT);
		break;
	case SIPP_OBUF2_CFG_SHADOW_ADR :
		dbyr.out.SetConfig(data, SHADOW);
		break;
	case SIPP_OBUF2_LS_ADR :
		dbyr.out.SetLineStride(data, DEFAULT);
		break;
	case SIPP_OBUF2_LS_SHADOW_ADR :
		dbyr.out.SetLineStride(data, SHADOW);
		break;
	case SIPP_OBUF2_PS_ADR :
		dbyr.out.SetPlaneStride(data, DEFAULT);
		break;
	case SIPP_OBUF2_PS_SHADOW_ADR :
		dbyr.out.SetPlaneStride(data, SHADOW);
		break;
	case SIPP_OBUF2_IR_ADR :
		dbyr.out.SetIrqConfig(data, DEFAULT);
		break;
	case SIPP_OBUF2_IR_SHADOW_ADR :
		dbyr.out.SetIrqConfig(data, SHADOW);
		break;
	case SIPP_OBUF2_FC_ADR :
		if (data & SIPP_INCDEC_BIT_MASK)
			dbyr.DecOutputBufferFillLevel();
		if (data & SIPP_CTXUP_BIT_MASK)
			dbyr.out.SetFillLevel(data & SIPP_NL_MASK);
		break;
	case SIPP_OCTX2_ADR :
		if (data & SIPP_CTXUP_BIT_MASK) {
			dbyr.SetOutputLineIdx(data & SIPP_IMGDIM_MASK);
			if(dbyr.out.GetBuffLines() == 0)
				dbyr.out.SetBufferIdxNoWrap((data >> SIPP_CBL_OFFSET) & SIPP_NL_MASK, dbyr.GetImgDimensions() >> SIPP_IMGDIM_SIZE);
			else
				dbyr.out.SetBufferIdx((data >> SIPP_CBL_OFFSET) & SIPP_NL_MASK);
		}
		break;

		// OBUF[3] - Chroma denoise filter output buffer
	case SIPP_OBUF3_BASE_ADR :
		cdn.out.SetBase(data, DEFAULT);
		break;
	case SIPP_OBUF3_BASE_SHADOW_ADR :
		cdn.out.SetBase(data, SHADOW);
		break;
	case SIPP_OBUF3_CFG_ADR :
		cdn.out.SetConfig(data, DEFAULT);
		break;
	case SIPP_OBUF3_CFG_SHADOW_ADR :
		cdn.out.SetConfig(data, SHADOW);
		break;
	case SIPP_OBUF3_LS_ADR :
		cdn.out.SetLineStride(data, DEFAULT);
		break;
	case SIPP_OBUF3_LS_SHADOW_ADR :
		cdn.out.SetLineStride(data, SHADOW);
		break;
	case SIPP_OBUF3_PS_ADR :
		cdn.out.SetPlaneStride(data, DEFAULT);
		break;
	case SIPP_OBUF3_PS_SHADOW_ADR :
		cdn.out.SetPlaneStride(data, SHADOW);
		break;
	case SIPP_OBUF3_IR_ADR :
		cdn.out.SetIrqConfig(data, DEFAULT);
		break;
	case SIPP_OBUF3_IR_SHADOW_ADR :
		cdn.out.SetIrqConfig(data, SHADOW);
		break;
	case SIPP_OBUF3_FC_ADR :
		if (data & SIPP_INCDEC_BIT_MASK)
			cdn.DecOutputBufferFillLevel();
		if (data & SIPP_CTXUP_BIT_MASK)
			cdn.out.SetFillLevel(data & SIPP_NL_MASK);
		break;
	case SIPP_OCTX3_ADR :
		if (data & SIPP_CTXUP_BIT_MASK) {
			cdn.SetOutputLineIdx(data & SIPP_IMGDIM_MASK);
			if(cdn.out.GetBuffLines() == 0)
				cdn.out.SetBufferIdxNoWrap((data >> SIPP_CBL_OFFSET) & SIPP_NL_MASK, cdn.GetImgDimensions() >> SIPP_IMGDIM_SIZE);
			else
				cdn.out.SetBufferIdx((data >> SIPP_CBL_OFFSET) & SIPP_NL_MASK);
		}
		break;

		// OBUF[4] - Luma denoise filter output buffer
	case SIPP_OBUF4_BASE_ADR :
		luma.out.SetBase(data, DEFAULT);
		break;
	case SIPP_OBUF4_BASE_SHADOW_ADR :
		luma.out.SetBase(data, SHADOW);
		break;
	case SIPP_OBUF4_CFG_ADR :
		luma.out.SetConfig(data, DEFAULT);
		break;
	case SIPP_OBUF4_CFG_SHADOW_ADR :
		luma.out.SetConfig(data, SHADOW);
		break;
	case SIPP_OBUF4_LS_ADR :
		luma.out.SetLineStride(data, DEFAULT);
		break;
	case SIPP_OBUF4_LS_SHADOW_ADR :
		luma.out.SetLineStride(data, SHADOW);
		break;
	case SIPP_OBUF4_PS_ADR :
		luma.out.SetPlaneStride(data, DEFAULT);
		break;
	case SIPP_OBUF4_PS_SHADOW_ADR :
		luma.out.SetPlaneStride(data, SHADOW);
		break;
	case SIPP_OBUF4_IR_ADR :
		luma.out.SetIrqConfig(data, DEFAULT);
		break;
	case SIPP_OBUF4_IR_SHADOW_ADR :
		luma.out.SetIrqConfig(data, SHADOW);
		break;
	case SIPP_OBUF4_FC_ADR :
		if (data & SIPP_INCDEC_BIT_MASK)
			luma.DecOutputBufferFillLevel();
		if (data & SIPP_CTXUP_BIT_MASK)
			luma.out.SetFillLevel(data & SIPP_NL_MASK);
		break;
	case SIPP_OCTX4_ADR :
		if (data & SIPP_CTXUP_BIT_MASK) {
			luma.SetOutputLineIdx(data & SIPP_IMGDIM_MASK);
			if(luma.out.GetBuffLines() == 0)
				luma.out.SetBufferIdxNoWrap((data >> SIPP_CBL_OFFSET) & SIPP_NL_MASK, luma.GetImgDimensions() >> SIPP_IMGDIM_SIZE);
			else
				luma.out.SetBufferIdx((data >> SIPP_CBL_OFFSET) & SIPP_NL_MASK);
		}
		break;

		// OBUF[5] - Sharpening filter output buffer
	case SIPP_OBUF5_BASE_ADR :
		usm.out.SetBase(data, DEFAULT);
		break;
	case SIPP_OBUF5_BASE_SHADOW_ADR :
		usm.out.SetBase(data, SHADOW);
		break;
	case SIPP_OBUF5_CFG_ADR :
		usm.out.SetConfig(data, DEFAULT);
		break;
	case SIPP_OBUF5_CFG_SHADOW_ADR :
		usm.out.SetConfig(data, SHADOW);
		break;
	case SIPP_OBUF5_LS_ADR :
		usm.out.SetLineStride(data, DEFAULT);
		break;
	case SIPP_OBUF5_LS_SHADOW_ADR :
		usm.out.SetLineStride(data, SHADOW);
		break;
	case SIPP_OBUF5_PS_ADR :
		usm.out.SetPlaneStride(data, DEFAULT);
		break;
	case SIPP_OBUF5_PS_SHADOW_ADR :
		usm.out.SetPlaneStride(data, SHADOW);
		break;
	case SIPP_OBUF5_IR_ADR :
		usm.out.SetIrqConfig(data, DEFAULT);
		break;
	case SIPP_OBUF5_IR_SHADOW_ADR :
		usm.out.SetIrqConfig(data, SHADOW);
		break;
	case SIPP_OBUF5_FC_ADR :
		if (data & SIPP_INCDEC_BIT_MASK)
			usm.DecOutputBufferFillLevel();
		if (data & SIPP_CTXUP_BIT_MASK)
			usm.out.SetFillLevel(data & SIPP_NL_MASK);
		break;
	case SIPP_OCTX5_ADR :
		if (data & SIPP_CTXUP_BIT_MASK) {
			usm.SetOutputLineIdx(data & SIPP_IMGDIM_MASK);
			if(usm.out.GetBuffLines() == 0)
				usm.out.SetBufferIdxNoWrap((data >> SIPP_CBL_OFFSET) & SIPP_NL_MASK, usm.GetImgDimensions() >> SIPP_IMGDIM_SIZE);
			else
				usm.out.SetBufferIdx((data >> SIPP_CBL_OFFSET) & SIPP_NL_MASK);
		}
		break;

		// OBUF[6] - Polyphase FIR filter output buffer
	case SIPP_OBUF6_BASE_ADR :
		upfirdn.out.SetBase(data, DEFAULT);
		break;
	case SIPP_OBUF6_BASE_SHADOW_ADR :
		upfirdn.out.SetBase(data, SHADOW);
		break;
	case SIPP_OBUF6_CFG_ADR :
		upfirdn.out.SetConfig(data, DEFAULT);
		break;
	case SIPP_OBUF6_CFG_SHADOW_ADR :
		upfirdn.out.SetConfig(data, SHADOW);
		break;
	case SIPP_OBUF6_LS_ADR :
		upfirdn.out.SetLineStride(data, DEFAULT);
		break;
	case SIPP_OBUF6_LS_SHADOW_ADR :
		upfirdn.out.SetLineStride(data, SHADOW);
		break;
	case SIPP_OBUF6_PS_ADR :
		upfirdn.out.SetPlaneStride(data, DEFAULT);
		break;
	case SIPP_OBUF6_PS_SHADOW_ADR :
		upfirdn.out.SetPlaneStride(data, SHADOW);
		break;
	case SIPP_OBUF6_IR_ADR :
		upfirdn.out.SetIrqConfig(data, DEFAULT);
		break;
	case SIPP_OBUF6_IR_SHADOW_ADR :
		upfirdn.out.SetIrqConfig(data, SHADOW);
		break;
	case SIPP_OBUF6_FC_ADR :
		if (data & SIPP_INCDEC_BIT_MASK)
			upfirdn.DecOutputBufferFillLevel();
		if (data & SIPP_CTXUP_BIT_MASK)
			upfirdn.out.SetFillLevel(data & SIPP_NL_MASK);
		break;
	case SIPP_OCTX6_ADR :
		if (data & SIPP_CTXUP_BIT_MASK) {
			upfirdn.SetOutputLineIdx(data & SIPP_IMGDIM_MASK);
			if(upfirdn.out.GetBuffLines() == 0)
				upfirdn.out.SetBufferIdxNoWrap((data >> SIPP_CBL_OFFSET) & SIPP_NL_MASK, upfirdn.GetOutImgDimensions() >> SIPP_IMGDIM_SIZE);
			else
				upfirdn.out.SetBufferIdx((data >> SIPP_CBL_OFFSET) & SIPP_NL_MASK);
		}
		break;

		// OBUF[7] - Median filter output buffer
	case SIPP_OBUF7_BASE_ADR :
		med.out.SetBase(data, DEFAULT);
		break;
	case SIPP_OBUF7_BASE_SHADOW_ADR :
		med.out.SetBase(data, SHADOW);
		break;
	case SIPP_OBUF7_CFG_ADR :
		med.out.SetConfig(data, DEFAULT);
		break;
	case SIPP_OBUF7_CFG_SHADOW_ADR :
		med.out.SetConfig(data, SHADOW);
		break;
	case SIPP_OBUF7_LS_ADR :
		med.out.SetLineStride(data, DEFAULT);
		break;
	case SIPP_OBUF7_LS_SHADOW_ADR :
		med.out.SetLineStride(data, SHADOW);
		break;
	case SIPP_OBUF7_PS_ADR :
		med.out.SetPlaneStride(data, DEFAULT);
		break;
	case SIPP_OBUF7_PS_SHADOW_ADR :
		med.out.SetPlaneStride(data, SHADOW);
		break;
	case SIPP_OBUF7_IR_ADR :
		med.out.SetIrqConfig(data, DEFAULT);
		break;
	case SIPP_OBUF7_IR_SHADOW_ADR :
		med.out.SetIrqConfig(data, SHADOW);
		break;
	case SIPP_OBUF7_FC_ADR :
		if (data & SIPP_INCDEC_BIT_MASK)
			med.DecOutputBufferFillLevel();
		if (data & SIPP_CTXUP_BIT_MASK) 
			med.out.SetFillLevel(data & SIPP_NL_MASK);
		break;
	case SIPP_OCTX7_ADR :
		if (data & SIPP_CTXUP_BIT_MASK) {
			med.SetOutputLineIdx(data & SIPP_IMGDIM_MASK);
			if(med.out.GetBuffLines() == 0)
				med.out.SetBufferIdxNoWrap((data >> SIPP_CBL_OFFSET) & SIPP_NL_MASK, med.GetImgDimensions() >> SIPP_IMGDIM_SIZE);
			else
				med.out.SetBufferIdx((data >> SIPP_CBL_OFFSET) & SIPP_NL_MASK);
		}
		break;

		// OBUF[8] - LUT output buffer
	case SIPP_OBUF8_BASE_ADR :
		lut.out.SetBase(data, DEFAULT);
		break;
	case SIPP_OBUF8_BASE_SHADOW_ADR :
		lut.out.SetBase(data, SHADOW);
		break;
	case SIPP_OBUF8_CFG_ADR :
		lut.out.SetConfig(data, DEFAULT);
		break;
	case SIPP_OBUF8_CFG_SHADOW_ADR :
		lut.out.SetConfig(data, SHADOW);
		break;
	case SIPP_OBUF8_LS_ADR :
		lut.out.SetLineStride(data, DEFAULT);
		break;
	case SIPP_OBUF8_LS_SHADOW_ADR :
		lut.out.SetLineStride(data, SHADOW);
		break;
	case SIPP_OBUF8_PS_ADR :
		lut.out.SetPlaneStride(data, DEFAULT);
		break;
	case SIPP_OBUF8_PS_SHADOW_ADR :
		lut.out.SetPlaneStride(data, SHADOW);
		break;
	case SIPP_OBUF8_IR_ADR :
		lut.out.SetIrqConfig(data, DEFAULT);
		break;
	case SIPP_OBUF8_IR_SHADOW_ADR :
		lut.out.SetIrqConfig(data, SHADOW);
		break;
	case SIPP_OBUF8_FC_ADR :
		if (data & SIPP_INCDEC_BIT_MASK)
			lut.DecOutputBufferFillLevel();
		if (data & SIPP_CTXUP_BIT_MASK)
			lut.out.SetFillLevel(data & SIPP_NL_MASK);
		break;
	case SIPP_OCTX8_ADR :
		if (data & SIPP_CTXUP_BIT_MASK) {
			lut.SetOutputLineIdx(data & SIPP_IMGDIM_MASK);
			if(lut.out.GetBuffLines() == 0)
				lut.out.SetBufferIdxNoWrap((data >> SIPP_CBL_OFFSET) & SIPP_NL_MASK, lut.GetImgDimensions() >> SIPP_IMGDIM_SIZE);
			else
				lut.out.SetBufferIdx((data >> SIPP_CBL_OFFSET) & SIPP_NL_MASK);
		}
		break;

		// OBUF[9] - Edge operator filter output buffer
	case SIPP_OBUF9_BASE_ADR :
		edge.out.SetBase(data, DEFAULT);
		break;
	case SIPP_OBUF9_BASE_SHADOW_ADR :
		edge.out.SetBase(data, SHADOW);
		break;
	case SIPP_OBUF9_CFG_ADR :
		edge.out.SetConfig(data, DEFAULT);
		break;
	case SIPP_OBUF9_CFG_SHADOW_ADR :
		edge.out.SetConfig(data, SHADOW);
		break;
	case SIPP_OBUF9_LS_ADR :
		edge.out.SetLineStride(data, DEFAULT);
		break;
	case SIPP_OBUF9_LS_SHADOW_ADR :
		edge.out.SetLineStride(data, SHADOW);
		break;
	case SIPP_OBUF9_PS_ADR :
		edge.out.SetPlaneStride(data, DEFAULT);
		break;
	case SIPP_OBUF9_PS_SHADOW_ADR :
		edge.out.SetPlaneStride(data, SHADOW);
		break;
	case SIPP_OBUF9_IR_ADR :
		edge.out.SetIrqConfig(data, DEFAULT);
		break;
	case SIPP_OBUF9_IR_SHADOW_ADR :
		edge.out.SetIrqConfig(data, SHADOW);
		break;
	case SIPP_OBUF9_FC_ADR :
		if (data & SIPP_INCDEC_BIT_MASK)
			edge.DecOutputBufferFillLevel();
		if (data & SIPP_CTXUP_BIT_MASK)
			edge.out.SetFillLevel(data & SIPP_NL_MASK);
		break;
	case SIPP_OCTX9_ADR :
		if (data & SIPP_CTXUP_BIT_MASK) {
			edge.SetOutputLineIdx(data & SIPP_IMGDIM_MASK);
			if(edge.out.GetBuffLines() == 0)
				edge.out.SetBufferIdxNoWrap((data >> SIPP_CBL_OFFSET) & SIPP_NL_MASK, edge.GetImgDimensions() >> SIPP_IMGDIM_SIZE);
			else
				edge.out.SetBufferIdx((data >> SIPP_CBL_OFFSET) & SIPP_NL_MASK);
		}
		break;

		// OBUF[10] - Programmable convolution filter output buffer
	case SIPP_OBUF10_BASE_ADR :
		conv.out.SetBase(data, DEFAULT);
		break;
	case SIPP_OBUF10_BASE_SHADOW_ADR :
		conv.out.SetBase(data, SHADOW);
		break;
	case SIPP_OBUF10_CFG_ADR :
		conv.out.SetConfig(data, DEFAULT);
		break;
	case SIPP_OBUF10_CFG_SHADOW_ADR :
		conv.out.SetConfig(data, SHADOW);
		break;
	case SIPP_OBUF10_LS_ADR :
		conv.out.SetLineStride(data, DEFAULT);
		break;
	case SIPP_OBUF10_LS_SHADOW_ADR :
		conv.out.SetLineStride(data, SHADOW);
		break;
	case SIPP_OBUF10_PS_ADR :
		conv.out.SetPlaneStride(data, DEFAULT);
		break;
	case SIPP_OBUF10_PS_SHADOW_ADR :
		conv.out.SetPlaneStride(data, SHADOW);
		break;
	case SIPP_OBUF10_IR_ADR :
		conv.out.SetIrqConfig(data, DEFAULT);
		break;
	case SIPP_OBUF10_IR_SHADOW_ADR :
		conv.out.SetIrqConfig(data, SHADOW);
		break;
	case SIPP_OBUF10_FC_ADR :
		if (data & SIPP_INCDEC_BIT_MASK)
			conv.DecOutputBufferFillLevel();
		if (data & SIPP_CTXUP_BIT_MASK) 
			conv.out.SetFillLevel(data & SIPP_NL_MASK);
		break;
	case SIPP_OCTX10_ADR :
		if (data & SIPP_CTXUP_BIT_MASK) {
			conv.SetOutputLineIdx(data & SIPP_IMGDIM_MASK);
			if(conv.out.GetBuffLines() == 0)
				conv.out.SetBufferIdxNoWrap((data >> SIPP_CBL_OFFSET) & SIPP_NL_MASK, conv.GetImgDimensions() >> SIPP_IMGDIM_SIZE);
			else
				conv.out.SetBufferIdx((data >> SIPP_CBL_OFFSET) & SIPP_NL_MASK);
		}
		break;

		// OBUF[11] - Harris corners filter output buffer
	case SIPP_OBUF11_BASE_ADR :
		harr.out.SetBase(data, DEFAULT);
		break;
	case SIPP_OBUF11_BASE_SHADOW_ADR :
		harr.out.SetBase(data, SHADOW);
		break;
	case SIPP_OBUF11_CFG_ADR :
		harr.out.SetConfig(data, DEFAULT);
		break;
	case SIPP_OBUF11_CFG_SHADOW_ADR :
		harr.out.SetConfig(data, SHADOW);
		break;
	case SIPP_OBUF11_LS_ADR :
		harr.out.SetLineStride(data, DEFAULT);
		break;
	case SIPP_OBUF11_LS_SHADOW_ADR :
		harr.out.SetLineStride(data, SHADOW);
		break;
	case SIPP_OBUF11_PS_ADR :
		harr.out.SetPlaneStride(data, DEFAULT);
		break;
	case SIPP_OBUF11_PS_SHADOW_ADR :
		harr.out.SetPlaneStride(data, SHADOW);
		break;
	case SIPP_OBUF11_IR_ADR :
		harr.out.SetIrqConfig(data, DEFAULT);
		break;
	case SIPP_OBUF11_IR_SHADOW_ADR :
		harr.out.SetIrqConfig(data, SHADOW);
		break;
	case SIPP_OBUF11_FC_ADR :
		if (data & SIPP_INCDEC_BIT_MASK)
			harr.DecOutputBufferFillLevel();
		if (data & SIPP_CTXUP_BIT_MASK) 
			harr.out.SetFillLevel(data & SIPP_NL_MASK);
		break;
	case SIPP_OCTX11_ADR :
		if (data & SIPP_CTXUP_BIT_MASK) {
			harr.SetOutputLineIdx(data & SIPP_IMGDIM_MASK);
			if(harr.out.GetBuffLines() == 0)
				harr.out.SetBufferIdxNoWrap((data >> SIPP_CBL_OFFSET) & SIPP_NL_MASK, harr.GetImgDimensions() >> SIPP_IMGDIM_SIZE);
			else
				harr.out.SetBufferIdx((data >> SIPP_CBL_OFFSET) & SIPP_NL_MASK);
		}
		break;

		// OBUF[12] - Colour combination RGB output buffer
	case SIPP_OBUF12_BASE_ADR :
		cc.out.SetBase(data, DEFAULT);
		break;
	case SIPP_OBUF12_CFG_ADR :
		cc.out.SetConfig(data, DEFAULT);
		break;
	case SIPP_OBUF12_LS_ADR :
		cc.out.SetLineStride(data, DEFAULT);
		break;
	case SIPP_OBUF12_PS_ADR :
		cc.out.SetPlaneStride(data, DEFAULT);
		break;
	case SIPP_OBUF12_IR_ADR :
		cc.out.SetIrqConfig(data, DEFAULT);
		break;
	case SIPP_OBUF12_FC_ADR :
		if (data & SIPP_INCDEC_BIT_MASK)
			cc.DecOutputBufferFillLevel();
		if (data & SIPP_CTXUP_BIT_MASK) 
			cc.out.SetFillLevel(data & SIPP_NL_MASK);
		break;
	case SIPP_OCTX12_ADR :
		if (data & SIPP_CTXUP_BIT_MASK) {
			cc.SetOutputLineIdx(data & SIPP_IMGDIM_MASK);
			if(cc.out.GetBuffLines() == 0)
				cc.out.SetBufferIdxNoWrap((data >> SIPP_CBL_OFFSET) & SIPP_NL_MASK, cc.GetImgDimensions() >> SIPP_IMGDIM_SIZE);
			else
				cc.out.SetBufferIdx((data >> SIPP_CBL_OFFSET) & SIPP_NL_MASK);
		}
		break;

		// OBUF[15] - RAW Stats collection
	case SIPP_OBUF15_BASE_ADR :
		raw.stats.SetBase(data, DEFAULT);
		break;
	case SIPP_OBUF15_BASE_SHADOW_ADR :
		raw.stats.SetBase(data, SHADOW);
		break;
	case SIPP_OBUF15_CFG_ADR :
		raw.stats.SetConfig(data, DEFAULT);
		break;
	case SIPP_OBUF15_CFG_SHADOW_ADR :
		raw.stats.SetConfig(data, SHADOW);
		break;
	case SIPP_OBUF15_LS_ADR :
		raw.stats.SetLineStride(data, DEFAULT);
		break;
	case SIPP_OBUF15_LS_SHADOW_ADR :
		raw.stats.SetLineStride(data, SHADOW);
		break;
	case SIPP_OBUF15_PS_ADR :
		raw.stats.SetPlaneStride(data, DEFAULT);
		break;
	case SIPP_OBUF15_PS_SHADOW_ADR :
		raw.stats.SetPlaneStride(data, SHADOW);
		break;
	case SIPP_OBUF15_IR_ADR :
		raw.stats.SetIrqConfig(data, DEFAULT);
		break;
	case SIPP_OBUF15_IR_SHADOW_ADR :
		raw.stats.SetIrqConfig(data, SHADOW);
		break;

		// OBUF[16] - MIPI Rx[0]
		// OBUF[17] - MIPI Rx[1]
		// OBUF[18] - MIPI Rx[2]
		// OBUF[19] - MIPI Rx[3]
		
		// OBUF[20] - Debayer post-processing median filter output buffer
	case SIPP_OBUF20_BASE_ADR :
		dbyrppm.out.SetBase(data, DEFAULT);
		break;
	case SIPP_OBUF20_BASE_SHADOW_ADR :
		dbyrppm.out.SetBase(data, SHADOW);
		break;
	case SIPP_OBUF20_CFG_ADR :
		dbyrppm.out.SetConfig(data, DEFAULT);
		break;
	case SIPP_OBUF20_CFG_SHADOW_ADR :
		dbyrppm.out.SetConfig(data, SHADOW);
		break;
	case SIPP_OBUF20_LS_ADR :
		dbyrppm.out.SetLineStride(data, DEFAULT);
		break;
	case SIPP_OBUF20_LS_SHADOW_ADR :
		dbyrppm.out.SetLineStride(data, SHADOW);
		break;
	case SIPP_OBUF20_PS_ADR :
		dbyrppm.out.SetPlaneStride(data, DEFAULT);
		break;
	case SIPP_OBUF20_PS_SHADOW_ADR :
		dbyrppm.out.SetPlaneStride(data, SHADOW);
		break;
	case SIPP_OBUF20_IR_ADR :
		dbyrppm.out.SetIrqConfig(data, DEFAULT);
		break;
	case SIPP_OBUF20_IR_SHADOW_ADR :
		dbyrppm.out.SetIrqConfig(data, SHADOW);
		break;
	case SIPP_OBUF20_FC_ADR :
		if (data & SIPP_INCDEC_BIT_MASK)
			dbyrppm.DecOutputBufferFillLevel();
		if (data & SIPP_CTXUP_BIT_MASK) 
			dbyrppm.out.SetFillLevel(data & SIPP_NL_MASK);
		break;
	case SIPP_OCTX20_ADR :
		if (data & SIPP_CTXUP_BIT_MASK) {
			dbyrppm.SetOutputLineIdx(data & SIPP_IMGDIM_MASK);
			if(dbyrppm.out.GetBuffLines() == 0)
				dbyrppm.out.SetBufferIdxNoWrap((data >> SIPP_CBL_OFFSET) & SIPP_NL_MASK, dbyrppm.GetImgDimensions() >> SIPP_IMGDIM_SIZE);
			else
				dbyrppm.out.SetBufferIdx((data >> SIPP_CBL_OFFSET) & SIPP_NL_MASK);
		}
		break;




		/*
		* SIPP Filter specific configuration
		*
		* Connect APB registers to corresponding filter object methods
		*/
	case SIPP_RAW_FRM_DIM_ADR :
		raw.SetImgDimensions(data, DEFAULT);
		break;
	case SIPP_RAW_FRM_DIM_SHADOW_ADR :
		raw.SetImgDimensions(data, SHADOW);
		break;
	case SIPP_STATS_FRM_DIM_ADR :
		raw.SetStatsWidth(data & 0xffff);
		break;
	case SIPP_STATS_FRM_DIM_SHADOW_ADR :
		raw.SetStatsWidth(data & 0xffff, SHADOW);
		break;
	case SIPP_RAW_CFG_ADR :
		raw.SetRawFormat(data & 0x1);
		raw.SetBayerPattern((data >> 1) & 0x3);
		raw.SetGrGbImbEn((data >> 3) & 0x1);
		raw.SetHotPixEn((data >> 4) & 0x1);
		raw.SetHotGreenPixEn((data >> 5) & 0x1);
		raw.SetStatsPatchEn((data >> 6) & 0x1);
		raw.SetStatsHistEn((data >> 7) & 0x1);
		raw.SetDataWidth((data >> 8) & 0xf);
		raw.SetBadPixelThr((data >> 16) & 0xff);
		break;
	case SIPP_RAW_CFG_SHADOW_ADR :
		raw.SetRawFormat(data & 0x1, SHADOW);
		raw.SetBayerPattern((data >> 1) & 0x3, SHADOW);
		raw.SetGrGbImbEn((data >> 3) & 0x1, SHADOW);
		raw.SetHotPixEn((data >> 4) & 0x1, SHADOW);
		raw.SetHotGreenPixEn((data >> 5) & 0x1, SHADOW);
		raw.SetStatsPatchEn((data >> 6) & 0x1, SHADOW);
		raw.SetStatsHistEn((data >> 7) & 0x1, SHADOW);
		raw.SetDataWidth((data >> 8) & 0xf, SHADOW);
		raw.SetBadPixelThr((data >> 16) & 0xff, SHADOW);
		break;
	case SIPP_GRGB_PLATO_ADR :
		raw.SetDarkPlato(data & 0x3fff);
		raw.SetBrightPlato((data >> 16) & 0x3fff);
		break;
	case SIPP_GRGB_PLATO_SHADOW_ADR :
		raw.SetDarkPlato(data & 0x3fff, SHADOW);
		raw.SetBrightPlato((data >> 16) & 0x3fff, SHADOW);
		break;
	case SIPP_GRGB_SLOPE_ADR :
		raw.SetDarkSlope(data & 0x3fff);
		raw.SetBrightSlope((data >> 16) & 0x3fff);
		break;
	case SIPP_GRGB_SLOPE_SHADOW_ADR :
		raw.SetDarkSlope(data & 0x3fff, SHADOW);
		raw.SetBrightSlope((data >> 16) & 0x3fff, SHADOW);
		break;
	case SIPP_BAD_PIXEL_CFG_ADR :
		raw.SetAlphaRBCold(data & 0xf);
		raw.SetAlphaRBHot((data >> 4) & 0xf);
		raw.SetAlphaGCold((data >> 8) & 0xf);
		raw.SetAlphaGHot((data >> 12) & 0xf);
		raw.SetNoiseLevel((data >> 16) & 0xffff);
		break;
	case SIPP_BAD_PIXEL_CFG_SHADOW_ADR :
		raw.SetAlphaRBCold(data & 0xf, SHADOW);
		raw.SetAlphaRBHot((data >> 4) & 0xf, SHADOW);
		raw.SetAlphaGCold((data >> 8) & 0xf, SHADOW);
		raw.SetAlphaGHot((data >> 12) & 0xf, SHADOW);
		raw.SetNoiseLevel((data >> 16) & 0xffff, SHADOW);
		break;
	case SIPP_GAIN_SATURATE_0_ADR :
		raw.SetGain0( data        & 0xffff);
		raw.SetSat0 ((data >> 16) & 0xffff);
		break;
	case SIPP_GAIN_SATURATE_0_SHADOW_ADR :
		raw.SetGain0( data        & 0xffff, SHADOW);
		raw.SetSat0 ((data >> 16) & 0xffff, SHADOW);
		break;
	case SIPP_GAIN_SATURATE_1_ADR :
		raw.SetGain1( data        & 0xffff);
		raw.SetSat1 ((data >> 16) & 0xffff);
		break;
	case SIPP_GAIN_SATURATE_1_SHADOW_ADR :
		raw.SetGain1( data        & 0xffff, SHADOW);
		raw.SetSat1 ((data >> 16) & 0xffff, SHADOW);
		break;
	case SIPP_GAIN_SATURATE_2_ADR :
		raw.SetGain2( data        & 0xffff);
		raw.SetSat2 ((data >> 16) & 0xffff);
		break;
	case SIPP_GAIN_SATURATE_2_SHADOW_ADR :
		raw.SetGain2( data        & 0xffff, SHADOW);
		raw.SetSat2 ((data >> 16) & 0xffff, SHADOW);
		break;
	case SIPP_GAIN_SATURATE_3_ADR :
		raw.SetGain3( data        & 0xffff);
		raw.SetSat3 ((data >> 16) & 0xffff);
		break;
	case SIPP_GAIN_SATURATE_3_SHADOW_ADR :
		raw.SetGain3( data        & 0xffff, SHADOW);
		raw.SetSat3 ((data >> 16) & 0xffff, SHADOW);
		break;
	case SIPP_STATS_PATCH_CFG_ADR :
		raw.SetNumberOfPatchesH(data & 0x3f);
		raw.SetNumberOfPatchesV((data >> 8) & 0x3f);
		raw.SetPatchWidth ((data >> 16) & 0xff);
		raw.SetPatchHeight((data >> 24) & 0xff);
		break;
	case SIPP_STATS_PATCH_CFG_SHADOW_ADR :
		raw.SetNumberOfPatchesH(data & 0x3f, SHADOW);
		raw.SetNumberOfPatchesV((data >> 8) & 0x3f, SHADOW);
		raw.SetPatchWidth ((data >> 16) & 0xff, SHADOW);
		raw.SetPatchHeight((data >> 24) & 0xff, SHADOW);
		break;
	case SIPP_STATS_PATCH_START_ADR :
		raw.SetStartX( data        & 0xffff);
		raw.SetStartY((data >> 16) & 0xffff);
		break;
	case SIPP_STATS_PATCH_START_SHADOW_ADR :
		raw.SetStartX( data        & 0xffff, SHADOW);
		raw.SetStartY((data >> 16) & 0xffff, SHADOW);
		break;
	case SIPP_STATS_PATCH_SKIP_ADR :
		raw.SetSkipX( data        & 0xffff);
		raw.SetSkipY((data >> 16) & 0xffff);
		break;
	case SIPP_STATS_PATCH_SKIP_SHADOW_ADR :
		raw.SetSkipX( data        & 0xffff, SHADOW);
		raw.SetSkipY((data >> 16) & 0xffff, SHADOW);
		break;
	case SIPP_RAW_STATS_PLANES_ADR :
		raw.SetActivePlanesNo((data >> 20) & 0x3);
		raw.SetActivePlanes(data & 0xfffff);
		break;
	case SIPP_RAW_STATS_PLANES_SHADOW_ADR :
		raw.SetActivePlanesNo((data >> 20) & 0x3, SHADOW);
		raw.SetActivePlanes(data & 0xfffff, SHADOW);
		break;

	case SIPP_LSC_FRM_DIM_ADR :
		lsc.SetImgDimensions(data, DEFAULT);
		break;
	case SIPP_LSC_FRM_DIM_SHADOW_ADR :
		lsc.SetImgDimensions(data, SHADOW);
		break;
	case SIPP_LSC_FRACTION_ADR :
		lsc.SetHorIncrement((data >>  0) & 0xffff);
		lsc.SetVerIncrement((data >> 16) & 0xffff);
		break;
	case SIPP_LSC_FRACTION_SHADOW_ADR :
		lsc.SetHorIncrement((data >>  0) & 0xffff, SHADOW);
		lsc.SetVerIncrement((data >> 16) & 0xffff, SHADOW);
		break;
	case SIPP_LSC_GM_DIM_ADR :
		lsc.SetLscGmWidth ((data >>  0) & 0x3ff);
		lsc.SetLscGmHeight((data >> 16) & 0x3ff);
		break;
	case SIPP_LSC_GM_DIM_SHADOW_ADR :
		lsc.SetLscGmWidth ((data >>  0) & 0x3ff, SHADOW);
		lsc.SetLscGmHeight((data >> 16) & 0x3ff, SHADOW);
		break;
	case SIPP_LSC_CFG_ADR :
		lsc.SetLscFormat(data & 0x1);
		lsc.SetDataWidth((data >> 4) & 0xf);
		break;
	case SIPP_LSC_CFG_SHADOW_ADR :
		lsc.SetLscFormat(data & 0x1, SHADOW);
		lsc.SetDataWidth((data >> 4) & 0xf, SHADOW);
		break;

	case SIPP_DBYR_FRM_DIM_ADR :
		dbyr.SetImgDimensions(data, DEFAULT);
		break;
	case SIPP_DBYR_FRM_DIM_SHADOW_ADR :
		dbyr.SetImgDimensions(data, SHADOW);
		break;
	case SIPP_DBYR_CFG_ADR :
		dbyr.SetBayerPattern(data & 0x3);
		dbyr.SetLumaOnly((data >> 2) & 0x1);
		dbyr.SetForceRbToZero((data >> 3) & 0x1);
		dbyr.SetInputDataWidth((data >> 4) & 0xf);
		dbyr.SetOutputDataWidth((data >> 8) & 0xf);
		dbyr.SetImageOrderOut((data >> 12) & 0x7);
		dbyr.SetPlaneMultiple((data >> 15) & 0x3);
		dbyr.SetGradientMul((data >> 24) & 0xff);
		break;
	case SIPP_DBYR_CFG_SHADOW_ADR :
		dbyr.SetBayerPattern(data & 0x3, SHADOW);
		dbyr.SetLumaOnly((data >> 2) & 0x1, SHADOW);
		dbyr.SetForceRbToZero((data >> 3) & 0x1, SHADOW);
		dbyr.SetInputDataWidth((data >> 4) & 0xf, SHADOW);
		dbyr.SetOutputDataWidth((data >> 8) & 0xf, SHADOW);
		dbyr.SetImageOrderOut((data >> 12) & 0x7, SHADOW);
		dbyr.SetPlaneMultiple((data >> 15) & 0x3, SHADOW);
		dbyr.SetGradientMul((data >> 24) & 0xff, SHADOW);
		break;
	case SIPP_DBYR_THRES_ADR :
		dbyr.SetThreshold1((data >>  0) & 0x1fff);
		dbyr.SetThreshold2((data >> 13) & 0x0fff);
		break;
	case SIPP_DBYR_THRES_SHADOW_ADR :
		dbyr.SetThreshold1((data >>  0) & 0x1fff, SHADOW);
		dbyr.SetThreshold2((data >> 13) & 0x0fff, SHADOW);
		break;
	case SIPP_DBYR_DEWORM_ADR :
		dbyr.SetSlope ((data >>  0) & 0xffff);
		dbyr.SetOffset((data >> 16) & 0xffff);
		break;
	case SIPP_DBYR_DEWORM_SHADOW_ADR :
		dbyr.SetSlope ((data >>  0) & 0xffff, SHADOW);
		dbyr.SetOffset((data >> 16) & 0xffff, SHADOW);
		break;

	case SIPP_DBYR_PPM_FRM_DIM_ADR :
		dbyrppm.SetImgDimensions(data, DEFAULT);
		break;
	case SIPP_DBYR_PPM_FRM_DIM_SHADOW_ADR :
		dbyrppm.SetImgDimensions(data, SHADOW);
		break;
	case SIPP_DBYR_PPM_CFG_ADR :
		dbyrppm.SetDataWidth(data);
		break;
	case SIPP_DBYR_PPM_CFG_SHADOW_ADR :
		dbyrppm.SetDataWidth(data, SHADOW);
		break;

	case SIPP_CHROMA_FRM_DIM_ADR :
		cdn.SetImgDimensions(data, DEFAULT);
		break;
	case SIPP_CHROMA_FRM_DIM_SHADOW_ADR :
		cdn.SetImgDimensions(data, SHADOW);
		break;
	case SIPP_CHROMA_CFG_ADR :
		cdn.SetHorEnable(data & 0x7);
		cdn.SetRefEnable((data >> 3) & 0x1);
		cdn.SetLimit((data >> 4) & 0xff);
		cdn.SetForceWeightsHor((data >> 12) & 0x1);
		cdn.SetForceWeightsVer((data >> 13) & 0x1);
		cdn.SetThreePlaneMode ((data >> 14) & 0x1);
		break;
	case SIPP_CHROMA_CFG_SHADOW_ADR :
		cdn.SetHorEnable(data & 0x7, SHADOW);
		cdn.SetRefEnable((data >> 3) & 0x1, SHADOW);
		cdn.SetLimit((data >> 4) & 0xff, SHADOW);
		cdn.SetForceWeightsHor((data >> 12) & 0x1, SHADOW);
		cdn.SetForceWeightsVer((data >> 13) & 0x1, SHADOW);
		cdn.SetThreePlaneMode ((data >> 14) & 0x1, SHADOW);
		break;
	case SIPP_CHROMA_THRESH_ADR :
		cdn.SetHorThreshold1((data >>  0) & 0xff);
		cdn.SetHorThreshold2((data >>  8) & 0xff);
		cdn.SetVerThreshold1((data >> 16) & 0xff);
		cdn.SetVerThreshold2((data >> 24) & 0xff);
		break;
	case SIPP_CHROMA_THRESH_SHADOW_ADR :
		cdn.SetHorThreshold1((data >>  0) & 0xff, SHADOW);
		cdn.SetHorThreshold2((data >>  8) & 0xff, SHADOW);
		cdn.SetVerThreshold1((data >> 16) & 0xff, SHADOW);
		cdn.SetVerThreshold2((data >> 24) & 0xff, SHADOW);
		break;
	case SIPP_CHROMA_THRESH2_ADR :
		cdn.SetHorThreshold3((data >>  0) & 0xff);
		cdn.SetVerThreshold3((data >> 16) & 0xff);
		break;
	case SIPP_CHROMA_THRESH2_SHADOW_ADR :
		cdn.SetHorThreshold3((data >>  0) & 0xff, SHADOW);
		cdn.SetVerThreshold3((data >> 16) & 0xff, SHADOW);
		break;

	case SIPP_LUMA_FRM_DIM_ADR :
		luma.SetImgDimensions(data, DEFAULT);
		break;
	case SIPP_LUMA_FRM_DIM_SHADOW_ADR :
		luma.SetImgDimensions(data, SHADOW);
		break;
	case SIPP_LUMA_CFG_ADR :
		luma.SetBitPosition(data & 0xf);
		luma.SetAlpha((data >> 8) & 0xffff);
		break;
	case SIPP_LUMA_CFG_SHADOW_ADR :
		luma.SetBitPosition(data & 0xf, SHADOW);
		luma.SetAlpha((data >> 8) & 0xffff, SHADOW);
		break;
	case SIPP_LUMA_LUT0700_ADR :
		luma.SetLUT0700(data);
		break;
	case SIPP_LUMA_LUT0700_SHADOW_ADR :
		luma.SetLUT0700(data, SHADOW);
		break;
	case SIPP_LUMA_LUT1508_ADR :
		luma.SetLUT1508(data);
		break;
	case SIPP_LUMA_LUT1508_SHADOW_ADR :
		luma.SetLUT1508(data, SHADOW);
		break;
	case SIPP_LUMA_LUT2316_ADR :
		luma.SetLUT2316(data);
		break;
	case SIPP_LUMA_LUT2316_SHADOW_ADR :
		luma.SetLUT2316(data, SHADOW);
		break;
	case SIPP_LUMA_LUT3124_ADR :
		luma.SetLUT3124(data);
		break;
	case SIPP_LUMA_LUT3124_SHADOW_ADR :
		luma.SetLUT3124(data, SHADOW);
		break;
	case SIPP_LUMA_F2LUT_ADR :
		luma.SetF2LUT(data);
		break;
	case SIPP_LUMA_F2LUT_SHADOW_ADR :
		luma.SetF2LUT(data, SHADOW);
		break;

	case SIPP_SHARPEN_FRM_DIM_ADR :
		usm.SetImgDimensions(data, DEFAULT);
		break;
	case SIPP_SHARPEN_FRM_DIM_SHADOW_ADR :
		usm.SetImgDimensions(data, SHADOW);
		break;
	case SIPP_SHARPEN_CFG_ADR :
		usm.SetKernelSize(data & 0x7);
		usm.SetOutputClamp((data >> 3) & 0x1);
		usm.SetMode((data >> 4) & 0x1 );
		usm.SetOutputDeltas((data >> 5) & 0x1);
		usm.SetThreshold((data >> 16) & 0xffff);
		break;
	case SIPP_SHARPEN_CFG_SHADOW_ADR :
		usm.SetKernelSize(data & 0x7);
		usm.SetOutputClamp((data >> 3) & 0x1);
		usm.SetMode((data >> 4) & 0x1 );
		usm.SetOutputDeltas((data >> 5) & 0x1);
		usm.SetThreshold((data >> 16) & 0xffff);
		break;
	case SIPP_SHARPEN_STREN_ADR :
		usm.SetPosStrength((data >>  0) & 0xffff);
		usm.SetNegStrength((data >> 16) & 0xffff);
		break;
	case SIPP_SHARPEN_STREN_SHADOW_ADR :
		usm.SetPosStrength((data >>  0) & 0xffff);
		usm.SetNegStrength((data >> 16) & 0xffff);
		break;
	case SIPP_SHARPEN_CLIP_ADR :
		usm.SetClippingAlpha(data & 0xffff);
		break;
	case SIPP_SHARPEN_CLIP_SHADOW_ADR :
		usm.SetClippingAlpha(data & 0xffff);
		break;
	case SIPP_SHARPEN_LIMIT_ADR :
		usm.SetUndershoot(data & 0xffff);
		usm.SetOvershoot((data >> 16) & 0xffff);
		break;
	case SIPP_SHARPEN_LIMIT_SHADOW_ADR :
		usm.SetUndershoot(data & 0xffff);
		usm.SetOvershoot((data >> 16) & 0xffff);
		break;
	case SIPP_SHARPEN_RANGETOP_1_0_ADR :
		usm.SetRngstop0(data & 0xffff);
		usm.SetRngstop1((data >> 16) & 0xffff);
		break;
	case SIPP_SHARPEN_RANGETOP_1_0_SHADOW_ADR :
		usm.SetRngstop0(data & 0xffff);
		usm.SetRngstop1((data >> 16) & 0xffff);
		break;
	case SIPP_SHARPEN_RANGETOP_3_2_ADR :
		usm.SetRngstop2(data & 0xffff);
		usm.SetRngstop3((data >> 16) & 0xffff);
		break;
	case SIPP_SHARPEN_RANGETOP_3_2_SHADOW_ADR :
		usm.SetRngstop2(data & 0xffff);
		usm.SetRngstop3((data >> 16) & 0xffff);
		break;	
	case SIPP_SHARPEN_GAUSIAN_1_0_ADR :
		usm.SetCoeff0(data & 0xffff);
		usm.SetCoeff1((data >> 16) & 0xffff);
		break;
	case SIPP_SHARPEN_GAUSIAN_1_0_SHADOW_ADR :
		usm.SetCoeff0(data & 0xffff);
		usm.SetCoeff1((data >> 16) & 0xffff);
		break;
	case SIPP_SHARPEN_GAUSIAN_3_2_ADR :
		usm.SetCoeff2(data & 0xffff);
		usm.SetCoeff3((data >> 16) & 0xffff);
		break;
	case SIPP_SHARPEN_GAUSIAN_3_2_SHADOW_ADR :
		usm.SetCoeff2(data & 0xffff);
		usm.SetCoeff3((data >> 16) & 0xffff);
		break;

	case SIPP_UPFIRDN_FRM_IN_DIM_ADR :
		upfirdn.SetImgDimensions(data, DEFAULT);
		break;
	case SIPP_UPFIRDN_FRM_IN_DIM_SHADOW_ADR :
		upfirdn.SetImgDimensions(data, SHADOW);
		break;
	case SIPP_UPFIRDN_FRM_OUT_DIM_ADR :
		upfirdn.SetOutImgDimensions(data, DEFAULT);
		break;
	case SIPP_UPFIRDN_FRM_OUT_DIM_SHADOW_ADR :
		upfirdn.SetOutImgDimensions(data, SHADOW);
		break;
	case SIPP_UPFIRDN_CFG_ADR :
		upfirdn.SetKernelSize(data & 0x7);
		upfirdn.SetOutputClamp((data >> 3) & 0x1);
		upfirdn.SetHorizontalDenominator((data >> 4) & 0x3f);
		upfirdn.SetHorizontalNumerator((data >> 10) & 0x1f);
		upfirdn.SetVerticalDenominator((data >> 16) & 0x3f);
		upfirdn.SetVerticalNumerator((data >> 22) & 0x1f);
		break;
	case SIPP_UPFIRDN_CFG_SHADOW_ADR :
		upfirdn.SetKernelSize(data & 0x7, SHADOW);
		upfirdn.SetOutputClamp((data >> 3) & 0x1, SHADOW);
		upfirdn.SetHorizontalDenominator((data >> 4) & 0x3f, SHADOW);
		upfirdn.SetHorizontalNumerator((data >> 10) & 0x1f, SHADOW);
		upfirdn.SetVerticalDenominator((data >> 16) & 0x3f, SHADOW);
		upfirdn.SetVerticalNumerator((data >> 22) & 0x1f, SHADOW);
		break;
	case SIPP_UPFIRDN_VPHASE_ADR :
		if (data & SIPP_CTXUP_BIT_MASK) {
			upfirdn.SetVerticalPhase(data & 0xf);
			upfirdn.SetVerticalDecimationCounter((data >> 8) & 0x7f);
		}
		break;
	case SIPP_UPFIRDN_VCOEFF_P0_1_0_ADR :
	case SIPP_UPFIRDN_VCOEFF_P0_3_2_ADR :
	case SIPP_UPFIRDN_VCOEFF_P0_5_4_ADR :
	case SIPP_UPFIRDN_VCOEFF_P0_7_6_ADR :
	case SIPP_UPFIRDN_VCOEFF_P1_1_0_ADR :
	case SIPP_UPFIRDN_VCOEFF_P1_3_2_ADR :
	case SIPP_UPFIRDN_VCOEFF_P1_5_4_ADR :
	case SIPP_UPFIRDN_VCOEFF_P1_7_6_ADR :
	case SIPP_UPFIRDN_VCOEFF_P2_1_0_ADR :
	case SIPP_UPFIRDN_VCOEFF_P2_3_2_ADR :
	case SIPP_UPFIRDN_VCOEFF_P2_5_4_ADR :
	case SIPP_UPFIRDN_VCOEFF_P2_7_6_ADR :
	case SIPP_UPFIRDN_VCOEFF_P3_1_0_ADR :
	case SIPP_UPFIRDN_VCOEFF_P3_3_2_ADR :
	case SIPP_UPFIRDN_VCOEFF_P3_5_4_ADR :
	case SIPP_UPFIRDN_VCOEFF_P3_7_6_ADR :
	case SIPP_UPFIRDN_VCOEFF_P4_1_0_ADR :
	case SIPP_UPFIRDN_VCOEFF_P4_3_2_ADR :
	case SIPP_UPFIRDN_VCOEFF_P4_5_4_ADR :
	case SIPP_UPFIRDN_VCOEFF_P4_7_6_ADR :
	case SIPP_UPFIRDN_VCOEFF_P5_1_0_ADR :
	case SIPP_UPFIRDN_VCOEFF_P5_3_2_ADR :
	case SIPP_UPFIRDN_VCOEFF_P5_5_4_ADR :
	case SIPP_UPFIRDN_VCOEFF_P5_7_6_ADR :
	case SIPP_UPFIRDN_VCOEFF_P6_1_0_ADR :
	case SIPP_UPFIRDN_VCOEFF_P6_3_2_ADR :
	case SIPP_UPFIRDN_VCOEFF_P6_5_4_ADR :
	case SIPP_UPFIRDN_VCOEFF_P6_7_6_ADR :
	case SIPP_UPFIRDN_VCOEFF_P7_1_0_ADR :
	case SIPP_UPFIRDN_VCOEFF_P7_3_2_ADR :
	case SIPP_UPFIRDN_VCOEFF_P7_5_4_ADR :
	case SIPP_UPFIRDN_VCOEFF_P7_7_6_ADR :
	case SIPP_UPFIRDN_VCOEFF_P8_1_0_ADR :
	case SIPP_UPFIRDN_VCOEFF_P8_3_2_ADR :
	case SIPP_UPFIRDN_VCOEFF_P8_5_4_ADR :
	case SIPP_UPFIRDN_VCOEFF_P8_7_6_ADR :
	case SIPP_UPFIRDN_VCOEFF_P9_1_0_ADR :
	case SIPP_UPFIRDN_VCOEFF_P9_3_2_ADR :
	case SIPP_UPFIRDN_VCOEFF_P9_5_4_ADR :
	case SIPP_UPFIRDN_VCOEFF_P9_7_6_ADR :
	case SIPP_UPFIRDN_VCOEFF_P10_1_0_ADR :
	case SIPP_UPFIRDN_VCOEFF_P10_3_2_ADR :
	case SIPP_UPFIRDN_VCOEFF_P10_5_4_ADR :
	case SIPP_UPFIRDN_VCOEFF_P10_7_6_ADR :
	case SIPP_UPFIRDN_VCOEFF_P11_1_0_ADR :
	case SIPP_UPFIRDN_VCOEFF_P11_3_2_ADR :
	case SIPP_UPFIRDN_VCOEFF_P11_5_4_ADR :
	case SIPP_UPFIRDN_VCOEFF_P11_7_6_ADR :
	case SIPP_UPFIRDN_VCOEFF_P12_1_0_ADR :
	case SIPP_UPFIRDN_VCOEFF_P12_3_2_ADR :
	case SIPP_UPFIRDN_VCOEFF_P12_5_4_ADR :
	case SIPP_UPFIRDN_VCOEFF_P12_7_6_ADR :
	case SIPP_UPFIRDN_VCOEFF_P13_1_0_ADR :
	case SIPP_UPFIRDN_VCOEFF_P13_3_2_ADR :
	case SIPP_UPFIRDN_VCOEFF_P13_5_4_ADR :
	case SIPP_UPFIRDN_VCOEFF_P13_7_6_ADR :
	case SIPP_UPFIRDN_VCOEFF_P14_1_0_ADR :
	case SIPP_UPFIRDN_VCOEFF_P14_3_2_ADR :
	case SIPP_UPFIRDN_VCOEFF_P14_5_4_ADR :
	case SIPP_UPFIRDN_VCOEFF_P14_7_6_ADR :
	case SIPP_UPFIRDN_VCOEFF_P15_1_0_ADR :
	case SIPP_UPFIRDN_VCOEFF_P15_3_2_ADR :
	case SIPP_UPFIRDN_VCOEFF_P15_5_4_ADR :
	case SIPP_UPFIRDN_VCOEFF_P15_7_6_ADR :
		{
			int lsb = addr & 0xfff;
			lsb -= SIPP_UPFIRDN_VCOEFF_P0_1_0_ADR;
			int ph = (lsb >> 4) & 0xf;
			int cf = (lsb & 0xf) >> 1;
			upfirdn.SetVCoeff(cf+0, data & 0xffff, ph);
			if (cf+1 <= 6)
				upfirdn.SetVCoeff(cf+1, (data >> 16) & 0xffff, ph);
			break;
		}
	case SIPP_UPFIRDN_VCOEFF_P0_1_0_SHADOW_ADR :
	case SIPP_UPFIRDN_VCOEFF_P0_3_2_SHADOW_ADR :
	case SIPP_UPFIRDN_VCOEFF_P0_5_4_SHADOW_ADR :
	case SIPP_UPFIRDN_VCOEFF_P0_7_6_SHADOW_ADR :
	case SIPP_UPFIRDN_VCOEFF_P1_1_0_SHADOW_ADR :
	case SIPP_UPFIRDN_VCOEFF_P1_3_2_SHADOW_ADR :
	case SIPP_UPFIRDN_VCOEFF_P1_5_4_SHADOW_ADR :
	case SIPP_UPFIRDN_VCOEFF_P1_7_6_SHADOW_ADR :
	case SIPP_UPFIRDN_VCOEFF_P2_1_0_SHADOW_ADR :
	case SIPP_UPFIRDN_VCOEFF_P2_3_2_SHADOW_ADR :
	case SIPP_UPFIRDN_VCOEFF_P2_5_4_SHADOW_ADR :
	case SIPP_UPFIRDN_VCOEFF_P2_7_6_SHADOW_ADR :
	case SIPP_UPFIRDN_VCOEFF_P3_1_0_SHADOW_ADR :
	case SIPP_UPFIRDN_VCOEFF_P3_3_2_SHADOW_ADR :
	case SIPP_UPFIRDN_VCOEFF_P3_5_4_SHADOW_ADR :
	case SIPP_UPFIRDN_VCOEFF_P3_7_6_SHADOW_ADR :
	case SIPP_UPFIRDN_VCOEFF_P4_1_0_SHADOW_ADR :
	case SIPP_UPFIRDN_VCOEFF_P4_3_2_SHADOW_ADR :
	case SIPP_UPFIRDN_VCOEFF_P4_5_4_SHADOW_ADR :
	case SIPP_UPFIRDN_VCOEFF_P4_7_6_SHADOW_ADR :
	case SIPP_UPFIRDN_VCOEFF_P5_1_0_SHADOW_ADR :
	case SIPP_UPFIRDN_VCOEFF_P5_3_2_SHADOW_ADR :
	case SIPP_UPFIRDN_VCOEFF_P5_5_4_SHADOW_ADR :
	case SIPP_UPFIRDN_VCOEFF_P5_7_6_SHADOW_ADR :
	case SIPP_UPFIRDN_VCOEFF_P6_1_0_SHADOW_ADR :
	case SIPP_UPFIRDN_VCOEFF_P6_3_2_SHADOW_ADR :
	case SIPP_UPFIRDN_VCOEFF_P6_5_4_SHADOW_ADR :
	case SIPP_UPFIRDN_VCOEFF_P6_7_6_SHADOW_ADR :
	case SIPP_UPFIRDN_VCOEFF_P7_1_0_SHADOW_ADR :
	case SIPP_UPFIRDN_VCOEFF_P7_3_2_SHADOW_ADR :
	case SIPP_UPFIRDN_VCOEFF_P7_5_4_SHADOW_ADR :
	case SIPP_UPFIRDN_VCOEFF_P7_7_6_SHADOW_ADR :
	case SIPP_UPFIRDN_VCOEFF_P8_1_0_SHADOW_ADR :
	case SIPP_UPFIRDN_VCOEFF_P8_3_2_SHADOW_ADR :
	case SIPP_UPFIRDN_VCOEFF_P8_5_4_SHADOW_ADR :
	case SIPP_UPFIRDN_VCOEFF_P8_7_6_SHADOW_ADR :
	case SIPP_UPFIRDN_VCOEFF_P9_1_0_SHADOW_ADR :
	case SIPP_UPFIRDN_VCOEFF_P9_3_2_SHADOW_ADR :
	case SIPP_UPFIRDN_VCOEFF_P9_5_4_SHADOW_ADR :
	case SIPP_UPFIRDN_VCOEFF_P9_7_6_SHADOW_ADR :
	case SIPP_UPFIRDN_VCOEFF_P10_1_0_SHADOW_ADR :
	case SIPP_UPFIRDN_VCOEFF_P10_3_2_SHADOW_ADR :
	case SIPP_UPFIRDN_VCOEFF_P10_5_4_SHADOW_ADR :
	case SIPP_UPFIRDN_VCOEFF_P10_7_6_SHADOW_ADR :
	case SIPP_UPFIRDN_VCOEFF_P11_1_0_SHADOW_ADR :
	case SIPP_UPFIRDN_VCOEFF_P11_3_2_SHADOW_ADR :
	case SIPP_UPFIRDN_VCOEFF_P11_5_4_SHADOW_ADR :
	case SIPP_UPFIRDN_VCOEFF_P11_7_6_SHADOW_ADR :
	case SIPP_UPFIRDN_VCOEFF_P12_1_0_SHADOW_ADR :
	case SIPP_UPFIRDN_VCOEFF_P12_3_2_SHADOW_ADR :
	case SIPP_UPFIRDN_VCOEFF_P12_5_4_SHADOW_ADR :
	case SIPP_UPFIRDN_VCOEFF_P12_7_6_SHADOW_ADR :
	case SIPP_UPFIRDN_VCOEFF_P13_1_0_SHADOW_ADR :
	case SIPP_UPFIRDN_VCOEFF_P13_3_2_SHADOW_ADR :
	case SIPP_UPFIRDN_VCOEFF_P13_5_4_SHADOW_ADR :
	case SIPP_UPFIRDN_VCOEFF_P13_7_6_SHADOW_ADR :
	case SIPP_UPFIRDN_VCOEFF_P14_1_0_SHADOW_ADR :
	case SIPP_UPFIRDN_VCOEFF_P14_3_2_SHADOW_ADR :
	case SIPP_UPFIRDN_VCOEFF_P14_5_4_SHADOW_ADR :
	case SIPP_UPFIRDN_VCOEFF_P14_7_6_SHADOW_ADR :
	case SIPP_UPFIRDN_VCOEFF_P15_1_0_SHADOW_ADR :
	case SIPP_UPFIRDN_VCOEFF_P15_3_2_SHADOW_ADR :
	case SIPP_UPFIRDN_VCOEFF_P15_5_4_SHADOW_ADR :
	case SIPP_UPFIRDN_VCOEFF_P15_7_6_SHADOW_ADR :
		{
			int lsb = addr & 0xfff;
			lsb -= SIPP_UPFIRDN_VCOEFF_P0_1_0_SHADOW_ADR;
			int ph = (lsb >> 4) & 0xf;
			int cf = (lsb & 0xf) >> 1;
			upfirdn.SetVCoeff(cf+0, data & 0xffff, ph, SHADOW);
			if (cf+1 <= 6)
				upfirdn.SetVCoeff(cf+1, (data >> 16) & 0xffff, ph, SHADOW);
			break;
		}
	case SIPP_UPFIRDN_HCOEFF_P0_1_0_ADR :
	case SIPP_UPFIRDN_HCOEFF_P0_3_2_ADR :
	case SIPP_UPFIRDN_HCOEFF_P0_5_4_ADR :
	case SIPP_UPFIRDN_HCOEFF_P0_7_6_ADR :
	case SIPP_UPFIRDN_HCOEFF_P1_1_0_ADR :
	case SIPP_UPFIRDN_HCOEFF_P1_3_2_ADR :
	case SIPP_UPFIRDN_HCOEFF_P1_5_4_ADR :
	case SIPP_UPFIRDN_HCOEFF_P1_7_6_ADR :
	case SIPP_UPFIRDN_HCOEFF_P2_1_0_ADR :
	case SIPP_UPFIRDN_HCOEFF_P2_3_2_ADR :
	case SIPP_UPFIRDN_HCOEFF_P2_5_4_ADR :
	case SIPP_UPFIRDN_HCOEFF_P2_7_6_ADR :
	case SIPP_UPFIRDN_HCOEFF_P3_1_0_ADR :
	case SIPP_UPFIRDN_HCOEFF_P3_3_2_ADR :
	case SIPP_UPFIRDN_HCOEFF_P3_5_4_ADR :
	case SIPP_UPFIRDN_HCOEFF_P3_7_6_ADR :
	case SIPP_UPFIRDN_HCOEFF_P4_1_0_ADR :
	case SIPP_UPFIRDN_HCOEFF_P4_3_2_ADR :
	case SIPP_UPFIRDN_HCOEFF_P4_5_4_ADR :
	case SIPP_UPFIRDN_HCOEFF_P4_7_6_ADR :
	case SIPP_UPFIRDN_HCOEFF_P5_1_0_ADR :
	case SIPP_UPFIRDN_HCOEFF_P5_3_2_ADR :
	case SIPP_UPFIRDN_HCOEFF_P5_5_4_ADR :
	case SIPP_UPFIRDN_HCOEFF_P5_7_6_ADR :
	case SIPP_UPFIRDN_HCOEFF_P6_1_0_ADR :
	case SIPP_UPFIRDN_HCOEFF_P6_3_2_ADR :
	case SIPP_UPFIRDN_HCOEFF_P6_5_4_ADR :
	case SIPP_UPFIRDN_HCOEFF_P6_7_6_ADR :
	case SIPP_UPFIRDN_HCOEFF_P7_1_0_ADR :
	case SIPP_UPFIRDN_HCOEFF_P7_3_2_ADR :
	case SIPP_UPFIRDN_HCOEFF_P7_5_4_ADR :
	case SIPP_UPFIRDN_HCOEFF_P7_7_6_ADR :
	case SIPP_UPFIRDN_HCOEFF_P8_1_0_ADR :
	case SIPP_UPFIRDN_HCOEFF_P8_3_2_ADR :
	case SIPP_UPFIRDN_HCOEFF_P8_5_4_ADR :
	case SIPP_UPFIRDN_HCOEFF_P8_7_6_ADR :
	case SIPP_UPFIRDN_HCOEFF_P9_1_0_ADR :
	case SIPP_UPFIRDN_HCOEFF_P9_3_2_ADR :
	case SIPP_UPFIRDN_HCOEFF_P9_5_4_ADR :
	case SIPP_UPFIRDN_HCOEFF_P9_7_6_ADR :
	case SIPP_UPFIRDN_HCOEFF_P10_1_0_ADR :
	case SIPP_UPFIRDN_HCOEFF_P10_3_2_ADR :
	case SIPP_UPFIRDN_HCOEFF_P10_5_4_ADR :
	case SIPP_UPFIRDN_HCOEFF_P10_7_6_ADR :
	case SIPP_UPFIRDN_HCOEFF_P11_1_0_ADR :
	case SIPP_UPFIRDN_HCOEFF_P11_3_2_ADR :
	case SIPP_UPFIRDN_HCOEFF_P11_5_4_ADR :
	case SIPP_UPFIRDN_HCOEFF_P11_7_6_ADR :
	case SIPP_UPFIRDN_HCOEFF_P12_1_0_ADR :
	case SIPP_UPFIRDN_HCOEFF_P12_3_2_ADR :
	case SIPP_UPFIRDN_HCOEFF_P12_5_4_ADR :
	case SIPP_UPFIRDN_HCOEFF_P12_7_6_ADR :
	case SIPP_UPFIRDN_HCOEFF_P13_1_0_ADR :
	case SIPP_UPFIRDN_HCOEFF_P13_3_2_ADR :
	case SIPP_UPFIRDN_HCOEFF_P13_5_4_ADR :
	case SIPP_UPFIRDN_HCOEFF_P13_7_6_ADR :
	case SIPP_UPFIRDN_HCOEFF_P14_1_0_ADR :
	case SIPP_UPFIRDN_HCOEFF_P14_3_2_ADR :
	case SIPP_UPFIRDN_HCOEFF_P14_5_4_ADR :
	case SIPP_UPFIRDN_HCOEFF_P14_7_6_ADR :
	case SIPP_UPFIRDN_HCOEFF_P15_1_0_ADR :
	case SIPP_UPFIRDN_HCOEFF_P15_3_2_ADR :
	case SIPP_UPFIRDN_HCOEFF_P15_5_4_ADR :
	case SIPP_UPFIRDN_HCOEFF_P15_7_6_ADR :
		{
			int lsb = addr & 0xfff;
			lsb -= SIPP_UPFIRDN_VCOEFF_P0_1_0_ADR;
			int ph = (lsb >> 4) & 0xf;
			int cf = (lsb & 0xf) >> 1;
			upfirdn.SetHCoeff(cf+0, data & 0xffff, ph);
			if (cf+1 <= 6)
				upfirdn.SetHCoeff(cf+1, (data >> 16) & 0xffff, ph);
			break;
		}
	case SIPP_UPFIRDN_HCOEFF_P0_1_0_SHADOW_ADR :
	case SIPP_UPFIRDN_HCOEFF_P0_3_2_SHADOW_ADR :
	case SIPP_UPFIRDN_HCOEFF_P0_5_4_SHADOW_ADR :
	case SIPP_UPFIRDN_HCOEFF_P0_7_6_SHADOW_ADR :
	case SIPP_UPFIRDN_HCOEFF_P1_1_0_SHADOW_ADR :
	case SIPP_UPFIRDN_HCOEFF_P1_3_2_SHADOW_ADR :
	case SIPP_UPFIRDN_HCOEFF_P1_5_4_SHADOW_ADR :
	case SIPP_UPFIRDN_HCOEFF_P1_7_6_SHADOW_ADR :
	case SIPP_UPFIRDN_HCOEFF_P2_1_0_SHADOW_ADR :
	case SIPP_UPFIRDN_HCOEFF_P2_3_2_SHADOW_ADR :
	case SIPP_UPFIRDN_HCOEFF_P2_5_4_SHADOW_ADR :
	case SIPP_UPFIRDN_HCOEFF_P2_7_6_SHADOW_ADR :
	case SIPP_UPFIRDN_HCOEFF_P3_1_0_SHADOW_ADR :
	case SIPP_UPFIRDN_HCOEFF_P3_3_2_SHADOW_ADR :
	case SIPP_UPFIRDN_HCOEFF_P3_5_4_SHADOW_ADR :
	case SIPP_UPFIRDN_HCOEFF_P3_7_6_SHADOW_ADR :
	case SIPP_UPFIRDN_HCOEFF_P4_1_0_SHADOW_ADR :
	case SIPP_UPFIRDN_HCOEFF_P4_3_2_SHADOW_ADR :
	case SIPP_UPFIRDN_HCOEFF_P4_5_4_SHADOW_ADR :
	case SIPP_UPFIRDN_HCOEFF_P4_7_6_SHADOW_ADR :
	case SIPP_UPFIRDN_HCOEFF_P5_1_0_SHADOW_ADR :
	case SIPP_UPFIRDN_HCOEFF_P5_3_2_SHADOW_ADR :
	case SIPP_UPFIRDN_HCOEFF_P5_5_4_SHADOW_ADR :
	case SIPP_UPFIRDN_HCOEFF_P5_7_6_SHADOW_ADR :
	case SIPP_UPFIRDN_HCOEFF_P6_1_0_SHADOW_ADR :
	case SIPP_UPFIRDN_HCOEFF_P6_3_2_SHADOW_ADR :
	case SIPP_UPFIRDN_HCOEFF_P6_5_4_SHADOW_ADR :
	case SIPP_UPFIRDN_HCOEFF_P6_7_6_SHADOW_ADR :
	case SIPP_UPFIRDN_HCOEFF_P7_1_0_SHADOW_ADR :
	case SIPP_UPFIRDN_HCOEFF_P7_3_2_SHADOW_ADR :
	case SIPP_UPFIRDN_HCOEFF_P7_5_4_SHADOW_ADR :
	case SIPP_UPFIRDN_HCOEFF_P7_7_6_SHADOW_ADR :
	case SIPP_UPFIRDN_HCOEFF_P8_1_0_SHADOW_ADR :
	case SIPP_UPFIRDN_HCOEFF_P8_3_2_SHADOW_ADR :
	case SIPP_UPFIRDN_HCOEFF_P8_5_4_SHADOW_ADR :
	case SIPP_UPFIRDN_HCOEFF_P8_7_6_SHADOW_ADR :
	case SIPP_UPFIRDN_HCOEFF_P9_1_0_SHADOW_ADR :
	case SIPP_UPFIRDN_HCOEFF_P9_3_2_SHADOW_ADR :
	case SIPP_UPFIRDN_HCOEFF_P9_5_4_SHADOW_ADR :
	case SIPP_UPFIRDN_HCOEFF_P9_7_6_SHADOW_ADR :
	case SIPP_UPFIRDN_HCOEFF_P10_1_0_SHADOW_ADR :
	case SIPP_UPFIRDN_HCOEFF_P10_3_2_SHADOW_ADR :
	case SIPP_UPFIRDN_HCOEFF_P10_5_4_SHADOW_ADR :
	case SIPP_UPFIRDN_HCOEFF_P10_7_6_SHADOW_ADR :
	case SIPP_UPFIRDN_HCOEFF_P11_1_0_SHADOW_ADR :
	case SIPP_UPFIRDN_HCOEFF_P11_3_2_SHADOW_ADR :
	case SIPP_UPFIRDN_HCOEFF_P11_5_4_SHADOW_ADR :
	case SIPP_UPFIRDN_HCOEFF_P11_7_6_SHADOW_ADR :
	case SIPP_UPFIRDN_HCOEFF_P12_1_0_SHADOW_ADR :
	case SIPP_UPFIRDN_HCOEFF_P12_3_2_SHADOW_ADR :
	case SIPP_UPFIRDN_HCOEFF_P12_5_4_SHADOW_ADR :
	case SIPP_UPFIRDN_HCOEFF_P12_7_6_SHADOW_ADR :
	case SIPP_UPFIRDN_HCOEFF_P13_1_0_SHADOW_ADR :
	case SIPP_UPFIRDN_HCOEFF_P13_3_2_SHADOW_ADR :
	case SIPP_UPFIRDN_HCOEFF_P13_5_4_SHADOW_ADR :
	case SIPP_UPFIRDN_HCOEFF_P13_7_6_SHADOW_ADR :
	case SIPP_UPFIRDN_HCOEFF_P14_1_0_SHADOW_ADR :
	case SIPP_UPFIRDN_HCOEFF_P14_3_2_SHADOW_ADR :
	case SIPP_UPFIRDN_HCOEFF_P14_5_4_SHADOW_ADR :
	case SIPP_UPFIRDN_HCOEFF_P14_7_6_SHADOW_ADR :
	case SIPP_UPFIRDN_HCOEFF_P15_1_0_SHADOW_ADR :
	case SIPP_UPFIRDN_HCOEFF_P15_3_2_SHADOW_ADR :
	case SIPP_UPFIRDN_HCOEFF_P15_5_4_SHADOW_ADR :
	case SIPP_UPFIRDN_HCOEFF_P15_7_6_SHADOW_ADR :
		{
			int lsb = addr & 0xfff;
			lsb -= SIPP_UPFIRDN_VCOEFF_P0_1_0_SHADOW_ADR;
			int ph = (lsb >> 4) & 0xf;
			int cf = (lsb & 0xf) >> 1;
			upfirdn.SetHCoeff(cf+0, data & 0xffff, ph, SHADOW);
			if (cf+1 <= 6)
				upfirdn.SetHCoeff(cf+1, (data >> 16) & 0xffff, ph, SHADOW);
			break;
		}

	case SIPP_MED_FRM_DIM_ADR :
		med.SetImgDimensions(data, DEFAULT);
	case SIPP_MED_FRM_DIM_SHADOW_ADR :
		med.SetImgDimensions(data, SHADOW);
		break;
	case SIPP_MED_CFG_ADR :
		med.SetKernelSize(data & 0x7);
        med.SetOutputSelection((data >> 8) & 0x3f);
		med.SetThreshold((data >> 16) & 0x1ff);
		break;
	case SIPP_MED_CFG_SHADOW_ADR :
		med.SetKernelSize(data & 0x7, SHADOW);
        med.SetOutputSelection((data >> 8) & 0x3f, SHADOW);
		med.SetThreshold((data >> 16) & 0x1ff, SHADOW);
		break;

	case SIPP_LUT_FRM_DIM_ADR :
		lut.SetImgDimensions(data, DEFAULT);
		break;
	case SIPP_LUT_FRM_DIM_SHADOW_ADR :
		lut.SetImgDimensions(data, SHADOW);
		break;
	case SIPP_LUT_CFG_ADR :
		lut.SetInterpolationModeEnable(data  & 0x1);
		lut.SetChannelModeEnable((data >> 1) & 0x1);
		lut.SetIntegerWidth((data >> 3) & 0x1f);
		lut.SetNumberOfLUTs((data >> 8) & 0xf);
		lut.SetNumberOfChannels((data >> 12) & 0x3);
		lut.SetLutLoadEnable   ((data >> 14) & 0x1);
		lut.SetApbAccessEnable ((data >> 15) & 0x1);
		break;
	case SIPP_LUT_CFG_SHADOW_ADR :
		lut.SetInterpolationModeEnable(data  & 0x1, SHADOW);
		lut.SetChannelModeEnable((data >> 1) & 0x1, SHADOW);
		lut.SetIntegerWidth((data >> 3) & 0x1f, SHADOW);
		lut.SetNumberOfLUTs((data >> 8) & 0xf, SHADOW);
		lut.SetNumberOfChannels((data >> 12) & 0x3, SHADOW);
		break;
	case SIPP_LUT_SIZES7_0_ADR :
		lut.SetRegions7_0(data);
		break;
	case SIPP_LUT_SIZES7_0_SHADOW_ADR :
		lut.SetRegions7_0(data, SHADOW);
		break;
	case SIPP_LUT_SIZES15_8_ADR :
		lut.SetRegions15_8(data);
		break;
	case SIPP_LUT_SIZES15_8_SHADOW_ADR :
		lut.SetRegions15_8(data, SHADOW);
		break;
	case SIPP_LUT_APB_ACCESS_ADR :
		lut.SetLutValue(data & 0xffff);
		lut.SetRamAddress((data >> 16) & 0x3ff);
		lut.SetRamInstance((data >> 26) & 0x7);
		lut.SetLutRW((data >> 29) & 0x1);
		break;

	case SIPP_EDGE_OP_FRM_DIM_ADR :
		edge.SetImgDimensions(data, DEFAULT);
		break;
	case SIPP_EDGE_OP_FRM_DIM_SHADOW_ADR :
		edge.SetImgDimensions(data, SHADOW);
		break;
	case SIPP_EDGE_OP_CFG_ADR :
		edge.SetInputMode ((data >> 0) & 0x3);
		edge.SetOutputMode((data >> 2) & 0x7);
		edge.SetThetaMode ((data >> 5) & 0x3);
		edge.SetMagnScf((data >> 16) & 0xffff);
		break;
	case SIPP_EDGE_OP_CFG_SHADOW_ADR :
		edge.SetInputMode ((data >> 0) & 0x3, SHADOW);
		edge.SetOutputMode((data >> 2) & 0x7, SHADOW);
		edge.SetThetaMode ((data >> 5) & 0x3, SHADOW);
		edge.SetMagnScf((data >> 16) & 0xffff, SHADOW);
		break;
	case SIPP_EDGE_OP_XCOEFF_ADR :
		edge.SetXCoeff_a( data        & 0x1f);
		edge.SetXCoeff_b((data >> 5 ) & 0x1f);
		edge.SetXCoeff_c((data >> 10) & 0x1f);
		edge.SetXCoeff_d((data >> 15) & 0x1f);
		edge.SetXCoeff_e((data >> 20) & 0x1f);
		edge.SetXCoeff_f((data >> 25) & 0x1f);
		break;
	case SIPP_EDGE_OP_XCOEFF_SHADOW_ADR :
		edge.SetXCoeff_a( data        & 0x1f, SHADOW);
		edge.SetXCoeff_b((data >> 5 ) & 0x1f, SHADOW);
		edge.SetXCoeff_c((data >> 10) & 0x1f, SHADOW);
		edge.SetXCoeff_d((data >> 15) & 0x1f, SHADOW);
		edge.SetXCoeff_e((data >> 20) & 0x1f, SHADOW);
		edge.SetXCoeff_f((data >> 25) & 0x1f, SHADOW);
		break;
	case SIPP_EDGE_OP_YCOEFF_ADR :
		edge.SetYCoeff_a( data        & 0x1f);
		edge.SetYCoeff_b((data >> 5 ) & 0x1f);
		edge.SetYCoeff_c((data >> 10) & 0x1f);
		edge.SetYCoeff_d((data >> 15) & 0x1f);
		edge.SetYCoeff_e((data >> 20) & 0x1f);
		edge.SetYCoeff_f((data >> 25) & 0x1f);
		break;
	case SIPP_EDGE_OP_YCOEFF_SHADOW_ADR :
		edge.SetYCoeff_a( data        & 0x1f, SHADOW);
		edge.SetYCoeff_b((data >> 5 ) & 0x1f, SHADOW);
		edge.SetYCoeff_c((data >> 10) & 0x1f, SHADOW);
		edge.SetYCoeff_d((data >> 15) & 0x1f, SHADOW);
		edge.SetYCoeff_e((data >> 20) & 0x1f, SHADOW);
		edge.SetYCoeff_f((data >> 25) & 0x1f, SHADOW);
		break;

	case SIPP_CONV_FRM_DIM_ADR :
		conv.SetImgDimensions(data, DEFAULT);
		break;
	case SIPP_CONV_FRM_DIM_SHADOW_ADR :
		conv.SetImgDimensions(data, SHADOW);
		break;
	case SIPP_CONV_CFG_ADR :
		conv.SetKernelSize(data & 0x7);
		conv.SetOutputClamp((data >> 3) & 0x1);
		conv.SetAbsBit(     (data >> 4) & 0x1);
		conv.SetSqBit(      (data >> 5) & 0x1);
		conv.SetAccumBit(   (data >> 6) & 0x1);
		conv.SetOdBit(      (data >> 7) & 0x1);
		conv.SetThreshold(  (data >> 8) & 0xffff);
		break;
	case SIPP_CONV_CFG_SHADOW_ADR :
		conv.SetKernelSize(data & 0x7, SHADOW);
		conv.SetOutputClamp((data >> 3) & 0x1, SHADOW);
		conv.SetAbsBit(     (data >> 4) & 0x1, SHADOW);
		conv.SetSqBit(      (data >> 5) & 0x1, SHADOW);
		conv.SetAccumBit(   (data >> 6) & 0x1, SHADOW);
		conv.SetOdBit(      (data >> 7) & 0x1, SHADOW);
		conv.SetThreshold(  (data >> 8) & 0xffff, SHADOW);
		break;
	case SIPP_CONV_ACCUM_ADR :
		// READ ONLY
		break;
	case SIPP_CONV_ACCUM_CNT_ADR :
		// READ ONLY
		break;
	case SIPP_CONV_COEFF_01_00_ADR :
		conv.SetCoeff00( data        & 0xffff);
		conv.SetCoeff01((data >> 16) & 0xffff);
		break;
	case SIPP_CONV_COEFF_01_00_SHADOW_ADR :
		conv.SetCoeff00( data        & 0xffff, SHADOW);
		conv.SetCoeff01((data >> 16) & 0xffff, SHADOW);
		break;
	case SIPP_CONV_COEFF_03_02_ADR :
		conv.SetCoeff02( data        & 0xffff);
		conv.SetCoeff03((data >> 16) & 0xffff);
		break;
	case SIPP_CONV_COEFF_03_02_SHADOW_ADR :
		conv.SetCoeff02( data        & 0xffff, SHADOW);
		conv.SetCoeff03((data >> 16) & 0xffff, SHADOW);
		break;
	case SIPP_CONV_COEFF_04_ADR :
		conv.SetCoeff04( data        & 0xffff);
		break;
	case SIPP_CONV_COEFF_04_SHADOW_ADR :
		conv.SetCoeff04( data        & 0xffff, SHADOW);
		break;
	case SIPP_CONV_COEFF_11_10_ADR :
		conv.SetCoeff10( data        & 0xffff);
		conv.SetCoeff11((data >> 16) & 0xffff);
		break;
	case SIPP_CONV_COEFF_11_10_SHADOW_ADR :
		conv.SetCoeff10( data        & 0xffff, SHADOW);
		conv.SetCoeff11((data >> 16) & 0xffff, SHADOW);
		break;
	case SIPP_CONV_COEFF_13_12_ADR :
		conv.SetCoeff12( data        & 0xffff);
		conv.SetCoeff13((data >> 16) & 0xffff);
		break;
	case SIPP_CONV_COEFF_13_12_SHADOW_ADR :
		conv.SetCoeff12( data        & 0xffff, SHADOW);
		conv.SetCoeff13((data >> 16) & 0xffff, SHADOW);
		break;
	case SIPP_CONV_COEFF_14_ADR :
		conv.SetCoeff14( data        & 0xffff);
		break;
	case SIPP_CONV_COEFF_14_SHADOW_ADR :
		conv.SetCoeff14( data        & 0xffff, SHADOW);
		break;
	case SIPP_CONV_COEFF_21_20_ADR :
		conv.SetCoeff20( data        & 0xffff);
		conv.SetCoeff21((data >> 16) & 0xffff);
		break;
	case SIPP_CONV_COEFF_21_20_SHADOW_ADR :
		conv.SetCoeff20( data        & 0xffff, SHADOW);
		conv.SetCoeff21((data >> 16) & 0xffff, SHADOW);
		break;
	case SIPP_CONV_COEFF_23_22_ADR :
		conv.SetCoeff22( data        & 0xffff);
		conv.SetCoeff23((data >> 16) & 0xffff);
		break;
	case SIPP_CONV_COEFF_23_22_SHADOW_ADR :
		conv.SetCoeff22( data        & 0xffff, SHADOW);
		conv.SetCoeff23((data >> 16) & 0xffff, SHADOW);
		break;
	case SIPP_CONV_COEFF_24_ADR :
		conv.SetCoeff24( data        & 0xffff);
		break;
	case SIPP_CONV_COEFF_24_SHADOW_ADR :
		conv.SetCoeff24( data        & 0xffff, SHADOW);
		break;
	case SIPP_CONV_COEFF_31_30_ADR :
		conv.SetCoeff30( data        & 0xffff);
		conv.SetCoeff31((data >> 16) & 0xffff);
		break;
	case SIPP_CONV_COEFF_31_30_SHADOW_ADR :
		conv.SetCoeff30( data        & 0xffff, SHADOW);
		conv.SetCoeff31((data >> 16) & 0xffff, SHADOW);
		break;
	case SIPP_CONV_COEFF_33_32_ADR :
		conv.SetCoeff32( data        & 0xffff);
		conv.SetCoeff33((data >> 16) & 0xffff);
		break;
	case SIPP_CONV_COEFF_33_32_SHADOW_ADR :
		conv.SetCoeff32( data        & 0xffff, SHADOW);
		conv.SetCoeff33((data >> 16) & 0xffff, SHADOW);
		break;
	case SIPP_CONV_COEFF_34_ADR :
		conv.SetCoeff34( data        & 0xffff);
		break;
	case SIPP_CONV_COEFF_34_SHADOW_ADR :
		conv.SetCoeff34( data        & 0xffff, SHADOW);
		break;
	case SIPP_CONV_COEFF_41_40_ADR :
		conv.SetCoeff40( data        & 0xffff);
		conv.SetCoeff41((data >> 16) & 0xffff);
		break;
	case SIPP_CONV_COEFF_41_40_SHADOW_ADR :
		conv.SetCoeff40( data        & 0xffff, SHADOW);
		conv.SetCoeff41((data >> 16) & 0xffff, SHADOW);
		break;
	case SIPP_CONV_COEFF_43_42_ADR :
		conv.SetCoeff42( data        & 0xffff);
		conv.SetCoeff43((data >> 16) & 0xffff);
		break;
	case SIPP_CONV_COEFF_43_42_SHADOW_ADR :
		conv.SetCoeff42( data        & 0xffff, SHADOW);
		conv.SetCoeff43((data >> 16) & 0xffff, SHADOW);
		break;
	case SIPP_CONV_COEFF_44_ADR :
		conv.SetCoeff44( data        & 0xffff);
		break;
	case SIPP_CONV_COEFF_44_SHADOW_ADR :
		conv.SetCoeff44( data        & 0xffff, SHADOW);
		break;

	case SIPP_HARRIS_FRM_DIM_ADR :
		harr.SetImgDimensions(data, DEFAULT);
		break;
	case SIPP_HARRIS_FRM_DIM_SHADOW_ADR :
		harr.SetImgDimensions(data, SHADOW);
		break;
	case SIPP_HARRIS_CFG_ADR :
		harr.SetKernelSize(data & 0xf);
		break;
	case SIPP_HARRIS_CFG_SHADOW_ADR :
		harr.SetKernelSize(data & 0xf, SHADOW);
		break;
	case SIPP_HARRIS_K_ADR :
		harr.SetK(data);
		break;
	case SIPP_HARRIS_K_SHADOW_ADR :
		harr.SetK(data, SHADOW);
		break;

	case SIPP_CC_FRM_DIM_ADR :
		cc.SetImgDimensions(data, DEFAULT);
		break;
	case SIPP_CC_CFG_ADR :
		cc.SetForceLumaOne(data & 0x1);
		cc.SetMulCoeff ((data >>  8) & 0xff);
		cc.SetThreshold((data >> 16) & 0xff);
		cc.SetPlaneMultiple((data >> 24) & 0x3);
		break;
	case SIPP_CC_KRGB0_ADR :
		cc.SetRPlaneCoeff((data >>  0) & 0xfff);
		cc.SetGPlaneCoeff((data >> 16) & 0xfff);
		break;
	case SIPP_CC_KRGB1_ADR :
		cc.SetBPlaneCoeff((data >>  0) & 0xfff);
		cc.SetEpsilon    ((data >> 16) & 0xfff);
		break;
	case SIPP_CC_CCM10_ADR :
		cc.SetCCM00((data >>  0) & 0xffff);
		cc.SetCCM01((data >> 16) & 0xffff);
		break;
	case SIPP_CC_CCM32_ADR :
		cc.SetCCM02((data >>  0) & 0xffff);
		cc.SetCCM10((data >> 16) & 0xffff);
		break;
	case SIPP_CC_CCM54_ADR :
		cc.SetCCM11((data >>  0) & 0xffff);
		cc.SetCCM12((data >> 16) & 0xffff);
		break;
	case SIPP_CC_CCM76_ADR :
		cc.SetCCM20((data >>  0) & 0xffff);
		cc.SetCCM21((data >> 16) & 0xffff);
		break;
	case SIPP_CC_CCM8_ADR :
		cc.SetCCM22((data >>  0) & 0xffff);
		break;

		// TODO - connect remaining APB registers to filter object methods



	default :
		cerr << setbase (16);
		cerr << "Warning: ApbWrite(): unmapped APB write access to 0x" << addr << endl;
		cerr << setbase (10);
		break;
	}
}




int SippHW::ApbRead (int addr) {
	int data = 0;
#ifdef MOVI_SIM_SIPP
    //printf("\n read address %x -> data %x\n", addr, data);
#endif
	switch (addr) {
    case SIPP_SOFTRST_ADR:
        //ToDo: read the current data.
        break;
	case SIPP_CONTROL_ADR :
		if (raw.IsEnabled())     data |= SIPP_RAW_ID_MASK;
		if (lsc.IsEnabled())     data |= SIPP_LSC_ID_MASK;
		if (dbyr.IsEnabled())    data |= SIPP_DBYR_ID_MASK;
		if (dbyrppm.IsEnabled()) data |= SIPP_DBYR_PPM_ID_MASK;
		if (cdn.IsEnabled())     data |= SIPP_CHROMA_ID_MASK;
		if (luma.IsEnabled())    data |= SIPP_LUMA_ID_MASK;
		if (usm.IsEnabled())     data |= SIPP_SHARPEN_ID_MASK;
		if (upfirdn.IsEnabled()) data |= SIPP_UPFIRDN_ID_MASK;
		if (med.IsEnabled())     data |= SIPP_MED_ID_MASK;
		if (lut.IsEnabled())     data |= SIPP_LUT_ID_MASK;
		if (edge.IsEnabled())    data |= SIPP_EDGE_OP_ID_MASK;
		if (conv.IsEnabled())    data |= SIPP_CONV_ID_MASK;
		if (harr.IsEnabled())    data |= SIPP_HARRIS_ID_MASK;
		if (cc.IsEnabled())      data |= SIPP_CC_ID_MASK;
		break;
	case SIPP_STATUS_ADR : 
		data = 0;
		break;
	case SIPP_INT0_STATUS_ADR :
		data = irq.GetStatus(0);
		break;
	case SIPP_INT0_ENABLE_ADR :
		data = irq.GetEnable(0);
		break;
	case SIPP_INT0_CLEAR_ADR :
		// WRITE ONLY
		break;
	case SIPP_INT1_STATUS_ADR :
		data = irq.GetStatus(1);
		break;
	case SIPP_INT1_ENABLE_ADR :
		data = irq.GetEnable(1);
		break;
	case SIPP_INT1_CLEAR_ADR :
		// WRITE ONLY
		break;
	case SIPP_INT2_STATUS_ADR :
		data = irq.GetStatus(2);
		break;
	case SIPP_INT2_ENABLE_ADR :
		data = irq.GetEnable(2);
		break;
	case SIPP_INT2_CLEAR_ADR :
		// WRITE ONLY
		break;
	case SIPP_SLC_SIZE_ADR :
		data  = SippBaseFilt::GetGlobalSliceSize () <<  0;
		data |= SippBaseFilt::GetGlobalFirstSlice() << 24;
		data |= SippBaseFilt::GetGlobalLastSlice () << 28;
		break;

		/*
		* SIPP input buffers configuration/control
		*
		* Connect APB registers to filter buffer objects
		*/
		// IBUF[0] - RAW input buffer
	case SIPP_IBUF0_BASE_ADR :
		data = raw.in.GetBase(DEFAULT);
		break;
	case SIPP_IBUF0_BASE_SHADOW_ADR :
		data = raw.in.GetBase(SHADOW);
		break;
	case SIPP_IBUF0_CFG_ADR :
		data = raw.in.GetConfig(DEFAULT);
		break;
	case SIPP_IBUF0_CFG_SHADOW_ADR :
		data = raw.in.GetConfig(SHADOW);
		break;
	case SIPP_IBUF0_LS_ADR :
		data = raw.in.GetLineStride(DEFAULT);
		break;
	case SIPP_IBUF0_LS_SHADOW_ADR :
		data = raw.in.GetLineStride(SHADOW);
		break;
	case SIPP_IBUF0_PS_ADR :
		data = raw.in.GetPlaneStride(DEFAULT);
		break;
	case SIPP_IBUF0_PS_SHADOW_ADR :
		data = raw.in.GetPlaneStride(SHADOW);
		break;
	case SIPP_IBUF0_IR_ADR :
		data = raw.in.GetIrqConfig(DEFAULT);
		break;
	case SIPP_IBUF0_IR_SHADOW_ADR :
		data = raw.in.GetIrqConfig(SHADOW);
		break;
	case SIPP_IBUF0_FC_ADR :
		data = raw.in.GetFillLevel();
		break;
	case SIPP_ICTX0_ADR :
		data = raw.GetLineIdx();
		if(raw.in.GetBuffLines() == 0)
			data |= (raw.in.GetBufferIdxNoWrap(raw.GetImgDimensions() >> SIPP_IMGDIM_SIZE) << SIPP_CBL_OFFSET);
		else
			data |= (raw.in.GetBufferIdx() << SIPP_CBL_OFFSET);
		break;

		// IBUF[1] - Lens shading correction input buffer
	case SIPP_IBUF1_BASE_ADR :
		data = lsc.in.GetBase(DEFAULT);
		break;
	case SIPP_IBUF1_BASE_SHADOW_ADR :
		data = lsc.in.GetBase(SHADOW);
		break;
	case SIPP_IBUF1_CFG_ADR :
		data = lsc.in.GetConfig(DEFAULT);
		break;
	case SIPP_IBUF1_CFG_SHADOW_ADR :
		data = lsc.in.GetConfig(SHADOW);
		break;
	case SIPP_IBUF1_LS_ADR :
		data = lsc.in.GetLineStride(DEFAULT);
		break;
	case SIPP_IBUF1_LS_SHADOW_ADR :
		data = lsc.in.GetLineStride(SHADOW);
		break;
	case SIPP_IBUF1_PS_ADR :
		data = lsc.in.GetPlaneStride(DEFAULT);
		break;
	case SIPP_IBUF1_PS_SHADOW_ADR :
		data = lsc.in.GetPlaneStride(SHADOW);
		break;
	case SIPP_IBUF1_IR_ADR :
		data = lsc.in.GetIrqConfig(DEFAULT);
		break;
	case SIPP_IBUF1_IR_SHADOW_ADR :
		data = lsc.in.GetIrqConfig(SHADOW);
		break;
	case SIPP_IBUF1_FC_ADR :
		data = lsc.in.GetFillLevel();
		break;
	case SIPP_ICTX1_ADR :
		data = lsc.GetLineIdx();
		if(lsc.in.GetBuffLines() == 0)
			data |= (lsc.in.GetBufferIdxNoWrap(lsc.GetImgDimensions() >> SIPP_IMGDIM_SIZE) << SIPP_CBL_OFFSET);
		else
			data |= (lsc.in.GetBufferIdx() << SIPP_CBL_OFFSET);		
		break;

		// IBUF[2] - Debayer input buffer
	case SIPP_IBUF2_BASE_ADR :
		data = dbyr.in.GetBase(DEFAULT);
		break;
	case SIPP_IBUF2_BASE_SHADOW_ADR :
		data = dbyr.in.GetBase(SHADOW);
		break;
	case SIPP_IBUF2_CFG_ADR :
		data = dbyr.in.GetConfig(DEFAULT);
		break;
	case SIPP_IBUF2_CFG_SHADOW_ADR :
		data = dbyr.in.GetConfig(SHADOW);
		break;
	case SIPP_IBUF2_LS_ADR :
		data = dbyr.in.GetLineStride(DEFAULT);
		break;
	case SIPP_IBUF2_LS_SHADOW_ADR :
		data = dbyr.in.GetLineStride(SHADOW);
		break;
	case SIPP_IBUF2_PS_ADR :
		data = dbyr.in.GetPlaneStride(DEFAULT);
		break;
	case SIPP_IBUF2_PS_SHADOW_ADR :
		data = dbyr.in.GetPlaneStride(SHADOW);
		break;
	case SIPP_IBUF2_IR_ADR :
		data = dbyr.in.GetIrqConfig(DEFAULT);
		break;
	case SIPP_IBUF2_IR_SHADOW_ADR :
		data = dbyr.in.GetIrqConfig(SHADOW);
		break;
	case SIPP_IBUF2_FC_ADR :
		data = dbyr.in.GetFillLevel();
		break;
	case SIPP_ICTX2_ADR :
		data = dbyr.GetLineIdx();
		if(dbyr.in.GetBuffLines() == 0)
			data |= (dbyr.in.GetBufferIdxNoWrap(dbyr.GetImgDimensions() >> SIPP_IMGDIM_SIZE) << SIPP_CBL_OFFSET);
		else
			data |= (dbyr.in.GetBufferIdx() << SIPP_CBL_OFFSET);
		break;
		
		// IBUF[3] - Chroma denoise input buffer
	case SIPP_IBUF3_BASE_ADR :
		data = cdn.in.GetBase(DEFAULT);
		break;
	case SIPP_IBUF3_BASE_SHADOW_ADR :
		data = cdn.in.GetBase(SHADOW);
		break;
	case SIPP_IBUF3_CFG_ADR :
		data = cdn.in.GetConfig(DEFAULT);
		break;
	case SIPP_IBUF3_CFG_SHADOW_ADR :
		data = cdn.in.GetConfig(SHADOW);
		break;
	case SIPP_IBUF3_LS_ADR :
		data = cdn.in.GetLineStride(DEFAULT);
		break;
	case SIPP_IBUF3_LS_SHADOW_ADR :
		data = cdn.in.GetLineStride(SHADOW);
		break;
	case SIPP_IBUF3_PS_ADR :
		data = cdn.in.GetPlaneStride(DEFAULT);
		break;
	case SIPP_IBUF3_PS_SHADOW_ADR :
		data = cdn.in.GetPlaneStride(SHADOW);
		break;
	case SIPP_IBUF3_IR_ADR :
		data = cdn.in.GetIrqConfig(DEFAULT);
		break;
	case SIPP_IBUF3_IR_SHADOW_ADR :
		data = cdn.in.GetIrqConfig(SHADOW);
		break;
	case SIPP_IBUF3_FC_ADR :
		data = cdn.in.GetFillLevel();
		break;
	case SIPP_ICTX3_ADR :
		data = cdn.GetLineIdx();
		if(cdn.in.GetBuffLines() == 0)
			data |= (cdn.in.GetBufferIdxNoWrap(cdn.GetImgDimensions() >> SIPP_IMGDIM_SIZE) << SIPP_CBL_OFFSET);
		else
			data |= (cdn.in.GetBufferIdx() << SIPP_CBL_OFFSET);
		break;

		// IBUF[4] - Luma denoise input buffer
	case SIPP_IBUF4_BASE_ADR :
		data = luma.in.GetBase(DEFAULT);
		break;
	case SIPP_IBUF4_BASE_SHADOW_ADR :
		data = luma.in.GetBase(SHADOW);
		break;
	case SIPP_IBUF4_CFG_ADR :
		data = luma.in.GetConfig(DEFAULT);
		break;
	case SIPP_IBUF4_CFG_SHADOW_ADR :
		data = luma.in.GetConfig(SHADOW);
		break;
	case SIPP_IBUF4_LS_ADR :
		data = luma.in.GetLineStride(DEFAULT);
		break;
	case SIPP_IBUF4_LS_SHADOW_ADR :
		data = luma.in.GetLineStride(SHADOW);
		break;
	case SIPP_IBUF4_PS_ADR :
		data = luma.in.GetPlaneStride(DEFAULT);
		break;
	case SIPP_IBUF4_PS_SHADOW_ADR :
		data = luma.in.GetPlaneStride(SHADOW);
		break;
	case SIPP_IBUF4_IR_ADR :
		data = luma.in.GetIrqConfig(DEFAULT);
		break;
	case SIPP_IBUF4_IR_SHADOW_ADR :
		data = luma.in.GetIrqConfig(SHADOW);
		break;
	case SIPP_IBUF4_FC_ADR :
		data = luma.in.GetFillLevel();
		break;
	case SIPP_ICTX4_ADR :
		data = luma.GetLineIdx();
		if(luma.in.GetBuffLines() == 0)
			data |= (luma.in.GetBufferIdxNoWrap(luma.GetImgDimensions() >> SIPP_IMGDIM_SIZE) << SIPP_CBL_OFFSET);
		else
			data |= (luma.in.GetBufferIdx() << SIPP_CBL_OFFSET);
		break;

		// IBUF[5] - Sharpen input buffer
	case SIPP_IBUF5_BASE_ADR :
		data = usm.in.GetBase(DEFAULT);
		break;
	case SIPP_IBUF5_BASE_SHADOW_ADR :
		data = usm.in.GetBase(SHADOW);
		break;
	case SIPP_IBUF5_CFG_ADR :
		data = usm.in.GetConfig(DEFAULT);
		break;
	case SIPP_IBUF5_CFG_SHADOW_ADR :
		data = usm.in.GetConfig(SHADOW);
		break;
	case SIPP_IBUF5_LS_ADR :
		data = usm.in.GetLineStride(DEFAULT);
		break;
	case SIPP_IBUF5_LS_SHADOW_ADR :
		data = usm.in.GetLineStride(SHADOW);
		break;
	case SIPP_IBUF5_PS_ADR :
		data = usm.in.GetPlaneStride(DEFAULT);
		break;
	case SIPP_IBUF5_PS_SHADOW_ADR :
		data = usm.in.GetPlaneStride(SHADOW);
		break;
	case SIPP_IBUF5_IR_ADR :
		data = usm.in.GetIrqConfig(DEFAULT);
		break;
	case SIPP_IBUF5_IR_SHADOW_ADR :
		data = usm.in.GetIrqConfig(SHADOW);
		break;
	case SIPP_IBUF5_FC_ADR :
		data = usm.in.GetFillLevel();
		break;
	case SIPP_ICTX5_ADR :
		data = usm.GetLineIdx();
		if(usm.in.GetBuffLines() == 0)
			data |= (usm.in.GetBufferIdxNoWrap(usm.GetImgDimensions() >> SIPP_IMGDIM_SIZE) << SIPP_CBL_OFFSET);
		else
			data |= (usm.in.GetBufferIdx() << SIPP_CBL_OFFSET);
		break;

		// IBUF[6] - Polyphase FIR input buffer
	case SIPP_IBUF6_BASE_ADR :
		data = upfirdn.in.GetBase(DEFAULT);
		break;
	case SIPP_IBUF6_BASE_SHADOW_ADR :
		data = upfirdn.in.GetBase(SHADOW);
		break;
	case SIPP_IBUF6_CFG_ADR :
		data = upfirdn.in.GetConfig(DEFAULT);
		break;
	case SIPP_IBUF6_CFG_SHADOW_ADR :
		data = upfirdn.in.GetConfig(SHADOW);
		break;
	case SIPP_IBUF6_LS_ADR :
		data = upfirdn.in.GetLineStride(DEFAULT);
		break;
	case SIPP_IBUF6_LS_SHADOW_ADR :
		data = upfirdn.in.GetLineStride(SHADOW);
		break;
	case SIPP_IBUF6_PS_ADR :
		data = upfirdn.in.GetPlaneStride(DEFAULT);
		break;
	case SIPP_IBUF6_PS_SHADOW_ADR :
		data = upfirdn.in.GetPlaneStride(SHADOW);
		break;
	case SIPP_IBUF6_IR_ADR :
		data = upfirdn.in.GetIrqConfig(DEFAULT);
		break;
	case SIPP_IBUF6_IR_SHADOW_ADR :
		data = upfirdn.in.GetIrqConfig(SHADOW);
		break;
	case SIPP_IBUF6_FC_ADR :
		data = upfirdn.in.GetFillLevel();
		break;
	case SIPP_ICTX6_ADR :
		data = upfirdn.GetLineIdx();
		if(upfirdn.in.GetBuffLines() == 0)
			data |= (upfirdn.in.GetBufferIdxNoWrap(upfirdn.GetImgDimensions() >> SIPP_IMGDIM_SIZE) << SIPP_CBL_OFFSET);
		else
			data |= (upfirdn.in.GetBufferIdx() << SIPP_CBL_OFFSET);
		break;

		// IBUF[7] - Median input buffer
	case SIPP_IBUF7_BASE_ADR :
		data = med.in.GetBase(DEFAULT);
		break;
	case SIPP_IBUF7_BASE_SHADOW_ADR :
		data = med.in.GetBase(SHADOW);
		break;
	case SIPP_IBUF7_CFG_ADR :
		data = med.in.GetConfig(DEFAULT);
		break;
	case SIPP_IBUF7_CFG_SHADOW_ADR :
		data = med.in.GetConfig(SHADOW);
		break;
	case SIPP_IBUF7_LS_ADR :
		data = med.in.GetLineStride(DEFAULT);
		break;
	case SIPP_IBUF7_LS_SHADOW_ADR :
		data = med.in.GetLineStride(SHADOW);
		break;
	case SIPP_IBUF7_PS_ADR :
		data = med.in.GetPlaneStride(DEFAULT);
		break;
	case SIPP_IBUF7_PS_SHADOW_ADR :
		data = med.in.GetPlaneStride(SHADOW);
		break;
	case SIPP_IBUF7_IR_ADR :
		data = med.in.GetIrqConfig(DEFAULT);
		break;
	case SIPP_IBUF7_IR_SHADOW_ADR :
		data = med.in.GetIrqConfig(SHADOW);
		break;
	case SIPP_IBUF7_FC_ADR :
		data = med.in.GetFillLevel();
		break;
	case SIPP_ICTX7_ADR :
		data = med.GetLineIdx();
		if(med.in.GetBuffLines() == 0)
			data |= (med.in.GetBufferIdxNoWrap(med.GetImgDimensions() >> SIPP_IMGDIM_SIZE) << SIPP_CBL_OFFSET);
		else
			data |= (med.in.GetBufferIdx() << SIPP_CBL_OFFSET);
		break;

		// IBUF[8] - LUT input buffer
	case SIPP_IBUF8_BASE_ADR :
		data = lut.in.GetBase(DEFAULT);
		break;
	case SIPP_IBUF8_BASE_SHADOW_ADR :
		data = lut.in.GetBase(SHADOW);
		break;
	case SIPP_IBUF8_CFG_ADR :
		data = lut.in.GetConfig(DEFAULT);
		break;
	case SIPP_IBUF8_CFG_SHADOW_ADR :
		data = lut.in.GetConfig(SHADOW);
		break;
	case SIPP_IBUF8_LS_ADR :
		data = lut.in.GetLineStride(DEFAULT);
		break;
	case SIPP_IBUF8_LS_SHADOW_ADR :
		data = lut.in.GetLineStride(SHADOW);
		break;
	case SIPP_IBUF8_PS_ADR :
		data = lut.in.GetPlaneStride(DEFAULT);
		break;
	case SIPP_IBUF8_PS_SHADOW_ADR :
		data = lut.in.GetPlaneStride(SHADOW);
		break;
	case SIPP_IBUF8_IR_ADR :
		data = lut.in.GetIrqConfig(DEFAULT);
		break;
	case SIPP_IBUF8_IR_SHADOW_ADR :
		data = lut.in.GetIrqConfig(SHADOW);
		break;
	case SIPP_IBUF8_FC_ADR :
		data = lut.in.GetFillLevel();
		break;
	case SIPP_ICTX8_ADR :
		data = lut.GetLineIdx();
		if(lut.in.GetBuffLines() == 0)
			data |= (lut.in.GetBufferIdxNoWrap(lut.GetImgDimensions() >> SIPP_IMGDIM_SIZE) << SIPP_CBL_OFFSET);
		else
			data |= (lut.in.GetBufferIdx() << SIPP_CBL_OFFSET);
		break;

		// IBUF[9] - Edge operator input buffer
	case SIPP_IBUF9_BASE_ADR :
		data = edge.in.GetBase(DEFAULT);
		break;
	case SIPP_IBUF9_BASE_SHADOW_ADR :
		data = edge.in.GetBase(SHADOW);
		break;
	case SIPP_IBUF9_CFG_ADR :
		data = edge.in.GetConfig(DEFAULT);
		break;
	case SIPP_IBUF9_CFG_SHADOW_ADR :
		data = edge.in.GetConfig(SHADOW);
		break;
	case SIPP_IBUF9_LS_ADR :
		data = edge.in.GetLineStride(DEFAULT);
		break;
	case SIPP_IBUF9_LS_SHADOW_ADR :
		data = edge.in.GetLineStride(SHADOW);
		break;
	case SIPP_IBUF9_PS_ADR :
		data = edge.in.GetPlaneStride(DEFAULT);
		break;
	case SIPP_IBUF9_PS_SHADOW_ADR :
		data = edge.in.GetPlaneStride(SHADOW);
		break;
	case SIPP_IBUF9_IR_ADR :
		data = edge.in.GetIrqConfig(DEFAULT);
		break;
	case SIPP_IBUF9_IR_SHADOW_ADR :
		data = edge.in.GetIrqConfig(SHADOW);
		break;
	case SIPP_IBUF9_FC_ADR :
		data = edge.in.GetFillLevel();
		break;
	case SIPP_ICTX9_ADR :
		data = edge.GetLineIdx();
		if(edge.in.GetBuffLines() == 0)
			data |= (edge.in.GetBufferIdxNoWrap(edge.GetImgDimensions() >> SIPP_IMGDIM_SIZE) << SIPP_CBL_OFFSET);
		else
			data |= (edge.in.GetBufferIdx() << SIPP_CBL_OFFSET);
		break;

		// IBUF[10] - Programmable convolution input buffer
	case SIPP_IBUF10_BASE_ADR :
		data = conv.in.GetBase(DEFAULT);
		break;
	case SIPP_IBUF10_BASE_SHADOW_ADR :
		data = conv.in.GetBase(SHADOW);
		break;
	case SIPP_IBUF10_CFG_ADR :
		data = conv.in.GetConfig(DEFAULT);
		break;
	case SIPP_IBUF10_CFG_SHADOW_ADR :
		data = conv.in.GetConfig(SHADOW);
		break;
	case SIPP_IBUF10_LS_ADR :
		data = conv.in.GetLineStride(DEFAULT);
		break;
	case SIPP_IBUF10_LS_SHADOW_ADR :
		data = conv.in.GetLineStride(SHADOW);
		break;
	case SIPP_IBUF10_PS_ADR :
		data = conv.in.GetPlaneStride(DEFAULT);
		break;
	case SIPP_IBUF10_PS_SHADOW_ADR :
		data = conv.in.GetPlaneStride(SHADOW);
		break;
	case SIPP_IBUF10_IR_ADR :
		data = conv.in.GetIrqConfig(DEFAULT);
		break;
	case SIPP_IBUF10_IR_SHADOW_ADR :
		data = conv.in.GetIrqConfig(SHADOW);
		break;
	case SIPP_IBUF10_FC_ADR :
		data = conv.in.GetFillLevel();
		break;
	case SIPP_ICTX10_ADR :
		data = conv.GetLineIdx();
		if(conv.in.GetBuffLines() == 0)
			data |= (conv.in.GetBufferIdxNoWrap(conv.GetImgDimensions() >> SIPP_IMGDIM_SIZE) << SIPP_CBL_OFFSET);
		else
			data |= (conv.in.GetBufferIdx() << SIPP_CBL_OFFSET);
		break;

		// IBUF[11] - Harris corners input buffer
	case SIPP_IBUF11_BASE_ADR :
		data = harr.in.GetBase(DEFAULT);
		break;
	case SIPP_IBUF11_BASE_SHADOW_ADR :
		data = harr.in.GetBase(SHADOW);
		break;
	case SIPP_IBUF11_CFG_ADR :
		data = harr.in.GetConfig(DEFAULT);
		break;
	case SIPP_IBUF11_CFG_SHADOW_ADR :
		data = harr.in.GetConfig(SHADOW);
		break;
	case SIPP_IBUF11_LS_ADR :
		data = harr.in.GetLineStride(DEFAULT);
		break;
	case SIPP_IBUF11_LS_SHADOW_ADR :
		data = harr.in.GetLineStride(SHADOW);
		break;
	case SIPP_IBUF11_PS_ADR :
		data = harr.in.GetPlaneStride(DEFAULT);
		break;
	case SIPP_IBUF11_PS_SHADOW_ADR :
		data = harr.in.GetPlaneStride(SHADOW);
		break;
	case SIPP_IBUF11_IR_ADR :
		data = harr.in.GetIrqConfig(DEFAULT);
		break;
	case SIPP_IBUF11_IR_SHADOW_ADR :
		data = harr.in.GetIrqConfig(SHADOW);
		break;
	case SIPP_IBUF11_FC_ADR :
		data = harr.in.GetFillLevel();
		break;
	case SIPP_ICTX11_ADR :
		data = harr.GetLineIdx();
		if(harr.in.GetBuffLines() == 0)
			data |= (harr.in.GetBufferIdxNoWrap(harr.GetImgDimensions() >> SIPP_IMGDIM_SIZE) << SIPP_CBL_OFFSET);
		else
			data |= (harr.in.GetBufferIdx() << SIPP_CBL_OFFSET);
		break;

		// IBUF[12] - Colour combination luma input buffer
	case SIPP_IBUF12_BASE_ADR :
		data = cc.in.GetBase(DEFAULT);
		break;
	case SIPP_IBUF12_CFG_ADR :
		data = cc.in.GetConfig(DEFAULT);
		break;
	case SIPP_IBUF12_LS_ADR :
		data = cc.in.GetLineStride(DEFAULT);
		break;
	case SIPP_IBUF12_PS_ADR :
		data = cc.in.GetPlaneStride(DEFAULT);
		break;
	case SIPP_IBUF12_IR_ADR :
		data = cc.in.GetIrqConfig(DEFAULT);
		break;
	case SIPP_IBUF12_FC_ADR :
		data = cc.in.GetFillLevel();
		break;
	case SIPP_ICTX12_ADR :
		data = cc.GetLineIdx();
		if(cc.in.GetBuffLines() == 0)
			data |= (cc.in.GetBufferIdxNoWrap(cc.GetImgDimensions() >> SIPP_IMGDIM_SIZE) << SIPP_CBL_OFFSET);
		else
			data |= (cc.in.GetBufferIdx() << SIPP_CBL_OFFSET);
		break;

		// TODO
		// IBUF[14] - MIPI Tx[0] input buffer
		// IBUF[15] - MIPI Tx[1] input buffer

		// IBUF[16] - Lens shading correction gain mesh buffer
	case SIPP_IBUF16_BASE_ADR :
		data = lsc.lsc_gmb.GetBase(DEFAULT);
		break;
	case SIPP_IBUF16_BASE_SHADOW_ADR :
		data = lsc.lsc_gmb.GetBase(SHADOW);
		break;
	case SIPP_IBUF16_CFG_ADR :
		data = lsc.lsc_gmb.GetConfig(DEFAULT);
		break;
	case SIPP_IBUF16_CFG_SHADOW_ADR :
		data = lsc.lsc_gmb.GetConfig(SHADOW);
		break;
	case SIPP_IBUF16_LS_ADR :
		data = lsc.lsc_gmb.GetLineStride(DEFAULT);
		break;
	case SIPP_IBUF16_LS_SHADOW_ADR :
		data = lsc.lsc_gmb.GetLineStride(SHADOW);
		break;
	case SIPP_IBUF16_PS_ADR :
		data = lsc.lsc_gmb.GetPlaneStride(DEFAULT);
		break;
	case SIPP_IBUF16_PS_SHADOW_ADR :
		data = lsc.lsc_gmb.GetPlaneStride(SHADOW);
		break;
	case SIPP_IBUF16_IR_ADR :
		data = lsc.lsc_gmb.GetIrqConfig(DEFAULT);
		break;
	case SIPP_IBUF16_IR_SHADOW_ADR :
		data = lsc.lsc_gmb.GetIrqConfig(SHADOW);
		break;
	case SIPP_IBUF16_FC_ADR :
		data = lsc.lsc_gmb.GetFillLevel();
		break;
	case SIPP_ICTX16_ADR :
		data = lsc.lsc_gmb.GetBufferIdx() << SIPP_CBL_OFFSET;
		break;

		// IBUF[17] - Chroma denoise reference input buffer
	case SIPP_IBUF17_BASE_ADR :
		data = cdn.ref.GetBase(DEFAULT);
		break;
	case SIPP_IBUF17_BASE_SHADOW_ADR :
		data = cdn.ref.GetBase(SHADOW);
		break;
	case SIPP_IBUF17_CFG_ADR :
		data = cdn.ref.GetConfig(DEFAULT);
		break;
	case SIPP_IBUF17_CFG_SHADOW_ADR :
		data = cdn.ref.GetConfig(SHADOW);
		break;
	case SIPP_IBUF17_LS_ADR :
		data = cdn.ref.GetLineStride(DEFAULT);
		break;
	case SIPP_IBUF17_LS_SHADOW_ADR :
		data = cdn.ref.GetLineStride(SHADOW);
		break;
	case SIPP_IBUF17_PS_ADR :
		data = cdn.ref.GetPlaneStride(DEFAULT);
		break;
	case SIPP_IBUF17_PS_SHADOW_ADR :
		data = cdn.ref.GetPlaneStride(SHADOW);
		break;
	case SIPP_IBUF17_IR_ADR :
		data = cdn.ref.GetIrqConfig(DEFAULT);
		break;
	case SIPP_IBUF17_IR_SHADOW_ADR :
		data = cdn.ref.GetIrqConfig(SHADOW);
		break;
	case SIPP_IBUF17_FC_ADR :
		data = cdn.ref.GetFillLevel();
		break;
	case SIPP_ICTX17_ADR :
		if(cdn.ref.GetBuffLines() == 0)
			data = (cdn.ref.GetBufferIdxNoWrap(cdn.GetImgDimensions() >> SIPP_IMGDIM_SIZE) << SIPP_CBL_OFFSET);
		else
			data = (cdn.ref.GetBufferIdx() << SIPP_CBL_OFFSET);
		break; 

		// IBUF[18] - Luma denoise reference input buffer
	case SIPP_IBUF18_BASE_ADR :
		data = luma.ref.GetBase(DEFAULT);
		break;
	case SIPP_IBUF18_BASE_SHADOW_ADR :
		data = luma.ref.GetBase(SHADOW);
		break;
	case SIPP_IBUF18_CFG_ADR :
		data = luma.ref.GetConfig(DEFAULT);
		break;
	case SIPP_IBUF18_CFG_SHADOW_ADR :
		data = luma.ref.GetConfig(SHADOW);
		break;
	case SIPP_IBUF18_LS_ADR :
		data = luma.ref.GetLineStride(DEFAULT);
		break;
	case SIPP_IBUF18_LS_SHADOW_ADR :
		data = luma.ref.GetLineStride(SHADOW);
		break;
	case SIPP_IBUF18_PS_ADR :
		data = luma.ref.GetPlaneStride(DEFAULT);
		break;
	case SIPP_IBUF18_PS_SHADOW_ADR :
		data = luma.ref.GetPlaneStride(SHADOW);
		break;
	case SIPP_IBUF18_IR_ADR :
		data = luma.ref.GetIrqConfig(DEFAULT);
		break;
	case SIPP_IBUF18_IR_SHADOW_ADR :
		data = luma.ref.GetIrqConfig(SHADOW);
		break;
	case SIPP_IBUF18_FC_ADR :
		data = luma.ref.GetFillLevel();
		break;
	case SIPP_ICTX18_ADR :
		if(luma.ref.GetBuffLines() == 0)
			data = (luma.ref.GetBufferIdxNoWrap(luma.GetImgDimensions() >> SIPP_IMGDIM_SIZE) << SIPP_CBL_OFFSET);
		else
			data = (luma.ref.GetBufferIdx() << SIPP_CBL_OFFSET);
		break;

		// IBUF[19] - LUT loader
	case SIPP_IBUF19_BASE_ADR :
		data = lut.loader.GetBase();
		break;
	case SIPP_IBUF19_CFG_ADR :
		data = lut.loader.GetConfig();
		break;

		// IBUF[20] - Debayer post-processing median filter input buffer
	case SIPP_IBUF20_BASE_ADR :
		data = dbyrppm.in.GetBase(DEFAULT);
		break;
	case SIPP_IBUF20_BASE_SHADOW_ADR :
		data = dbyrppm.in.GetBase(SHADOW);
		break;
	case SIPP_IBUF20_CFG_ADR :
		data = dbyrppm.in.GetConfig(DEFAULT);
		break;
	case SIPP_IBUF20_CFG_SHADOW_ADR :
		data = dbyrppm.in.GetConfig(SHADOW);
		break;
	case SIPP_IBUF20_LS_ADR :
		data = dbyrppm.in.GetLineStride(DEFAULT);
		break;
	case SIPP_IBUF20_LS_SHADOW_ADR :
		data = dbyrppm.in.GetLineStride(SHADOW);
		break;
	case SIPP_IBUF20_PS_ADR :
		data = dbyrppm.in.GetPlaneStride(DEFAULT);
		break;
	case SIPP_IBUF20_PS_SHADOW_ADR :
		data = dbyrppm.in.GetPlaneStride(SHADOW);
		break;
	case SIPP_IBUF20_IR_ADR :
		data = dbyrppm.in.GetIrqConfig(DEFAULT);
		break;
	case SIPP_IBUF20_IR_SHADOW_ADR :
		data = dbyrppm.in.GetIrqConfig(SHADOW);
		break;
	case SIPP_IBUF20_FC_ADR :
		data = dbyrppm.in.GetFillLevel();
		break;
	case SIPP_ICTX20_ADR :
		data = dbyrppm.GetLineIdx();
		if(dbyrppm.in.GetBuffLines() == 0)
			data |= (dbyrppm.in.GetBufferIdxNoWrap(dbyrppm.GetImgDimensions() >> SIPP_IMGDIM_SIZE) << SIPP_CBL_OFFSET);
		else
			data |= (dbyrppm.in.GetBufferIdx() << SIPP_CBL_OFFSET);
		break;

		// IBUF[21] - Colour combination chroma input buffer
	case SIPP_IBUF21_BASE_ADR :
		data = cc.chr.GetBase(DEFAULT);
		break;
	case SIPP_IBUF21_CFG_ADR :
		data = cc.chr.GetConfig(DEFAULT);
		break;
	case SIPP_IBUF21_LS_ADR :
		data = cc.chr.GetLineStride(DEFAULT);
		break;
	case SIPP_IBUF21_PS_ADR :
		data = cc.chr.GetPlaneStride(DEFAULT);
		break;
	case SIPP_IBUF21_IR_ADR :
		data = cc.chr.GetIrqConfig(DEFAULT);
		break;
	case SIPP_IBUF21_FC_ADR :
		data = cc.chr.GetFillLevel();
		break;
	case SIPP_ICTX21_ADR :
		if(cc.chr.GetBuffLines() == 0)
			data = (cc.chr.GetBufferIdxNoWrap((cc.GetImgDimensions() >> SIPP_IMGDIM_SIZE) >> 1) << SIPP_CBL_OFFSET);
		else
			data = (cc.chr.GetBufferIdx() << SIPP_CBL_OFFSET);
		break;


		/*
		* SIPP output buffers configuration/control
		*
		* Connect APB registers to filter output buffer objects
		*/
		// OBUF[0] - RAW output buffer
	case SIPP_OBUF0_BASE_ADR :
		data = raw.out.GetBase(DEFAULT);
		break;
	case SIPP_OBUF0_BASE_SHADOW_ADR :
		data = raw.out.GetBase(SHADOW);
		break;
	case SIPP_OBUF0_CFG_ADR :
		data = raw.out.GetConfig(DEFAULT);
		break;
	case SIPP_OBUF0_CFG_SHADOW_ADR :
		data = raw.out.GetConfig(SHADOW);
		break;
	case SIPP_OBUF0_LS_ADR :
		data = raw.out.GetLineStride(DEFAULT);
		break;
	case SIPP_OBUF0_LS_SHADOW_ADR :
		data = raw.out.GetLineStride(SHADOW);
		break;
	case SIPP_OBUF0_PS_ADR :
		data = raw.out.GetPlaneStride(DEFAULT);
		break;
	case SIPP_OBUF0_PS_SHADOW_ADR :
		data = raw.out.GetPlaneStride(SHADOW);
		break;
	case SIPP_OBUF0_IR_ADR :
		data = raw.out.GetIrqConfig(DEFAULT);
		break;
	case SIPP_OBUF0_IR_SHADOW_ADR :
		data = raw.out.GetIrqConfig(SHADOW);
		break;
	case SIPP_OBUF0_FC_ADR :
		data = raw.out.GetFillLevel();
		break;
	case SIPP_OCTX0_ADR :
		data  = raw.GetOutputLineIdx();
		data |= (raw.out.GetBufferIdx() << SIPP_CBL_OFFSET);
		break;

		// OBUF[1] - Lens shading correction output buffer
	case SIPP_OBUF1_BASE_ADR :
		data = lsc.out.GetBase(DEFAULT);
		break;
	case SIPP_OBUF1_BASE_SHADOW_ADR :
		data = lsc.out.GetBase(SHADOW);
		break;
	case SIPP_OBUF1_CFG_ADR :
		data = lsc.out.GetConfig(DEFAULT);
		break;
	case SIPP_OBUF1_CFG_SHADOW_ADR :
		data = lsc.out.GetConfig(SHADOW);
		break;
	case SIPP_OBUF1_LS_ADR :
		data = lsc.out.GetLineStride(DEFAULT);
		break;
	case SIPP_OBUF1_LS_SHADOW_ADR :
		data = lsc.out.GetLineStride(SHADOW);
		break;
	case SIPP_OBUF1_PS_ADR :
		data = lsc.out.GetPlaneStride(DEFAULT);
		break;
	case SIPP_OBUF1_PS_SHADOW_ADR :
		data = lsc.out.GetPlaneStride(SHADOW);
		break;
	case SIPP_OBUF1_IR_ADR :
		data = lsc.out.GetIrqConfig(DEFAULT);
		break;
	case SIPP_OBUF1_IR_SHADOW_ADR :
		data = lsc.out.GetIrqConfig(SHADOW);
		break;
	case SIPP_OBUF1_FC_ADR :
		data = lsc.out.GetFillLevel();
		break;
	case SIPP_OCTX1_ADR :
		data = lsc.GetOutputLineIdx();
		if(lsc.out.GetBuffLines() == 0)
			data |= (lsc.out.GetBufferIdxNoWrap(lsc.GetImgDimensions() >> SIPP_IMGDIM_SIZE) << SIPP_CBL_OFFSET);
		else
			data |= (lsc.out.GetBufferIdx() << SIPP_CBL_OFFSET);
		break;

		// OBUF[2] - Debayer output buffer
	case SIPP_OBUF2_BASE_ADR :
		data = dbyr.out.GetBase(DEFAULT);
		break;
	case SIPP_OBUF2_BASE_SHADOW_ADR :
		data = dbyr.out.GetBase(SHADOW);
		break;
	case SIPP_OBUF2_CFG_ADR :
		data = dbyr.out.GetConfig(DEFAULT);
		break;
	case SIPP_OBUF2_CFG_SHADOW_ADR :
		data = dbyr.out.GetConfig(SHADOW);
		break;
	case SIPP_OBUF2_LS_ADR :
		data = dbyr.out.GetLineStride(DEFAULT);
		break;
	case SIPP_OBUF2_LS_SHADOW_ADR :
		data = dbyr.out.GetLineStride(SHADOW);
		break;
	case SIPP_OBUF2_PS_ADR :
		data = dbyr.out.GetPlaneStride(DEFAULT);
		break;
	case SIPP_OBUF2_PS_SHADOW_ADR :
		data = dbyr.out.GetPlaneStride(SHADOW);
		break;
	case SIPP_OBUF2_IR_ADR :
		data = dbyr.out.GetIrqConfig(DEFAULT);
		break;
	case SIPP_OBUF2_IR_SHADOW_ADR :
		data = dbyr.out.GetIrqConfig(SHADOW);
		break;
	case SIPP_OBUF2_FC_ADR :
		data = dbyr.out.GetFillLevel();
		break;
	case SIPP_OCTX2_ADR :
		data = dbyr.GetOutputLineIdx();
		if(dbyr.out.GetBuffLines() == 0)
			data |= (dbyr.out.GetBufferIdxNoWrap(dbyr.GetImgDimensions() >> SIPP_IMGDIM_SIZE) << SIPP_CBL_OFFSET);
		else
			data |= (dbyr.out.GetBufferIdx() << SIPP_CBL_OFFSET);
		break;

		// OBUF[3] - Chroma denoise output buffer
	case SIPP_OBUF3_BASE_ADR :
		data = cdn.out.GetBase(DEFAULT);
		break;
	case SIPP_OBUF3_BASE_SHADOW_ADR :
		data = cdn.out.GetBase(SHADOW);
		break;
	case SIPP_OBUF3_CFG_ADR :
		data = cdn.out.GetConfig(DEFAULT);
		break;
	case SIPP_OBUF3_CFG_SHADOW_ADR :
		data = cdn.out.GetConfig(SHADOW);
		break;
	case SIPP_OBUF3_LS_ADR :
		data = cdn.out.GetLineStride(DEFAULT);
		break;
	case SIPP_OBUF3_LS_SHADOW_ADR :
		data = cdn.out.GetLineStride(SHADOW);
		break;
	case SIPP_OBUF3_PS_ADR :
		data = cdn.out.GetPlaneStride(DEFAULT);
		break;
	case SIPP_OBUF3_PS_SHADOW_ADR :
		data = cdn.out.GetPlaneStride(SHADOW);
		break;
	case SIPP_OBUF3_IR_ADR :
		data = cdn.out.GetIrqConfig(DEFAULT);
		break;
	case SIPP_OBUF3_IR_SHADOW_ADR :
		data = cdn.out.GetIrqConfig(SHADOW);
		break;
	case SIPP_OBUF3_FC_ADR :
		data = cdn.out.GetFillLevel();
		break;
	case SIPP_OCTX3_ADR :
		data = cdn.GetOutputLineIdx();
		if(cdn.out.GetBuffLines() == 0)
			data |= (cdn.out.GetBufferIdxNoWrap(cdn.GetImgDimensions() >> SIPP_IMGDIM_SIZE) << SIPP_CBL_OFFSET);
		else
			data |= (cdn.out.GetBufferIdx() << SIPP_CBL_OFFSET);
		break;

		// OBUF[4] - Luma denoise output buffer
	case SIPP_OBUF4_BASE_ADR :
		data = luma.out.GetBase(DEFAULT);
		break;
	case SIPP_OBUF4_BASE_SHADOW_ADR :
		data = luma.out.GetBase(SHADOW);
		break;
	case SIPP_OBUF4_CFG_ADR :
		data = luma.out.GetConfig(DEFAULT);
		break;
	case SIPP_OBUF4_CFG_SHADOW_ADR :
		data = luma.out.GetConfig(SHADOW);
		break;
	case SIPP_OBUF4_LS_ADR :
		data = luma.out.GetLineStride(DEFAULT);
		break;
	case SIPP_OBUF4_LS_SHADOW_ADR :
		data = luma.out.GetLineStride(SHADOW);
		break;
	case SIPP_OBUF4_PS_ADR :
		data = luma.out.GetPlaneStride(DEFAULT);
		break;
	case SIPP_OBUF4_PS_SHADOW_ADR :
		data = luma.out.GetPlaneStride(SHADOW);
		break;
	case SIPP_OBUF4_IR_ADR :
		data = luma.out.GetIrqConfig(DEFAULT);
		break;
	case SIPP_OBUF4_IR_SHADOW_ADR :
		data = luma.out.GetIrqConfig(SHADOW);
		break;
	case SIPP_OBUF4_FC_ADR :
		data = luma.out.GetFillLevel();
		break;
	case SIPP_OCTX4_ADR :
		data = luma.GetOutputLineIdx();
		if(luma.out.GetBuffLines() == 0)
			data |= (luma.out.GetBufferIdxNoWrap(luma.GetImgDimensions() >> SIPP_IMGDIM_SIZE) << SIPP_CBL_OFFSET);
		else
			data |= (luma.out.GetBufferIdx() << SIPP_CBL_OFFSET);
		break;

		// OBUF[5] - Sharpen output buffer
	case SIPP_OBUF5_BASE_ADR :
		data = usm.out.GetBase();
		break;
	case SIPP_OBUF5_CFG_ADR :
		data = usm.out.GetConfig();
		break;
	case SIPP_OBUF5_LS_ADR :
		data = usm.out.GetLineStride();
		break;
	case SIPP_OBUF5_PS_ADR :
		data = usm.out.GetPlaneStride();
		break;
	case SIPP_OBUF5_IR_ADR :
		data = usm.out.GetIrqConfig();
		break;
	case SIPP_OBUF5_FC_ADR :
		data = usm.out.GetFillLevel();
		break;
	case SIPP_OCTX5_ADR :
		data = usm.GetOutputLineIdx();
		if(usm.out.GetBuffLines() == 0)
			data |= (usm.out.GetBufferIdxNoWrap(usm.GetImgDimensions() >> SIPP_IMGDIM_SIZE) << SIPP_CBL_OFFSET);
		else
			data |= (usm.out.GetBufferIdx() << SIPP_CBL_OFFSET);
		break;

		// OBUF[6] - Polyphase FIR output buffer
	case SIPP_OBUF6_BASE_ADR :
		data = upfirdn.out.GetBase(DEFAULT);
		break;
	case SIPP_OBUF6_BASE_SHADOW_ADR :
		data = upfirdn.out.GetBase(SHADOW);
		break;
	case SIPP_OBUF6_CFG_ADR :
		data = upfirdn.out.GetConfig(DEFAULT);
		break;
	case SIPP_OBUF6_CFG_SHADOW_ADR :
		data = upfirdn.out.GetConfig(SHADOW);
		break;
	case SIPP_OBUF6_LS_ADR :
		data = upfirdn.out.GetLineStride(DEFAULT);
		break;
	case SIPP_OBUF6_LS_SHADOW_ADR :
		data = upfirdn.out.GetLineStride(SHADOW);
		break;
	case SIPP_OBUF6_PS_ADR :
		data = upfirdn.out.GetPlaneStride(DEFAULT);
		break;
	case SIPP_OBUF6_PS_SHADOW_ADR :
		data = upfirdn.out.GetPlaneStride(SHADOW);
		break;
	case SIPP_OBUF6_IR_ADR :
		data = upfirdn.out.GetIrqConfig(DEFAULT);
		break;
	case SIPP_OBUF6_IR_SHADOW_ADR :
		data = upfirdn.out.GetIrqConfig(SHADOW);
		break;
	case SIPP_OBUF6_FC_ADR :
		data = upfirdn.out.GetFillLevel();
		break;
	case SIPP_OCTX6_ADR :
		data = upfirdn.GetOutputLineIdx();
		if(upfirdn.out.GetBuffLines() == 0)
			data |= (upfirdn.out.GetBufferIdxNoWrap(upfirdn.GetOutImgDimensions() >> SIPP_IMGDIM_SIZE) << SIPP_CBL_OFFSET);
		else
			data |= (upfirdn.out.GetBufferIdx() << SIPP_CBL_OFFSET);
		break;

		// OBUF[7] - Median output buffer
	case SIPP_OBUF7_BASE_ADR :
		data = med.out.GetBase(DEFAULT);
		break;
	case SIPP_OBUF7_BASE_SHADOW_ADR :
		data = med.out.GetBase(SHADOW);
		break;
	case SIPP_OBUF7_CFG_ADR :
		data = med.out.GetConfig(DEFAULT);
		break;
	case SIPP_OBUF7_CFG_SHADOW_ADR :
		data = med.out.GetConfig(SHADOW);
		break;
	case SIPP_OBUF7_LS_ADR :
		data = med.out.GetLineStride(DEFAULT);
		break;
	case SIPP_OBUF7_LS_SHADOW_ADR :
		data = med.out.GetLineStride(SHADOW);
		break;
	case SIPP_OBUF7_PS_ADR :
		data = med.out.GetPlaneStride(DEFAULT);
		break;
	case SIPP_OBUF7_PS_SHADOW_ADR :
		data = med.out.GetPlaneStride(SHADOW);
		break;
	case SIPP_OBUF7_IR_ADR :
		data = med.out.GetIrqConfig(DEFAULT);
		break;
	case SIPP_OBUF7_IR_SHADOW_ADR :
		data = med.out.GetIrqConfig(SHADOW);
		break;
	case SIPP_OBUF7_FC_ADR :
		data = med.out.GetFillLevel();
		break;
	case SIPP_OCTX7_ADR :
		data = med.GetOutputLineIdx();
		if(med.out.GetBuffLines() == 0)
			data |= (med.out.GetBufferIdxNoWrap(med.GetImgDimensions() >> SIPP_IMGDIM_SIZE) << SIPP_CBL_OFFSET);
		else
			data |= (med.out.GetBufferIdx() << SIPP_CBL_OFFSET);
		break;

		// OBUF[8] - LUT output buffer
	case SIPP_OBUF8_BASE_ADR :
		data = lut.out.GetBase(DEFAULT);
		break;
	case SIPP_OBUF8_BASE_SHADOW_ADR :
		data = lut.out.GetBase(SHADOW);
		break;
	case SIPP_OBUF8_CFG_ADR :
		data = lut.out.GetConfig(DEFAULT);
		break;
	case SIPP_OBUF8_CFG_SHADOW_ADR :
		data = lut.out.GetConfig(SHADOW);
		break;
	case SIPP_OBUF8_LS_ADR :
		data = lut.out.GetLineStride(DEFAULT);
		break;
	case SIPP_OBUF8_LS_SHADOW_ADR :
		data = lut.out.GetLineStride(SHADOW);
		break;
	case SIPP_OBUF8_PS_ADR :
		data = lut.out.GetPlaneStride(DEFAULT);
		break;
	case SIPP_OBUF8_PS_SHADOW_ADR :
		data = lut.out.GetPlaneStride(SHADOW);
		break;
	case SIPP_OBUF8_IR_ADR :
		data = lut.out.GetIrqConfig(DEFAULT);
		break;
	case SIPP_OBUF8_IR_SHADOW_ADR :
		data = lut.out.GetIrqConfig(SHADOW);
		break;
	case SIPP_OBUF8_FC_ADR :
		data = lut.out.GetFillLevel();
		break;
	case SIPP_OCTX8_ADR :
		data = lut.GetOutputLineIdx();
		if(lut.out.GetBuffLines() == 0)
			data |= (lut.out.GetBufferIdxNoWrap(lut.GetImgDimensions() >> SIPP_IMGDIM_SIZE) << SIPP_CBL_OFFSET);
		else
			data |= (lut.out.GetBufferIdx() << SIPP_CBL_OFFSET);
		break;

		// OBUF[9] - Edge operator output buffer
	case SIPP_OBUF9_BASE_ADR :
		data = edge.out.GetBase(DEFAULT);
		break;
	case SIPP_OBUF9_BASE_SHADOW_ADR :
		data = edge.out.GetBase(SHADOW);
		break;
	case SIPP_OBUF9_CFG_ADR :
		data = edge.out.GetConfig(DEFAULT);
		break;
	case SIPP_OBUF9_CFG_SHADOW_ADR :
		data = edge.out.GetConfig(SHADOW);
		break;
	case SIPP_OBUF9_LS_ADR :
		data = edge.out.GetLineStride(DEFAULT);
		break;
	case SIPP_OBUF9_LS_SHADOW_ADR :
		data = edge.out.GetLineStride(SHADOW);
		break;
	case SIPP_OBUF9_PS_ADR :
		data = edge.out.GetPlaneStride(DEFAULT);
		break;
	case SIPP_OBUF9_PS_SHADOW_ADR :
		data = edge.out.GetPlaneStride(SHADOW);
		break;
	case SIPP_OBUF9_IR_ADR :
		data = edge.out.GetIrqConfig(DEFAULT);
		break;
	case SIPP_OBUF9_IR_SHADOW_ADR :
		data = edge.out.GetIrqConfig(SHADOW);
		break;
	case SIPP_OBUF9_FC_ADR :
		data = edge.out.GetFillLevel();
		break;
	case SIPP_OCTX9_ADR :
		data = edge.GetOutputLineIdx();
		if(edge.out.GetBuffLines() == 0)
			data |= (edge.out.GetBufferIdxNoWrap(edge.GetImgDimensions() >> SIPP_IMGDIM_SIZE) << SIPP_CBL_OFFSET);
		else
			data |= (edge.out.GetBufferIdx() << SIPP_CBL_OFFSET);
		break;

		// OBUF[10] - Programmable convolution output buffer
	case SIPP_OBUF10_BASE_ADR :
		data = conv.out.GetBase(DEFAULT);
		break;
	case SIPP_OBUF10_BASE_SHADOW_ADR :
		data = conv.out.GetBase(SHADOW);
		break;
	case SIPP_OBUF10_CFG_ADR :
		data = conv.out.GetConfig(DEFAULT);
		break;
	case SIPP_OBUF10_CFG_SHADOW_ADR :
		data = conv.out.GetConfig(SHADOW);
		break;
	case SIPP_OBUF10_LS_ADR :
		data = conv.out.GetLineStride(DEFAULT);
		break;
	case SIPP_OBUF10_LS_SHADOW_ADR :
		data = conv.out.GetLineStride(SHADOW);
		break;
	case SIPP_OBUF10_PS_ADR :
		data = conv.out.GetPlaneStride(DEFAULT);
		break;
	case SIPP_OBUF10_PS_SHADOW_ADR :
		data = conv.out.GetPlaneStride(SHADOW);
		break;
	case SIPP_OBUF10_IR_ADR :
		data = conv.out.GetIrqConfig(DEFAULT);
		break;
	case SIPP_OBUF10_IR_SHADOW_ADR :
		data = conv.out.GetIrqConfig(SHADOW);
		break;
	case SIPP_OBUF10_FC_ADR :
		data = conv.out.GetFillLevel();
		break;
	case SIPP_OCTX10_ADR :
		data = conv.GetOutputLineIdx();
		if(conv.out.GetBuffLines() == 0)
			data |= (conv.out.GetBufferIdxNoWrap(conv.GetImgDimensions() >> SIPP_IMGDIM_SIZE) << SIPP_CBL_OFFSET);
		else
			data |= (conv.out.GetBufferIdx() << SIPP_CBL_OFFSET);		
		break;

		// OBUF[11] - Harris corners output buffer
	case SIPP_OBUF11_BASE_ADR :
		data = harr.out.GetBase(DEFAULT);
		break;
	case SIPP_OBUF11_BASE_SHADOW_ADR :
		data = harr.out.GetBase(SHADOW);
		break;
	case SIPP_OBUF11_CFG_ADR :
		data = harr.out.GetConfig(DEFAULT);
		break;
	case SIPP_OBUF11_CFG_SHADOW_ADR :
		data = harr.out.GetConfig(SHADOW);
		break;
	case SIPP_OBUF11_LS_ADR :
		data = harr.out.GetLineStride(DEFAULT);
		break;
	case SIPP_OBUF11_LS_SHADOW_ADR :
		data = harr.out.GetLineStride(SHADOW);
		break;
	case SIPP_OBUF11_PS_ADR :
		data = harr.out.GetPlaneStride(DEFAULT);
		break;
	case SIPP_OBUF11_PS_SHADOW_ADR :
		data = harr.out.GetPlaneStride(SHADOW);
		break;
	case SIPP_OBUF11_IR_ADR :
		data = harr.out.GetIrqConfig(DEFAULT);
		break;
	case SIPP_OBUF11_IR_SHADOW_ADR :
		data = harr.out.GetIrqConfig(SHADOW);
		break;
	case SIPP_OBUF11_FC_ADR :
		data = harr.out.GetFillLevel();
		break;
	case SIPP_OCTX11_ADR :
		data = harr.GetOutputLineIdx();
		if(harr.out.GetBuffLines() == 0)
			data |= (harr.out.GetBufferIdxNoWrap(harr.GetImgDimensions() >> SIPP_IMGDIM_SIZE) << SIPP_CBL_OFFSET);
		else
			data |= (harr.out.GetBufferIdx() << SIPP_CBL_OFFSET);
		break;

		// OBUF[12] - Colour combination RGB output buffer
	case SIPP_OBUF12_BASE_ADR :
		data = cc.out.GetBase(DEFAULT);
		break;
	case SIPP_OBUF12_CFG_ADR :
		data = cc.out.GetConfig(DEFAULT);
		break;
	case SIPP_OBUF12_LS_ADR :
		data = cc.out.GetLineStride(DEFAULT);
		break;
	case SIPP_OBUF12_PS_ADR :
		data = cc.out.GetPlaneStride(DEFAULT);
		break;
	case SIPP_OBUF12_IR_ADR :
		data = cc.out.GetIrqConfig(DEFAULT);
		break;
	case SIPP_OBUF12_FC_ADR :
		data = cc.out.GetFillLevel();
		break;
	case SIPP_OCTX12_ADR :
		data = cc.GetOutputLineIdx();
		if(cc.out.GetBuffLines() == 0)
			data |= (cc.out.GetBufferIdxNoWrap(cc.GetImgDimensions() >> SIPP_IMGDIM_SIZE) << SIPP_CBL_OFFSET);
		else
			data |= (cc.out.GetBufferIdx() << SIPP_CBL_OFFSET);
		break;

		// OBUF[15] - RAW Stats collection
	case SIPP_OBUF15_BASE_ADR :
		data = raw.stats.GetBase(DEFAULT);
		break;
	case SIPP_OBUF15_BASE_SHADOW_ADR :
		data = raw.stats.GetBase(SHADOW);
		break;
	case SIPP_OBUF15_CFG_ADR :
		data = raw.stats.GetConfig(DEFAULT);
		break;
	case SIPP_OBUF15_CFG_SHADOW_ADR :
		data = raw.stats.GetConfig(SHADOW);
		break;
	case SIPP_OBUF15_LS_ADR :
		data = raw.stats.GetLineStride(DEFAULT);
		break;
	case SIPP_OBUF15_LS_SHADOW_ADR :
		data = raw.stats.GetLineStride(SHADOW);
		break;
	case SIPP_OBUF15_PS_ADR :
		data = raw.stats.GetPlaneStride(DEFAULT);
		break;
	case SIPP_OBUF15_PS_SHADOW_ADR :
		data = raw.stats.GetPlaneStride(SHADOW);
		break;
	case SIPP_OBUF15_IR_ADR :
		data = raw.stats.GetIrqConfig(DEFAULT);
		break;
	case SIPP_OBUF15_IR_SHADOW_ADR :
		data = raw.stats.GetIrqConfig(SHADOW);
		break;

		// OBUF[16] - MIPI Rx[0]
		// OBUF[17] - MIPI Rx[1]
		// OBUF[18] - MIPI Rx[2]
		// OBUF[19] - MIPI Rx[3]
		
		// OBUF[20] - Debayer post-processing median filter output buffer
	case SIPP_OBUF20_BASE_ADR :
		data = dbyrppm.out.GetBase(DEFAULT);
		break;
	case SIPP_OBUF20_BASE_SHADOW_ADR :
		data = dbyrppm.out.GetBase(SHADOW);
		break;
	case SIPP_OBUF20_CFG_ADR :
		data = dbyrppm.out.GetConfig(DEFAULT);
		break;
	case SIPP_OBUF20_CFG_SHADOW_ADR :
		data = dbyrppm.out.GetConfig(SHADOW);
		break;
	case SIPP_OBUF20_LS_ADR :
		data = dbyrppm.out.GetLineStride(DEFAULT);
		break;
	case SIPP_OBUF20_LS_SHADOW_ADR :
		data = dbyrppm.out.GetLineStride(SHADOW);
		break;
	case SIPP_OBUF20_PS_ADR :
		data = dbyrppm.out.GetPlaneStride(DEFAULT);
		break;
	case SIPP_OBUF20_PS_SHADOW_ADR :
		data = dbyrppm.out.GetPlaneStride(SHADOW);
		break;
	case SIPP_OBUF20_IR_ADR :
		data = dbyrppm.out.GetIrqConfig(DEFAULT);
		break;
	case SIPP_OBUF20_IR_SHADOW_ADR :
		data = dbyrppm.out.GetIrqConfig(SHADOW);
		break;
	case SIPP_OBUF20_FC_ADR :
		data = dbyrppm.out.GetFillLevel();
		break;
	case SIPP_OCTX20_ADR :
		data = dbyrppm.GetOutputLineIdx();
		if(dbyrppm.out.GetBuffLines() == 0)
			data |= (dbyrppm.out.GetBufferIdxNoWrap(dbyrppm.GetImgDimensions() >> SIPP_IMGDIM_SIZE) << SIPP_CBL_OFFSET);
		else
			data |= (dbyrppm.out.GetBufferIdx() << SIPP_CBL_OFFSET);
		break;




		/*
		* SIPP Filter specific configuration
		*
		* Connect APB registers to corresponding filter object methods
		*/
	case SIPP_RAW_FRM_DIM_ADR :
		data = raw.GetImgDimensions(DEFAULT);
		break;
	case SIPP_RAW_FRM_DIM_SHADOW_ADR :
		data = raw.GetImgDimensions(SHADOW);
		break;
	case SIPP_STATS_FRM_DIM_ADR :
		data = raw.GetStatsWidth();
		break;
	case SIPP_STATS_FRM_DIM_SHADOW_ADR :
		data = raw.GetStatsWidth(SHADOW);
		break;
	case SIPP_RAW_CFG_ADR :
		data  = raw.GetRawFormat();
		data |= raw.GetBayerPattern()  << 1;
		data |= raw.GetGrGbImbEn()     << 3;
		data |= raw.GetHotPixEn()      << 4;
		data |= raw.GetHotGreenPixEn() << 5;
		data |= raw.GetStatsPatchEn()  << 6;
		data |= raw.GetStatsHistEn()   << 7;
		data |= raw.GetDataWidth()     << 8;
		data |= raw.GetBadPixelThr()   << 16;
		break;
	case SIPP_RAW_CFG_SHADOW_ADR :
		data  = raw.GetRawFormat(SHADOW);
		data |= raw.GetBayerPattern(SHADOW)  << 1;
		data |= raw.GetGrGbImbEn(SHADOW)     << 3;
		data |= raw.GetHotPixEn(SHADOW)      << 4;
		data |= raw.GetHotGreenPixEn(SHADOW) << 5;
		data |= raw.GetStatsPatchEn(SHADOW)  << 6;
		data |= raw.GetStatsHistEn(SHADOW)   << 7;
		data |= raw.GetDataWidth(SHADOW)     << 8;
		data |= raw.GetBadPixelThr(SHADOW)   << 16;
		break;
	case SIPP_GRGB_PLATO_ADR :
		data  = raw.GetDarkPlato();
		data |= raw.GetBrightPlato() << 16;
		break;
	case SIPP_GRGB_PLATO_SHADOW_ADR :
		data  = raw.GetDarkPlato(SHADOW);
		data |= raw.GetBrightPlato(SHADOW) << 16;
		break;
	case SIPP_GRGB_SLOPE_ADR :
		data  = raw.GetDarkSlope();
		data |= raw.GetBrightSlope() << 16;
		break;
	case SIPP_GRGB_SLOPE_SHADOW_ADR :
		data  = raw.GetDarkSlope(SHADOW);
		data |= raw.GetBrightSlope(SHADOW) << 16;
		break;
	case SIPP_BAD_PIXEL_CFG_ADR :
		data  = raw.GetAlphaRBCold();
		data |= raw.GetAlphaRBHot() << 4;
		data |= raw.GetAlphaGHot()  << 8;
		data |= raw.GetAlphaGHot()  << 12;
		data |= raw.GetNoiseLevel() << 16;
		break;
	case SIPP_BAD_PIXEL_CFG_SHADOW_ADR :
		data  = raw.GetAlphaRBCold();
		data |= raw.GetAlphaRBHot(SHADOW) << 4;
		data |= raw.GetAlphaGHot(SHADOW)  << 8;
		data |= raw.GetAlphaGHot(SHADOW)  << 12;
		data |= raw.GetNoiseLevel(SHADOW) << 16;
		break;
	case SIPP_GAIN_SATURATE_0_ADR :
		data  = raw.GetGain0();
		data |= raw.GetSat0() << 16;
		break;
	case SIPP_GAIN_SATURATE_0_SHADOW_ADR :
		data  = raw.GetGain0(SHADOW);
		data |= raw.GetSat0(SHADOW) << 16;
		break;
	case SIPP_GAIN_SATURATE_1_ADR :
		data  = raw.GetGain1();
		data |= raw.GetSat1() << 16;
		break;
	case SIPP_GAIN_SATURATE_1_SHADOW_ADR :
		data  = raw.GetGain1(SHADOW);
		data |= raw.GetSat1(SHADOW) << 16;
		break;
	case SIPP_GAIN_SATURATE_2_ADR :
		data  = raw.GetGain2();
		data |= raw.GetSat2() << 16;
		break;
	case SIPP_GAIN_SATURATE_2_SHADOW_ADR :
		data  = raw.GetGain2(SHADOW);
		data |= raw.GetSat2(SHADOW) << 16;
		break;
	case SIPP_GAIN_SATURATE_3_ADR :
		data  = raw.GetGain3();
		data |= raw.GetSat3() << 16;
		break;
	case SIPP_GAIN_SATURATE_3_SHADOW_ADR :
		data  = raw.GetGain3(SHADOW);
		data |= raw.GetSat3(SHADOW) << 16;
		break;
	case SIPP_STATS_PATCH_CFG_ADR :
		data  = raw.GetNumberOfPatchesH();
		data |= raw.GetNumberOfPatchesV() << 8;
		data |= raw.GetPatchWidth() << 16;
		data |= raw.GetPatchHeight() << 24;
		break;
	case SIPP_STATS_PATCH_CFG_SHADOW_ADR :
		data  = raw.GetNumberOfPatchesH(SHADOW);
		data |= raw.GetNumberOfPatchesV(SHADOW) << 8;
		data |= raw.GetPatchWidth(SHADOW) << 16;
		data |= raw.GetPatchHeight(SHADOW) << 24;
		break;
	case SIPP_STATS_PATCH_START_ADR :
		data  = raw.GetStartX();
		data |= raw.GetStartY() << 16;
		break;
	case SIPP_STATS_PATCH_START_SHADOW_ADR :
		data  = raw.GetStartX(SHADOW);
		data |= raw.GetStartY(SHADOW) << 16;
		break;
	case SIPP_STATS_PATCH_SKIP_ADR :
		data  = raw.GetSkipX();
		data |= raw.GetSkipY() << 16;
		break;
	case SIPP_STATS_PATCH_SKIP_SHADOW_ADR :
		data  = raw.GetSkipX(SHADOW);
		data |= raw.GetSkipY(SHADOW) << 16;
		break;
	case SIPP_RAW_STATS_PLANES_ADR :
		data  = raw.GetActivePlanes();
		data |= raw.GetActivePlanesNo() << 20;
		break;
	case SIPP_RAW_STATS_PLANES_SHADOW_ADR :
		data  = raw.GetActivePlanes(SHADOW);
		data |= raw.GetActivePlanesNo(SHADOW) << 20;
		break;

	case SIPP_LSC_FRM_DIM_ADR :
		data = lsc.GetImgDimensions(DEFAULT);
		break;
	case SIPP_LSC_FRM_DIM_SHADOW_ADR :
		data = lsc.GetImgDimensions(SHADOW);
		break;
	case SIPP_LSC_FRACTION_ADR :
		data  = lsc.GetHorIncrement();
		data |= lsc.GetVerIncrement() << 16;
		break;
	case SIPP_LSC_FRACTION_SHADOW_ADR :
		data  = lsc.GetHorIncrement(SHADOW);
		data |= lsc.GetVerIncrement(SHADOW) << 16;
		break;
	case SIPP_LSC_GM_DIM_ADR :
		data  = lsc.GetLscGmWidth();
		data |= lsc.GetLscGmHeight() << 16;
		break;
	case SIPP_LSC_GM_DIM_SHADOW_ADR :
		data  = lsc.GetLscGmWidth(SHADOW);
		data |= lsc.GetLscGmHeight(SHADOW) << 16;
		break;
	case SIPP_LSC_CFG_ADR :
		data  = lsc.GetLscFormat();
		data |= lsc.GetDataWidth() << 4;
		break;
	case SIPP_LSC_CFG_SHADOW_ADR :
		data  = lsc.GetLscFormat(SHADOW);
		data |= lsc.GetDataWidth(SHADOW) << 4;
		break;

	case SIPP_DBYR_FRM_DIM_ADR :
		data = dbyr.GetImgDimensions(DEFAULT);
		break;
	case SIPP_DBYR_FRM_DIM_SHADOW_ADR :
		data = dbyr.GetImgDimensions(SHADOW);
		break;
	case SIPP_DBYR_CFG_ADR :
		data  = dbyr.GetBayerPattern();
		data |= dbyr.GetLumaOnly() << 2;
		data |= dbyr.GetForceRbToZero() << 3;
		data |= dbyr.GetInputDataWidth() << 4;
		data |= dbyr.GetOutputDataWidth() << 8;
		data |= dbyr.GetImageOrderOut() << 12;
		data |= dbyr.GetPlaneMultiple() << 15;
		data |= dbyr.GetGradientMul() << 24;
		break;
	case SIPP_DBYR_CFG_SHADOW_ADR :
		data  = dbyr.GetBayerPattern(SHADOW);
		data |= dbyr.GetLumaOnly(SHADOW) << 2;
		data |= dbyr.GetForceRbToZero(SHADOW) << 3;
		data |= dbyr.GetInputDataWidth(SHADOW) << 4;
		data |= dbyr.GetOutputDataWidth(SHADOW) << 8;
		data |= dbyr.GetImageOrderOut(SHADOW) << 12;
		data |= dbyr.GetPlaneMultiple(SHADOW) << 15;
		data |= dbyr.GetGradientMul(SHADOW) << 24;
		break;
	case SIPP_DBYR_THRES_ADR :
		data  = dbyr.GetThreshold1();
		data |= dbyr.GetThreshold2() << 13;
		break;
	case SIPP_DBYR_THRES_SHADOW_ADR :
		data  = dbyr.GetThreshold1(SHADOW);
		data |= dbyr.GetThreshold2(SHADOW) << 13;
		break;
	case SIPP_DBYR_DEWORM_ADR :
		data  = dbyr.GetSlope();
		data |= dbyr.GetOffset() << 16;
		break;
	case SIPP_DBYR_DEWORM_SHADOW_ADR :
		data  = dbyr.GetSlope(SHADOW);
		data |= dbyr.GetOffset(SHADOW) << 16;
		break;

	case SIPP_DBYR_PPM_FRM_DIM_ADR :
		data = dbyrppm.GetImgDimensions(DEFAULT);
		break;
	case SIPP_DBYR_PPM_FRM_DIM_SHADOW_ADR :
		data = dbyrppm.GetImgDimensions(SHADOW);
		break;
	case SIPP_DBYR_PPM_CFG_ADR :
		data = dbyrppm.GetDataWidth();
		break;
	case SIPP_DBYR_PPM_CFG_SHADOW_ADR :
		data = dbyrppm.GetDataWidth(SHADOW);
		break;

	case SIPP_CHROMA_FRM_DIM_ADR :
		data = cdn.GetImgDimensions(DEFAULT);
		break;
	case SIPP_CHROMA_FRM_DIM_SHADOW_ADR :
		data = cdn.GetImgDimensions(SHADOW);
		break;
	case SIPP_CHROMA_CFG_ADR :
		data  = cdn.GetHorEnable();
		data |= cdn.GetRefEnable() << 3;
		data |= cdn.GetLimit()     << 4;
		data |= cdn.GetForceWeightsHor() << 12;
		data |= cdn.GetForceWeightsVer() << 13;
		data |= cdn.GetThreePlaneMode () << 14;
		break;
	case SIPP_CHROMA_CFG_SHADOW_ADR :
		data  = cdn.GetHorEnable(SHADOW);
		data |= cdn.GetRefEnable(SHADOW) << 3;
		data |= cdn.GetLimit(SHADOW)     << 4;
		data |= cdn.GetForceWeightsHor(SHADOW) << 12;
		data |= cdn.GetForceWeightsVer(SHADOW) << 13;
		data |= cdn.GetThreePlaneMode (SHADOW) << 14;
		break;
	case SIPP_CHROMA_THRESH_ADR :
		data  = cdn.GetHorThreshold1() <<  0;
		data |= cdn.GetHorThreshold2() <<  8;
		data |= cdn.GetVerThreshold1() << 16;
		data |= cdn.GetVerThreshold2() << 24;
		break;
	case SIPP_CHROMA_THRESH_SHADOW_ADR :
		data  = cdn.GetHorThreshold1(SHADOW) <<  0;
		data |= cdn.GetHorThreshold2(SHADOW) <<  8;
		data |= cdn.GetVerThreshold1(SHADOW) << 16;
		data |= cdn.GetVerThreshold2(SHADOW) << 24;
		break;
	case SIPP_CHROMA_THRESH2_ADR :
		data  = cdn.GetHorThreshold3() <<  0;
		data |= cdn.GetVerThreshold3() << 16;
		break;
	case SIPP_CHROMA_THRESH2_SHADOW_ADR :
		data  = cdn.GetHorThreshold3(SHADOW) <<  0;
		data |= cdn.GetVerThreshold3(SHADOW) << 16;
		break;

	case SIPP_LUMA_FRM_DIM_ADR :
		data = luma.GetImgDimensions(DEFAULT);
		break;
	case SIPP_LUMA_FRM_DIM_SHADOW_ADR :
		data = luma.GetImgDimensions(SHADOW);
		break;
	case SIPP_LUMA_CFG_ADR :
		data  = luma.GetBitPosition();
		data |= luma.GetAlpha() << 8;
		break;
	case SIPP_LUMA_CFG_SHADOW_ADR :
		data  = luma.GetBitPosition(SHADOW);
		data |= luma.GetAlpha(SHADOW) << 8;
		break;

	case SIPP_SHARPEN_FRM_DIM_ADR :
		data = usm.GetImgDimensions(DEFAULT);
		break;
	case SIPP_SHARPEN_FRM_DIM_SHADOW_ADR :
		data = usm.GetImgDimensions(SHADOW);
		break;
	case SIPP_SHARPEN_CFG_ADR :
		data  = usm.GetKernelSize();
		data |= usm.GetOutputClamp() << 3;
		data |= usm.GetMode() << 4;
		data |= usm.GetOutputDeltas() << 5;
		data |= usm.GetThreshold() << 16;
		break;
	case SIPP_SHARPEN_CFG_SHADOW_ADR :
		data  = usm.GetKernelSize(SHADOW);
		data |= usm.GetOutputClamp(SHADOW) << 3;
		data |= usm.GetMode(SHADOW) << 4;
		data |= usm.GetOutputDeltas(SHADOW) << 5;
		data |= usm.GetThreshold(SHADOW) << 16;
		break;
	case SIPP_SHARPEN_STREN_ADR :
		data  = usm.GetPosStrength();
		data |= usm.GetNegStrength() << 16;
		break;
	case SIPP_SHARPEN_STREN_SHADOW_ADR :
		data  = usm.GetPosStrength(SHADOW);
		data |= usm.GetNegStrength(SHADOW) << 16;
		break;
	case SIPP_SHARPEN_CLIP_ADR :
		data  = usm.GetClippingAlpha();
		break;
	case SIPP_SHARPEN_CLIP_SHADOW_ADR :
		data  = usm.GetClippingAlpha(SHADOW);
		break;
	case SIPP_SHARPEN_LIMIT_ADR :
		data  = usm.GetUndershoot();
		data |= usm.GetOvershoot() << 16;
		break;
	case SIPP_SHARPEN_LIMIT_SHADOW_ADR :
		data  = usm.GetUndershoot(SHADOW);
		data |= usm.GetOvershoot(SHADOW) << 16;
		break;

	case SIPP_UPFIRDN_FRM_IN_DIM_ADR :
		data = upfirdn.GetImgDimensions(DEFAULT);
		break;
	case SIPP_UPFIRDN_FRM_IN_DIM_SHADOW_ADR :
		data = upfirdn.GetImgDimensions(SHADOW);
		break;
	case SIPP_UPFIRDN_FRM_OUT_DIM_ADR :
		data = upfirdn.GetOutImgDimensions(DEFAULT);
		break;
	case SIPP_UPFIRDN_FRM_OUT_DIM_SHADOW_ADR :
		data = upfirdn.GetOutImgDimensions(SHADOW);
		break;
	case SIPP_UPFIRDN_CFG_ADR :
		data  = upfirdn.GetKernelSize();
		data |= upfirdn.GetOutputClamp() << 3;
		data |= upfirdn.GetHorizontalDenominator() << 4;
		data |= upfirdn.GetHorizontalNumerator() << 10;
		data |= upfirdn.GetVerticalDenominator() << 16;
		data |= upfirdn.GetVerticalNumerator() << 22;
		break;
	case SIPP_UPFIRDN_CFG_SHADOW_ADR :
		data  = upfirdn.GetKernelSize(SHADOW);
		data |= upfirdn.GetOutputClamp(SHADOW) << 3;
		data |= upfirdn.GetHorizontalDenominator(SHADOW) << 4;
		data |= upfirdn.GetHorizontalNumerator(SHADOW) << 10;
		data |= upfirdn.GetVerticalDenominator(SHADOW) << 16;
		data |= upfirdn.GetVerticalNumerator(SHADOW) << 22;
		break;
	case SIPP_UPFIRDN_VPHASE_ADR :
		data  = upfirdn.GetVerticalPhase();
		data |= upfirdn.GetVerticalDecimationCounter() << 8;
		break;

	case SIPP_MED_FRM_DIM_ADR :
		data = med.GetImgDimensions(DEFAULT);
		break;
	case SIPP_MED_FRM_DIM_SHADOW_ADR :
		data = med.GetImgDimensions(SHADOW);
		break;
	case SIPP_MED_CFG_ADR :
		data  = med.GetKernelSize();
        data |= med.GetOutputSelection() << 8;
		data |= med.GetThreshold() << 16;
		break;
	case SIPP_MED_CFG_SHADOW_ADR :
		data  = med.GetKernelSize(SHADOW);
        data |= med.GetOutputSelection(SHADOW) << 8;
		data |= med.GetThreshold(SHADOW) << 16;
		break;

	case SIPP_LUT_FRM_DIM_ADR :
		data = lut.GetImgDimensions(DEFAULT);
		break;
	case SIPP_LUT_FRM_DIM_SHADOW_ADR :
		data = lut.GetImgDimensions(SHADOW);
		break;
	case SIPP_LUT_CFG_ADR : 
		data  = lut.GetInterpolationModeEnable();
		data |= lut.GetChannelModeEnable() << 1;
		data |= lut.GetIntegerWidth() << 3;
		data |= lut.GetNumbefOfLUTs() << 8;
		data |= lut.GetNumberOfChannels() << 12;
		data |= lut.GetLutLoadEnable()    << 14;
		data |= lut.GetApbAccessEnable()  << 15;
		break;
	case SIPP_LUT_CFG_SHADOW_ADR : 
		data  = lut.GetInterpolationModeEnable(SHADOW);
		data |= lut.GetChannelModeEnable(SHADOW) << 1;
		data |= lut.GetIntegerWidth(SHADOW) << 3;
		data |= lut.GetNumbefOfLUTs(SHADOW) << 8;
		data |= lut.GetNumberOfChannels(SHADOW) << 12;
		data |= lut.GetLutLoadEnable()    << 14;
		data |= lut.GetApbAccessEnable()  << 15;
		break;
	case SIPP_LUT_APB_RDATA_ADR :
		data = lut.GetLutValue();
		break;

	case SIPP_EDGE_OP_FRM_DIM_ADR :
		data = edge.GetImgDimensions(DEFAULT);
		break;
	case SIPP_EDGE_OP_FRM_DIM_SHADOW_ADR :
		data = edge.GetImgDimensions(SHADOW);
		break;
	case SIPP_EDGE_OP_CFG_ADR :
		data  = edge.GetInputMode();
		data |= edge.GetOutputMode() << 2;
		data |= edge.GetThetaMode() << 5;
		data |= edge.GetMagnScf() << 16;
		break;
	case SIPP_EDGE_OP_CFG_SHADOW_ADR :
		data  = edge.GetInputMode(SHADOW);
		data |= edge.GetOutputMode(SHADOW) << 2;
		data |= edge.GetThetaMode(SHADOW) << 5;
		data |= edge.GetMagnScf(SHADOW) << 16;
		break;

	case SIPP_CONV_FRM_DIM_ADR :
		data = conv.GetImgDimensions(DEFAULT);
		break;
	case SIPP_CONV_FRM_DIM_SHADOW_ADR :
		data = conv.GetImgDimensions(SHADOW);
		break;
	case SIPP_CONV_CFG_ADR :
		data  = conv.GetKernelSize();
		data |= conv.GetOutputClamp() << 3;
		data |= conv.GetAbsBit()      << 4;
		data |= conv.GetSqBit()       << 5;
		data |= conv.GetAccumBit()    << 6;
		data |= conv.GetOdBit()       << 7;
		data |= conv.GetThreshold()   << 8;
		break;
	case SIPP_CONV_CFG_SHADOW_ADR :
		data  = conv.GetKernelSize(SHADOW);
		data |= conv.GetOutputClamp(SHADOW) << 3;
		data |= conv.GetAbsBit(SHADOW)      << 4;
		data |= conv.GetSqBit(SHADOW)       << 5;
		data |= conv.GetAccumBit(SHADOW)    << 6;
		data |= conv.GetOdBit(SHADOW)       << 7;
		data |= conv.GetThreshold(SHADOW)   << 8;
		break;
	case SIPP_CONV_ACCUM_ADR :
		data = conv.GetAccumResult();
		break;
	case SIPP_CONV_ACCUM_CNT_ADR :
		data = conv.GetAccumCnt();
		break;

	case SIPP_HARRIS_FRM_DIM_ADR :
		data = harr.GetImgDimensions(DEFAULT);
		break;
	case SIPP_HARRIS_FRM_DIM_SHADOW_ADR :
		data = harr.GetImgDimensions(SHADOW);
		break;
	case SIPP_HARRIS_CFG_ADR :
		data = harr.GetKernelSize();
		break;
	case SIPP_HARRIS_CFG_SHADOW_ADR :
		data = harr.GetKernelSize(SHADOW);
		break;
	case SIPP_HARRIS_K_ADR :
		data = harr.GetK();
		break;
	case SIPP_HARRIS_K_SHADOW_ADR :
		data = harr.GetK(SHADOW);
		break;

	case SIPP_CC_FRM_DIM_ADR :
		data = cc.GetImgDimensions(DEFAULT);
		break;
	case SIPP_CC_CFG_ADR :
		data  = cc.GetForceLumaOne();
		data |= cc.GetMulCoeff() << 8;
		data |= cc.GetThreshold() << 16;
		data |= cc.GetPlaneMultiple() << 24;
		break;
	case SIPP_CC_KRGB0_ADR :
		data  = cc.GetRPlaneCoeff();
		data |= cc.GetGPlaneCoeff() << 16;
		break;
	case SIPP_CC_KRGB1_ADR :
		data  = cc.GetBPlaneCoeff();
		data |= cc.GetEpsilon() << 16;
		break;

		// TODO - connect remaining APB registers to filter object methods



	default :
		cerr << setbase (16);
		cerr << "Warning: ApbWrite(): unmapped APB read access to 0x" << addr << endl;
		cerr << setbase (10);
		break;
	}
	return data;
}

SippHW sippHwAcc;  //create an instance of the HW accelerators