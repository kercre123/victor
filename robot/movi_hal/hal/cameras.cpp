#include "cameras.h"
#include "anki/cozmo/robot/cozmoConfig.h" // for calibration parameters
#include "anki/common/robot/trig_fast.h"

// Unchanged interrupt level from Movidius
#define CIF_INTERRUPT_LEVEL 3

#define FLOAT_FIXED(x) ((u32)((x) * 1024) & 0xFFF)

namespace Anki
{
  namespace Cozmo
  {
    namespace HAL
    {
      // "Private member variables"
      namespace {
        CameraMode headCamMode_;

        // Whether we've already started continuous mode
        bool m_continuousModeStarted = false;

        // Intrinsic calibration parameters for each camera:
        CameraInfo headCamInfo_;
        CameraInfo matCamInfo_;
      }

      inline f32 ComputeVerticalFOV(const u16 height, const f32 fy)
      {
        return 2.f * atan_fast(static_cast<f32>(height) / (2.f * fy));
      }

      const CameraInfo* GetHeadCamInfo(void)
      {
        headCamInfo_ = {
          HEAD_CAM_CALIB_FOCAL_LENGTH_X,
          HEAD_CAM_CALIB_FOCAL_LENGTH_Y,
          ComputeVerticalFOV(HEAD_CAM_CALIB_HEIGHT,
                             HEAD_CAM_CALIB_FOCAL_LENGTH_Y),
          HEAD_CAM_CALIB_CENTER_X,
          HEAD_CAM_CALIB_CENTER_Y,
          0.f,
          HEAD_CAM_CALIB_HEIGHT,
          HEAD_CAM_CALIB_WIDTH
        };

        return &headCamInfo_;
      }

      const CameraInfo* GetMatCamInfo(void)
      {
        matCamInfo_ = {
          MAT_CAM_CALIB_FOCAL_LENGTH_X,
          MAT_CAM_CALIB_FOCAL_LENGTH_Y,
          ComputeVerticalFOV(MAT_CAM_CALIB_HEIGHT,
                             MAT_CAM_CALIB_FOCAL_LENGTH_Y),
          MAT_CAM_CALIB_CENTER_X,
          MAT_CAM_CALIB_CENTER_Y,
          0.f,
          MAT_CAM_CALIB_HEIGHT,
          MAT_CAM_CALIB_WIDTH
        };

        return &matCamInfo_;
      }


      typedef struct
      {
        const u32 MXI_CAMX_BASE_ADR;
        const u32 IRQ_CIF_X;
        const u32 CIFX_INT_CLEAR_ADR;
        const u32 CIFX_INT_ENABLE_ADR;
        const u32 CIFX_INT_STATUS_ADR;
        const u32 CIFX_LINE_COUNT_ADR;
        const u32 CIFX_DMA0_CFG_ADR;
        CameraMode lastMode;
        CameraUpdateMode updateMode;
        volatile bool isEOF;
        volatile bool isWaitingForFrame;
      } CamHWRegs;

      static CamHWRegs m_cams[CAMERA_COUNT] =
      {
        {
          MXI_CAM1_BASE_ADR,
          IRQ_CIF_1,
          CIF1_INT_CLEAR_ADR,
          CIF1_INT_ENABLE_ADR,
          CIF1_INT_STATUS_ADR,
          CIF1_LINE_COUNT_ADR,
          MXI_CAM1_BASE_ADR + CIF_DMA0_CFG_OFFSET,
          CAMERA_MODE_NONE,
          CAMERA_UPDATE_CONTINUOUS,
          false, false
        },
        {
          MXI_CAM2_BASE_ADR,
          IRQ_CIF_2,
          CIF2_INT_CLEAR_ADR,
          CIF2_INT_ENABLE_ADR,
          CIF2_INT_STATUS_ADR,
          CIF2_LINE_COUNT_ADR,
          MXI_CAM2_BASE_ADR + CIF_DMA0_CFG_OFFSET,
          CAMERA_MODE_NONE,
          CAMERA_UPDATE_CONTINUOUS,
          false, false
        }
      };

      CameraMode GetHeadCamMode(void)
      {
        return headCamMode_;
      }

