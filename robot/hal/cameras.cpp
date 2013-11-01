#include "cameras.h"

// Unchanged interrupt level from Movidius
#define CIF_INTERRUPT_LEVEL 3

typedef struct
{
  //unsigned int CPR_CAMX_CLK_CTRL_ADR;
  unsigned int MXI_CAMX_BASE_ADR;
  unsigned int IRQ_CIF_X;
  unsigned int CIFX_INT_CLEAR_ADR;
  unsigned int CIFX_INT_ENABLE_ADR;
  unsigned int CIFX_INT_STATUS_ADR;
  unsigned int CIFX_LINE_COUNT_ADR;
} CamHwRegs;

static const CamHwRegs m_cams[2] = {
  {
    MXI_CAM1_BASE_ADR,
    IRQ_CIF_1,
    CIF1_INT_CLEAR_ADR,
    CIF1_INT_ENABLE_ADR,
    CIF1_INT_STATUS_ADR,
    CIF1_LINE_COUNT_ADR
  },
  {
    MXI_CAM2_BASE_ADR,
    IRQ_CIF_2,
    CIF2_INT_CLEAR_ADR,
    CIF2_INT_ENABLE_ADR,
    CIF2_INT_STATUS_ADR,
    CIF2_LINE_COUNT_ADR
  }
};

static CameraHandle* m_handles[2] = {NULL, NULL};

static void CameraIRQ(u32 source);

static void ConfigureI2CRegisters(CameraHandle* handle,
    CameraSpecification* camSpec, u8 camWriteProto[])
{
  SleepTicks(400000);  // TODO: Fix magic number

  u32 address = camSpec->sensorI2CAddress;
  for (int i = 0; i < camSpec->registerCount; i++)
  {
    u8 value = camSpec->regValues[i][1];
    DrvI2cMTransaction(handle->i2cHandle, address, camSpec->regValues[i][0],
        camWriteProto, &value, 1);
    // Movidius says to not change this part, as it will randomly affect
    // flip/fps/noise etc.
    if (i < 20)
      SleepMs(10);
  }
}

