///
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved.
///            For License Warranty see: common/license.txt
///
/// @brief     USB Driver
///
/// USB Driver API function implementations
///

// 1: Includes
// ----------------------------------------------------------------------------
#include "isaac_registers.h"
#include "DrvCpr.h"
#include "DrvSvuDefines.h"
#include "swcShaveLoader.h"
#include "swcLeonUtils.h"

#include "usbCoreApi.h"
#include "../shave/usb.h"
#include "DrvSvu.h"

// 2:  Source Specific #defines and types  (typedef,enum,struct)
// ----------------------------------------------------------------------------

// 3: Global Data (Only if absolutely necessary)
// ----------------------------------------------------------------------------

// mbin start addresses
extern unsigned char binary_usb_mbin_start[];
// shave functions  addresses
extern unsigned char SVE0_USB_Main[];
// data structures addresses
extern unsigned char SVE0_usb_ctrl[];

// 4: Static Local Data
// ----------------------------------------------------------------------------
volatile usb_ctrl_t *p_usb = 0;
volatile unsigned int usbCoreSvu = USB_CORE_INVALID;

// 5: Static Function Prototypes
// ----------------------------------------------------------------------------

// 6: Functions Implementation
// ----------------------------------------------------------------------------

unsigned int usbInit(unsigned int svuNr)
{
	unsigned int swapAddr;
	unsigned int retValue = USB_CORE_BUSY;
	if (p_usb == 0)
	{
		usbCoreSvu = svuNr;
		// usb calibration
		*(u32 *)CPR_USB_CTRL_ADR = 0x00003004;
		//*(u32 *)CPR_USB_PHY_CTRL_ADR = 0x01C18D18;
		*(u32 *)CPR_USB_PHY_CTRL_ADR = 0x01C18D07;
		// hw init
		DrvCprSysDeviceAction(ENABLE_CLKS, DEV_USB);
		DrvCprSysDeviceAction(RESET_DEVICES, DEV_USB);

		swapAddr = 1<<usbCoreSvu;
		DrvCprSysDeviceAction(ENABLE_CLKS, ((u64)swapAddr) << 32);
		DrvCprSysDeviceAction(RESET_DEVICES, ((u64)swapAddr) << 32);
		// set desired cmx config
		swapAddr = GET_REG_WORD_VAL(LHB_CMX_RAMLAYOUT_CFG);
		swapAddr &= ~(0x0F<<(usbCoreSvu<<2));
		swapAddr |= (0x06<<(usbCoreSvu<<2));
		SET_REG_WORD(LHB_CMX_RAMLAYOUT_CFG, swapAddr);
		// Set shave window
		swcSetShaveWindow(usbCoreSvu, 0, MXI_CMX_BASE_ADR + (LHB_CMX_SLICE_SIZE*usbCoreSvu) + USB_CODE_SECTION_SIZE);
		swcSetShaveWindow(usbCoreSvu, 1, MXI_CMX_BASE_ADR + (LHB_CMX_SLICE_SIZE*usbCoreSvu));
		// Reset shave
		swcResetShave(usbCoreSvu);
		// Load USB mbin
		swcLoadMbin((unsigned char *)binary_usb_mbin_start, usbCoreSvu);
		
		// init data structure
		swapAddr = (unsigned int)SVE0_usb_ctrl;
		p_usb = (usb_ctrl_t *)(LHB_CMX_BASE_ADR + (LHB_CMX_SLICE_SIZE*usbCoreSvu) + USB_CODE_SECTION_SIZE + (swapAddr&0x00FFFFFF));
		// set stack pointer
		DrvSvutIrfWrite(usbCoreSvu, 19, (WINDOW_A+USB_STACKTOP_OFFSET));
		// Start SHAVE
		swcStartShave(usbCoreSvu, (unsigned int)SVE0_USB_Main);

		retValue = USB_CORE_OK;
	}
	return retValue;
}

unsigned int usbDeinit(void)
{
	unsigned int swapAddr;
	unsigned int retValue = USB_CORE_OK;
	
	if (p_usb != 0)
	{
		swcResetShave(usbCoreSvu);
		swapAddr = 1<<usbCoreSvu;
		DrvCprSysDeviceAction(DISABLE_CLKS, ((u64)swapAddr) << 32);
		DrvCprSysDeviceAction(RESET_DEVICES, DEV_USB);
		DrvCprSysDeviceAction(DISABLE_CLKS,  DEV_USB);
		usbCoreSvu = USB_CORE_INVALID;
		p_usb = 0;
	}
	return retValue;
}

