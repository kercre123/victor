///
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved.
///            For License Warranty see: common/license.txt
///
/// @brief     Lcd Functions.
///
/// This is the implementation of Lcd library.
///

// 1: Includes
// ----------------------------------------------------------------------------
#include "LcdApi.h"
#include "DrvLcd.h"
#include "DrvLcdDefines.h"
#include "DrvSvu.h"
#include "DrvIcbDefines.h"
#include "DrvIcb.h"
#include <DrvCpr.h>

#ifdef LCD_BLEN8
#define LCD_BLEN_BURST  D_LCD_DMA_LAYER_AXI_BURST_8
#else
#define LCD_BLEN_BURST  D_LCD_DMA_LAYER_AXI_BURST_16
#endif

// 2:  Source Specific #defines and types  (typedef,enum,struct)
// ----------------------------------------------------------------------------
typedef struct
{
    unsigned int MXI_DISPX_BASE_ADR;
    unsigned int IRQ_LCD_X;
    unsigned int LCDX_INT_CLEAR_ADR;
} LCDRegisters;
// 3: Global Data (Only if absolutely necessary)
// ----------------------------------------------------------------------------
// 4: Static Local Data
// ----------------------------------------------------------------------------
//Conversion (yuv->rgb) matrix, see formula to understand
static const u32 csc_coef_lcd[N_COEFS] =
{
        1024, 0, 1436,
        1024, -352, -731,
        1024, 1814, 0,
        -179, 125, -226
};

static LCDHandle *LCDHndl[2] =
{ NULL, NULL };

static const LCDRegisters lcds[2] =
{
    {
        .MXI_DISPX_BASE_ADR = MXI_DISP1_BASE_ADR,
        .IRQ_LCD_X          = IRQ_LCD_1,
        .LCDX_INT_CLEAR_ADR = LCD1_INT_CLEAR_ADR
    },
    {
        .MXI_DISPX_BASE_ADR = MXI_DISP2_BASE_ADR,
        .IRQ_LCD_X          = IRQ_LCD_2,
        .LCDX_INT_CLEAR_ADR = LCD2_INT_CLEAR_ADR
    }
};

// 5: Static Function Prototypes
// ----------------------------------------------------------------------------
// 6: Functions Implementation
// ----------------------------------------------------------------------------

// Function to assign callbacks
void LCD_CODE_SECTION(LCDSetupCallbacks)(LCDHandle *hndl,
        allocateLcdFn * assignFrame, frameReadyLcdFn *frameReady,
        freqLcdFn *highres, freqLcdFn *lowres)
{
    hndl->cbGetFrame = assignFrame;
    hndl->cbFrameReady = frameReady;
    hndl->callBackFctHighRes = highres;
    hndl->callBackFctLowRes = lowres;
}

inline void LCD_CODE_SECTION(LCDSetLayerBuffersNormalRegisters)(LCDHandle *hndl,
        int layer)
{
    //Layer 0
    if (layer == VL1 && hndl->currentFrameVL1 != NULL)
    {
        if (hndl->currentFrameVL1->p1 != NULL)
            SET_REG_WORD(
                lcds[hndl->lcdNumber].MXI_DISPX_BASE_ADR + LCD_LAYER0_DMA_START_ADR_OFFSET,
                (unsigned int )hndl->currentFrameVL1->p1);
        if (hndl->currentFrameVL1->p2 != NULL)
            SET_REG_WORD(
                lcds[hndl->lcdNumber].MXI_DISPX_BASE_ADR + LCD_LAYER0_DMA_START_CB_ADR_OFFSET,
                (unsigned int )hndl->currentFrameVL1->p2);
        if (hndl->currentFrameVL1->p3 != NULL)
            SET_REG_WORD(
                lcds[hndl->lcdNumber].MXI_DISPX_BASE_ADR + LCD_LAYER0_DMA_START_CR_ADR_OFFSET,
                (unsigned int )hndl->currentFrameVL1->p3);
    }
    //Layer 1
    if (layer == VL2 && hndl->currentFrameVL2 != NULL)
    {
        if (hndl->currentFrameVL2->p1 != NULL)
            SET_REG_WORD(
                lcds[hndl->lcdNumber].MXI_DISPX_BASE_ADR + LCD_LAYER1_DMA_START_ADR_OFFSET,
                (unsigned int )hndl->currentFrameVL2->p1);
        if (hndl->currentFrameVL2->p2 != NULL)
            SET_REG_WORD(
                lcds[hndl->lcdNumber].MXI_DISPX_BASE_ADR + LCD_LAYER1_DMA_START_CB_ADR_OFFSET,
                (unsigned int )hndl->currentFrameVL2->p2);
        if (hndl->currentFrameVL2->p3 != NULL)
            SET_REG_WORD(
                lcds[hndl->lcdNumber].MXI_DISPX_BASE_ADR + LCD_LAYER1_DMA_START_CR_ADR_OFFSET,
                (unsigned int )hndl->currentFrameVL2->p3);
    }
    //Layer 2 GL1
    if (layer == GL1 && hndl->currentFrameGL1 != NULL)
        if (hndl->currentFrameGL1->p1 != NULL)
            SET_REG_WORD(
                lcds[hndl->lcdNumber].MXI_DISPX_BASE_ADR + LCD_LAYER2_DMA_START_ADR_OFFSET,
                (unsigned int )hndl->currentFrameGL1->p1);

    //Layer 3 GL2
    if (layer == GL2 && hndl->currentFrameGL2 != NULL)
        if (hndl->currentFrameGL2->p1 != NULL)
            SET_REG_WORD(
                lcds[hndl->lcdNumber].MXI_DISPX_BASE_ADR + LCD_LAYER3_DMA_START_ADR_OFFSET,
                (unsigned int )hndl->currentFrameGL2->p1);
}

static inline void LCD_CODE_SECTION(LCDSetLayerBuffersShadowRegisters)(
    LCDHandle *hndl, int layer)
{
    //Layer 0
    if (layer == VL1 && hndl->currentFrameVL1 != NULL)
    {
        if (hndl->currentFrameVL1->p1 != NULL)
            SET_REG_WORD(
                lcds[hndl->lcdNumber].MXI_DISPX_BASE_ADR + LCD_LAYER0_DMA_START_SHADOW_OFFSET,
                (unsigned int )hndl->currentFrameVL1->p1);
        if (hndl->currentFrameVL1->p2 != NULL)
            SET_REG_WORD(
                lcds[hndl->lcdNumber].MXI_DISPX_BASE_ADR + LCD_LAYER0_DMA_START_CB_SHADOW_OFFSET,
                (unsigned int )hndl->currentFrameVL1->p2);
        if (hndl->currentFrameVL1->p3 != NULL)
            SET_REG_WORD(
                lcds[hndl->lcdNumber].MXI_DISPX_BASE_ADR + LCD_LAYER0_DMA_START_CR_SHADOW_OFFSET,
                (unsigned int )hndl->currentFrameVL1->p3);
    }
    //Layer 1
    if (layer == VL2 && hndl->currentFrameVL2 != NULL)
    {
        if (hndl->currentFrameVL2->p1 != NULL)
            SET_REG_WORD(
                lcds[hndl->lcdNumber].MXI_DISPX_BASE_ADR + LCD_LAYER1_DMA_START_SHADOW_OFFSET,
                (unsigned int )hndl->currentFrameVL2->p1);
        if (hndl->currentFrameVL2->p2 != NULL)
            SET_REG_WORD(
                lcds[hndl->lcdNumber].MXI_DISPX_BASE_ADR + LCD_LAYER1_DMA_START_CB_SHADOW_OFFSET,
                (unsigned int )hndl->currentFrameVL2->p2);
        if (hndl->currentFrameVL2->p3 != NULL)
            SET_REG_WORD(
                lcds[hndl->lcdNumber].MXI_DISPX_BASE_ADR + LCD_LAYER1_DMA_START_CR_SHADOW_OFFSET,
                (unsigned int )hndl->currentFrameVL2->p3);
    }
    //Layer 2 GL1
    if (layer == GL1 && hndl->currentFrameGL1 != NULL)
        if (hndl->currentFrameGL1->p1 != NULL)
            SET_REG_WORD(
                lcds[hndl->lcdNumber].MXI_DISPX_BASE_ADR + LCD_LAYER2_DMA_START_SHADOW_OFFSET,
                (unsigned int )hndl->currentFrameGL1->p1);
    //Layer 3 GL2
    if (layer == GL2 && hndl->currentFrameGL2 != NULL)
        if (hndl->currentFrameGL2->p1 != NULL)
            SET_REG_WORD(
                lcds[hndl->lcdNumber].MXI_DISPX_BASE_ADR + LCD_LAYER3_DMA_START_SHADOW_OFFSET,
                (unsigned int )hndl->currentFrameGL2->p1);
}

