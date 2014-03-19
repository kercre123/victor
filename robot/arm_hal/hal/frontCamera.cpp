#include "lib/stm32f4xx.h"
#include "anki/cozmo/robot/hal.h"
#include "anki/common/robot/trig_fast.h"
#include "hal/portable.h"

#include "anki/cozmo/robot/cozmoConfig.h" // for calibration parameters

namespace Anki
{
  namespace Cozmo
  {
    namespace HAL
    {
      GPIO_PIN_SOURCE(D0, GPIOA, 9);
      GPIO_PIN_SOURCE(D1, GPIOA, 10);
      GPIO_PIN_SOURCE(D2, GPIOG, 10);
      GPIO_PIN_SOURCE(D3, GPIOG, 11);
      GPIO_PIN_SOURCE(D4, GPIOE, 4);
      GPIO_PIN_SOURCE(D5, GPIOI, 4);
      GPIO_PIN_SOURCE(D6, GPIOI, 6);
      GPIO_PIN_SOURCE(D7, GPIOI, 7);
      GPIO_PIN_SOURCE(VSYNC, GPIOI, 5);
      GPIO_PIN_SOURCE(HSYNC, GPIOA, 4);
      GPIO_PIN_SOURCE(PCLK, GPIOA, 6);

      GPIO_PIN_SOURCE(XCLK, GPIOA, 3);
      GPIO_PIN_SOURCE(RESET_N, GPIOE, 3);
      GPIO_PIN_SOURCE(PWDN, GPIOE, 2);

      GPIO_PIN_SOURCE(SCL, GPIOB, 6);
      GPIO_PIN_SOURCE(SDA, GPIOB, 7);
      
      const u32 DCMI_TIMEOUT_MAX = 10000;
      const u8 I2C_ADDR = 0x42;
      
      const u8 OV7725_VGA[][2] =
      {
        0x12, 0x80,
        //0x3d, 0x03,
        0x12, 0x40,  // QVGA | "YUV"/"Bayer"
        0x17, 0x3f,
        0x18, 0x50,
        0x19, 0x03,
        0x1a, 0x78,
        0x32, 0x00,
        0x29, 0x50,
        0x2c, 0x78,
        0x2a, 0x00,
        0x11, 0x02,  // pre-scaler

        0x15, 0x00,  // Change HREF to HSYNC

        0x42, 0x7f,  // TGT_B
        0x4d, 0x09,  // Analog fixed gain amplifier
        0x63, 0xe0,  // AWB Control Byte 0
        0x64, 0xff,  // DSP_Ctrl1
        0x65, 0x20,  // DSP_Ctrl2
        0x0c, 0x10,  // flip Y with UV
        0x66, 0x00,  // DSP_Ctrl3
        //0x67, 0x4a,  // DSP_Ctrl4 - Output Selection = RAW8
        0x13, 0xf0,  // COM8 - gain control stuff... | AGC enable
        0x0d, 0xf2,  // PLL = 8x | AEC evaluate 1/4 window
        0x0f, 0xc5,  // Reserved | auto window setting ON/OFF selection when format changes
        0x14, 0x11,  // COM9 - Automatic Gain Ceiling | Reserved
        0x22, 0xff,  // ff/7f/3f/1f for 60/30/15/7.5fps  -- banding filter
        0x23, 0x01,  // 01/03/07/0f for 60/30/15/7.5fps
        0x24, 0x40,  // AEW - AGC/AEC Stable Operation Region (Upper Limit)
        0x25, 0x30,  // AEB - ^^^ (Lower Limit)
        0x26, 0xa1,  // VPT - AGC/AEC Fast Mode Operating Region
        0x2b, 0x00,  // Dummy bytes LSB
        0x6b, 0xaa,  // AWB mode select - Simple AWB
        0x13, 0xe6,  // COM8 - AGC stuff... Enable all but AEC
        0x90, 0x05,  // Sharpness Control 1 - threshold detection
        0x91, 0x01,  // Auto De-noise Threshold Control
        0x92, 0x03,  // Sharpness Strength Upper Limit
        0x93, 0x00,  // Sharpness Strength Lower Limit
        0x94, 0xb0,  // MTX1 - Matrix Coefficient 1
        0x95, 0x9d,
        0x96, 0x13,
        0x97, 0x16,
        0x98, 0x7b,
        0x99, 0x91,  // MTX6
        0x9a, 0x1e,  // MTX_Ctrl (sign bits and stuff)
        0x9b, 0x08,  // Brightness Control
        0x9c, 0x20,  // Constain Gain == Gain * 0x20
        0x9e, 0x81,  // Auto UV Adjust Control 0
        
        0xa6, 0x04,  // Special Digital Effect Control - Contrast/Brightness enable
        
        0x7e, 0x0c,
        0x7f, 0x16,
        0x80, 0x2a,
        0x81, 0x4e,
        0x82, 0x61,
        0x83, 0x6f,
        0x84, 0x7b,
        0x85, 0x86,
        0x86, 0x8e,
        0x87, 0x97,
        0x88, 0xa4,
        0x89, 0xaf,
        0x8a, 0xc5,
        0x8b, 0xd7,
        0x8c, 0xe8,
        0x8d, 0x20,
      };
      
