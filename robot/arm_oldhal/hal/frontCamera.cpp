#include "lib/stm32f4xx.h"
#include "anki/cozmo/robot/hal.h"
#include "anki/common/robot/trig_fast.h"
#include "hal/portable.h"

#include "anki/cozmo/shared/cozmoConfig.h" // for calibration parameters
#include "anki/common/robot/config.h"
#include "anki/common/robot/benchmarking.h"

namespace Anki
{
  namespace Cozmo
  {
    namespace HAL
    {
      // For headboard 2.1
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
      GPIO_PIN_SOURCE(RESET_N, GPIOE, 6); // 2.1
      GPIO_PIN_SOURCE(PWDN, GPIOE, 2);
      
      GPIO_PIN_SOURCE(FSIN, GPIOA, 1);  // New in 2.1

      GPIO_PIN_SOURCE(SCL, GPIOE, 3); // 2.1
      GPIO_PIN_SOURCE(SDA, GPIOE, 5); // 2.1

      const u32 DCMI_TIMEOUT_MAX = 10000;

      const u8 I2C_ADDR = 0x78;        // Pre-shifted address for OV7739 (0x78 for Read, 0x79 for Write)
      
      unsigned short m_camScript[] =
        // YUV_QVGA_60fps
        {
        0x3008,0x82 // SYSTEM CTRL
        ,0x3008,0x42 // SYSTEM CTRL
        ,0x3104,0x03 // OTP something
        ,0x3017,0x7f // PAD OEN01 (Output enable: 7f for 2.1  (7e/00 for shorted 2.1 boards))
        ,0x3018,0xf0 // PAD OEN02 (Output enable: f0 for 2.1)
        ,0x3602,0x14 // undoc
        ,0x3611,0x44 // undoc
        ,0x3631,0x22 // undoc
        ,0x3622,0x00 // ANA ARRAY02
        ,0x3633,0x25 // undoc
        ,0x370d,0x04 // ARRAY CTRL02
        ,0x3620,0x32 // undoc
        ,0x3714,0x2c // undoc
        ,0x401c,0x00 // undoc
        ,0x401e,0x11 // undoc
        ,0x4702,0x01 // undoc
        ,0x5000,0x0e // ISP CTRL00
        ,0x5001,0x00 // ISP CTRL01
        ,0x3a00,0x7a // AEC CTRL00
        ,0x3a18,0x00 // AEC GAIN CEILING
        ,0x3a19,0x3f // AEC GAIN CEILING
        ,0x300f,0x88 // PLL1 CTRL00
        ,0x3011,0x08 // PLL1 CTRL02
        //,0x3011,0x80  // PLL1: disable - 1/4 the framerate
        ,0x4303,0xff // YMAX VALUE (Set Y max clip value[7:0])
        ,0x4307,0xff // UMAX VALUE (Set U max clip value[7:0])
        ,0x430b,0xff // VMAX VALUE (Set V max clip value[7:0])
        ,0x4305,0x00 // YMIN VALUE (Set Y min clip value[7:0])
        ,0x4309,0x00 // UMIN VALUE (Set U min clip value[7:0])
        ,0x430d,0x00 // VMIN VALUE (Set V min clip value[7:0])
        //,0x5000,0x4f // ISP CTRL00
        //,0x5001,0x47 // ISP CTRL01
        ,0x4300,0x30 // FORMAT CTRL00 (Output format selection) (YUV422 0x30)
        ,0x4301,0x80 // undoc (possibly format related?)
        ,0x501f,0x01 // ISP CTRL1F (ISP raw 0x00) (YUV422 0x01)
        ,0x3800,0x00 // image cropping (horizontal start)
        ,0x3801,0x6e // image cropping (horizontal start)
        ,0x3804,0x01 // image cropping (horizontal width)
        ,0x3805,0x40 // image cropping (horizontal width)
        ,0x3802,0x00 // image cropping (vertical start)
        ,0x3803,0x0e // image cropping (vertical start)
        ,0x3806,0x00 // image cropping (vertical height)
        ,0x3807,0xf0 // image cropping (vertical height)
        ,0x3808,0x01 // TIMING DVPHO
        ,0x3809,0x40 // TIMING DVPHO
        ,0x380a,0x00 // TIMING DVPVO
        ,0x380b,0xf0 // TIMING DVPVO
        ,0x380c,0x03 // TIMING HTS
        ,0x380d,0x10 // TIMING HTS
        ,0x380e,0x01 // TIMING VTS
        ,0x380f,0x00 // TIMING VTS
        ,0x3810,0x08 // TIMING HOFFS
        ,0x3811,0x04 // TIMING VOFFS
        ,0x370d,0x0c // vertical binning
        ,0x3622,0x68 // horizontal skip or binning
        ,0x3818,0x81 // mirror vertical/horizontal (plus some undoc bits?)
        ,0x3a08,0x00 // AEC B50 STEP
        ,0x3a09,0x99 // AEC B50 STEP
        ,0x3a0a,0x00 // AEC B60 STEP
        ,0x3a0b,0x80 // AEC B60 STEP
        ,0x3a0d,0x02 // AEC CTRL0D
        ,0x3a0e,0x01 // AEC CTRL0E 
        ,0x3705,0xdc // undoc
        ,0x3a1a,0x05 // NOT USED (But it's set?!)
        ,0x3008,0x02 // ISP CTRL02 (ISP subsample)
        ,0x5180,0x02 // SYSTEM CTRL
        //,0x5181,0x02 // AWB CTRL01
        //,0x3a0f,0x35 // AEC CONTROL 0F
        //,0x3a10,0x2c // AEC CONTROL 10
        //,0x3a1b,0x36 // AEC CONTROL 1B
        //,0x3a1e,0x2d // AEC CONTROL 1E
        //,0x3a11,0x90 // AEC CONTROL 11
        //,0x3a1f,0x10 // AEC CONTROL 1F
        ,0x5000,0x47 // ISP CTRL00 + 0x40 for Gamma
        ,0x5481,0x0a // GAMMA YST1
        ,0x5482,0x13 // GAMMA YST2
        ,0x5483,0x23 // GAMMA YST3
        ,0x5484,0x40 // GAMMA YST4
        ,0x5485,0x4d // GAMMA YST5
        ,0x5486,0x58 // GAMMA YST6
        ,0x5487,0x64 // GAMMA YST7
        ,0x5488,0x6e // GAMMA YST8
        ,0x5489,0x78 // GAMMA YST9
        ,0x548a,0x81 // GAMMA YST10
        ,0x548b,0x92 // GAMMA YST11
        ,0x548c,0xa1 // GAMMA YST12
        ,0x548d,0xbb // GAMMA YST13
        ,0x548e,0xcf // GAMMA YST14
        ,0x548f,0xe3 // GAMMA YST15
        ,0x5490,0x26 // GAMMA YSLP15 
        //,0x5380,0x47 // CMX COEFFICIENT11 
        //,0x5381,0x3c // CMX COEFFICIENT12
        //,0x5382,0x06 // CMX COEFFICIENT13
        //,0x5383,0x17 // CMX COEFFICIENT14
        //,0x5384,0x3a // CMX COEFFICIENT15
        //,0x5385,0x52 // CMX COEFFICIENT16
        //,0x5392,0x1e // CMX SIGN
        //,0x5801,0x00 // LENC CTRL1
        //,0x5802,0x06 // LENC CTRL2
        //,0x5803,0x0a // LENC CTRL3
        //,0x5804,0x42 // LENC CTRL4
        //,0x5805,0x2a // LENC CTRL5
        //,0x5806,0x25 // LENC CTRL6
        ,0x5001,0x42 // ISP CTRL01
        ,0x5580,0x00 // Special digital effects
        //,0x5583,0x40 // Special digital effects
        //,0x5584,0x26 // Special digital effects
        //,0x5589,0x10 // Special digital effects
        //,0x558a,0x00 // Special digital effects
        //,0x558b,0x3e // Special digital effects
        ,0x5300,0x00 // CIP CTRL0 (Sharpenmt threshold 1)
        ,0x5301,0x00 // CIP CTRL1 (Sharpenmt threshold 2)
        ,0x5302,0x00 // CIP CTRL2 (Sharpenmt offset 1)
        ,0x5303,0x00 // CIP CTRL3 (Sharpenmt offset 2)
        ,0x5304,0x10 // CIP CTRL4 (Denoise threshold 1)
        ,0x5305,0xce // CIP CTRL5 (Denoise threshold 2)
        ,0x5306,0x06 // CIP CTRL6 (Denoise offset 1)
        ,0x5307,0xd0 // CIP CTRL7 (Denoise offset 2)
        ,0x5680,0x00 // Undocumented, maybe AEC related
        ,0x5681,0x50 // Undocumented, maybe AEC related
        ,0x5682,0x00 // Undocumented, maybe AEC related
        ,0x5683,0x3c // Undocumented, maybe AEC related
        ,0x5684,0x11 // Undocumented, maybe AEC related
        ,0x5685,0xe0 // Undocumented, maybe AEC related
        ,0x5686,0x0d // Undocumented, maybe AEC related
        ,0x5687,0x68 // Undocumented, maybe AEC related
        ,0x5688,0x03 // AVERAGE CTRL8
        ,0x4000,0x05 // BLC CTRL00
        ,0x4002,0x45 // BLC CTRL02
        ,0x4008,0x10 // BLC CTRL08
        ,0x3500,0x00 // 16.4 exposure time msb (4 bits) [19:16]
        ,0x3501,0x04 // 16.4 exposure time middle (8 bits) [15:8]
        ,0x3502,0x00 // 16.4 exposure time lsb (4.4 bits) [7:0]
        ,0x3503,0x03 // AEC MANUAL
        ,0,0
        };

