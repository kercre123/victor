///
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved.
///            For License Warranty see: common/license.txt
///
/// @brief     Sensor Functions.
///
/// This is the implementation of a Sensor library.
///
///

// 1: Includes
// ----------------------------------------------------------------------------
// Drivers includes

#include <isaac_registers.h>
#include "DrvCif.h"
#include <DrvGpio.h>
#include "DrvIcbDefines.h"
#include "DrvSvu.h"
#include <DrvIcb.h>
#include <DrvTimer.h>

#include "DrvCpr.h"
#include "DrvI2cMaster.h"

//Project specific defines
#include "CIFGenericApi.h"
#include "CIFGenericApiDefines.h"
#include "CIFGenericPrivateDefines.h"
#include "assert.h"

// 2:  Source Specific #defines and types  (typedef,enum,struct)
// ----------------------------------------------------------------------------
typedef struct
{
    unsigned int CPR_CAMX_CLK_CTRL_ADR;
    unsigned int MXI_CAMX_BASE_ADR;
    unsigned int IRQ_CIF_X;
    unsigned int CIFX_INT_CLEAR_ADR;
    unsigned int CIFX_INT_ENABLE_ADR;
    unsigned int CIFX_INT_STATUS_ADR;
    unsigned int CIFX_LINE_COUNT_ADR;
} CamHwRegs;

// 3: Global Data (Only if absolutely necessary)
// ----------------------------------------------------------------------------
// 4: Static Local Data
// ----------------------------------------------------------------------------

const CamHwRegs cams[2] = {
{
    .MXI_CAMX_BASE_ADR         = MXI_CAM1_BASE_ADR,
    .IRQ_CIF_X                 = IRQ_CIF_1,
    .CIFX_INT_CLEAR_ADR        = CIF1_INT_CLEAR_ADR,
    .CIFX_INT_ENABLE_ADR       = CIF1_INT_ENABLE_ADR,
    .CIFX_INT_STATUS_ADR       = CIF1_INT_STATUS_ADR,
    .CIFX_LINE_COUNT_ADR       = CIF1_LINE_COUNT_ADR
},
{
    .MXI_CAMX_BASE_ADR         = MXI_CAM2_BASE_ADR,
    .IRQ_CIF_X                 = IRQ_CIF_2,
    .CIFX_INT_CLEAR_ADR        = CIF2_INT_CLEAR_ADR,
    .CIFX_INT_ENABLE_ADR       = CIF2_INT_ENABLE_ADR,
    .CIFX_INT_STATUS_ADR       = CIF2_INT_STATUS_ADR,
    .CIFX_LINE_COUNT_ADR       = CIF2_LINE_COUNT_ADR
}
};

static SensorHandle *CIFHndl[2]= {NULL, NULL};

static u8 camWriteProto[] = I2C_PROTO_WRITE_16BA;
static u8 camReadProto[] = I2C_PROTO_READ_16BA;

volatile unsigned int dma_underrun1, dma_underrun2, dma_overflow1, dma_overflow2;

// 5: Static Function Prototypes
// ----------------------------------------------------------------------------

// 6: Functions Implementation
// ----------------------------------------------------------------------------

// Configuration of the sensors (See OV5_720p.h content for the set of commands sent to the sensor)
void CIF_CODE_SECTION(I2CRegConfiguration) (SensorHandle* hndl, CamSpec* camSpec)
{
    unsigned int i;
    unsigned char nextByte[2];

    SleepTicks(camSpec->cfgDelay);

    if (camSpec->regValues == NULL)
    {
        return;
    }

    //maintain deprecated usecase with no regSize defined in camspec
    if (camSpec->regSize == 0)
        camSpec->regSize = 1;

    for (i = 0; i < camSpec->regNumber; i++)
    {
		switch(camSpec->regSize)
		{
		case 1:
			nextByte[0] = camSpec->regValues[i][1];
			break;
		case 2:
			nextByte[0] = camSpec->regValues[i][1] >> 8;
			nextByte[1] = camSpec->regValues[i][1] & 0xff;
			break;
		default:
			break;
		}

        (void)DrvI2cMTransaction(hndl->pI2cHandle, camSpec->sensorI2CAddress, camSpec->regValues[i][0] ,camWriteProto, nextByte , camSpec->regSize);
		if(i < 20) SleepMs(10); // do not change, it will randomly affect flip/fps/noise etc.	
    }

}