static inline void LCD_CODE_SECTION(LCDSetLayerBuffersNormalAndShadowRegisters)(
    LCDHandle *hndl, int layer)
{
    LCDSetLayerBuffersNormalRegisters(hndl, layer);
    LCDSetLayerBuffersShadowRegisters(hndl, layer);
}

void LCD_CODE_SECTION(LCDConfigureLayers)(LCDHandle *hndl,
        const LCDDisplayCfg *lcdsp, u32 dmaUpdateType, u32 lcdControlEdited)
{
    unsigned int x = 0;
    unsigned int y = 0;
    unsigned int lutWidthCorrection = 1;

    unsigned int VL1Cfg = D_LCD_LAYER_FIFO_100;
    unsigned int VL2Cfg = D_LCD_LAYER_FIFO_100;

    unsigned int GL1Cfg = D_LCD_LAYER_ALPHA_EMBED | D_LCD_LAYER_8BPP
                          | D_LCD_LAYER_TRANSPARENT_EN | D_LCD_LAYER_FIFO_50;
    unsigned int GL2Cfg = D_LCD_LAYER_FIFO_50 | D_LCD_LAYER_8BPP
                          | D_LCD_LAYER_TRANSPARENT_EN;


    unsigned int LCtrl = 0/*D_LCD_DMA_LAYER_ENABLE*/| dmaUpdateType
                         | LCD_BLEN_BURST | D_LCD_DMA_LAYER_V_STRIDE_EN;

    if(hndl->frameSpec != NULL)
    {

        switch (hndl->frameSpec->type)
        {
        case YUV420p:

            VL1Cfg = VL1Cfg | D_LCD_LAYER_8BPP | D_LCD_LAYER_CRCB_ORDER
                     | D_LCD_LAYER_FORMAT_YCBCR420PLAN | D_LCD_LAYER_PLANAR_STORAGE
                     | D_LCD_LAYER_CSC_EN;
            LCtrl = LCtrl | D_LCD_DMA_LAYER_V_STRIDE_EN;
            break;
        case YUV422p:
            VL1Cfg = VL1Cfg | D_LCD_LAYER_8BPP | D_LCD_LAYER_CRCB_ORDER
                     | D_LCD_LAYER_FORMAT_YCBCR422PLAN | D_LCD_LAYER_PLANAR_STORAGE
                     | D_LCD_LAYER_CSC_EN;
            LCtrl = LCtrl | D_LCD_DMA_LAYER_V_STRIDE_EN;
            break;
        case YUV422i:
            VL1Cfg = VL1Cfg | D_LCD_LAYER_32BPP | D_LCD_LAYER_FORMAT_YCBCR422LIN
                     | D_LCD_LAYER_CSC_EN;
            break;
        case RGB888:
            VL1Cfg = VL1Cfg | D_LCD_LAYER_24BPP | D_LCD_LAYER_FORMAT_RGB888;
            LCtrl = LCtrl | D_LCD_CTRL_OUTPUT_RGB;
            break;
        case RAW16:
            VL1Cfg = VL1Cfg | D_LCD_LAYER_16BPP | D_LCD_LAYER_FORMAT_RGB888;
            LCtrl = LCtrl | D_LCD_CTRL_OUTPUT_RGB;
            break;
        default:
            break;
        }

        ////////////////////////////////////////////////////////////////////////////////
        ///////////  VL1
        ////////////////////////////////////////////////////////////////////////////////
        if (hndl->layerEnabled & VL2_ENABLED)
        {
            DrvLcdLayer0(lcds[hndl->lcdNumber].MXI_DISPX_BASE_ADR,
                         hndl->frameSpec->width / 2, hndl->frameSpec->height, 0, 0,
                         VL1Cfg);
        }
        else
        {
            if (hndl->lcdSpec->width != hndl->frameSpec->width)
            {
                x = (hndl->lcdSpec->width - hndl->frameSpec->width) / 2;
            }

            if (hndl->lcdSpec->height != hndl->frameSpec->height)
            {
                y = (hndl->lcdSpec->height - hndl->frameSpec->height) / 2;
            }
            DrvLcdLayer0(lcds[hndl->lcdNumber].MXI_DISPX_BASE_ADR,
                         hndl->frameSpec->width, hndl->frameSpec->height, x, y, VL1Cfg);
        }

        DrvLcdCsc(lcds[hndl->lcdNumber].MXI_DISPX_BASE_ADR, VL1,
                  (u32 *) csc_coef_lcd);

        //Different settings if multiple video layers enabled (screen split in half)

        if (hndl->layerEnabled & VL2_ENABLED)
        {
            //Layer 0
            if (hndl->frameSpec->type == YUV420p)
            {
                DrvLcdCbPlanar(lcds[hndl->lcdNumber].MXI_DISPX_BASE_ADR, VL1,
                               (unsigned int) hndl->currentFrameVL1->p2,
                               hndl->frameSpec->width / 4, hndl->frameSpec->stride / 2);
                DrvLcdCrPlanar(lcds[hndl->lcdNumber].MXI_DISPX_BASE_ADR, VL1,
                               (unsigned int) hndl->currentFrameVL1->p3,
                               hndl->frameSpec->width / 4, hndl->frameSpec->stride / 2);
            }
            if (hndl->frameSpec->type == YUV422p)
            {
                DrvLcdCbPlanar(lcds[hndl->lcdNumber].MXI_DISPX_BASE_ADR, VL1,
                               (unsigned int) hndl->currentFrameVL1->p2,
                               hndl->frameSpec->width / 2, hndl->frameSpec->stride);
                DrvLcdCrPlanar(lcds[hndl->lcdNumber].MXI_DISPX_BASE_ADR, VL1,
                               (unsigned int) hndl->currentFrameVL1->p3,
                               hndl->frameSpec->width / 2, hndl->frameSpec->stride);
            }
            DrvLcdDmaLayer0(lcds[hndl->lcdNumber].MXI_DISPX_BASE_ADR,
                            (unsigned int) hndl->currentFrameVL1->p1,
                            hndl->frameSpec->width / 2, hndl->frameSpec->height,
                            hndl->frameSpec->bytesPP, hndl->frameSpec->stride, LCtrl);
        }
        else
        {
            //Layer 0
            if (hndl->frameSpec->type == YUV420p)
            {
                DrvLcdCbPlanar(lcds[hndl->lcdNumber].MXI_DISPX_BASE_ADR, VL1,
                               (unsigned int) hndl->currentFrameVL1->p2,
                               hndl->frameSpec->width / 2, hndl->frameSpec->stride / 2);
                DrvLcdCrPlanar(lcds[hndl->lcdNumber].MXI_DISPX_BASE_ADR, VL1,
                               (unsigned int) hndl->currentFrameVL1->p3,
                               hndl->frameSpec->width / 2, hndl->frameSpec->stride / 2);
            }
            if (hndl->frameSpec->type == YUV422p)
            {
                DrvLcdCbPlanar(lcds[hndl->lcdNumber].MXI_DISPX_BASE_ADR, VL1,
                               (unsigned int) hndl->currentFrameVL1->p2,
                               hndl->frameSpec->width, hndl->frameSpec->stride);
                DrvLcdCrPlanar(lcds[hndl->lcdNumber].MXI_DISPX_BASE_ADR, VL1,
                               (unsigned int) hndl->currentFrameVL1->p3,
                               hndl->frameSpec->width, hndl->frameSpec->stride);
            }

            DrvLcdDmaLayer0(lcds[hndl->lcdNumber].MXI_DISPX_BASE_ADR,
                            (unsigned int) hndl->currentFrameVL1->p1,
                            hndl->frameSpec->width, hndl->frameSpec->height,
                            hndl->frameSpec->bytesPP, hndl->frameSpec->stride, LCtrl);
        }

        ////////////////////////////////////////////////////////////////////////////////
        ///////////  VL2
        ////////////////////////////////////////////////////////////////////////////////
        if (hndl->layerEnabled & VL2_ENABLED)
        {
            DrvLcdLayer1(lcds[hndl->lcdNumber].MXI_DISPX_BASE_ADR,
                         hndl->frameSpec->width / 2, hndl->frameSpec->height,
                         hndl->frameSpec->width / 2, 0, VL1Cfg);
            DrvLcdCsc(lcds[hndl->lcdNumber].MXI_DISPX_BASE_ADR, VL2,
                      (u32 *) csc_coef_lcd);

            if (hndl->frameSpec->type == YUV420p)
            {
                DrvLcdCbPlanar(lcds[hndl->lcdNumber].MXI_DISPX_BASE_ADR, VL2,
                               (unsigned int) hndl->currentFrameVL2->p2,
                               hndl->frameSpec->width / 4, hndl->frameSpec->stride / 2);
                DrvLcdCrPlanar(lcds[hndl->lcdNumber].MXI_DISPX_BASE_ADR, VL2,
                               (unsigned int) hndl->currentFrameVL2->p3,
                               hndl->frameSpec->width / 4, hndl->frameSpec->stride / 2);
            }
            if (hndl->frameSpec->type == YUV422p)
            {
                DrvLcdCbPlanar(lcds[hndl->lcdNumber].MXI_DISPX_BASE_ADR, VL2,
                               (unsigned int) hndl->currentFrameVL2->p2,
                               hndl->frameSpec->width / 2, hndl->frameSpec->stride);
                DrvLcdCrPlanar(lcds[hndl->lcdNumber].MXI_DISPX_BASE_ADR, VL2,
                               (unsigned int) hndl->currentFrameVL2->p3,
                               hndl->frameSpec->width / 2, hndl->frameSpec->stride);
            }
            DrvLcdDmaLayer1(lcds[hndl->lcdNumber].MXI_DISPX_BASE_ADR,
                            (unsigned int) hndl->currentFrameVL2->p1,
                            hndl->frameSpec->width / 2, hndl->frameSpec->height,
                            hndl->frameSpec->bytesPP, hndl->frameSpec->stride, LCtrl);
            lcdControlEdited |= D_LCD_CTRL_VL2_ENABLE | D_LCD_CTRL_ALPHA_BOTTOM_VL2
                                | D_LCD_CTRL_ALPHA_BLEND_VL1;
        }
    }
    else // if(hndl->frameSpec == NULL)
    {
        ////////////////////////////////////////////////////////////////////////////////
        ///////////  VL1
        ////////////////////////////////////////////////////////////////////////////////
        if (hndl->layerEnabled & VL1_ENABLED)
        {
            unsigned int VL1LCtrl = LCtrl;

            switch (hndl->VL1frameSpec->type)
            {
            case YUV420p:

                VL1Cfg = VL1Cfg | D_LCD_LAYER_8BPP | D_LCD_LAYER_CRCB_ORDER
                         | D_LCD_LAYER_FORMAT_YCBCR420PLAN
                         | D_LCD_LAYER_PLANAR_STORAGE | D_LCD_LAYER_CSC_EN;
                VL1LCtrl = LCtrl | D_LCD_DMA_LAYER_V_STRIDE_EN;
                break;
            case YUV422p:
                VL1Cfg = VL1Cfg | D_LCD_LAYER_8BPP | D_LCD_LAYER_CRCB_ORDER
                         | D_LCD_LAYER_FORMAT_YCBCR422PLAN
                         | D_LCD_LAYER_PLANAR_STORAGE;
                VL1LCtrl = LCtrl | D_LCD_DMA_LAYER_V_STRIDE_EN;
                break;
            case YUV422i:
                VL1Cfg = VL1Cfg | D_LCD_LAYER_32BPP
                         | D_LCD_LAYER_FORMAT_YCBCR422LIN | D_LCD_LAYER_CSC_EN;
                break;
            case RGB888:
                VL1Cfg = VL1Cfg | D_LCD_LAYER_24BPP | D_LCD_LAYER_FORMAT_RGB888;
                VL1LCtrl = LCtrl | D_LCD_CTRL_OUTPUT_RGB;
                break;
            case RAW16:
                VL1Cfg = VL1Cfg | D_LCD_LAYER_16BPP | D_LCD_LAYER_FORMAT_RGB888;
                VL1LCtrl = LCtrl | D_LCD_CTRL_OUTPUT_RGB;
                break;
            default:
                break;
            }

            DrvLcdLayer0(lcds[hndl->lcdNumber].MXI_DISPX_BASE_ADR,
                         hndl->VL1frameSpec->width,
                         hndl->VL1frameSpec->height,
                         hndl->VL1offset.x,
                         hndl->VL1offset.y,
                         VL1Cfg);
            if(VL1Cfg & D_LCD_LAYER_CSC_EN)
            {
                DrvLcdCsc(lcds[hndl->lcdNumber].MXI_DISPX_BASE_ADR, VL1,
                          (u32 *) csc_coef_lcd);
            }

            if (hndl->VL1frameSpec->type == YUV420p)
            {
                DrvLcdCbPlanar(lcds[hndl->lcdNumber].MXI_DISPX_BASE_ADR, VL1,
                               (unsigned int) hndl->currentFrameVL1->p2,
                               hndl->VL1frameSpec->width / 2,
                               hndl->VL1frameSpec->stride / 2);
                DrvLcdCrPlanar(lcds[hndl->lcdNumber].MXI_DISPX_BASE_ADR, VL1,
                               (unsigned int) hndl->currentFrameVL1->p3,
                               hndl->VL1frameSpec->width / 2,
                               hndl->VL1frameSpec->stride / 2);
            }
            if (hndl->VL1frameSpec->type == YUV422p)
            {
                DrvLcdCbPlanar(lcds[hndl->lcdNumber].MXI_DISPX_BASE_ADR, VL1,
                               (unsigned int) hndl->currentFrameVL1->p2,
                               hndl->VL1frameSpec->width,
                               hndl->VL1frameSpec->stride);
                DrvLcdCrPlanar(lcds[hndl->lcdNumber].MXI_DISPX_BASE_ADR, VL1,
                               (unsigned int) hndl->currentFrameVL1->p3,
                               hndl->VL1frameSpec->width,
                               hndl->VL1frameSpec->stride);
            }
            DrvLcdDmaLayer0(lcds[hndl->lcdNumber].MXI_DISPX_BASE_ADR,
                            (unsigned int) hndl->currentFrameVL1->p1,
                            hndl->VL1frameSpec->width,
                            hndl->VL1frameSpec->height,
                            hndl->VL1frameSpec->bytesPP,
                            hndl->VL1frameSpec->stride,
                            VL1LCtrl);

            lcdControlEdited |= D_LCD_CTRL_VL1_ENABLE
                                | D_LCD_CTRL_ALPHA_TOP_VL1 | D_LCD_CTRL_ALPHA_BLEND_VL1;
        }

        ////////////////////////////////////////////////////////////////////////////////
        ///////////  VL2
        ////////////////////////////////////////////////////////////////////////////////
        if (hndl->layerEnabled & VL2_ENABLED)
        {
            unsigned int VL2LCtrl = LCtrl;

            switch (hndl->VL2frameSpec->type)
            {
            case YUV420p:

                VL2Cfg = VL2Cfg | D_LCD_LAYER_8BPP | D_LCD_LAYER_CRCB_ORDER
                         | D_LCD_LAYER_FORMAT_YCBCR420PLAN
                         | D_LCD_LAYER_PLANAR_STORAGE ;
                VL2LCtrl = LCtrl | D_LCD_DMA_LAYER_V_STRIDE_EN;
                break;
            case YUV422p:
                VL2Cfg = VL2Cfg | D_LCD_LAYER_8BPP | D_LCD_LAYER_CRCB_ORDER
                         | D_LCD_LAYER_FORMAT_YCBCR422PLAN
                         | D_LCD_LAYER_PLANAR_STORAGE ;
                VL2LCtrl = LCtrl | D_LCD_DMA_LAYER_V_STRIDE_EN;
                break;
            case YUV422i:
                VL2Cfg = VL2Cfg | D_LCD_LAYER_32BPP
                         | D_LCD_LAYER_FORMAT_YCBCR422LIN ;
                break;
            case RGB888:
                VL2Cfg = VL2Cfg | D_LCD_LAYER_24BPP | D_LCD_LAYER_FORMAT_RGB888;
                VL2LCtrl = LCtrl | D_LCD_CTRL_OUTPUT_RGB;
                break;
            case RAW16:
                VL2Cfg = VL2Cfg | D_LCD_LAYER_16BPP | D_LCD_LAYER_FORMAT_RGB888;
                VL2LCtrl = LCtrl | D_LCD_CTRL_OUTPUT_RGB;
                break;
            default:
                break;
            }

            DrvLcdLayer1(lcds[hndl->lcdNumber].MXI_DISPX_BASE_ADR,
                         hndl->VL2frameSpec->width,
                         hndl->VL2frameSpec->height,
                         hndl->VL2offset.x,
                         hndl->VL2offset.y,
                         VL2Cfg);
            if(VL2Cfg & D_LCD_LAYER_CSC_EN)
            {
                DrvLcdCsc(lcds[hndl->lcdNumber].MXI_DISPX_BASE_ADR, VL2,
                          (u32 *) csc_coef_lcd);
            }

            if (hndl->VL2frameSpec->type == YUV420p)
            {
                DrvLcdCbPlanar(lcds[hndl->lcdNumber].MXI_DISPX_BASE_ADR, VL2,
                               (unsigned int) hndl->currentFrameVL2->p2,
                               hndl->VL2frameSpec->width / 2,
                               hndl->VL2frameSpec->stride / 2);
                DrvLcdCrPlanar(lcds[hndl->lcdNumber].MXI_DISPX_BASE_ADR, VL2,
                               (unsigned int) hndl->currentFrameVL2->p3,
                               hndl->VL2frameSpec->width / 2,
                               hndl->VL2frameSpec->stride / 2);
            }
            if (hndl->VL2frameSpec->type == YUV422p)
            {
                DrvLcdCbPlanar(lcds[hndl->lcdNumber].MXI_DISPX_BASE_ADR, VL2,
                               (unsigned int) hndl->currentFrameVL2->p2,
                               hndl->VL2frameSpec->width,
                               hndl->VL2frameSpec->stride);
                DrvLcdCrPlanar(lcds[hndl->lcdNumber].MXI_DISPX_BASE_ADR, VL2,
                               (unsigned int) hndl->currentFrameVL2->p3,
                               hndl->VL2frameSpec->width,
                               hndl->VL2frameSpec->stride);
            }
            DrvLcdDmaLayer1(lcds[hndl->lcdNumber].MXI_DISPX_BASE_ADR,
                            (unsigned int) hndl->currentFrameVL2->p1,
                            hndl->VL2frameSpec->width,
                            hndl->VL2frameSpec->height,
                            hndl->VL2frameSpec->bytesPP,
                            hndl->VL2frameSpec->stride,
                            VL2LCtrl);
            lcdControlEdited |= D_LCD_CTRL_VL2_ENABLE
                                | D_LCD_CTRL_ALPHA_BOTTOM_VL2 | D_LCD_CTRL_ALPHA_BLEND_VL1;
        }
    }

    ////////////////////////////////////////////////////////////////////////////////
    ///////////  GL1
    ////////////////////////////////////////////////////////////////////////////////
    if (hndl->layerEnabled & GL1_ENABLED)
    {
        lcdControlEdited |= D_LCD_CTRL_GL1_ENABLE | D_LCD_CTRL_ALPHA_TOP_GL1;

        switch (hndl->GL1FrameSpec->type)
        {
        case LUT2:
            GL1Cfg |= D_LCD_LAYER_FORMAT_CLUT | D_LCD_LAYER_LUT_2ENT;
            lutWidthCorrection = 8;
            break;
        case LUT4:
            GL1Cfg |= D_LCD_LAYER_FORMAT_CLUT | D_LCD_LAYER_LUT_4ENT;
            lutWidthCorrection = 4;
            break;
        case LUT16:
            GL1Cfg |= D_LCD_LAYER_FORMAT_CLUT | D_LCD_LAYER_LUT_16ENT;
            lutWidthCorrection = 2;
            break;
        case YUV422i:
            GL1Cfg = D_LCD_LAYER_FIFO_100 | D_LCD_LAYER_32BPP
                     | D_LCD_LAYER_FORMAT_YCBCR422LIN;
            break;
        default:
            break;
        }
        // Configure LCD Layer2 (Graphic Layer 1)
        DrvLcdLayer2(lcds[hndl->lcdNumber].MXI_DISPX_BASE_ADR,
                     hndl->GL1FrameSpec->width,                             // height
                     hndl->GL1FrameSpec->height,                             // width
                     hndl->GL1offset.x,                              // x start point
                     hndl->GL1offset.y,                              // y start point
                     GL1Cfg);

        switch (hndl->GL1FrameSpec->type)
        {
        case LUT2:
        case LUT4:
        case LUT16:
            // Configure LCD DMA for this layer
            DrvLcdDmaLayer2(lcds[hndl->lcdNumber].MXI_DISPX_BASE_ADR,

                            ((unsigned int) hndl->currentFrameGL1->p1 & 0x00FFFFFF)
                            | (0x10000000),
                            hndl->GL1FrameSpec->width / lutWidthCorrection,     // width
                            hndl->GL1FrameSpec->height,                        // height
                            hndl->GL1FrameSpec->bytesPP,                          // bbp
                            hndl->GL1FrameSpec->width / lutWidthCorrection,    // stride
                            LCtrl | D_LCD_DMA_LAYER_V_STRIDE_EN);
            break;
        case YUV422i:
            DrvLcdDmaLayer2(lcds[hndl->lcdNumber].MXI_DISPX_BASE_ADR,
                            (unsigned int) hndl->currentFrameGL1->p1,
                            hndl->GL1FrameSpec->width, hndl->GL1FrameSpec->height,
                            hndl->GL1FrameSpec->bytesPP, hndl->GL1FrameSpec->stride,
                            LCtrl);

            break;
        default:
            break;
        }
    }

    ////////////////////////////////////////////////////////////////////////////////
    ///////////  GL2
    ////////////////////////////////////////////////////////////////////////////////
    if (hndl->layerEnabled & GL2_ENABLED)
    {
        lcdControlEdited |= D_LCD_CTRL_GL2_ENABLE | D_LCD_CTRL_ALPHA_BLEND_GL2;

        switch (hndl->GL2FrameSpec->type)
        {
        case LUT2:
            GL2Cfg |= D_LCD_LAYER_FORMAT_CLUT | D_LCD_LAYER_LUT_2ENT;
            lutWidthCorrection = 8;
            break;
        case LUT4:
            GL2Cfg |= D_LCD_LAYER_FORMAT_CLUT | D_LCD_LAYER_LUT_4ENT;
            lutWidthCorrection = 4;
            break;
        case LUT16:
            GL2Cfg |= D_LCD_LAYER_FORMAT_CLUT | D_LCD_LAYER_LUT_16ENT;
            lutWidthCorrection = 2;
            break;
        case YUV422i:
            GL2Cfg = D_LCD_LAYER_FIFO_100 | D_LCD_LAYER_32BPP
                     | D_LCD_LAYER_FORMAT_YCBCR422LIN;
            break;
        default:
            break;
        }
        DrvLcdLayer3(lcds[hndl->lcdNumber].MXI_DISPX_BASE_ADR,
                     hndl->GL2FrameSpec->width,
                     hndl->GL2FrameSpec->height,
                     hndl->GL2offset.x,
                     hndl->GL2offset.y,
                     GL2Cfg);

        switch (hndl->GL2FrameSpec->type)
        {
        case LUT2:
        case LUT4:
        case LUT16:
            // Configure LCD DMA for this layer
            DrvLcdDmaLayer3(lcds[hndl->lcdNumber].MXI_DISPX_BASE_ADR,
                            (u32) hndl->currentFrameGL2->p1,
                            hndl->GL2FrameSpec->width / lutWidthCorrection,
                            hndl->GL2FrameSpec->height, hndl->GL2FrameSpec->bytesPP,
                            hndl->GL2FrameSpec->stride,
                            D_LCD_DMA_LAYER_ENABLE |
                            D_LCD_DMA_LAYER_CONT_UPDATE |
                            D_LCD_DMA_LAYER_AXI_BURST_16);
            break;
        case YUV422i:
            DrvLcdDmaLayer3(lcds[hndl->lcdNumber].MXI_DISPX_BASE_ADR,
                            (unsigned int) hndl->currentFrameGL2->p1,
                            hndl->GL2FrameSpec->width, hndl->GL2FrameSpec->height,
                            hndl->GL2FrameSpec->bytesPP, hndl->GL2FrameSpec->stride,
                            LCtrl);

            break;
        default:
            break;
        }
    }

    if(hndl->frameSpec != NULL)
    {
        DrvLcdCtrl(lcds[hndl->lcdNumber].MXI_DISPX_BASE_ADR, 0x00000,
                   lcdsp->outputFormat, lcdControlEdited | D_LCD_CTRL_VL1_ENABLE);
    }
    else
    {
        DrvLcdCtrl(lcds[hndl->lcdNumber].MXI_DISPX_BASE_ADR, 0x00000,
                   lcdsp->outputFormat, lcdControlEdited);
    }
    return;
}