      // Set by the DMA Transfer Complete interrupt
      volatile bool m_isEOF = false;
      
      // Camera exposure value
      u16 m_exposure;
      
      // DMA is limited to 256KB - 1
      const u32 BUFFER_SIZE = 320 * 240 * 2;
      OFFCHIP u8 m_buffer[BUFFER_SIZE];
      
      // Soft I2C stack, borrowed from Arduino (BSD license)
      static void DriveSCL(u8 bit)
      {
        if (bit)
          GPIO_SET(GPIO_SCL, PIN_SCL);
        else
          GPIO_RESET(GPIO_SCL, PIN_SCL);
        
        MicroWait(2);
      }
      
      static void DriveSDA(u8 bit)
      {
        if (bit)
          GPIO_SET(GPIO_SDA, PIN_SDA);
        else
          GPIO_RESET(GPIO_SDA, PIN_SDA);

        MicroWait(2);
      }

      // Read SDA bit by allowing it to float for a while
      // Make sure to start reading the bit before the clock edge that needs it
      static u8 ReadSDA(void)
      {
        GPIO_SET(GPIO_SDA, PIN_SDA);
        MicroWait(10);
        return !!(GPIO_READ(GPIO_SDA) & PIN_SDA);
      }

      static u8 Read(u8 last)
      {
        u8 b = 0, i;

        for (i = 0; i < 8; i++)
        {
          b <<= 1;
          ReadSDA();
          DriveSCL(1);
          b |= ReadSDA();
          DriveSCL(0);
        }
        
        // send Ack or Nak  
        if (last)
          DriveSDA(1);
        else
          DriveSDA(0);
        
        DriveSCL(1);
        DriveSCL(0);
        DriveSDA(1);
        
        return b;
      }
      
      // Issue a Stop condition
      static void Stop(void)
      {
        DriveSDA(0);
        DriveSCL(1);
        DriveSDA(1);
      }
      
      // Write byte and return true for Ack or false for Nak
      static u8 Write(u8 b)
      {
        u8 m;
        // Write byte
        for (m = 0x80; m != 0; m >>= 1)
        {
          DriveSDA(m & b);
          
          DriveSCL(1);
          //if (m == 1)
          //  ReadSDA();  // Let SDA fall prior to last bit
          DriveSCL(0);
        }
        
        DriveSCL(1);
        b = ReadSDA();
        DriveSCL(0);
        
        return b;
      }
      
      // Issue a Start condition for I2C address with Read/Write bit
      static u8 Start(u8 addressRW)
      {
        DriveSDA(0);
        DriveSCL(0);
        
        return Write(addressRW);
      }

      static void CamWrite(u8 reg, u8 val)
      {
        Start(I2C_ADDR);    // Base address is Write
        Write(reg);
        Write(val);
        Stop();
      }

      static int CamRead(u8 reg)
      {
        int val;
        Start(I2C_ADDR);      // Base address is Write (for writing address)
        Write(reg);
        Stop();
        Start(I2C_ADDR + 1);  // Base address + 1 is Read (for Reading address)
        val = Read(1);        // 1 for 'last Read'
        Stop();
        return val;
      }