      // Set by the DMA Transfer Complete interrupt
      volatile bool m_isEOF = false;

      // Camera exposure value
      u32 m_exposure;

      bool m_enableVignettingCorrection;

      // DMA is limited to 256KB - 1
      const int TOTAL_COLS = 320, TOTAL_ROWS = 240, BUFFER_ROWS = 8, BYTES_PER_PIX = 2;
      const int BUFFER_SIZE = TOTAL_COLS * BUFFER_ROWS * BYTES_PER_PIX;  // 8 rows of YUYV (2 bytes each)
      ONCHIP u8 m_buffer[2][BUFFER_SIZE];   // Double buffer
      volatile int m_readyBuffer = 1, m_readyRow = -8;
     
      // Get raw data from the camera
      u8* CamGetRaw() { return m_buffer[m_readyBuffer]; }
      int CamGetReadyRow() { return m_readyRow; }
        
      // This delay helps miss a race condition in Omnivision-supplied configuration scripts
      // Omnivision is aware of this problem but could not suggest a workaround
      const u32 I2C_WAIT = 8;   // 8uS between clock edges - or 62.5KHz I2C
        
      // Soft I2C stack, borrowed from Arduino (BSD license)
      static void DriveSCL(u8 bit)
      {
        if (bit)
          GPIO_SET(GPIO_SCL, PIN_SCL);
        else
          GPIO_RESET(GPIO_SCL, PIN_SCL);

        MicroWait(I2C_WAIT);
      }