static void LCD_CODE_SECTION(LCDEnableInterface) (LCDHandle *handle)
{
    u32 controlRegisterAddress = lcds[handle->lcdNumber].MXI_DISPX_BASE_ADR
                                 + LCD_CONTROL_OFFSET;
    u32 previousValue = GET_REG_WORD_VAL(controlRegisterAddress);
    SET_REG_WORD(controlRegisterAddress,
                 previousValue |= D_LCD_CTRL_TIM_GEN_ENABLE | D_LCD_CTRL_ENABLE);
}

void LCD_CODE_SECTION(LCDStop) (LCDHandle *hndl)
{
    hndl->LCDFrameCount = 0;

    if (hndl->lcdNumber == LCD1)
    {
        // Clock the Lcd interface
        DrvCprSysDeviceAction(ENABLE_CLKS, DEV_LCD1);
    }
    else
    {
        // Clock the Lcd interface
        DrvCprSysDeviceAction(ENABLE_CLKS, DEV_LCD2);
    }

    // Disable Lcd Interrupts
    DrvIcbDisableIrq(lcds[hndl->lcdNumber].IRQ_LCD_X);

    // Disable Layers DMA's
    SET_REG_WORD(lcds[hndl->lcdNumber].MXI_DISPX_BASE_ADR + LCD_CONTROL_OFFSET,
                 0);
    SET_REG_WORD(
        lcds[hndl->lcdNumber].MXI_DISPX_BASE_ADR + LCD_LAYER0_DMA_CFG_OFFSET,
        0);
    SET_REG_WORD(
        lcds[hndl->lcdNumber].MXI_DISPX_BASE_ADR + LCD_LAYER1_DMA_CFG_OFFSET,
        0);
    SET_REG_WORD(
        lcds[hndl->lcdNumber].MXI_DISPX_BASE_ADR + LCD_LAYER2_DMA_CFG_OFFSET,
        0);
    SET_REG_WORD(
        lcds[hndl->lcdNumber].MXI_DISPX_BASE_ADR + LCD_LAYER3_DMA_CFG_OFFSET,
        0);

    //Wait for DMA's to be inactive
    while (((GET_REG_WORD_VAL(lcds[hndl->lcdNumber].MXI_DISPX_BASE_ADR + LCD_LAYER0_DMA_CFG_OFFSET))
            & D_LCD_DMA_LAYER_STATUS))
        ;
    while (((GET_REG_WORD_VAL(lcds[hndl->lcdNumber].MXI_DISPX_BASE_ADR + LCD_LAYER1_DMA_CFG_OFFSET))
            & D_LCD_DMA_LAYER_STATUS))
        ;
    while (((GET_REG_WORD_VAL(lcds[hndl->lcdNumber].MXI_DISPX_BASE_ADR + LCD_LAYER2_DMA_CFG_OFFSET))
            & D_LCD_DMA_LAYER_STATUS))
        ;
    while (((GET_REG_WORD_VAL(lcds[hndl->lcdNumber].MXI_DISPX_BASE_ADR + LCD_LAYER3_DMA_CFG_OFFSET))
            & D_LCD_DMA_LAYER_STATUS))
        ;

    //TODO: Create a timer to avoid indefinite timeouts

    if (hndl->lcdNumber == LCD1)
    {
        // Reset the Lcd interface
        DrvCprSysDeviceAction(RESET_DEVICES, DEV_LCD1);
    }
    else
    {
        // Reset the Lcd interface
        DrvCprSysDeviceAction(RESET_DEVICES, DEV_LCD2);
    }
}