      // TODO: there is a copy of this in sim_hal.cpp -- consolidate into one location.
      void SetHeadCamMode(const u8 frameResHeader)
      {
        bool found = false;
        for(CameraMode mode = CAMERA_MODE_VGA;
            not found && mode != CAMERA_MODE_COUNT; mode = (CameraMode)(mode + 1))
        {
          if(frameResHeader == CameraModeInfo[mode].header) {
            headCamMode_ = mode;
            found = true;
          }
        }

        if(not found) {
          PRINT("ERROR(SetCameraMode): Unknown frame res: %d", frameResHeader);
        }
      } //SetHeadCamMode()

      static CameraHandle* m_handles[CAMERA_COUNT] = {NULL, NULL};

      static void CameraIRQ(u32 source);

      static void ConfigureI2CRegisters(CameraHandle* handle, CameraMode mode)
      {
        CameraSpecification* camSpec = &handle->camSpec;
        u32 address = camSpec->sensorI2CAddress;
        CameraRegisters* i2c = &camSpec->registerValues[mode];
        for (int i = 0; i < i2c->count; i++)
        {
          u8 value = i2c->registers[i * 2 + 1];
          DrvI2cMTransaction(handle->i2cHandle, address, i2c->registers[i * 2 + 0],
              camSpec->writePrototype, &value, 1);
          // Movidius says to not change this part, as it will randomly affect
          // flip/fps/noise etc.
          // TODO: Look into this.
          if (i < 20)
            SleepMs(10);
        }
      }

      static void CameraConfigure(CameraHandle* handle, u32 cifBase,
          CameraMode mode, u8* frameBuffer)
      {
        u32 outCfg;
        u32 inputFormat;
        u32 control = 0;

        // Write the setup script to the camera over I2C
        ConfigureI2CRegisters(handle, mode);

        CamHWRegs* cam = &m_cams[handle->cameraID];
        CameraSpecification* camSpec = &handle->camSpec;

        switch (camSpec->type)
        {
          case BAYER:
            outCfg = D_CIF_OUTF_FORMAT_444 |
              D_CIF_OUTF_CHROMA_SUB_CO_SITE_CENTER;
            inputFormat = D_CIF_INFORM_FORMAT_RGB_BAYER |
              D_CIF_INFORM_BAYER_BGBG_GRGR |
              D_CIF_INFORM_DAT_SIZE_8;
            break;

          case BAYER_TO_Y:
            outCfg = D_CIF_OUTF_FORMAT_444 |
              D_CIF_OUTF_CHROMA_SUB_CO_SITE_CENTER |
              D_CIF_OUTF_STORAGE_PLANAR;
            inputFormat = D_CIF_INFORM_FORMAT_RGB_BAYER |
              D_CIF_INFORM_BAYER_BGBG_GRGR |
              D_CIF_INFORM_DAT_SIZE_8;
            break;

          default:
            // ASSERT(0);
            break;
        }

        //CIF Timing config
        DrvCifTimingCfg (cifBase, CameraModeInfo[mode].width, CameraModeInfo[mode].height, 0, 0, 0, 0);

        DrvCifInOutCfg (cifBase, inputFormat, 0x0000, outCfg, 0);

        switch (camSpec->type)
        {
          case BAYER:
            control =
              D_CIF_CTRL_ENABLE |
              D_CIF_CTRL_RGB_BAYER_EN |
              D_CIF_CTRL_STATISTICS_FULL;

            DrvCifDma0CfgPP(cifBase,
                      (u32)frameBuffer,
                      (u32)frameBuffer,
                      CameraModeInfo[mode].width,
                      CameraModeInfo[mode].height,
                      camSpec->bytesPP,
                      D_CIF_DMA_AXI_BURST_16,
                      0);
            break;

          case BAYER_TO_Y:
            control =
              D_CIF_CTRL_ENABLE |
              D_CIF_CTRL_RGB_BAYER_EN |
              D_CIF_CTRL_CSC_ENABLE |  // Color space conversion
              D_CIF_CTRL_STATISTICS_FULL;

            // Capture only the Y-channel
            DrvCifDma0CfgPP(cifBase,
                    (u32)frameBuffer,
                    (u32)frameBuffer,
                     CameraModeInfo[mode].width,
                     CameraModeInfo[mode].height,
                    camSpec->bytesPP,
                    D_CIF_DMA_AXI_BURST_8,
                    0);

            // Make sure the other DMA channels are disabled
            REG_WORD(cifBase + CIF_DMA1_CFG_OFFSET) = 0;
            REG_WORD(cifBase + CIF_DMA2_CFG_OFFSET) = 0;
            REG_WORD(cifBase + CIF_DMA3_CFG_OFFSET) = 0;

            // Setup color space conversion matrix for RGB -> YUV
            REG_WORD(cifBase + CIF_CSC_COEFF11_OFFSET) = FLOAT_FIXED(0.299);
            REG_WORD(cifBase + CIF_CSC_COEFF12_OFFSET) = FLOAT_FIXED(0.587);
            REG_WORD(cifBase + CIF_CSC_COEFF13_OFFSET) = FLOAT_FIXED(0.114);

            REG_WORD(cifBase + CIF_CSC_COEFF21_OFFSET) = FLOAT_FIXED(-0.147);
            REG_WORD(cifBase + CIF_CSC_COEFF22_OFFSET) = FLOAT_FIXED(-0.289);
            REG_WORD(cifBase + CIF_CSC_COEFF23_OFFSET) = FLOAT_FIXED(0.436);

            REG_WORD(cifBase + CIF_CSC_COEFF31_OFFSET) = FLOAT_FIXED(0.615);
            REG_WORD(cifBase + CIF_CSC_COEFF32_OFFSET) = FLOAT_FIXED(-0.515);
            REG_WORD(cifBase + CIF_CSC_COEFF33_OFFSET) = FLOAT_FIXED(-0.100);
           break;

          default:
           // ASSERT(0);
            break;
        }

        DrvCifCtrlCfg (cifBase, CameraModeInfo[mode].width, CameraModeInfo[mode].height, control);
     }