void CIF_CODE_SECTION(CifCfg) (SensorHandle* hndl, unsigned int cifBase, CamSpec *camSpec,  unsigned int width, unsigned int height)
{
    unsigned int outCfg;
	unsigned int outPrevCfg;
	unsigned int inputFormat;
	
    switch (hndl->frameSpec->type)
    {
    case YUV420p:
        outCfg = D_CIF_OUTF_FORMAT_420 | D_CIF_OUTF_CHROMA_SUB_CO_SITE_CENTER | D_CIF_OUTF_STORAGE_PLANAR | D_CIF_OUTF_Y_AFTER_CBCR;
        outPrevCfg = 0;
        inputFormat=D_CIF_INFORM_FORMAT_YUV422 | D_CIF_INFORM_DAT_SIZE_8;
		break;
    case YUV422p:
        outCfg = D_CIF_OUTF_FORMAT_422 | D_CIF_OUTF_CHROMA_SUB_CO_SITE_CENTER | D_CIF_OUTF_STORAGE_PLANAR;
        outPrevCfg = 0;
        inputFormat=D_CIF_INFORM_FORMAT_YUV422 | D_CIF_INFORM_DAT_SIZE_8;
		break;
    case YUV422i:
        outCfg =  D_CIF_OUTF_FORMAT_422;
        outPrevCfg = 0;
        inputFormat=D_CIF_INFORM_FORMAT_YUV422 | D_CIF_INFORM_DAT_SIZE_8;
		break;
    case RAW16:
        outCfg = D_CIF_OUTF_BAYER;
        outPrevCfg = D_CIF_PREV_OUTF_RGB_BY | D_CIF_PREV_SEL_COL_MAP_LUT;
        inputFormat=D_CIF_INFORM_FORMAT_RGB_BAYER | D_CIF_INFORM_DAT_SIZE_16;
        break;
    default:
        outCfg =  0;
		outPrevCfg = 0;
		inputFormat=D_CIF_INFORM_FORMAT_YUV422 | D_CIF_INFORM_DAT_SIZE_8;
        break;
    }
    //CIF Timing config
    DrvCifTimingCfg (cifBase, camSpec->cifTiming.width, camSpec->cifTiming.height,
            camSpec->cifTiming.hBackP, camSpec->cifTiming.hFrontP, camSpec->cifTiming.vBackP, camSpec->cifTiming.vFrontP);

    DrvCifInOutCfg (cifBase,
    		inputFormat,
            0x0000,
            outCfg,
            outPrevCfg );

    switch (hndl->frameSpec->type)
    {
    case RAW16:
    	DrvCifDma0CfgPP(cifBase,
                (unsigned int) hndl->currentFrame->p1,
                (unsigned int) hndl->currentFrame->p1,
                hndl->frameSpec->width,
                hndl->frameSpec->height,
                hndl->frameSpec->bytesPP,
                D_CIF_DMA_AUTO_RESTART_PING_PONG | D_CIF_DMA_ENABLE | D_CIF_DMA_AXI_BURST_16,
                0);
        break;
    case YUV420p:
        DrvCifDma0CfgPP(cifBase,
                (unsigned int) hndl->currentFrame->p1,
                (unsigned int) hndl->currentFrame->p1,
                hndl->frameSpec->width,
                hndl->frameSpec->height,
                hndl->frameSpec->bytesPP,
                D_CIF_DMA_AUTO_RESTART_PING_PONG | D_CIF_DMA_ENABLE | D_CIF_DMA_AXI_BURST_8,
                0);

        DrvCifDma1CfgPP(cifBase,
                (unsigned int) hndl->currentFrame->p2,
                (unsigned int) hndl->currentFrame->p2,
                hndl->frameSpec->width / 2,
                hndl->frameSpec->height / 2,
                hndl->frameSpec->bytesPP,
                D_CIF_DMA_AUTO_RESTART_PING_PONG | D_CIF_DMA_ENABLE | D_CIF_DMA_AXI_BURST_8,
                0);

        DrvCifDma2CfgPP(cifBase,
                (unsigned int) hndl->currentFrame->p3,
                (unsigned int) hndl->currentFrame->p3,
                hndl->frameSpec->width / 2,
                hndl->frameSpec->height / 2,
                hndl->frameSpec->bytesPP,
                D_CIF_DMA_AUTO_RESTART_PING_PONG | D_CIF_DMA_ENABLE | D_CIF_DMA_AXI_BURST_8,
                0);
        break;
    case YUV422p:
        DrvCifDma0CfgPP(cifBase,
                (unsigned int) hndl->currentFrame->p1,
                (unsigned int) hndl->currentFrame->p1,
                hndl->frameSpec->width,
                hndl->frameSpec->height,
                hndl->frameSpec->bytesPP,
                D_CIF_DMA_AUTO_RESTART_PING_PONG | D_CIF_DMA_ENABLE | D_CIF_DMA_AXI_BURST_8,
                0);

        DrvCifDma1CfgPP(cifBase,
                (unsigned int) hndl->currentFrame->p2,
                (unsigned int) hndl->currentFrame->p2,
                hndl->frameSpec->width / 2,
                hndl->frameSpec->height,
                hndl->frameSpec->bytesPP,
                D_CIF_DMA_AUTO_RESTART_PING_PONG | D_CIF_DMA_ENABLE | D_CIF_DMA_AXI_BURST_8,
                0);

        DrvCifDma2CfgPP(cifBase,
                (unsigned int) hndl->currentFrame->p3,
                (unsigned int) hndl->currentFrame->p3,
                hndl->frameSpec->width / 2,
                hndl->frameSpec->height,
                hndl->frameSpec->bytesPP,
                D_CIF_DMA_AUTO_RESTART_PING_PONG | D_CIF_DMA_ENABLE | D_CIF_DMA_AXI_BURST_8,
                0);
        break;
    case YUV422i:
        if (hndl->cbGetBlock == NULL)
        {
            DrvCifDma0CfgPP(cifBase,
                    (unsigned int) hndl->currentFrame->p1,
                    (unsigned int) hndl->currentFrame->p1,
                    hndl->frameSpec->width,
                    hndl->frameSpec->height,
                    hndl->frameSpec->bytesPP,
                    D_CIF_DMA_AUTO_RESTART_PING_PONG | D_CIF_DMA_ENABLE | D_CIF_DMA_AXI_BURST_16,
                    0);
        }
        else
        {
            DrvCifDma0CfgPP(cifBase,
                    (unsigned int) hndl->currentFrame->p1,
                    (unsigned int) hndl->currentFrame->p1,
                    hndl->frameSpec->width,
                    1,
                    hndl->frameSpec->bytesPP,
                    D_CIF_DMA_AUTO_RESTART_PING_PONG | D_CIF_DMA_ENABLE | D_CIF_DMA_AXI_BURST_16,
                    0);
        }
        break;
    default :
        break;
    }

    if(hndl->camSpec->cifTiming.generateSync == GENERATE_SYNCS)
    {
    	DrvGpioMode(116, D_GPIO_DIR_OUT); // CAM1_VSYNC
    	DrvGpioMode(117, D_GPIO_DIR_OUT); // CAM1_HSYNC

    	SET_REG_WORD(cams[hndl->sensorNumber].MXI_CAMX_BASE_ADR+CIF_VSYNC_WIDTH_OFFSET, 16); // VSW
    	SET_REG_WORD(cams[hndl->sensorNumber].MXI_CAMX_BASE_ADR+CIF_HSYNC_WIDTH_OFFSET, 16); // HSW

    	SET_REG_WORD(cams[hndl->sensorNumber].MXI_CAMX_BASE_ADR+CIF_INPUT_IF_CFG_OFFSET, (D_CIF_IN_SINC_DRIVED_BY_SABRE));

    	DrvCifCtrlCfg (cifBase, width, height, D_CIF_CTRL_ENABLE | D_CIF_CTRL_STATISTICS_FULL | D_CIF_CTRL_TIM_GEN_EN);
    }
    else
    	DrvCifCtrlCfg (cifBase, width, height, D_CIF_CTRL_ENABLE | D_CIF_CTRL_STATISTICS_FULL);
}