static void LCD_CODE_SECTION(LCDSaveCurrentFramesAsPreviousFrames) (LCDHandle *handle,
        frameBuffer **previousLCDBuffers)
{
    previousLCDBuffers[0] = handle->currentFrameVL1;
    previousLCDBuffers[1] = handle->currentFrameVL2;
    previousLCDBuffers[2] = handle->currentFrameGL1;
    previousLCDBuffers[3] = handle->currentFrameGL2;
}

static void LCD_CODE_SECTION(LCDGiveBackPreviousBuffers) (LCDHandle *handle,
                                       frameBuffer **previousLCDBuffers)
{
    if (handle->cbFrameReady != NULL)
        handle->cbFrameReady(previousLCDBuffers[0], previousLCDBuffers[1],
                             previousLCDBuffers[2], previousLCDBuffers[3]);
}

static void LCD_CODE_SECTION (LCDCollectNewLayerBuffersUsingGetFrameCallback) (LCDHandle *handle)
{
    if (handle->cbGetFrame != NULL)
    {
        handle->LCDFrameCount++;

        handle->currentFrameVL1 = handle->cbGetFrame(VL1);
        LCDSetLayerBuffersNormalAndShadowRegisters(handle, VL1);

        if (handle->layerEnabled & VL2_ENABLED)
        {
            handle->currentFrameVL2 = handle->cbGetFrame(VL2);
            LCDSetLayerBuffersNormalAndShadowRegisters(handle, VL2);
        }

        if (handle->layerEnabled & GL1_ENABLED)
        {
            handle->currentFrameGL1 = handle->cbGetFrame(GL1);
            LCDSetLayerBuffersNormalAndShadowRegisters(handle, GL1);
        }

        if (handle->layerEnabled & GL2_ENABLED)
        {
            handle->currentFrameGL2 = handle->cbGetFrame(GL2);
            LCDSetLayerBuffersNormalAndShadowRegisters(handle, GL2);
        }
    }
}