      void CameraInit(CameraHandle* handle, CameraID cameraID,
          FrameType frameType, u32 bytesPP, u32 referenceFrequency,
          u32 sensorI2CAddress, I2CM_Device* i2cHandle, const u16* regVGA,
          u32 regVGACount, const u16* regQVGA, u32 regQVGACount,
          const u16* regQQVGA, u32 regQQVGACount, u32 resetPin,
          bool isActiveLow, u8 camWritePrototype[])
      {
        handle->cameraID = cameraID;
        handle->i2cHandle = i2cHandle;

        CameraSpecification* camSpec = &handle->camSpec;
        camSpec->type = frameType;
        camSpec->bytesPP = bytesPP;
        camSpec->referenceFrequency = referenceFrequency;
        camSpec->sensorI2CAddress = sensorI2CAddress;
        camSpec->writePrototype = camWritePrototype;

        camSpec->registerValues[CAMERA_MODE_VGA].registers = (u16*)regVGA;
        camSpec->registerValues[CAMERA_MODE_VGA].count = regVGACount;

        camSpec->registerValues[CAMERA_MODE_QVGA].registers = (u16*)regQVGA;
        camSpec->registerValues[CAMERA_MODE_QVGA].count = regQVGACount;

        camSpec->registerValues[CAMERA_MODE_QQVGA].registers = (u16*)regQQVGA;
        camSpec->registerValues[CAMERA_MODE_QQVGA].count = regQQVGACount;

        m_handles[cameraID] = handle;

        tyCIFDevice currentCamera;
        if (cameraID == CAMERA_FRONT)
          currentCamera = CAMERA_1;
        else
          currentCamera = CAMERA_2;

        DrvCifInit(currentCamera);

        u32 frequencyKHz = referenceFrequency * 1000;
        DrvCifSetMclkFrequency(currentCamera, frequencyKHz);

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

        // Reset the CIF for this camera
        DrvCifReset(currentCamera);

        // Disable ICB while setting new interrupt
        u32 irq = m_cams[handle->cameraID].IRQ_CIF_X;
        DrvIcbDisableIrq(irq);
        DrvIcbIrqClear(irq);

        // Configure Leon to accept traps on any level
        swcLeonSetPIL(0);

        // Setup the interrupt and handler
        DrvIcbSetIrqHandler(irq, CameraIRQ);
        DrvIcbConfigureIrq(irq, CIF_INTERRUPT_LEVEL, POS_EDGE_INT);
        DrvIcbEnableIrq(irq);

        CamHWRegs* cam = &m_cams[handle->cameraID];
        SET_REG_WORD(cam->CIFX_INT_ENABLE_ADR, D_CIF_INT_DMA0_DONE);

        swcLeonEnableTraps();
      }