static void CameraConfigure(CameraHandle* handle, unsigned int cifBase, 
    CameraSpecification *camSpec,  unsigned int width, unsigned int height)
{
  unsigned int outCfg;
	unsigned int outPrevCfg;
	unsigned int inputFormat;

  const CamHwRegs* cam = &m_cams[handle->sensorNumber];
	
  switch (handle->camSpec->type)
  {
    case YUV420p:
      outCfg = D_CIF_OUTF_FORMAT_420 | 
        D_CIF_OUTF_CHROMA_SUB_CO_SITE_CENTER | 
        D_CIF_OUTF_STORAGE_PLANAR | 
        D_CIF_OUTF_Y_AFTER_CBCR;
      outPrevCfg = 0;
      inputFormat = D_CIF_INFORM_FORMAT_YUV422 | 
        D_CIF_INFORM_DAT_SIZE_8;
      break;

    case YUV422p:
      outCfg = D_CIF_OUTF_FORMAT_422 | 
        D_CIF_OUTF_CHROMA_SUB_CO_SITE_CENTER | 
        D_CIF_OUTF_STORAGE_PLANAR;
      outPrevCfg = 0;
      inputFormat = D_CIF_INFORM_FORMAT_YUV422 | 
        D_CIF_INFORM_DAT_SIZE_8;
      break;

    case YUV422i:
      outCfg =  D_CIF_OUTF_FORMAT_422;
      outPrevCfg = 0;
      inputFormat = D_CIF_INFORM_FORMAT_YUV422 | 
        D_CIF_INFORM_DAT_SIZE_8;
      break;

    case RAW16:
      outCfg = D_CIF_OUTF_BAYER;
      outPrevCfg = D_CIF_PREV_OUTF_RGB_BY | 
        D_CIF_PREV_SEL_COL_MAP_LUT;
      inputFormat = D_CIF_INFORM_FORMAT_RGB_BAYER | 
        D_CIF_INFORM_DAT_SIZE_16;
      break;

    default:
      outCfg =  0;
      outPrevCfg = 0;
      inputFormat = D_CIF_INFORM_FORMAT_YUV422 | 
        D_CIF_INFORM_DAT_SIZE_8;
      break;
  }

  //CIF Timing config
  DrvCifTimingCfg (cifBase, camSpec->width, camSpec->height, 0, 0, 0, 0);

  DrvCifInOutCfg (cifBase,
      inputFormat,
          0x0000,
          outCfg,
          outPrevCfg );

  switch (handle->camSpec->type)
  {
    case RAW16:
    	DrvCifDma0CfgPP(cifBase,
                (unsigned int) handle->currentFrame->p1,
                (unsigned int) handle->currentFrame->p1,
                handle->camSpec->width,
                handle->camSpec->height,
                handle->camSpec->bytesPP,
                D_CIF_DMA_AUTO_RESTART_PING_PONG | 
                  D_CIF_DMA_ENABLE | 
                  D_CIF_DMA_AXI_BURST_16,
                0);
      break;

    case YUV420p:
      DrvCifDma0CfgPP(cifBase,
              (unsigned int) handle->currentFrame->p1,
              (unsigned int) handle->currentFrame->p1,
              handle->camSpec->width,
              handle->camSpec->height,
              handle->camSpec->bytesPP,
              D_CIF_DMA_AUTO_RESTART_PING_PONG | 
                D_CIF_DMA_ENABLE | 
                D_CIF_DMA_AXI_BURST_8,
              0);

      DrvCifDma1CfgPP(cifBase,
              (unsigned int) handle->currentFrame->p2,
              (unsigned int) handle->currentFrame->p2,
              handle->camSpec->width / 2,
              handle->camSpec->height / 2,
              handle->camSpec->bytesPP,
              D_CIF_DMA_AUTO_RESTART_PING_PONG | 
                D_CIF_DMA_ENABLE | 
                D_CIF_DMA_AXI_BURST_8,
              0);

      DrvCifDma2CfgPP(cifBase,
              (unsigned int) handle->currentFrame->p3,
              (unsigned int) handle->currentFrame->p3,
              handle->camSpec->width / 2,
              handle->camSpec->height / 2,
              handle->camSpec->bytesPP,
              D_CIF_DMA_AUTO_RESTART_PING_PONG | 
                D_CIF_DMA_ENABLE | 
                D_CIF_DMA_AXI_BURST_8,
              0);
      break;

    case YUV422p:
      DrvCifDma0CfgPP(cifBase,
              (unsigned int) handle->currentFrame->p1,
              (unsigned int) handle->currentFrame->p1,
              handle->camSpec->width,
              handle->camSpec->height,
              handle->camSpec->bytesPP,
              D_CIF_DMA_AUTO_RESTART_PING_PONG | 
                D_CIF_DMA_ENABLE | 
                D_CIF_DMA_AXI_BURST_8,
              0);

      DrvCifDma1CfgPP(cifBase,
              (unsigned int) handle->currentFrame->p2,
              (unsigned int) handle->currentFrame->p2,
              handle->camSpec->width / 2,
              handle->camSpec->height,
              handle->camSpec->bytesPP,
              D_CIF_DMA_AUTO_RESTART_PING_PONG | 
                D_CIF_DMA_ENABLE | 
                D_CIF_DMA_AXI_BURST_8,
              0);

      DrvCifDma2CfgPP(cifBase,
              (unsigned int) handle->currentFrame->p3,
              (unsigned int) handle->currentFrame->p3,
              handle->camSpec->width / 2,
              handle->camSpec->height,
              handle->camSpec->bytesPP,
              D_CIF_DMA_AUTO_RESTART_PING_PONG | 
                D_CIF_DMA_ENABLE | 
                D_CIF_DMA_AXI_BURST_8,
              0);
      break;

    case YUV422i:
      DrvCifDma0CfgPP(cifBase,
              (unsigned int) handle->currentFrame->p1,
              (unsigned int) handle->currentFrame->p1,
              handle->camSpec->width,
              handle->camSpec->height,
              handle->camSpec->bytesPP,
              D_CIF_DMA_AUTO_RESTART_PING_PONG | 
                D_CIF_DMA_ENABLE | 
                D_CIF_DMA_AXI_BURST_16,
              0);
      break;

    default:
      break;
  }

/*  if(handle->camSpec->cifTiming.generateSync == GENERATE_SYNCS)
  {
    DrvGpioMode(116, D_GPIO_DIR_OUT); // CAM1_VSYNC
    DrvGpioMode(117, D_GPIO_DIR_OUT); // CAM1_HSYNC

    SET_REG_WORD(cam->MXI_CAMX_BASE_ADR+CIF_VSYNC_WIDTH_OFFSET, 16); // VSW
    SET_REG_WORD(cam->MXI_CAMX_BASE_ADR+CIF_HSYNC_WIDTH_OFFSET, 16); // HSW

    SET_REG_WORD(cams[hndl->sensorNumber].MXI_CAMX_BASE_ADR+CIF_INPUT_IF_CFG_OFFSET, (D_CIF_IN_SINC_DRIVED_BY_SABRE));

    DrvCifCtrlCfg (cifBase, width, height, D_CIF_CTRL_ENABLE | D_CIF_CTRL_STATISTICS_FULL | D_CIF_CTRL_TIM_GEN_EN);
  }
  else */
    DrvCifCtrlCfg (cifBase, width, height, 
        D_CIF_CTRL_ENABLE | D_CIF_CTRL_STATISTICS_FULL);
}