      void OV7725Init()
      {
        m_exposure = 0;
        
        // Configure the camera interface
        DCMI_InitTypeDef DCMI_InitStructure;
        DCMI_InitStructure.DCMI_CaptureMode = DCMI_CaptureMode_Continuous;
        DCMI_InitStructure.DCMI_SynchroMode = DCMI_SynchroMode_Hardware;
        DCMI_InitStructure.DCMI_PCKPolarity = DCMI_PCKPolarity_Rising;
        DCMI_InitStructure.DCMI_VSPolarity = DCMI_VSPolarity_High;
        DCMI_InitStructure.DCMI_HSPolarity = DCMI_HSPolarity_Low;
        DCMI_InitStructure.DCMI_CaptureRate = DCMI_CaptureRate_All_Frame;
        DCMI_InitStructure.DCMI_ExtendedDataMode = DCMI_ExtendedDataMode_8b;
        DCMI_Init(&DCMI_InitStructure);
        
        // Write the configuration registers
        for(u32 i = 0; i < (sizeof(OV7725_VGA) / 2); i++)
        {
          CamWrite(OV7725_VGA[i][0], OV7725_VGA[i][1]);
          MicroWait(100);
        }
        
        // Configure DMA2_Stream1 channel 1 for DMA from DCMI->DR to RAM
        DMA_DeInit(DMA2_Stream1);
        DMA_InitTypeDef DMA_InitStructure;
        DMA_InitStructure.DMA_Channel = DMA_Channel_1;
        DMA_InitStructure.DMA_PeripheralBaseAddr = (u32)&DCMI->DR;
        DMA_InitStructure.DMA_Memory0BaseAddr = (u32)m_buffer;
        DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralToMemory;
        DMA_InitStructure.DMA_BufferSize = BUFFER_SIZE / 4;  // numBytes / sizeof(word)
        DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
        DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
        DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Word;
        DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Word;
        DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;
        DMA_InitStructure.DMA_Priority = DMA_Priority_VeryHigh;
        DMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Enable;
        DMA_InitStructure.DMA_FIFOThreshold = DMA_FIFOThreshold_Full;
        DMA_InitStructure.DMA_MemoryBurst = DMA_MemoryBurst_Single;
        DMA_InitStructure.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;
        DMA_Init(DMA2_Stream1, &DMA_InitStructure);
        
        // Enable the DMA interrupt for transfer complete to give enough time
        // between frames to do some extra work
        DMA_ITConfig(DMA2_Stream1, DMA_IT_TC, ENABLE);
        
        NVIC_InitTypeDef NVIC_InitStructure;
        NVIC_InitStructure.NVIC_IRQChannel = DMA2_Stream1_IRQn;
        NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
        NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
        NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
        NVIC_Init(&NVIC_InitStructure);
        
        // Enable DMA
        DMA_Cmd(DMA2_Stream1, ENABLE);
        
        // Enable DCMI
        DCMI_Cmd(ENABLE);
        
        // Enable the DCMI peripheral to capture frames from vsync
        DCMI_CaptureCmd(ENABLE);
      }
      