      static void CameraIRQ(u32 source)
      {
        int index = source == IRQ_CIF_1 ? 0 : 1;
        CamHWRegs* cam = &m_cams[index];
        CameraHandle* handle = m_handles[index];

        u32 status = GET_REG_WORD_VAL(cam->CIFX_INT_STATUS_ADR);
        u32 lineCount = GET_REG_WORD_VAL(cam->CIFX_LINE_COUNT_ADR);

        // Clear all pending interrupts
        REG_WORD(cam->CIFX_INT_CLEAR_ADR) = 0xFFFFffff;
        // Clear the interrupt
        DrvIcbIrqClear(cam->IRQ_CIF_X);

        bool isContinuous = cam->updateMode == CAMERA_UPDATE_CONTINUOUS;
        cam->isEOF = cam->isWaitingForFrame || isContinuous;
        cam->isWaitingForFrame = false;
      }

      void CameraStartFrame(CameraID cameraID, u8* frame, CameraMode mode,
          CameraUpdateMode updateMode, u16 exposure, bool enableLight)
      {
        CamHWRegs* cam = &m_cams[cameraID];
        if(cam->lastMode == mode &&
           cam->updateMode == updateMode &&
           updateMode == CAMERA_UPDATE_CONTINUOUS &&
           m_continuousModeStarted)
        {
          // Everything is already set up the way we want.  Nothing to do.
          return;
        }


        CameraHandle* handle = m_handles[cameraID];

        // Reconfigure the camera for a different resolution
        if (cam->lastMode != mode)
        {
          CameraConfigure(handle, cam->MXI_CAMX_BASE_ADR, mode, frame);
          cam->lastMode = mode;
          m_continuousModeStarted = false;
        }

        IRQDisable();
        cam->updateMode = updateMode;
        cam->isWaitingForFrame = true;
        IRQEnable();

        // Set new framebuffer address
        u32 cif = cam->MXI_CAMX_BASE_ADR;
        REG_WORD(cif + CIF_DMA0_START_ADR_OFFSET) = (u32)frame;
        REG_WORD(cif + CIF_DMA0_START_SHADOW_OFFSET) = (u32)frame;

        // Setup continuous mode here to start running
        u32 address = cam->CIFX_DMA0_CFG_ADR;
        if (updateMode == CAMERA_UPDATE_CONTINUOUS &&
            not m_continuousModeStarted)
        {
          REG_WORD(address) |=
            D_CIF_DMA_ENABLE |
            D_CIF_DMA_AUTO_RESTART_CONTINUOUS;

          m_continuousModeStarted = true;

        } else {
          // Otherwise, run a single frame
          REG_WORD(address) &= ~D_CIF_DMA_AUTO_RESTART_CONTINUOUS;
          REG_WORD(address) |=
            D_CIF_DMA_ENABLE |
            D_CIF_DMA_AUTO_RESTART_ONCE_SHADOW;

          m_continuousModeStarted = false;
        }
      }

      u32 CameraGetReceivedLines(CameraID cameraID)
      {
        CamHWRegs* cam = &m_cams[cameraID];
        return REG_WORD(cam->CIFX_LINE_COUNT_ADR);
      }

      bool CameraIsEndOfFrame(CameraID cameraID)
      {
        return m_cams[cameraID].isEOF;
      }

      void CameraSetIsEndOfFrame(CameraID cameraID, bool isEOF)
      {
        m_cams[cameraID].isEOF = isEOF;
      }
	 
	  void DisableCamera(CameraID cameraID)
	  {
		u32 irq = m_cams[cameraID].IRQ_CIF_X;
        DrvIcbDisableIrq(irq);
	  }
    }
  }
}