//Configure Sensor Sensor
void CIF_CODE_SECTION(CifI2CConfiguration) (SensorHandle* hndl, int resetpin)
{
    tyCIFDevice currentCamera = CAMERA_1;

    // TODO: This If should be removed, and the sensorNumber changed to a tyCifType 
    if (hndl->sensorNumber == 0)
        currentCamera = CAMERA_1;
    else if (hndl->sensorNumber == 1)
        currentCamera = CAMERA_2;
    else
    {
        assert(FALSE);
    }

    DrvCifSetMclkFrequency(currentCamera , (hndl->camSpec->idealRefFreq * 1000));
    if(resetpin != NULL)
	{
		SleepMs(10);
    	DrvCifReset(currentCamera);
    	DrvGpioSetPinLo(resetpin);

    	SleepMs(10);
    	DrvGpioSetPinHi(resetpin);

    	SleepMs(10); // Need 15K cycles of 27Mhz (sensor refclk)
	}
    I2CRegConfiguration(hndl, hndl->camSpec);

    DrvCifReset(currentCamera);

    return;
}

// This function should be called to set the address for the next sensor sensor output (set start address for the new DMA transfer)
inline void CIF_CODE_SECTION(SensorSpecificSetBuffers) (SensorHandle *hndl, int sensorBaseAdr, unsigned int c)
{
    //Set buffers for next transfer
    SET_REG_WORD(sensorBaseAdr + CIF_DMA0_START_ADR_OFFSET, (unsigned int)c);
    SET_REG_WORD(sensorBaseAdr + CIF_DMA0_START_SHADOW_OFFSET, (unsigned int)c);
}