void LCDHandler(u32 source)
{
    u32 lcdNum = (source == IRQ_LCD_1) ? 0 : 1;
    frameBuffer *previousLCDBuffers[4] =
    { NULL, NULL, NULL, NULL };

    if (LCDHndl[lcdNum] != NULL)
    {
        LCDSaveCurrentFramesAsPreviousFrames(LCDHndl[lcdNum],
                                             previousLCDBuffers);
        LCDCollectNewLayerBuffersUsingGetFrameCallback(LCDHndl[lcdNum]);
    }

    // Clear the interrupt that this handler cares about
    // Clear the interrupt flag in the peripheral
    SET_REG_WORD(lcds[lcdNum].LCDX_INT_CLEAR_ADR, D_LCD_INT_LINE_CMP);
    // Clear the interrupt flag in the ICB
    DrvIcbIrqClear(lcds[lcdNum].IRQ_LCD_X);

    if (LCDHndl[lcdNum] != NULL)
    {
        LCDGiveBackPreviousBuffers(LCDHndl[lcdNum], previousLCDBuffers);
    }
}

void LCD_CODE_SECTION(LCDInitVL2Enable)(LCDHandle *hndl)
{
    hndl->layerEnabled |= VL2_ENABLED;
}

void LCD_CODE_SECTION(LCDInitVL2Disable)(LCDHandle *hndl)
{
    hndl->layerEnabled &= ~VL2_ENABLED;
}

