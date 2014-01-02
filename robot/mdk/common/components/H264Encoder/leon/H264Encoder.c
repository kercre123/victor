///
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved.
///            For License Warranty see: common/license.txt
///
/// @brief
///


// 1: Includes
// ----------------------------------------------------------------------------
#include "isaac_registers.h"
#include "DrvCpr.h"
#include "DrvSvu.h"
#include "DrvIcb.h"
#include "swcShaveLoader.h"
#include "swcLeonUtils.h"
#include "DrvTimer.h"

// 2:  Source Specific #defines and types  (typedef,enum,struct)
// ----------------------------------------------------------------------------
#include "H264EncoderApi.h"
#include "H264EncoderPrivateDefines.h"


// 3: Global Data (Only if absolutely necessary)
// ----------------------------------------------------------------------------
// mbin start addresses
extern unsigned char binary_enc_mbin_start[];
extern unsigned char binary_ec_mbin_start[];
// shave functions  addresses
extern unsigned int SS_ENC_Main;
extern unsigned int SS_EC_Main;
// data structures addresses
extern unsigned int SS_encoder;

// 4: Static Local Data
// ----------------------------------------------------------------------------
volatile h264_encoder_t *p_enc = 0;
unsigned int h264_encode_svu = H264_ENCODE_INVALID;
unsigned int h264_encode_flag = 0;
h264_encode_param_t h264_encode_get;
h264_encode_param_t h264_encode_set;

// 5: Static Function Prototypes
// ----------------------------------------------------------------------------

// 6: Functions Implementation
// ----------------------------------------------------------------------------


void H264_ENC_CODE_SECTION(H264EncodeIsr) (unsigned int source)
{
    unsigned int svuNr;

    // read from waiting shave encoded stream parameters
    h264_encode_get.address = h264_encode_flag;

    h264_encode_get.nbytes = swcLeonReadNoCacheU32(&p_enc->cmdId);
    h264_encode_get.nbytes -= h264_encode_get.address;
    h264_encode_flag = 0;
    // if next buffer is available latch into shave register to resume encoding
    if (h264_encode_set.nbytes != 0)
    {
        swcLeonWriteNoCache32(&p_enc->naluNext, h264_encode_set.address);
        h264_encode_flag = h264_encode_set.address;
        h264_encode_set.nbytes = 0;
    }
    // clear interrupt before return
    svuNr = source - IRQ_SVE_0;
    SET_REG_WORD(SVU_CTRL_ADDR[svuNr] + SLC_OFFSET_SVU + SVU_IRR, 0x20);
    DrvIcbIrqClear(source);
}

// usually get is called in pair with set in order to not miss stream parts
unsigned int H264_ENC_CODE_SECTION(H264EncodeGetBuffer) (h264_encode_param_t * param)
{
    unsigned int retValue = H264_ENCODE_INIT;

    if (p_enc != 0)
    {
        // if we have an encoded stream part return it
        if (h264_encode_get.nbytes != 0)
        {
            param->address =  h264_encode_get.address;
            param->nbytes = h264_encode_get.nbytes;
            h264_encode_get.nbytes = 0;
            retValue = H264_ENCODE_OK;
        }
        else
        {
            retValue = H264_ENCODE_BUSY;
        }
    }

    return retValue;
}

unsigned int H264_ENC_CODE_SECTION(H264EncodeSetBuffer) (h264_encode_param_t * param)
{
    unsigned int buffer, config, svuId;
    unsigned int retValue = H264_ENCODE_INIT;

    if (p_enc != 0)
    {
        // special handling to detect startup conditions
        buffer = swcLeonReadNoCacheU32(&p_enc->naluBuffer);
        if (buffer == 0)
        {
            h264_encode_set.address = param->address;
            h264_encode_set.nbytes = 0;
            swcLeonWriteNoCache32(&p_enc->naluBuffer, h264_encode_set.address);
            h264_encode_flag = h264_encode_set.address;
            // check start condition
            buffer = swcLeonReadNoCacheU32(&p_enc->origFrame);
            if (buffer != 0)
            {
                config = swcLeonReadNoCacheU32(&p_enc->cmdCfg);
                svuId = (config >> 16) & 0x07;
                swcStartShave(svuId, (u32)&SS_EC_Main);
                SleepMicro(100);
                // start all encoders
                for (svuId = 0; svuId < 8; svuId++)
                {
                    if (((1 << svuId) & config) != 0)
                    {
                        SleepMicro(100);
                        swcStartShave(svuId, (u32)&SS_ENC_Main);
                    }
                }
            }
            retValue = H264_ENCODE_OK;
        } else
        {
            // see if we can accept new buffer
            if (h264_encode_set.nbytes == 0)
            {
                h264_encode_set.address = param->address;
                h264_encode_set.nbytes = param->nbytes;
                // handle delayed set
                if (h264_encode_flag == 0)
                {
                    swcLeonWriteNoCache32(&p_enc->naluNext, h264_encode_set.address);
                    h264_encode_flag = h264_encode_set.address;
                    h264_encode_set.nbytes = 0;
                }
                retValue = H264_ENCODE_OK;
            } else
            {
                retValue = H264_ENCODE_BUSY;
            }
        }
    }
    return retValue;
}

