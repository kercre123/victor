

// 1: Includes
// ----------------------------------------------------------------------------
#include "isaac_registers.h"
#include "DrvCpr.h"
#include "swcShaveLoader.h"

#include "biResizeApi.h"

// 2:  Source Specific #defines and types  (typedef,enum,struct)
// ----------------------------------------------------------------------------

// 3: Global Data (Only if absolutely necessary)
// ----------------------------------------------------------------------------

// mbin start addresses
extern unsigned char binary_resize_mbin_start[];
// shave functions  addresses
extern unsigned char SVE0_Resize[];
// data structures addresses
extern unsigned char SVE0_ResizeData[];

// 4: Static Local Data
// ----------------------------------------------------------------------------
volatile biResize_param_t *p_biResize = 0;
volatile unsigned int biResizeSvu = BI_RESIZE_INVALID;

// 5: Static Function Prototypes
// ----------------------------------------------------------------------------

// 6: Functions Implementation
// ----------------------------------------------------------------------------

unsigned int biResizeInit(unsigned int svuNr)
{
    unsigned int swapAddr;
    unsigned int retValue = BI_RESIZE_BUSY;
    if (p_biResize == 0)
    {
        // Resize, static architecture
        biResizeSvu = svuNr;
        swapAddr = (1 << biResizeSvu);
        DrvCprSysDeviceAction(ENABLE_CLKS, ((u64)swapAddr) << 32);
        DrvCprSysDeviceAction(RESET_DEVICES, ((u64)swapAddr) << 32);

        swapAddr = GET_REG_WORD_VAL(LHB_CMX_RAMLAYOUT_CFG);
        swapAddr &= ~(0x0F << (biResizeSvu << 2));
        swapAddr |= (0x01 << (biResizeSvu << 2));
        SET_REG_WORD(LHB_CMX_RAMLAYOUT_CFG, swapAddr);

        // Set SHAVE window
        swcSetShaveWindow(biResizeSvu, 0, MXI_CMX_BASE_ADR + (0x20000 * biResizeSvu) + 0x4000);
        swcSetShaveWindow(biResizeSvu, 1, MXI_CMX_BASE_ADR + (0x20000 * biResizeSvu));

        // Reset SHAVE
        swcResetShave(biResizeSvu);

        // Load resize mbin
        swcLoadMbin((u8 *)binary_resize_mbin_start, biResizeSvu);

        // Set Resize
        swapAddr = (u32)SVE0_ResizeData;
        p_biResize = (biResize_param_t *)(LHB_CMX_BASE_ADR + (0x20000 * biResizeSvu) + (swapAddr & 0x00FFFFFF));

        retValue = BI_RESIZE_OK;
    }
    return retValue;
}

unsigned int biResizeDeinit(void)
{
    unsigned int swapAddr;
    unsigned int retValue = BI_RESIZE_BUSY;

    if (p_biResize != 0)
    {
        swcResetShave(biResizeSvu);
        swapAddr = 1 << biResizeSvu;
        DrvCprSysDeviceAction(DISABLE_CLKS, ((u64)swapAddr) << 32);
        DrvCprSysDeviceAction(RESET_DEVICES, DEV_USB);
        DrvCprSysDeviceAction(DISABLE_CLKS,  DEV_USB);
        biResizeSvu = BI_RESIZE_INVALID;
        p_biResize = 0;
    }
    return retValue;
}

unsigned int biResizeGet(biResize_param_t* param)
{
    unsigned int retValue = BI_RESIZE_INIT;

    if (p_biResize != 0)
    {
        param->wi        = p_biResize->wi;
        param->hi        = p_biResize->hi;
        param->wo        = p_biResize->wo;
        param->ho        = p_biResize->ho;
        param->offsetX   = p_biResize->offsetX;
        param->offsetY   = p_biResize->offsetY;
        param->inBuffer  = p_biResize->inBuffer;
        param->outBuffer = p_biResize->outBuffer;
        retValue = BI_RESIZE_OK;
    }
    return retValue;
}

unsigned int biResizeSet(biResize_param_t* param)
{
    unsigned int retValue = BI_RESIZE_INIT;

    if (p_biResize != 0)
    {
        if (swcShaveRunning(biResizeSvu) == 0)
        {
            p_biResize->wi        = param->wi;
            p_biResize->hi        = param->hi;
            p_biResize->wo        = param->wo;
            p_biResize->ho        = param->ho;
            p_biResize->offsetX   = param->offsetX;
            p_biResize->offsetY   = param->offsetY;
            p_biResize->inBuffer  = param->inBuffer;
            p_biResize->outBuffer = param->outBuffer;
            swcResetShave(biResizeSvu);
            swcStartShave(biResizeSvu, (unsigned int)SVE0_Resize);
            retValue = BI_RESIZE_OK;
        } else
        {
            retValue = BI_RESIZE_BUSY;
        }
    }
    return retValue;
}