void LCD_CODE_SECTION(LCDSetColorTable)(LCDHandle *hndl, int layer, unsigned int *colorTable,
                      int length)
{
    if (layer == 1)
    {
        DrvLcdTransparency(lcds[hndl->lcdNumber].MXI_DISPX_BASE_ADR, GL1,
                           colorTable[0], 255);
        DrvLcdLut(lcds[hndl->lcdNumber].MXI_DISPX_BASE_ADR, GL1,
                  (u32*) colorTable, length);
    }
    else if (layer == 2)
    {
        DrvLcdTransparency(lcds[hndl->lcdNumber].MXI_DISPX_BASE_ADR, GL2,
                           colorTable[0], 255);
        DrvLcdLut(lcds[hndl->lcdNumber].MXI_DISPX_BASE_ADR, GL2,
                  (u32*) colorTable, length);
    }
}

void LCD_CODE_SECTION(LCDInitGLEnable)(LCDHandle *hndl, int layer,
                                       const frameSpec *fr)
{
    if (layer == 1)
    {
        hndl->layerEnabled |= GL1_ENABLED;
        hndl->GL1FrameSpec = fr;
    }
    if (layer == 2)
    {
        hndl->layerEnabled |= GL2_ENABLED;
        hndl->GL2FrameSpec = fr;
    }
}

void LCD_CODE_SECTION(LCDInitGLDisable)(LCDHandle *hndl, int layer)
{
    if (layer == 1)
        hndl->layerEnabled &= ~GL1_ENABLED;
    if (layer == 2)
        hndl->layerEnabled &= ~GL2_ENABLED;
}

void LCD_CODE_SECTION(LCDInitLayer)(LCDHandle *hndl, int layer, frameSpec *fsp, LCDLayerOffset position)
{
    switch(layer)
    {
    case VL1:
        hndl->layerEnabled |= VL1_ENABLED;
        hndl->VL1frameSpec = fsp;
        hndl->VL1offset = position;
        break;
    case VL2:
        hndl->layerEnabled |= VL2_ENABLED;
        hndl->VL2frameSpec = fsp;
        hndl->VL2offset = position;
        break;
    case GL1:
        hndl->layerEnabled |= GL1_ENABLED;
        hndl->GL1FrameSpec = fsp;
        hndl->GL1offset = position;
        break;
    case GL2:
        hndl->layerEnabled |= GL2_ENABLED;
        hndl->GL2FrameSpec = fsp;
        hndl->GL2offset = position;
        break;
    }
}

void LCD_CODE_SECTION(LCDInit)(LCDHandle *hndl, const LCDDisplayCfg *lcdSpec,
                               const frameSpec *fsp, unsigned int lcdNum)
{
    hndl->frameSpec = fsp;
    hndl->lcdSpec = lcdSpec;
    hndl->lcdNumber = lcdNum;
    if(hndl->frameSpec != NULL)
        hndl->layerEnabled = VL1_ENABLED;
    else
        hndl->layerEnabled = 0;
    LCDHndl[lcdNum] = hndl;
}