unsigned int H264_ENC_CODE_SECTION(H264EncodeGetFrame) (h264_encode_param_t *param)
{
    unsigned int width, height;
    unsigned int retValue = H264_ENCODE_INIT;

    if (p_enc != 0)
    {
        param->address = swcLeonReadNoCacheU32(&p_enc->origFrame);
        width = swcLeonReadNoCacheU32(&p_enc->width);
        height = swcLeonReadNoCacheU32(&p_enc->height);
        param->nbytes  = (3 * width * height) >> 1;
        retValue = H264_ENCODE_OK;
    }
    return retValue;
}

unsigned int H264_ENC_CODE_SECTION(H264EncodeSetFrame) (h264_encode_param_t *param)
{
    unsigned int buffer, config, svuId;
    unsigned int retValue = H264_ENCODE_INIT;

    if (p_enc != 0)
    {
        buffer = swcLeonReadNoCacheU32(&p_enc->origFrame);
        if (buffer == 0)
        {
            swcLeonWriteNoCache32(&p_enc->origFrame, param->address);
            buffer = swcLeonReadNoCacheU32(&p_enc->naluBuffer);
            if (buffer != 0)
            {
                config = swcLeonReadNoCacheU32(&p_enc->cmdCfg);
                svuId = (config >> 16) & 0x07;
                swcStartShave(svuId, (u32)&SS_EC_Main);
                SleepMicro(100);
                // start all encoders
                for (svuId = 0; svuId < 8; svuId++)
                {
                    if (((1 << svuId) & config) != 0)
                    {
                        swcStartShave(svuId, (u32)&SS_ENC_Main);
                        SleepMicro(100);
                    }
                }
            }
            retValue = H264_ENCODE_OK;
        } else
        {
            buffer = swcLeonReadNoCacheU32(&p_enc->nextFrame);
            if (buffer == 0)
            {
                swcLeonWriteNoCache32(&p_enc->nextFrame, param->address);
                retValue = H264_ENCODE_OK;
            } else
            {
                retValue = H264_ENCODE_BUSY;
            }
        }
    }

    return retValue;
}