// This function should be called to set the address for the next sensor sensor output (set start address for the new DMA transfer)
inline void CIF_CODE_SECTION(SensorSetBuffers) (SensorHandle *hndl, int sensorBaseAdr)
{
    if ((hndl->currentFrame != NULL) && (hndl->currentFrame->p1 != NULL))
    {
        SET_REG_WORD(sensorBaseAdr + CIF_DMA0_START_ADR_OFFSET,    hndl->currentFrame->p1);
        SET_REG_WORD(sensorBaseAdr + CIF_DMA0_START_SHADOW_OFFSET, hndl->currentFrame->p1);
    }
    if ((hndl->currentFrame != NULL) && (hndl->currentFrame->p2 != NULL))
    {
        SET_REG_WORD(sensorBaseAdr + CIF_DMA1_START_ADR_OFFSET,    hndl->currentFrame->p2);
        SET_REG_WORD(sensorBaseAdr + CIF_DMA1_START_SHADOW_OFFSET, hndl->currentFrame->p2);
    }
    if ((hndl->currentFrame != NULL) && (hndl->currentFrame->p3 != NULL))
    {
        SET_REG_WORD(sensorBaseAdr + CIF_DMA2_START_ADR_OFFSET,    hndl->currentFrame->p3);
        SET_REG_WORD(sensorBaseAdr + CIF_DMA2_START_SHADOW_OFFSET, hndl->currentFrame->p3);
    }
}