inline void LCD_CODE_SECTION(LCDSetTimingFromDisplayCfg)(LCDHandle *hndl,
                                       const LCDDisplayCfg *lcdSpec)
{
    // LCD timing
    DrvLcdTiming(lcds[hndl->lcdNumber].MXI_DISPX_BASE_ADR, lcdSpec->width,
                 lcdSpec->hPulseWidth, lcdSpec->hBackP, lcdSpec->hFrontP,
                 lcdSpec->height, lcdSpec->vPulseWidth, lcdSpec->vBackP,
                 lcdSpec->vFrontP);
}

static void LCD_CODE_SECTION(LCDCallbackFrequencySetter)(LCDHandle *hndl)
{
    if ((hndl->lcdSpec->pixelClockkHz == 148500)
            && (hndl->callBackFctHighRes != NULL))
    {
        hndl->callBackFctHighRes();
    }
    else if (hndl->callBackFctLowRes != NULL)
    {
        hndl->callBackFctLowRes();
    }
}

void LCD_CODE_SECTION(LCDStart)(LCDHandle *hndl)
{
    if (hndl->lcdNumber == LCD1)
    {
        // Clock the Lcd interface
        DrvCprSysDeviceAction(ENABLE_CLKS, DEV_LCD1);
    }
    else
    {
        // Clock the Lcd interface
        DrvCprSysDeviceAction(ENABLE_CLKS, DEV_LCD2);
    }

    LCDCallbackFrequencySetter(hndl);

    LCDSetTimingFromDisplayCfg(hndl, hndl->lcdSpec);

    LCDCollectNewLayerBuffersUsingGetFrameCallback(hndl);

    LCDConfigureLayers(hndl, hndl->lcdSpec,
                       D_LCD_DMA_LAYER_CONT_UPDATE | D_LCD_DMA_LAYER_ENABLE,
                       hndl->lcdSpec->control & ~D_LCD_CTRL_DISPLAY_MODE_ONE_SHOT);

    // Disable ICB (Interrupt Control Block) while setting new interrupt
    DrvIcbDisableIrq(lcds[hndl->lcdNumber].IRQ_LCD_X);
    DrvIcbIrqClear(lcds[hndl->lcdNumber].IRQ_LCD_X);
    // Configure Leon to accept traps on any level
    swcLeonSetPIL(0);

    DrvIcbSetIrqHandler(lcds[hndl->lcdNumber].IRQ_LCD_X, LCDHandler);

    // Configure interrupts
    DrvIcbConfigureIrq(lcds[hndl->lcdNumber].IRQ_LCD_X,
                       LCDGENERIC_INTERRUPT_LEVEL, POS_EDGE_INT);

    // Trigger the interrupts
    DrvIcbEnableIrq(lcds[hndl->lcdNumber].IRQ_LCD_X);

    if(hndl->frameSpec != NULL)
    {
        SET_REG_WORD(
            lcds[hndl->lcdNumber].MXI_DISPX_BASE_ADR + LCD_LINE_COMPARE_OFFSET,
            hndl->frameSpec->height - 5);
    }
    else
    {
        unsigned int lowestLine = 0;

        if(hndl->layerEnabled & VL1_ENABLED )
            if(hndl->VL1offset.y + hndl->VL1frameSpec->height > lowestLine)
                lowestLine = hndl->VL1offset.y + hndl->VL1frameSpec->height;

        if(hndl->layerEnabled & VL2_ENABLED )
            if(hndl->VL2offset.y + hndl->VL2frameSpec->height > lowestLine)
                lowestLine = hndl->VL2offset.y + hndl->VL2frameSpec->height;

        if(hndl->layerEnabled & GL1_ENABLED )
            if(hndl->GL1offset.y + hndl->GL1FrameSpec->height > lowestLine)
                lowestLine = hndl->GL1offset.y + hndl->GL1FrameSpec->height;

        if(hndl->layerEnabled & GL2_ENABLED )
            if(hndl->GL2offset.y + hndl->GL2FrameSpec->height > lowestLine)
                lowestLine = hndl->GL2offset.y + hndl->GL2FrameSpec->height;

        SET_REG_WORD(
            lcds[hndl->lcdNumber].MXI_DISPX_BASE_ADR + LCD_LINE_COMPARE_OFFSET,
            lowestLine - 5);
    }
    SET_REG_WORD(
        lcds[hndl->lcdNumber].MXI_DISPX_BASE_ADR + LCD_INT_CLEAR_OFFSET,
        D_LCD_INT_LINE_CMP);
    SET_REG_WORD(
        lcds[hndl->lcdNumber].MXI_DISPX_BASE_ADR + LCD_INT_ENABLE_OFFSET,
        D_LCD_INT_LINE_CMP);


    // Can enable the interrupt now
    swcLeonEnableTraps();

    LCDEnableInterface(hndl);
}
void LCDHandlerOneShot(u32 source)
{
    u32 lcdNum = (source == IRQ_LCD_1) ? 0 : 1;

    // Clear the interrupt that this handler cares about
    // Clear the interrupt flag in the peripheral
    SET_REG_WORD(lcds[lcdNum].LCDX_INT_CLEAR_ADR, D_LCD_INT_EOF);
    // Clear the interrupt flag in the ICB
    DrvIcbIrqClear(lcds[lcdNum].IRQ_LCD_X);

    if (LCDHndl[lcdNum] != NULL)
    {
        LCDGiveBackPreviousBuffers(LCDHndl[lcdNum],
                                   LCDHndl[lcdNum]->queuedFrames[LCDHndl[lcdNum]->qfront]);
        LCDHndl[lcdNum]->qfront = (LCDHndl[lcdNum]->qfront + 1) % 3;
    }
}

void LCD_CODE_SECTION(LCDStartOneShot)(LCDHandle *hndl)
{
    LCDCallbackFrequencySetter(hndl);

    LCDSetTimingFromDisplayCfg(hndl, hndl->lcdSpec);

    LCDConfigureLayers(hndl, hndl->lcdSpec, 0 /* 0: no auto restart */,
                       hndl->lcdSpec->control | D_LCD_CTRL_DISPLAY_MODE_ONE_SHOT);

    hndl->firstFrameQueued = 0;

    if (hndl->cbFrameReady != NULL)
    {
        hndl->qfront = 0;
        hndl->qback = 2;
        // Disable ICB (Interrupt Control Block) while setting new interrupt
        DrvIcbDisableIrq(lcds[hndl->lcdNumber].IRQ_LCD_X);
        DrvIcbIrqClear(lcds[hndl->lcdNumber].IRQ_LCD_X);
        // Configure Leon to accept traps on any level
        swcLeonSetPIL(0);

        DrvIcbSetIrqHandler(lcds[hndl->lcdNumber].IRQ_LCD_X, LCDHandlerOneShot);

        // Configure interrupts
        DrvIcbConfigureIrq(lcds[hndl->lcdNumber].IRQ_LCD_X,
                           LCDGENERIC_INTERRUPT_LEVEL, POS_EDGE_INT);

        // Trigger the interrupts
        DrvIcbEnableIrq(lcds[hndl->lcdNumber].IRQ_LCD_X);

        SET_REG_WORD(
            lcds[hndl->lcdNumber].MXI_DISPX_BASE_ADR + LCD_INT_CLEAR_OFFSET,
            D_LCD_INT_EOF);
        SET_REG_WORD(
            lcds[hndl->lcdNumber].MXI_DISPX_BASE_ADR + LCD_INT_ENABLE_OFFSET,
            D_LCD_INT_EOF);

        // Can enable the interrupt now
        swcLeonEnableTraps();
    }
}