void CameraInit(CameraHandle* handle, tyCIFDevice deviceType,
    I2CM_Device* i2cHandle, FrameBuffer* currentFrame, 
    CameraSpecification* camSpec, FrameReadyCallback cbFrameReady)
{
  handle->sensorNumber = deviceType == CAMERA_2;
  handle->i2cHandle = i2cHandle;
  handle->camSpec = camSpec;
  handle->cbFrameReady = cbFrameReady;
  handle->currentFrame = currentFrame;

  m_handles[handle->sensorNumber] = handle;

  DrvCifInit(deviceType);
}

void CameraStart(CameraHandle* handle, int resetPin, bool isActiveLow, 
    u8 camWriteProto[])
{
  tyCIFDevice currentCamera = handle->sensorNumber == 0 ? CAMERA_1 : CAMERA_2;
  u32 frequency = handle->camSpec->referenceFrequency * 1000;
  DrvCifSetMclkFrequency(currentCamera, frequency);

  SleepMs(10);
  DrvCifReset(currentCamera);
  if (isActiveLow)
  {
    DrvGpioSetPinLo(resetPin);
    SleepMs(10);
    DrvGpioSetPinHi(resetPin);
  } else {
    DrvGpioSetPinHi(resetPin);
    SleepMs(10);
    DrvGpioSetPinLo(resetPin);
  }

  SleepMs(10);  // Need 15K cycles of 27Mhz (sensor refclk)

  // Write the setup script to the camera over I2C
  ConfigureI2CRegisters(handle, handle->camSpec, camWriteProto);

  //handle->currentFrame = handle->cbGetFrame();

  DrvCifReset(currentCamera);

  // Disable ICB while setting new interrupt
  u32 irq = m_cams[handle->sensorNumber].IRQ_CIF_X;
  DrvIcbDisableIrq(irq);
  DrvIcbIrqClear(irq);

  // Configure Leon to accept traps on any level
  swcLeonSetPIL(0);

  // Setup the interrupt and handler
  DrvIcbSetIrqHandler(irq, CameraIRQ);
  DrvIcbConfigureIrq(irq, CIF_INTERRUPT_LEVEL, POS_EDGE_INT);
  DrvIcbEnableIrq(irq);

  const CamHwRegs* cam = &m_cams[handle->sensorNumber];

  SET_REG_WORD(cam->CIFX_INT_ENABLE_ADR, D_CIF_INT_DMA0_DONE);

  swcLeonEnableTraps();

  CameraConfigure(handle, cam->MXI_CAMX_BASE_ADR, handle->camSpec, 
      handle->camSpec->width, handle->camSpec->height);
}

static void CameraIRQ(u32 source)
{
  int index = source == IRQ_CIF_1 ? 0 : 1;
  const CamHwRegs* cam = &m_cams[index];
  CameraHandle* handle = m_handles[index];
  FrameBuffer* previousFrame = NULL;

  u32 status = GET_REG_WORD_VAL(cam->CIFX_INT_STATUS_ADR);
  u32 lineCount = GET_REG_WORD_VAL(cam->CIFX_LINE_COUNT_ADR);

/*  if (handle->cbGetFrame)
  {
    previousFrame = handle->currentFrame;
    handle->currentFrame = handle->cbGetFrame();

    if (handle->currentFrame->p1)
    {
      SET_REG_WORD(cam->MXI_CAMX_BASE_ADR + CIF_DMA0_START_ADR_OFFSET, handle->currentFrame->p1);
      SET_REG_WORD(cam->MXI_CAMX_BASE_ADR + CIF_DMA0_START_SHADOW_OFFSET, handle->currentFrame->p1);
    }
    if (handle->currentFrame->p2)
    {
      SET_REG_WORD(cam->MXI_CAMX_BASE_ADR + CIF_DMA0_START_ADR_OFFSET, handle->currentFrame->p2);
      SET_REG_WORD(cam->MXI_CAMX_BASE_ADR + CIF_DMA0_START_SHADOW_OFFSET, handle->currentFrame->p2);
    }
    if (handle->currentFrame->p3)
    {
      SET_REG_WORD(cam->MXI_CAMX_BASE_ADR + CIF_DMA0_START_ADR_OFFSET, handle->currentFrame->p3);
      SET_REG_WORD(cam->MXI_CAMX_BASE_ADR + CIF_DMA0_START_SHADOW_OFFSET, handle->currentFrame->p3);
    }
  }*/

//  if (handle->cbEOF && (status & D_CIF_INT_EOF))
//  {
//    handle->cbEOF();
//  }

  // Clear all pending interrupts
  SET_REG_WORD(cam->CIFX_INT_CLEAR_ADR, 0xFFFFffff);
  // Clear the interrupt
  DrvIcbIrqClear(cam->IRQ_CIF_X);

  handle->cbFrameReady(previousFrame);
}