// used to return buffer size in case of OUT streaming, difficult to use must watch out that streaming does not starve
unsigned int usbGetBuffer(usb_stream_idx_e idx, usb_stream_sink_e sink, usb_stream_param_t* param)
{
	unsigned int retValue = USB_CORE_INIT;
	
	if (p_usb != 0)
	{
		if (sink == USB_CORE_PRIMARY_SINK)
		{
			param->address = swcLeonReadNoCacheU32(&p_usb->stream[idx].PriAddr);
			param->nbytes  = swcLeonReadNoCacheU32(&p_usb->stream[idx].PriLength);
		} else
		{
			param->address = swcLeonReadNoCacheU32(&p_usb->stream[idx].SecAddr);
			param->nbytes  = swcLeonReadNoCacheU32(&p_usb->stream[idx].SecLength);
		}
		if ( (param->address == 0) && (param->nbytes != 0) ) retValue = USB_CORE_OK; else retValue = USB_CORE_BUSY;
	}
	return retValue;
}

// used to submit buffers for IN/OUT streaming
unsigned int usbSetBuffer(usb_stream_idx_e idx, usb_stream_sink_e sink, usb_stream_param_t* param)
{
	unsigned int retValue = USB_CORE_INIT;
	
	if (p_usb != 0)
	{
		if (sink == USB_CORE_PRIMARY_SINK)
		{
			retValue = USB_CORE_BUSY;
			if (swcLeonReadNoCacheU32(&p_usb->stream[idx].PriAddr)== 0)
			{
				swcLeonWriteNoCache32(&p_usb->stream[idx].PriLength, param->nbytes);
				swcLeonWriteNoCache32(&p_usb->stream[idx].PriAddr, param->address);
			    retValue = USB_CORE_OK;
			}
		} else
		{
			retValue = USB_CORE_BUSY;
			if (swcLeonReadNoCacheU32(&p_usb->stream[idx].SecAddr) == 0)
			{
				swcLeonWriteNoCache32(&p_usb->stream[idx].SecLength, param->nbytes);
				swcLeonWriteNoCache32(&p_usb->stream[idx].SecAddr, param->address);
			    retValue = USB_CORE_OK;
			}
		}
	}
	return retValue;
}

// used to submit buffers for IN/OUT streaming after stream is starved
unsigned int usbSetBufferSequential(usb_stream_idx_e idx, usb_stream_sink_e sink, usb_stream_param_t* param)
{
	unsigned int retValue = USB_CORE_INIT;

	if (p_usb != 0)
	{
		if (sink == USB_CORE_PRIMARY_SINK)
		{
			retValue = USB_CORE_BUSY;
			if ( (swcLeonReadNoCacheU32(&p_usb->stream[idx].PriAddr) == 0) && (swcLeonReadNoCacheU32(&p_usb->stream[idx].PriAddrShadow) == 0) )
			{
				swcLeonWriteNoCache32(&p_usb->stream[idx].PriLength, param->nbytes);
				swcLeonWriteNoCache32(&p_usb->stream[idx].PriAddr, param->address);
			    retValue = USB_CORE_OK;
			}
		} else
		{
			retValue = USB_CORE_BUSY;
			if ( (swcLeonReadNoCacheU32(&p_usb->stream[idx].SecAddr) == 0) && (swcLeonReadNoCacheU32(&p_usb->stream[idx].SecAddrShadow) == 0) )
			{
				swcLeonWriteNoCache32(&p_usb->stream[idx].SecLength, param->nbytes);
				swcLeonWriteNoCache32(&p_usb->stream[idx].SecAddr, param->address);
			    retValue = USB_CORE_OK;
			}
		}
	}
	return retValue;
}

// get one of the configurable signals
unsigned int usbGetSignal(unsigned int idx)
{
	unsigned int retValue = 0xFFFFFFFF;
	
	if ( (p_usb != 0) && (idx < USB_SIGNAL_NR) )
	{
		retValue = swcLeonReadNoCacheU32(&p_usb->interruptSignal[idx]);
	}
	
	return retValue;
}


// set one of the configurable signals
void usbSetSignal(unsigned int idx, unsigned int value)
{
	if ( (p_usb != 0) && (idx < USB_SIGNAL_NR) )
	{
		swcLeonWriteNoCache32(&p_usb->interruptSignal[idx], value);
	}
}


unsigned int usbGetAltInterface(unsigned int idx)
{
	unsigned int retValue = 0;
	
	if (p_usb != 0)
	{
		if (idx<4)
		{
			retValue = (swcLeonReadNoCacheU32(&p_usb->interfaceAlter03)>>(idx<<3))&0xFF;
		} else
		{
			retValue = (swcLeonReadNoCacheU32(&p_usb->interfaceAlter47)>>((idx-4)<<3))&0xFF;
		}
	}

	return retValue;
}