int LCDQueueFrame(LCDHandle *handle, frameBuffer *VL1Buffer,
                  frameBuffer *VL2Buffer, frameBuffer *GL1Buffer, frameBuffer *GL2Buffer)
{
    if (!LCDCanQueueFrame(handle))
        return -1;
    handle->qback = (handle->qback + 1) % 3;

    handle->queuedFrames[handle->qback][0] = handle->currentFrameVL1 =
                VL1Buffer;
    handle->queuedFrames[handle->qback][1] = handle->currentFrameVL2 =
                VL2Buffer;
    handle->queuedFrames[handle->qback][2] = handle->currentFrameGL1 =
                GL1Buffer;
    handle->queuedFrames[handle->qback][3] = handle->currentFrameGL2 =
                GL2Buffer;
    if (!handle->firstFrameQueued)
    {
        handle->firstFrameQueued = 1;
        handle->LCDFrameCount = 1;

        if (handle->layerEnabled & VL1_ENABLED)
        {
            LCDSetLayerBuffersNormalAndShadowRegisters(handle, VL1);
            SET_REG_WORD(
                lcds[handle->lcdNumber].MXI_DISPX_BASE_ADR + LCD_LAYER0_DMA_CFG_OFFSET,
                GET_REG_WORD_VAL(lcds[handle->lcdNumber].MXI_DISPX_BASE_ADR + LCD_LAYER0_DMA_CFG_OFFSET) | D_LCD_DMA_LAYER_ENABLE);
        }
        if (handle->layerEnabled & VL2_ENABLED)
        {
            LCDSetLayerBuffersNormalAndShadowRegisters(handle, VL2);
            SET_REG_WORD(
                lcds[handle->lcdNumber].MXI_DISPX_BASE_ADR + LCD_LAYER1_DMA_CFG_OFFSET,
                GET_REG_WORD_VAL(lcds[handle->lcdNumber].MXI_DISPX_BASE_ADR + LCD_LAYER1_DMA_CFG_OFFSET) | D_LCD_DMA_LAYER_ENABLE);
        }
        if (handle->layerEnabled & GL1_ENABLED)
        {
            LCDSetLayerBuffersNormalAndShadowRegisters(handle, GL1);
            SET_REG_WORD(
                lcds[handle->lcdNumber].MXI_DISPX_BASE_ADR + LCD_LAYER2_DMA_CFG_OFFSET,
                GET_REG_WORD_VAL(lcds[handle->lcdNumber].MXI_DISPX_BASE_ADR + LCD_LAYER2_DMA_CFG_OFFSET) | D_LCD_DMA_LAYER_ENABLE);
        }
        if (handle->layerEnabled & GL2_ENABLED)
        {
            LCDSetLayerBuffersNormalAndShadowRegisters(handle, GL2);
            SET_REG_WORD(
                lcds[handle->lcdNumber].MXI_DISPX_BASE_ADR + LCD_LAYER3_DMA_CFG_OFFSET,
                GET_REG_WORD_VAL(lcds[handle->lcdNumber].MXI_DISPX_BASE_ADR + LCD_LAYER3_DMA_CFG_OFFSET) | D_LCD_DMA_LAYER_ENABLE);
        }

        LCDEnableInterface(handle);
    }
    else
    {
        handle->LCDFrameCount++;

        if (handle->layerEnabled & VL1_ENABLED)
        {
            LCDSetLayerBuffersShadowRegisters(handle, VL1);
            SET_REG_WORD(
                lcds[handle->lcdNumber].MXI_DISPX_BASE_ADR + LCD_LAYER0_DMA_CFG_OFFSET,
                GET_REG_WORD_VAL(lcds[handle->lcdNumber].MXI_DISPX_BASE_ADR + LCD_LAYER0_DMA_CFG_OFFSET) | D_LCD_DMA_LAYER_AUTO_UPDATE);
        }
        if (handle->layerEnabled & VL2_ENABLED)
        {
            LCDSetLayerBuffersShadowRegisters(handle, VL2);
            SET_REG_WORD(
                lcds[handle->lcdNumber].MXI_DISPX_BASE_ADR + LCD_LAYER1_DMA_CFG_OFFSET,
                GET_REG_WORD_VAL(lcds[handle->lcdNumber].MXI_DISPX_BASE_ADR + LCD_LAYER1_DMA_CFG_OFFSET) | D_LCD_DMA_LAYER_AUTO_UPDATE);
        }
        if (handle->layerEnabled & GL1_ENABLED)
        {
            LCDSetLayerBuffersShadowRegisters(handle, GL1);
            SET_REG_WORD(
                lcds[handle->lcdNumber].MXI_DISPX_BASE_ADR + LCD_LAYER2_DMA_CFG_OFFSET,
                GET_REG_WORD_VAL(lcds[handle->lcdNumber].MXI_DISPX_BASE_ADR + LCD_LAYER2_DMA_CFG_OFFSET) | D_LCD_DMA_LAYER_AUTO_UPDATE);
        }
        if (handle->layerEnabled & GL2_ENABLED)
        {
            LCDSetLayerBuffersShadowRegisters(handle, GL2);
            SET_REG_WORD(
                lcds[handle->lcdNumber].MXI_DISPX_BASE_ADR + LCD_LAYER3_DMA_CFG_OFFSET,
                GET_REG_WORD_VAL(lcds[handle->lcdNumber].MXI_DISPX_BASE_ADR + LCD_LAYER3_DMA_CFG_OFFSET) | D_LCD_DMA_LAYER_AUTO_UPDATE);
        }
        SET_REG_WORD(
            lcds[handle->lcdNumber].MXI_DISPX_BASE_ADR + LCD_TIMING_GEN_TRIG_OFFSET,
            1);
    }
    return 0;
}

int LCD_CODE_SECTION(LCDCanQueueFrame)(LCDHandle *handle)
{
    int canQueueVL1 =
        ((GET_REG_WORD_VAL(lcds[handle->lcdNumber].MXI_DISPX_BASE_ADR + LCD_LAYER0_DMA_CFG_OFFSET) >> 2) & 3) == 0;
    int canQueueVL2 =
        ((GET_REG_WORD_VAL(lcds[handle->lcdNumber].MXI_DISPX_BASE_ADR + LCD_LAYER1_DMA_CFG_OFFSET) >> 2) & 3) == 0;
    int canQueueGL1 =
        ((GET_REG_WORD_VAL(lcds[handle->lcdNumber].MXI_DISPX_BASE_ADR + LCD_LAYER2_DMA_CFG_OFFSET) >> 2) & 3) == 0;
    int canQueueGL2 =
        ((GET_REG_WORD_VAL(lcds[handle->lcdNumber].MXI_DISPX_BASE_ADR + LCD_LAYER3_DMA_CFG_OFFSET) >> 2) & 3) == 0;
    if ((handle->layerEnabled & VL1_ENABLED) && !canQueueVL1)
        return 0;
    if ((handle->layerEnabled & VL2_ENABLED) && !canQueueVL2)
        return 0;
    if ((handle->layerEnabled & GL1_ENABLED) && !canQueueGL1)
        return 0;
    if ((handle->layerEnabled & GL2_ENABLED) && !canQueueGL2)
        return 0;
    return 1;
}