      static void DriveSDA(u8 bit)
      {
        if (bit)
          GPIO_SET(GPIO_SDA, PIN_SDA);
        else
          GPIO_RESET(GPIO_SDA, PIN_SDA);

        MicroWait(I2C_WAIT);
      }

      // Read SDA bit by allowing it to float for a while
      // Make sure to start reading the bit before the clock edge that needs it
      static u8 ReadSDA(void)
      {
        GPIO_SET(GPIO_SDA, PIN_SDA);
        MicroWait(I2C_WAIT);
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
          if (m == 1)
            ReadSDA();  // Let SDA fall prior to last bit
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

      static void CamWrite(int reg, int val)
      {
        Start(I2C_ADDR);    // Base address is Write
        Write(reg >> 8);
        Write(reg & 0xff);
        Write(val);
        Stop();
      }

      static int CamRead(int reg)
      {
        int val;
        Start(I2C_ADDR);    // Base address is Write (for writing address)
        Write(reg >> 8);
        Write(reg & 0xff);
        Stop();
        Start(I2C_ADDR+1);  // Base address + 1 is Read (for Reading address)
        val = Read(1);      // 1 for 'last Read'
        Stop();
        return val;
      }

      void UARTPutHex(u8 c);
      
      // For self-test purposes only.
      static void CamSetPulls(GPIO_TypeDef* GPIOx, u32 pin, int index)
      {
        GPIO_InitTypeDef GPIO_InitStructure;
        GPIO_InitStructure.GPIO_PuPd = index ? GPIO_PuPd_UP : GPIO_PuPd_DOWN;
        GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
        GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
        GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
        GPIO_InitStructure.GPIO_Pin =  pin;
        GPIO_Init(GPIOx, &GPIO_InitStructure);
      }
      static int CamGetPulls(GPIO_TypeDef* GPIOx, u32 pin, int index)
      {
        return (!!(GPIO_READ(GPIOx) & pin)) << index;
      }
      static int CamReadDB(int pullup)
      {
        CamSetPulls(GPIO_D0, PIN_D0, pullup & 1);
        CamSetPulls(GPIO_D1, PIN_D1, pullup & 2);
        CamSetPulls(GPIO_D2, PIN_D2, pullup & 4);
        CamSetPulls(GPIO_D3, PIN_D3, pullup & 8);
        CamSetPulls(GPIO_D4, PIN_D4, pullup & 16);
        CamSetPulls(GPIO_D5, PIN_D5, pullup & 32);
        CamSetPulls(GPIO_D6, PIN_D6, pullup & 64);
        CamSetPulls(GPIO_D7, PIN_D7, pullup & 128);
        
        MicroWait(25);
        
        return
          CamGetPulls(GPIO_D0, PIN_D0, 0) |
          CamGetPulls(GPIO_D1, PIN_D1, 1) |
          CamGetPulls(GPIO_D2, PIN_D2, 2) |
          CamGetPulls(GPIO_D3, PIN_D3, 3) |
          CamGetPulls(GPIO_D4, PIN_D4, 4) |
          CamGetPulls(GPIO_D5, PIN_D5, 5) |
          CamGetPulls(GPIO_D6, PIN_D6, 6) |
          CamGetPulls(GPIO_D7, PIN_D7, 7);
      }

      void OV7739Init()
      {
        m_exposure = u32_MAX;
        m_enableVignettingCorrection = false;

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

        CamRead(0x3004);        // Get the I2C bus into a proper state
        
        UARTPutHex(CamRead(0x3015));  // XXX - For debug only

        // Write the configuration registers        
        unsigned short* p = m_camScript;
        while (*p) {
          CamWrite(p[0], p[1]);
          p += 2;
          // This delay helps miss a race condition in Omnivision-supplied configuration scripts
          // Omnivision is aware of this problem but could not suggest a workaround
          MicroWait(I2C_WAIT*12);
        }

        // Configure DMA2_Stream1 channel 1 for DMA from DCMI->DR to RAM
        DMA_DeInit(DMA2_Stream1);
        DMA_InitTypeDef DMA_InitStructure;
        DMA_InitStructure.DMA_Channel = DMA_Channel_1;
        DMA_InitStructure.DMA_PeripheralBaseAddr = (u32)&DCMI->DR;
        DMA_InitStructure.DMA_Memory0BaseAddr = (u32)m_buffer[0];
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
        
        // Set up double buffering
        DMA_DoubleBufferModeConfig(DMA2_Stream1, (u32)m_buffer[1], DMA_Memory_0);
        DMA_DoubleBufferModeCmd(DMA2_Stream1, ENABLE);

        // Enable the DMA interrupt for transfer complete to give enough time
        // between frames to do some extra work
        DMA_ITConfig(DMA2_Stream1, DMA_IT_TC, ENABLE);

        NVIC_InitTypeDef NVIC_InitStructure;
        NVIC_InitStructure.NVIC_IRQChannel = DMA2_Stream1_IRQn;
        NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
        NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
        NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
        NVIC_Init(&NVIC_InitStructure);

        // Enable DMA
        DMA_Cmd(DMA2_Stream1, ENABLE);

        // Enable DCMI
        DCMI_Cmd(ENABLE);

        // Enable the DCMI peripheral to capture frames from vsync
        DCMI_CaptureCmd(ENABLE);
        
        CamWrite(0x3008,0x02);  // Exit reset
        
        // // Let the I2C lines float after config
        //PIN_IN(GPIO_SDA, SOURCE_SDA);
        //PIN_IN(GPIO_SCL, SOURCE_SCL);
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
        
        // TODO: Check that the GPIOs are okay
        //for (u8 i = 1; i; i <<= 1)
        //  printf("\r\nCam dbus: set %x, got %x", i, CamReadDB(i));        
        
        // Configure XCLK for 12.85 MHz (evenly divisible by 180 MHz)
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

        // XXX-NDM:  I screwed up the duty cycle here to get 12MHz/15fps, so shoot me
        TIM_TimeBaseStructure.TIM_Period = 14;  // clockFrequency = 180MHz / (period+1)
        TIM_OCInitStructure.TIM_Pulse = 8;     // pulse = (period+1) / 2

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
        GPIO_SET(GPIO_PWDN, PIN_PWDN);
        GPIO_RESET(GPIO_RESET_N, PIN_RESET_N);
        GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
        GPIO_InitStructure.GPIO_Pin = PIN_PWDN | PIN_RESET_N;
        GPIO_Init(GPIOE, &GPIO_InitStructure);

        GPIO_RESET(GPIO_FSIN, PIN_FSIN);
        GPIO_InitStructure.GPIO_Pin = PIN_FSIN;
        GPIO_Init(GPIO_FSIN, &GPIO_InitStructure);

        // Initialize the I2C pins
        GPIO_SET(GPIO_SCL, PIN_SCL);
        GPIO_SET(GPIO_SDA, PIN_SDA);

        GPIO_InitStructure.GPIO_Pin = PIN_SCL | PIN_SDA;
        GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
        GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
        GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;
        GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
        GPIO_Init(GPIOE, &GPIO_InitStructure);       

        // Reset the camera
        MicroWait(50000);
        PIN_IN(GPIO_PWDN, SOURCE_PWDN);
        MicroWait(50000);
        PIN_IN(GPIO_RESET_N, SOURCE_RESET_N);
        MicroWait(100000);  // XXX-WHY?

        OV7739Init();
      }

      void CameraSetParameters(f32 exposure, bool enableVignettingCorrection)
      {
        //TODO: vignetting correction
        
        const f32 maxExposure = 0xf00; // Determined empirically
        
        f32 correctedExposure = exposure;
        
        if(exposure < 0.0f)
        {
          correctedExposure = 0;
        } else if(exposure > 1.0f)
        {
          correctedExposure = 1.0f;
        } 
        
        const u32 exposureU32 = (u32) floorf(correctedExposure * maxExposure + 0.5f);
        
        if (m_exposure != exposureU32)
        {
          m_exposure = exposureU32;
          
          CamWrite(0x3501, (exposureU32 >> 8) & 0xFF);
          MicroWait(100);
          CamWrite(0x3502, exposureU32 & 0xFF);
        }

        if(enableVignettingCorrection) {
          AnkiAssert(false);
        }
        
        /*
        if(m_enableVignettingCorrection != enableVignettingCorrection)
        {
          m_enableVignettingCorrection = enableVignettingCorrection;

          const u8 newValue = enableVignettingCorrection ? 0x01 : 0x00;

          MicroWait(100);
          // XXX-NDM was OV7739 CamWrite(0x46, newValue);
        }
        */
      }

      void CameraGetFrame(u8* frame, Vision::CameraResolution res, bool enableLight)
      {
        m_isEOF = false;

        // Wait until the frame has completed (based on DMA_FLAG_TCIF1)
        while (!m_isEOF)
        {  
        }
        
        return;
      }

      const CameraInfo* GetHeadCamInfo(void)
      {
        static CameraInfo s_headCamInfo = {
          HEAD_CAM_CALIB_FOCAL_LENGTH_X,
          HEAD_CAM_CALIB_FOCAL_LENGTH_Y,
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

  m_readyBuffer = !m_readyBuffer;   // Other buffer is now the ready one
  m_readyRow += BUFFER_ROWS;        // A new row is now ready
  if (m_readyRow >= TOTAL_ROWS)
    m_readyRow = 0;
}

/*extern "C"
void DCMI_IRQHandler(void)
{
  using namespace Anki::Cozmo::HAL;

  //m_isEOF = true;

  // Clear all interrupts (5-bits)
  DCMI->ICR = 0x1f;
}*/