unsigned int H264_ENC_CODE_SECTION(H264EncodeInit) (h264_encoder_t *param)
{
    unsigned int config, lastAddr, swapAddr, svuId;
    unsigned int retValue = H264_ENCODE_BUSY;

    if (p_enc == 0)
    {
        // Enable NAL Unit clk
        DrvCprSysDeviceAction(ENABLE_CLKS, DEV_NAL);
        SleepMicro(100);
        // Reset NAL device
        DrvCprSysDeviceAction(RESET_DEVICES, DEV_NAL);
        SleepMicro(100);
        // identify entropy codec and initialize
        config = param->cmdCfg;
        svuId = (config >> 16) & 0x07;
        // Enable clock
        swapAddr = (1 << svuId);
        DrvCprSysDeviceAction(ENABLE_CLKS, ((u64)swapAddr) << 32);
        DrvCprSysDeviceAction(RESET_DEVICES, ((u64)swapAddr) << 32);
        // Configure CMX
        swapAddr = GET_REG_WORD_VAL(LHB_CMX_RAMLAYOUT_CFG);
        swapAddr &= ~(0x0F << (svuId << 2));
        swapAddr |= (0x01 << (svuId << 2));
        SET_REG_WORD(LHB_CMX_RAMLAYOUT_CFG, swapAddr);
        // Set Shave Window
        swcSetShaveWindow(svuId, 0, MXI_CMX_BASE_ADR + (0x20000 * svuId) + 0x4000);
        swcSetShaveWindow(svuId, 1, MXI_CMX_BASE_ADR + (0x20000 * svuId));
        // Reset Shave
        swcResetShave(svuId);
        // Load encoder mbin
        swcLoadMbin((u8 *)binary_ec_mbin_start, svuId);
        DrvIcbSetIrqHandler((IRQ_SVE_0 + svuId),(irq_handler) H264EncodeIsr);
        DrvIcbConfigureIrq((IRQ_SVE_0 + svuId), H264_ENCODE_INTERRUPT_LEVEL, POS_LEVEL_INT);
        DrvIcbEnableIrq(IRQ_SVE_0 + svuId);

        h264_encode_svu = svuId;
        h264_encode_flag = 0;
        h264_encode_get.nbytes = 0;
        h264_encode_set.nbytes = 0;
        swapAddr = (unsigned int)&SS_encoder;
        p_enc = (h264_encoder_t *) (LHB_CMX_BASE_ADR + (0x20000 * svuId) + 0x4000 + (swapAddr & 0x00FFFFFF));
        swcLeonWriteNoCache32(&p_enc->cmdId, param->cmdId);
        swcLeonWriteNoCache32(&p_enc->cmdCfg, param->cmdCfg);
        swcLeonWriteNoCache32(&p_enc->width, param->width);
        swcLeonWriteNoCache32(&p_enc->height, param->height);
        swcLeonWriteNoCache32(&p_enc->frameNum, param->frameNum);
        swcLeonWriteNoCache32(&p_enc->frameQp, (param->frameQp-12));
        swcLeonWriteNoCache32(&p_enc->fps, param->fps);
        swcLeonWriteNoCache32(&p_enc->tgtRate, param->tgtRate);
        swcLeonWriteNoCache32(&p_enc->avgRate, param->avgRate);
        swcLeonWriteNoCache32(&p_enc->frameGOP, param->frameGOP);
        swcLeonWriteNoCache32(&p_enc->naluBuffer, param->naluBuffer);
        swcLeonWriteNoCache32(&p_enc->naluNext, param->naluNext);
        swcLeonWriteNoCache32(&p_enc->origFrame, param->origFrame);
        swcLeonWriteNoCache32(&p_enc->nextFrame, param->nextFrame);
        swcLeonWriteNoCache32(&p_enc->currFrame, param->currFrame);
        swcLeonWriteNoCache32(&p_enc->refFrame, param->refFrame);

        // initialize encoders
        lastAddr = 0;
        svuId = 8;
        do
        {
            svuId--;
            if ( ((1 << svuId) & config) != 0 )
            {
                // Enable clock
                swapAddr = (1 << svuId);
                DrvCprSysDeviceAction(ENABLE_CLKS, ((u64)swapAddr) << 32);
                DrvCprSysDeviceAction(RESET_DEVICES, ((u64)swapAddr) << 32);
                // Configure CMX
                swapAddr = GET_REG_WORD_VAL(LHB_CMX_RAMLAYOUT_CFG);
                swapAddr &= ~(0x0F << (svuId << 2));
                swapAddr |= (0x01 << (svuId << 2));
                SET_REG_WORD(LHB_CMX_RAMLAYOUT_CFG, swapAddr);
                // Set Shave Window
                swcSetShaveWindow(svuId, 0, MXI_CMX_BASE_ADR + (0x20000 * svuId) + 0x4000);
                swcSetShaveWindow(svuId, 1, MXI_CMX_BASE_ADR + (0x20000 * svuId));
                swcSetShaveWindow(svuId, 2, lastAddr);
                swcSetShaveWindow(svuId, 3, lastAddr);
                // Reset Shave
                swcResetShave(svuId);
                // Load encoder mbin
                swcLoadMbin((u8 *)&binary_enc_mbin_start, svuId);
                lastAddr = MXI_CMX_BASE_ADR + (0x20000 * svuId) + 0x4000;
            }
        } while (svuId > 0);

        lastAddr = swcLeonReadNoCacheU32(&p_enc->origFrame);
        swapAddr = swcLeonReadNoCacheU32(&p_enc->naluBuffer);

        // check for start conditions
        if ((lastAddr != 0) && (swapAddr != 0))
        {
            config = swcLeonReadNoCacheU32(&p_enc->cmdCfg);
            svuId = (config >> 16) & 0x07;
            swcStartShave(svuId, (u32)&SS_EC_Main);
            SleepMicro(100);
            // start all encoders
            for (svuId = 0; svuId < 8; svuId++)
            {
                if (((1 << svuId) & config) != 0)
                {
                    swcStartShave(svuId, (u32)&SS_ENC_Main);
                    SleepMicro(100);
                }
            }
        }

        retValue = H264_ENCODE_OK;
    }

    return retValue;
}

unsigned int H264_ENC_CODE_SECTION(H264EncodeDeinit) (void)
{
    unsigned int config, swapAddr, svuId;
    unsigned int retValue = H264_ENCODE_BUSY;

    swapAddr = swcLeonReadNoCacheU32(&p_enc->naluNext);
    if ( (p_enc != 0) && (swapAddr == 0) )
    {
        config = swcLeonReadNoCacheU32(&p_enc->cmdCfg);
        // shut down entropy
        svuId = (config >> 16) & 0x07;
        swapAddr = (1 << svuId);
        DrvIcbDisableIrq(IRQ_SVE_0 + svuId);
        swcResetShave(svuId);
        DrvCprSysDeviceAction(DISABLE_CLKS, ((u64)swapAddr) << 32);
        p_enc = (void *)0;
        // shut down each shave
        for (svuId = 0; svuId < 8; svuId++)
        {
            swapAddr = (1 << svuId);
            if ((swapAddr&config) != 0)
            {
                swcResetShave(svuId);
                DrvCprSysDeviceAction(DISABLE_CLKS, ((u64)swapAddr) << 32);
            }
        }
        retValue = H264_ENCODE_OK;
    }

    return retValue;
}