// Sensor interrupt handler
void CIFHandler(u32 source)
{
    unsigned int CIFLineCnt;
    unsigned int CIFStatus;
    frameBuffer* previousCIFframe = NULL;

    u32 camNum = (source == IRQ_CIF_1) ? 0 : 1;

    CIFStatus = GET_REG_WORD_VAL(cams[camNum].CIFX_INT_STATUS_ADR);
    CIFLineCnt = GET_REG_WORD_VAL(cams[camNum].CIFX_LINE_COUNT_ADR);

	//DMA underrun on CIF1?
	if((CIFStatus & D_CIF_INT_DMA0_UNDER) && (camNum ==0))
		dma_underrun1++;
	if((CIFStatus & D_CIF_INT_DMA0_OVER) && (camNum ==0))
		dma_overflow1++;
		
	//DMA underrun on CIF2?
	if((CIFStatus & D_CIF_INT_DMA0_UNDER) && (camNum ==1))
		dma_underrun2++;
	if((CIFStatus & D_CIF_INT_DMA0_OVER) && (camNum ==1))
		dma_overflow2++;
    
    if (CIFHndl[camNum]->cbGetBlock != NULL)
    {
        if (CIFStatus & D_CIF_INT_DMA0_DONE)
        {
            //Clear that interrupt
            SET_REG_WORD(cams[camNum].CIFX_INT_CLEAR_ADR, D_CIF_INT_DMA0_DONE);

            unsigned char * c = CIFHndl[camNum]->cbGetBlock(CIFLineCnt);

            SensorSpecificSetBuffers(CIFHndl[camNum], cams[camNum].MXI_CAMX_BASE_ADR, (unsigned int)c);

            //And also return if EOF is not also enabled
            if (CIFStatus & D_CIF_INT_EOF)
            {
                //end of frame
            }
            else
            {
                //more blocks are available
                if (CIFHndl[camNum]->cbBlockReady != NULL)
                    CIFHndl[camNum]->cbBlockReady(CIFLineCnt);
                return;
            }
        }
    }
    else
    {
        if ((CIFHndl[camNum] != NULL)             &&
                (CIFHndl[camNum]->cbGetFrame != NULL))
        {
            previousCIFframe = CIFHndl[camNum]->currentFrame;
            CIFHndl[camNum]->currentFrame = CIFHndl[camNum]->cbGetFrame();
            SensorSetBuffers(CIFHndl[camNum], cams[camNum].MXI_CAMX_BASE_ADR);
        }

    }

    if ((CIFHndl[camNum]->cbEOF != NULL) && (CIFStatus & D_CIF_INT_EOF))
    {
        CIFHndl[camNum]->cbEOF();
    }

    // clear all pending interrupts for CIF
    SET_REG_WORD(cams[camNum].CIFX_INT_CLEAR_ADR, 0xFFFFFFFF);
    // Clear interrupt
    DrvIcbIrqClear(cams[camNum].IRQ_CIF_X);

    if ((CIFHndl[camNum] != NULL)              &&
            (CIFHndl[camNum]->cbFrameReady != NULL))
    {
        CIFHndl[camNum]->cbFrameReady(previousCIFframe);
    }

}

// Function to get current sensor frame specification
const frameSpec* CIF_CODE_SECTION(CifGetFrameSpec)(SensorHandle* hndl)
{
    return (const frameSpec*) hndl->frameSpec;
}


// Function to assign callbacks
void CIF_CODE_SECTION(CifSetupCallbacks)(SensorHandle* hndl,
        allocateFn*  getFrame, frameReadyFn* frameReady, allocateBlockFn*  getBlock, blockReadyFn* blockReady)
{
    hndl->cbGetFrame   = getFrame;
    hndl->cbFrameReady = frameReady;
    hndl->cbGetBlock    = getBlock;
    hndl->cbBlockReady  = blockReady;
}

void CifEnableEOFCallback(SensorHandle* hndl, EOFFn* callbackEOF)
{
    hndl->cbEOF  = callbackEOF;
}


int CIF_CODE_SECTION(CifInit) (SensorHandle *hndl, CamSpec *camSpec, frameSpec *fpSp, I2CM_Device *pI2cHandle, unsigned int sensorNumber)
{
    // Initialize the handle
    hndl->sensorNumber = sensorNumber;
    hndl->frameSpec = fpSp;
    hndl->pI2cHandle = pI2cHandle;
    hndl->currentFrame = NULL;
    hndl->camSpec = camSpec;
    hndl->cbEOF = NULL;

    CIFHndl[hndl->sensorNumber] = hndl;
    // TODO: This If should be removed, and the sensorNumber changed to a tyCifType
    if (hndl->sensorNumber == 0)
        DrvCifInit(CAMERA_1);
    if (hndl->sensorNumber == 1)
        DrvCifInit(CAMERA_2);

    //In the future, return error codes if we find any use for them
    return 0;
}


frameBuffer tempframe;