      void FrontCameraInit()
      {
        // Clock configuration
        RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
        RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);
        RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOG, ENABLE);
        RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOE, ENABLE);
        RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOI, ENABLE);
        
        RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA2, ENABLE);
        RCC_AHB2PeriphClockCmd(RCC_AHB2Periph_DCMI, ENABLE);
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM9, ENABLE);
        
        // Configure XCLK for 22.5 MHz (evenly divisible by 180 MHz)
        TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
        TIM_OCInitTypeDef  TIM_OCInitStructure;
        
        TIM_TimeBaseStructure.TIM_Prescaler = 0;
        TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
        TIM_TimeBaseStructure.TIM_ClockDivision = 0;
        TIM_TimeBaseStructure.TIM_RepetitionCounter = 0;

        TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
        TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
        TIM_OCInitStructure.TIM_OutputNState = TIM_OutputNState_Enable;
        TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_Low;
        TIM_OCInitStructure.TIM_OCNPolarity = TIM_OCNPolarity_High;
        TIM_OCInitStructure.TIM_OCIdleState = TIM_OCIdleState_Set;
        TIM_OCInitStructure.TIM_OCNIdleState = TIM_OCIdleState_Reset;
        
        TIM_TimeBaseStructure.TIM_Period = 31;   // 180MHz/N+1
        TIM_OCInitStructure.TIM_Pulse = 16;   // Half of (period+1)
        
        TIM_TimeBaseInit(TIM9, &TIM_TimeBaseStructure);
        TIM_OC2Init(TIM9, &TIM_OCInitStructure);
        TIM_Cmd(TIM9, ENABLE);
        TIM_CtrlPWMOutputs(TIM9, ENABLE);
        
        // Configure the pins for DCMI in AF mode
        GPIO_PinAFConfig(GPIO_D0, SOURCE_D0, GPIO_AF_DCMI);
        GPIO_PinAFConfig(GPIO_D1, SOURCE_D1, GPIO_AF_DCMI);
        GPIO_PinAFConfig(GPIO_D2, SOURCE_D2, GPIO_AF_DCMI);
        GPIO_PinAFConfig(GPIO_D3, SOURCE_D3, GPIO_AF_DCMI);
        GPIO_PinAFConfig(GPIO_D4, SOURCE_D4, GPIO_AF_DCMI);
        GPIO_PinAFConfig(GPIO_D5, SOURCE_D5, GPIO_AF_DCMI);
        GPIO_PinAFConfig(GPIO_D6, SOURCE_D6, GPIO_AF_DCMI);
        GPIO_PinAFConfig(GPIO_D7, SOURCE_D7, GPIO_AF_DCMI);
        GPIO_PinAFConfig(GPIO_VSYNC, SOURCE_VSYNC, GPIO_AF_DCMI);
        GPIO_PinAFConfig(GPIO_HSYNC, SOURCE_HSYNC, GPIO_AF_DCMI);
        GPIO_PinAFConfig(GPIO_PCLK, SOURCE_PCLK, GPIO_AF_DCMI);
        GPIO_PinAFConfig(GPIO_XCLK, SOURCE_XCLK, GPIO_AF_TIM9);
        
        // Initialize the camera pins
        GPIO_InitTypeDef GPIO_InitStructure;
        GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
        GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
        GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
        GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
        
        GPIO_InitStructure.GPIO_Pin = PIN_D0 | PIN_D1 | PIN_HSYNC | PIN_PCLK | PIN_XCLK;
        GPIO_Init(GPIOA, &GPIO_InitStructure);
        
        GPIO_InitStructure.GPIO_Pin = PIN_D2 | PIN_D3;
        GPIO_Init(GPIOG, &GPIO_InitStructure);
        
        GPIO_InitStructure.GPIO_Pin = PIN_D4;
        GPIO_Init(GPIOE, &GPIO_InitStructure);
        
        GPIO_InitStructure.GPIO_Pin = PIN_D5 | PIN_D6 | PIN_D7 | PIN_VSYNC;
        GPIO_Init(GPIOI, &GPIO_InitStructure);
        
        // PWDN and RESET_N are normal GPIO
        GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
        GPIO_InitStructure.GPIO_Pin = PIN_PWDN | PIN_RESET_N;
        GPIO_Init(GPIOE, &GPIO_InitStructure);
        
        // Initialize the I2C pins
        GPIO_SET(GPIO_SCL, PIN_SCL);
        GPIO_SET(GPIO_SDA, PIN_SDA);
        
        GPIO_InitStructure.GPIO_Pin = PIN_SCL | PIN_SDA;
        GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
        GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
        GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;
        GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
        GPIO_Init(GPIOB, &GPIO_InitStructure);
        
        // Reset the camera
        GPIO_RESET(GPIO_RESET_N, PIN_RESET_N);
        GPIO_RESET(GPIO_PWDN, PIN_PWDN);  // Make sure it's in normal mode
        MicroWait(10000);
        GPIO_SET(GPIO_RESET_N, PIN_RESET_N);
        MicroWait(100000);
        
        OV7725Init();
      }
      
      void CameraGetFrame(u8* frame, CameraMode mode, f32 exposure, bool enableLight)
      {
        // Update the exposure
        u16 exp = (u16)(exposure * 0xFF);
        if (exp > 0xFFFF)
          exp = 0xFFFF;
        if (m_exposure != exp)
        {
          m_exposure = exp;
          
          //CamWrite(0x08, (exposure >> 8));  // AEC[15:8]
          //MicroWait(100);
          CamWrite(0x10, m_exposure);  // AEC[7:0]
        }
        
        m_isEOF = false;
        
        // Wait until the frame has completed (based on DMA_FLAG_TCIF1)
        while (!m_isEOF)
        {
        }
        
        // TODO: Change this to DMA mem-to-mem when we have a different camera
        // and we support resolution changes in the actual hardware
        
        // Copy the Y-channel into frame
        u32 xRes = CameraModeInfo[mode].width;
        u32 yRes = CameraModeInfo[mode].height;
        u32 xSkip = 320 / xRes;
        u32 ySkip = 240 / yRes;
        
        u32 dataY = 0;
        for (u32 y = 0; y < 240; y += ySkip, dataY++)
        {
          u32 dataX = 0;
          for (u32 x = 0; x < 320; x += xSkip, dataX++)
          {
            frame[dataY * xRes + dataX] = m_buffer[y * 320 * 2 + x * 2];
          }
        }
      }
      
      inline f32 ComputeVerticalFOV(const u16 height, const f32 fy)
      {
        return 2.f * atan_fast(static_cast<f32>(height) / (2.f * fy));
      }
      
      const CameraInfo* GetHeadCamInfo(void)
      {
        static CameraInfo s_headCamInfo = {
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

        return &s_headCamInfo;
      }
    }
  }
}

extern "C"
void DMA2_Stream1_IRQHandler(void)
{
  using namespace Anki::Cozmo::HAL;
  
  // Clear the DMA Transfer Complete flag
  //DMA_ClearFlag(DMA2_Stream1, DMA_FLAG_TCIF1);
  DMA2->LIFCR = DMA_FLAG_TCIF1 & 0x0F7D0F7D;  // Direct version of call above
  
  m_isEOF = true;
}

/*extern "C"
void DCMI_IRQHandler(void)
{
  using namespace Anki::Cozmo::HAL;
  
  //m_isEOF = true;
  
  // Clear all interrupts (5-bits)
  DCMI->ICR = 0x1f;
}*/