int CIF_CODE_SECTION(CifStart) (SensorHandle *hndl, int resetpin)
{
    unsigned char * c;

    if (hndl == NULL)
        return -1;

    if (hndl->pI2cHandle != NULL)
    {
        CifI2CConfiguration(hndl,resetpin);
    }

    if ((hndl->cbGetBlock == NULL) && (hndl->cbGetFrame != NULL))
    {
        hndl->currentFrame = hndl->cbGetFrame();
    }
    else
    {
        if (hndl->cbGetBlock != NULL)
        {
            c = hndl->cbGetBlock(0);
			tempframe.p1 = c;
            hndl->currentFrame = &tempframe;
        }
        else
            return -1;
    }
    // Disable ICB (Interrupt Control Block) while setting new interrupt
    DrvIcbDisableIrq(cams[hndl->sensorNumber].IRQ_CIF_X);
    DrvIcbIrqClear(cams[hndl->sensorNumber].IRQ_CIF_X);

    // Configure Leon to accept traps on any level
    swcLeonSetPIL(0);

    DrvIcbSetIrqHandler(cams[hndl->sensorNumber].IRQ_CIF_X, CIFHandler);

    // Configure interrupts
    DrvIcbConfigureIrq(cams[hndl->sensorNumber].IRQ_CIF_X, CIFGENERIC_INTERRUPT_LEVEL, POS_EDGE_INT);
    // Trigger the interrupts
    DrvIcbEnableIrq(cams[hndl->sensorNumber].IRQ_CIF_X );

    if (hndl->cbGetBlock != NULL)
    {
        SET_REG_WORD(cams[hndl->sensorNumber].CIFX_INT_ENABLE_ADR , D_CIF_INT_DMA0_DONE | D_CIF_INT_EOF);
        SET_REG_WORD(cams[hndl->sensorNumber].CIFX_INT_CLEAR_ADR , 0xFFFFFFFF);
    }
    else
        if (hndl->cbEOF != NULL)
        {
        	SET_REG_WORD(cams[hndl->sensorNumber].CIFX_INT_ENABLE_ADR , D_CIF_INT_DMA0_DONE | D_CIF_INT_EOF);
        }
    	else
    	{
    	    SET_REG_WORD(cams[hndl->sensorNumber].CIFX_INT_ENABLE_ADR , D_CIF_INT_DMA0_DONE);
    	}

    // Can enable the interrupt now
    swcLeonEnableTraps();

    CifCfg(hndl, cams[hndl->sensorNumber].MXI_CAMX_BASE_ADR, hndl->camSpec, hndl->frameSpec->width, hndl->frameSpec->height);

    return 1;
}

void CIF_CODE_SECTION(CifStop) (SensorHandle *hndl)
{

    // Prepare for a safe Cif reset 3 steps needed :
    // 1. disable input interface so no new data arrives
    // 2. disable output dma controller logic
    // 3. wait for any outstanding bus burst transactions to complete

/*    SET_REG_WORD(cams[hndl->sensorNumber].MXI_CAMX_BASE_ADR + CIF_CONTROL_OFFSET,  0);
    SET_REG_WORD(cams[hndl->sensorNumber].MXI_CAMX_BASE_ADR + CIF_DMA0_CFG_OFFSET, 0);
    SET_REG_WORD(cams[hndl->sensorNumber].MXI_CAMX_BASE_ADR + CIF_DMA1_CFG_OFFSET, 0);
    SET_REG_WORD(cams[hndl->sensorNumber].MXI_CAMX_BASE_ADR + CIF_DMA2_CFG_OFFSET, 0);
    SET_REG_WORD(cams[hndl->sensorNumber].MXI_CAMX_BASE_ADR + CIF_DMA3_CFG_OFFSET, 0);*/

    SET_REG_WORD(cams[hndl->sensorNumber].MXI_CAMX_BASE_ADR + CIF_DMA0_CFG_OFFSET, (GET_REG_WORD_VAL(cams[hndl->sensorNumber].MXI_CAMX_BASE_ADR + CIF_DMA0_CFG_OFFSET) &( ~ (1 << 2)) & ( ~ (1 << 3))));
    SET_REG_WORD(cams[hndl->sensorNumber].MXI_CAMX_BASE_ADR + CIF_DMA1_CFG_OFFSET, (GET_REG_WORD_VAL(cams[hndl->sensorNumber].MXI_CAMX_BASE_ADR + CIF_DMA0_CFG_OFFSET) &( ~ (1 << 2)) & ( ~ (1 << 3))));
    SET_REG_WORD(cams[hndl->sensorNumber].MXI_CAMX_BASE_ADR + CIF_DMA2_CFG_OFFSET, (GET_REG_WORD_VAL(cams[hndl->sensorNumber].MXI_CAMX_BASE_ADR + CIF_DMA0_CFG_OFFSET) &( ~ (1 << 2)) & ( ~ (1 << 3))));
    SET_REG_WORD(cams[hndl->sensorNumber].MXI_CAMX_BASE_ADR + CIF_DMA3_CFG_OFFSET, (GET_REG_WORD_VAL(cams[hndl->sensorNumber].MXI_CAMX_BASE_ADR + CIF_DMA0_CFG_OFFSET) &( ~ (1 << 2)) & ( ~ (1 << 3))));


    while(((GET_REG_WORD_VAL(cams[hndl->sensorNumber].MXI_CAMX_BASE_ADR + CIF_DMA0_CFG_OFFSET)) & D_CIF_DMA_ACTIVITY_MASK));
    while(((GET_REG_WORD_VAL(cams[hndl->sensorNumber].MXI_CAMX_BASE_ADR + CIF_DMA1_CFG_OFFSET)) & D_CIF_DMA_ACTIVITY_MASK));
    while(((GET_REG_WORD_VAL(cams[hndl->sensorNumber].MXI_CAMX_BASE_ADR + CIF_DMA2_CFG_OFFSET)) & D_CIF_DMA_ACTIVITY_MASK));
    while(((GET_REG_WORD_VAL(cams[hndl->sensorNumber].MXI_CAMX_BASE_ADR + CIF_DMA3_CFG_OFFSET)) & D_CIF_DMA_ACTIVITY_MASK));

    SET_REG_WORD(cams[hndl->sensorNumber].MXI_CAMX_BASE_ADR + CIF_CONTROL_OFFSET,  0);

    //TODO: Create a timer to avoid indefinite timeouts

    // Reset Cif
    // TODO: This If should be removed, and the sensorNumber changed to a tyCifType
    if (hndl->sensorNumber == 0)
        DrvCifReset(CAMERA_1);
    if (hndl->sensorNumber == 1)
        DrvCifReset(CAMERA_2);

    hndl->CIFFrameCount = 0;
}

unsigned int CIF_CODE_SECTION(CifGetFrameCounter) (SensorHandle *hndl)
{
    return hndl->CIFFrameCount;
}

void CIF_CODE_SECTION(camSim) (SensorHandle* hndl, u32 HSW, u32 VSW)
{
	DrvGpioMode(116, D_GPIO_DIR_OUT); // CAM1_VSYNC
	DrvGpioMode(117, D_GPIO_DIR_OUT); // CAM1_HSYNC

	SET_REG_WORD(cams[hndl->sensorNumber].MXI_CAMX_BASE_ADR+CIF_VSYNC_WIDTH_OFFSET, VSW); // VSW
	SET_REG_WORD(cams[hndl->sensorNumber].MXI_CAMX_BASE_ADR+CIF_HSYNC_WIDTH_OFFSET, HSW); // HSW

	SET_REG_WORD(cams[hndl->sensorNumber].MXI_CAMX_BASE_ADR+CIF_INPUT_IF_CFG_OFFSET, ( /*D_CIF_IN_PIX_CLK_CPR |*/ D_CIF_IN_HSINK_ON_BLANK_YES | D_CIF_IN_SINC_DRIVED_BY_SABRE));

	DrvCifCtrlCfg(cams[hndl->sensorNumber].MXI_CAMX_BASE_ADR,
				hndl->frameSpec->width,
				hndl->frameSpec->height,
		        D_CIF_CTRL_ENABLE | D_CIF_CTRL_TIM_GEN_EN
		        );
}
