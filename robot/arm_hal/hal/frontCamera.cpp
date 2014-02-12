#include "lib/stm32f4xx.h"
#include "anki/cozmo/robot/hal.h"
#include "hal/portable.h"

#define DCMI_TIMEOUT_MAX  (10000)

#define OV2640_DEVICE_WRITE_ADDRESS    0x60
#define OV2640_DEVICE_READ_ADDRESS     0x61

#define I2C_ADDR OV2640_DEVICE_WRITE_ADDRESS

GPIO_PIN_SOURCE(D0, GPIOC, 6);
GPIO_PIN_SOURCE(D1, GPIOC, 7);
GPIO_PIN_SOURCE(D2, GPIOC, 8);
GPIO_PIN_SOURCE(D3, GPIOC, 9);
GPIO_PIN_SOURCE(D4, GPIOC, 11);
GPIO_PIN_SOURCE(D5, GPIOD, 3);
GPIO_PIN_SOURCE(D6, GPIOB, 8);
GPIO_PIN_SOURCE(D7, GPIOE, 6);
//GPIO_PIN_SOURCE(D8, GPIOC, 10);
//GPIO_PIN_SOURCE(D9, GPIOC, 12);
//GPIO_PIN_SOURCE(D10, GPIOD, 6);
//GPIO_PIN_SOURCE(D11, GPIOD, 2);
GPIO_PIN_SOURCE(VSYNC, GPIOB, 7);
GPIO_PIN_SOURCE(HSYNC, GPIOA, 4);
GPIO_PIN_SOURCE(PCLK, GPIOA, 6);
//GPIO_PIN_SOURCE(RESET, ...);  // No reset pin on the eval board, do software reset

GPIO_PIN_SOURCE(SCL, GPIOB, 6);
GPIO_PIN_SOURCE(SDA, GPIOB, 9);

bool isEOF = false;
u8 m_buffer1[320*240 * 4] __attribute__((align(32)));
//static u8 m_buffer2[320*240*2];

namespace Anki
{
  namespace Cozmo
  {
    namespace HAL
    {
      const unsigned char OV2640_QVGA[][2]=
      {
        // Switch to Bank 0
        0xff, 0x00,
        0x2c, 0xff,  // Reserved
        0x2e, 0xdf,  // Reserved
        
        // Switch to Bank 1
        0xff, 0x01,
        0x3c, 0x32,  // Reserved
        0x11, 0x00,  // CLRKC
        0x09, 0x02,  // COM2 - Output drive = 2x
        0x04, 0x28,  // REG04 - HREF bit[0] set
        0x13, 0xe5,  // COM8 - Banding filter | AGC auto | Exposure auto
        0x14, 0x48,  // COM9 - AGC gain ceiling
        0x2c, 0x0c,  // Reserved
        0x33, 0x78,  // Reserved
        0x3a, 0x33,  // Reserved
        0x3b, 0xfB,  // Reserved
        0x3e, 0x00,  // Reserved
        0x43, 0x11,  // Reserved
        0x16, 0x10,  // Reserved
        0x4a, 0x81,  // Reserved
        0x21, 0x99,  // Reserved
        0x24, 0x40,  // AEW
        0x25, 0x38,  // AEB
        0x26, 0x82,  // VV
        0x5c, 0x00,  // Reserved
        0x63, 0x00,  // Reserved
        0x46, 0x3f,  // FLL - Frame Length LSBs
        0x0c, 0x3c,  // COM3 - 50Hz | Auto set banding
        0x61, 0x70,  // HISTO_LOW
        0x62, 0x80,  // HISTO_HIGH
        0x7c, 0x05,  // Reserved
        0x20, 0x80,  // Reserved
        0x28, 0x30,  // Reserved
        0x6c, 0x00,  // Reserved
        0x6d, 0x80,  // Reserved
        0x6e, 0x00,  // Reserved
        0x70, 0x02,  // Reserved
        0x71, 0x94,  // Reserved
        0x73, 0xc1,  // Reserved
        0x3d, 0x34,  // Reserved
        0x5a, 0x57,  // Reserved
        0x12, 0x00,  // COM7 - UXGA
        0x11, 0x00,  // CLKRC
        0x17, 0x11,  // HREFST - bit[10:3]  == 0x8e == 148
        0x18, 0x75,  // HREFEND - UXGA - bit[10:3], each LSB represents 2 pixels == 944 -> 796
        0x19, 0x01,  // VSTRT - UXGA - bit[9:2] == 10?
        0x1a, 0x97,  // VEND - bit[9:2] == 610 -> 600
        0x32, 0x36,  // REG2 - bit[5:3] = HREFEND[2:0], bit[2:0] = HREFST[2:0]
        0x03, 0x0f,  // COM1 - bit[3:2] = VEND[1:0], bit[1:0] = VSTRT[1:0]
        0x37, 0x40,  // Reserved
        0x4f, 0xbb,  // 50Hz Banding AEC 8 LSBs
        0x50, 0x9c,  // 60Hz Banding AEC 8 LSBs
        0x5a, 0x57,  // Reserved
        0x6d, 0x80,  // Reserved
        0x6d, 0x38,  // Reserved
        0x39, 0x02,  // Reserved
        0x35, 0x88,  // Reserved
        0x22, 0x0a,  // Reserved
        0x37, 0x40,  // Reserved
        0x23, 0x00,  // Reserved
        0x34, 0xa0,  // ARCOM2
        0x36, 0x1a,  // Reserved
        0x06, 0x02,  // Reserved
        0x07, 0xc0,  // Reserved
        0x0d, 0xb7,  // COM4 - Clock output power-down pin status
        0x0e, 0x01,  // Reserved
        0x4c, 0x00,  // Reserved
        
        0x15, 0x00,  // COM10 - HREF,VSYNC polarity options, etc
        
        // Switch to Bank 0
        0xff, 0x00,
        0xe5, 0x7f,  // Reserved
        0xf9, 0xc0,  // MC_BIST - Microcontroller Reset | Boot ROM select
        0x41, 0x24,  // Reserved
        0xe0, 0x14,  // RESET - JPEG | DVP
        0x76, 0xff,  // Reserved
        0x33, 0xa0,  // Reserved
        0x42, 0x20,  // Reserved
        0x43, 0x18,  // Reserved
        0x4c, 0x00,  // Reserved
        0x87, 0xd0,  // CTRL3 - Module Enable: BPC | WPC
        0x88, 0x3f,  // Reserved
        0xd7, 0x03,  // Reserved
        0xd9, 0x10,  // Reserved
        0xd3, 0x82,  // R_DVP_SP - Auto mode | DVP PCLK = sysclk (48)/[6:0] (YUV0) or 48/(2**[6:0])
        0xc8, 0x08,
        0xc9, 0x80,
        0x7d, 0x00,
        0x7c, 0x03,
        0x7d, 0x48,
        0x7c, 0x08,
        0x7d, 0x20,
        0x7d, 0x10,
        0x7d, 0x0e,
        0x90, 0x00,
        0x91, 0x0e,
        0x91, 0x1a,
        0x91, 0x31,
        0x91, 0x5a,
        0x91, 0x69,
        0x91, 0x75,
        0x91, 0x7e,
        0x91, 0x88,
        0x91, 0x8f,
        0x91, 0x96,
        0x91, 0xa3,
        0x91, 0xaf,
        0x91, 0xc4,
        0x91, 0xd7,
        0x91, 0xe8,
        0x91, 0x20,
        0x92, 0x00,
        0x93, 0x06,
        0x93, 0xe3,
        0x93, 0x02,
        0x93, 0x02,
        0x93, 0x00,
        0x93, 0x04,
        0x93, 0x00,
        0x93, 0x03,
        0x93, 0x00,
        0x93, 0x00,
        0x93, 0x00,
        0x93, 0x00,
        0x93, 0x00,
        0x93, 0x00,
        0x93, 0x00,
        0x96, 0x00,
        0x97, 0x08,
        0x97, 0x19,
        0x97, 0x02,
        0x97, 0x0c,
        0x97, 0x24,
        0x97, 0x30,
        0x97, 0x28,
        0x97, 0x26,
        0x97, 0x02,
        0x97, 0x98,
        0x97, 0x80,
        0x97, 0x00,
        0x97, 0x00,
        0xc3, 0xef,
        
        // Switch to Bank 0
        0xff, 0x00,
        0xba, 0xdc,
        0xbb, 0x08,
        0xb6, 0x24,
        0xb8, 0x33,
        0xb7, 0x20,
        0xb9, 0x30,
        0xb3, 0xb4,
        0xb4, 0xca,
        0xb5, 0x43,
        0xb0, 0x5c,
        0xb1, 0x4f,
        0xb2, 0x06,
        0xc7, 0x00,
        0xc6, 0x51,
        0xc5, 0x11,
        0xc4, 0x9c,
        0xbf, 0x00,
        0xbc, 0x64,
        0xa6, 0x00,
        0xa7, 0x1e,
        0xa7, 0x6b,
        0xa7, 0x47,
        0xa7, 0x33,
        0xa7, 0x00,
        0xa7, 0x23,
        0xa7, 0x2e,
        0xa7, 0x85,
        0xa7, 0x42,
        0xa7, 0x33,
        0xa7, 0x00,
        0xa7, 0x23,
        0xa7, 0x1b,
        0xa7, 0x74,
        0xa7, 0x42,
        0xa7, 0x33,
        0xa7, 0x00,
        0xa7, 0x23,
        0xc0, 0xc8,
        0xc1, 0x96,
        0x8c, 0x00,
        0x86, 0x3d,
        0x50, 0x92,
        0x51, 0x90,
        0x52, 0x2c,
        0x53, 0x00,
        0x54, 0x00,
        0x55, 0x88,
        0x5a, 0x50,
        0x5b, 0x3c,
        0x5c, 0x00,
        0xd3, 0x04,
        0x7f, 0x00,
        0xda, 0x00,
        0xe5, 0x1f,
        0xe1, 0x67,
        0xe0, 0x00,
        0xdd, 0x7f,
        0x05, 0x00,
        
        // Switch to Bank 0
        0xff, 0x00,
        0xe0, 0x04,
        0xc0, 0xc8,
        0xc1, 0x96,
        0x86, 0x3d,
        0x50, 0x92,
        0x51, 0x90,
        0x52, 0x2c,
        0x53, 0x00,
        0x54, 0x00,
        0x55, 0x88,
        0x57, 0x00,
        0x5a, 0x50,
        0x5b, 0x3c,
        0x5c, 0x00,
        0xd3, 0x04,
        0xe0, 0x00,
        
        // Switch to Bank 0
        0xff, 0x00,
        0x05, 0x00,
        0xda, 0x00,  // IMAGE_MODE - YUV422
        //0xda, 0x09,
        0x98, 0x00,
        0x99, 0x00,
        0x00, 0x00, // */
    


/*        0xff, 0x00, 
        0x2c, 0xff, 
        0x2e, 0xdf, 
        
        0xff, 0x01, 
        0x3c, 0x32, 
        0x11, 0x00,  // CLKRC
        0x09, 0x02, 
        0x04, 0x28, 
        0x13, 0xe5, 
        0x14, 0x48, 
        0x2c, 0xc, 
        0x33, 0x78, 
        0x3a, 0x33, 
        0x3b, 0xfb, 
        0x3e, 0x0, 
        0x43, 0x11, 
        0x16, 0x10, 
        0x39, 0x2, 
        0x35, 0x88, 

        0x22, 0xa, 
        0x37, 0x40, 
        0x23, 0x0, 
        0x34, 0xa0, 
        0x6, 0x2, 
        0x6, 0x88, 
        0x7, 0xc0, 
        0xd, 0xb7, 
        0xe, 0x1, 
        0x4c, 0x0, 
        0x4a, 0x81, 
        0x21, 0x99, 
        0x24, 0x40, 
        0x25, 0x38, 
        0x26, 0x82, 
        0x5c, 0x0, 
        0x63, 0x0, 
        0x46, 0x22, 
        0xc, 0x3a, 
        0x5d, 0x55, 
        0x5e, 0x7d, 
        0x5f, 0x7d, 
        0x60, 0x55, 
        0x61, 0x70, 
        0x62, 0x80, 
        0x7c, 0x5, 
        0x20, 0x80, 
        0x28, 0x30, 
        0x6c, 0x0, 
        0x6d, 0x80, 
        0x6e, 0x0, 
        0x70, 0x2, 
        0x71, 0x94, 
        0x73, 0xc1, 
        0x3d, 0x34, 
        0x12, 0x4, 
        0x5a, 0x57, 
        0x4f, 0xbb, 
        0x50, 0x9c, 
        
        //0x15, 0x00,  // COM10
        
        0xff, 0x0, 
        0xe5, 0x7f, 
        0xf9, 0xc0, 
        0x41, 0x24, 
        0xe0, 0x14, 
        0x76, 0xff, 
        0x33, 0xa0, 
        0x42, 0x20, 
        0x43, 0x18, 
        0x4c, 0x0, 
        0x87, 0xd0, 
        0x88, 0x3f, 
        0xd7, 0x3, 
        0xd9, 0x10, 
        0xd3, 0x82, 
        0xc8, 0x8, 
        0xc9, 0x80, 
        0x7c, 0x0, 
        0x7d, 0x0, 
        0x7c, 0x3, 
        0x7d, 0x48, 
        0x7d, 0x48, 
        0x7c, 0x8, 
        0x7d, 0x20, 
        0x7d, 0x10, 
        0x7d, 0xe, 
        0x90, 0x0, 
        0x91, 0xe, 
        0x91, 0x1a, 
        0x91, 0x31, 
        0x91, 0x5a, 
        0x91, 0x69, 
        0x91, 0x75, 
        0x91, 0x7e, 
        0x91, 0x88, 
        0x91, 0x8f, 
        0x91, 0x96, 
        0x91, 0xa3, 
        0x91, 0xaf, 
        0x91, 0xc4, 
        0x91, 0xd7, 
        0x91, 0xe8, 
        0x91, 0x20, 
        0x92, 0x0, 

        0x93, 0x6, 
        0x93, 0xe3, 
        0x93, 0x3, 
        0x93, 0x3, 
        0x93, 0x0, 
        0x93, 0x2, 
        0x93, 0x0, 
        0x93, 0x0, 
        0x93, 0x0, 
        0x93, 0x0, 
        0x93, 0x0, 
        0x93, 0x0, 
        0x93, 0x0, 
        0x96, 0x0, 
        0x97, 0x8, 
        0x97, 0x19, 
        0x97, 0x2, 
        0x97, 0xc, 
        0x97, 0x24, 
        0x97, 0x30, 
        0x97, 0x28, 
        0x97, 0x26, 
        0x97, 0x2, 
        0x97, 0x98, 
        0x97, 0x80, 
        0x97, 0x0, 
        0x97, 0x0, 
        0xa4, 0x0, 
        0xa8, 0x0, 
        0xc5, 0x11, 
        0xc6, 0x51, 
        0xbf, 0x80, 
        0xc7, 0x10, 
        0xb6, 0x66, 
        0xb8, 0xa5, 
        0xb7, 0x64, 
        0xb9, 0x7c, 
        0xb3, 0xaf, 
        0xb4, 0x97, 
        0xb5, 0xff, 
        0xb0, 0xc5, 
        0xb1, 0x94, 
        0xb2, 0xf, 
        0xc4, 0x5c, 
        0xa6, 0x0, 
        0xa7, 0x20, 
        0xa7, 0xd8, 
        0xa7, 0x1b, 
        0xa7, 0x31, 
        0xa7, 0x0, 
        0xa7, 0x18, 
        0xa7, 0x20, 
        0xa7, 0xd8, 
        0xa7, 0x19, 
        0xa7, 0x31, 
        0xa7, 0x0, 
        0xa7, 0x18, 
        0xa7, 0x20, 
        0xa7, 0xd8, 
        0xa7, 0x19, 
        0xa7, 0x31, 
        0xa7, 0x0, 
        0xa7, 0x18, 
        0x7f, 0x0, 
        0xe5, 0x1f, 
        0xe1, 0x77, 
        0xdd, 0x7f, 
        0xc2, 0xe, 
        
        0xff, 0x0, 
        0xe0, 0x4, 
        0xc0, 0xc8, 
        0xc1, 0x96, 
        0x86, 0x3d, 
        0x51, 0x90, 
        0x52, 0x2c, 
        0x53, 0x0, 
        0x54, 0x0, 
        0x55, 0x88, 
        0x57, 0x0, 
        
        0x50, 0x92, 
        0x5a, 0x50, 
        0x5b, 0x3c, 
        0x5c, 0x00, 
        0xd3, 0x04, 
        0xe0, 0x00, 
        
        0xff, 0x0, 
        0x05, 0x00, 
        
        0xda, 0x00, //0x08, 
        0xd7, 0x3, 
        0xe0, 0x0, 
        
        0x05, 0x00,   // Bypass DSP = off/on

        
        0xff,0xff, */
      };
      
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

      void OV2640Init()
      {
        // Configure the camera interface
        DCMI_InitTypeDef DCMI_InitStructure;
        DCMI_InitStructure.DCMI_CaptureMode = DCMI_CaptureMode_SnapShot;
        DCMI_InitStructure.DCMI_SynchroMode = DCMI_SynchroMode_Hardware;
        DCMI_InitStructure.DCMI_PCKPolarity = DCMI_PCKPolarity_Rising;
        DCMI_InitStructure.DCMI_VSPolarity = DCMI_VSPolarity_Low;
        DCMI_InitStructure.DCMI_HSPolarity = DCMI_HSPolarity_Low;
        DCMI_InitStructure.DCMI_CaptureRate = DCMI_CaptureRate_All_Frame;
        DCMI_InitStructure.DCMI_ExtendedDataMode = DCMI_ExtendedDataMode_8b;
        DCMI_Init(&DCMI_InitStructure);
        
        // Configure DMA2_Stream1 channel 1 for DMA from DCMI->DR to RAM
        DMA_DeInit(DMA2_Stream1);
        DMA_InitTypeDef DMA_InitStructure;
        DMA_InitStructure.DMA_Channel = DMA_Channel_1;
        DMA_InitStructure.DMA_PeripheralBaseAddr = (u32)&DCMI->DR;
        DMA_InitStructure.DMA_Memory0BaseAddr = (u32)m_buffer1;
        DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralToMemory;
        DMA_InitStructure.DMA_BufferSize = 320*240 * 4;
        DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
        DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
        DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Word;
        DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Word;
        DMA_InitStructure.DMA_Mode = DMA_Mode_Normal; //Circular;
        DMA_InitStructure.DMA_Priority = DMA_Priority_VeryHigh;
        DMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Enable;
        DMA_InitStructure.DMA_FIFOThreshold = DMA_FIFOThreshold_Full;
        DMA_InitStructure.DMA_MemoryBurst = DMA_MemoryBurst_Single;
        DMA_InitStructure.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;
        DMA_Init(DMA2_Stream1, &DMA_InitStructure);
        
        // Reset the camera
        CamWrite(0xFF, 0x01);  // OV2640_DSP_RA_DLMT
        CamWrite(0x12, 0x80);  // OV2640_SENSOR_COM7
        MicroWait(2000);
        
        u8 val = CamRead(0x04);
        printf("val == %02X\r\n", val);
        
        // Write the configuration registers
        for(u32 i = 0; i < (sizeof(OV2640_QVGA) / 2); i++)
        {
          CamWrite(OV2640_QVGA[i][0], OV2640_QVGA[i][1]);
          MicroWait(2000);
        }
        
        // Enable DMA
        DMA_Cmd(DMA2_Stream1, ENABLE);
        
        // Enable the DCMI interrupt for the end of frame
        DCMI_ITConfig(DCMI_IT_FRAME, ENABLE);
        
        NVIC_InitTypeDef NVIC_InitStructure;
        NVIC_InitStructure.NVIC_IRQChannel = DCMI_IRQn;
        NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
        NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
        NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
        NVIC_Init(&NVIC_InitStructure);
        
        // Enable DCMI
        DCMI_Cmd(ENABLE);
        
        // Wait 200ms
        MicroWait(200000);
        
        // Enable the DCMI peripheral to capture frames from vsync
        DCMI_CaptureCmd(ENABLE);
      }
      
      void FrontCameraInit()
      {
        // Clock configuration
        RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
        RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);
        RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);
        RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);
        RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOE, ENABLE);
        
        RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA2, ENABLE);
        RCC_AHB2PeriphClockCmd(RCC_AHB2Periph_DCMI, ENABLE);
        
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
        
        // Initialize the camera pins
        GPIO_InitTypeDef GPIO_InitStructure;
        GPIO_InitStructure.GPIO_Pin = PIN_D0 | PIN_D1 | PIN_D2 | PIN_D3 | PIN_D4;
        GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
        GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
        GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
        GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
        GPIO_Init((GPIO_TypeDef*)GPIO_D0, &GPIO_InitStructure);  // GPIOC
        
        GPIO_InitStructure.GPIO_Pin = PIN_D5;
        GPIO_Init((GPIO_TypeDef*)GPIO_D5, &GPIO_InitStructure);  // GPIOD
        
        GPIO_InitStructure.GPIO_Pin = PIN_D6 | PIN_VSYNC;
        GPIO_Init((GPIO_TypeDef*)GPIO_D6, &GPIO_InitStructure);  // GPIOB
        
        GPIO_InitStructure.GPIO_Pin = PIN_D7;
        GPIO_Init((GPIO_TypeDef*)GPIO_D7, &GPIO_InitStructure);  // GPIOE
        
        GPIO_InitStructure.GPIO_Pin = PIN_HSYNC | PIN_PCLK;
        GPIO_Init((GPIO_TypeDef*)GPIO_HSYNC, &GPIO_InitStructure);  // GPIOA
        
        // Initialize the I2C pins
        GPIO_SET(GPIO_SCL, PIN_SCL);
        GPIO_SET(GPIO_SDA, PIN_SDA);
        
        GPIO_InitStructure.GPIO_Pin = PIN_SCL | PIN_SDA;
        GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
        GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
        GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;
        GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
        GPIO_Init((GPIO_TypeDef*)GPIO_SCL, &GPIO_InitStructure);  // GPIOB
        
        OV2640Init();
      }
    }
  }
}

void StartFrame(void)
{
  isEOF = false;
  
  DMA_DeInit(DMA2_Stream1);
  DMA_InitTypeDef DMA_InitStructure;
  DMA_InitStructure.DMA_Channel = DMA_Channel_1;
  DMA_InitStructure.DMA_PeripheralBaseAddr = (u32)&DCMI->DR;
  DMA_InitStructure.DMA_Memory0BaseAddr = (u32)m_buffer1;
  DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralToMemory;
  DMA_InitStructure.DMA_BufferSize = 320*240 * 4;
  DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
  DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
  DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Word;
  DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Word;
  DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
  DMA_InitStructure.DMA_Priority = DMA_Priority_VeryHigh;
  DMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Enable;
  DMA_InitStructure.DMA_FIFOThreshold = DMA_FIFOThreshold_Full;
  DMA_InitStructure.DMA_MemoryBurst = DMA_MemoryBurst_Single;
  DMA_InitStructure.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;
  DMA_Init(DMA2_Stream1, &DMA_InitStructure);
        
  DMA_Cmd(DMA2_Stream1, ENABLE);
  
  DCMI_CaptureCmd(ENABLE);
}

extern "C"
void DCMI_IRQHandler(void)
{
  /*static u32 x = 0;
  if (x++ >= 100)
  {
    DMA_DeInit(DMA2_Stream1);
    
    NVIC_InitTypeDef NVIC_InitStructure;
    NVIC_InitStructure.NVIC_IRQChannel = DCMI_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = DISABLE;
    NVIC_Init(&NVIC_InitStructure);
  }*/
  
  //printf("FRAME\r\n")
  
  isEOF = true;
  
  // Clear all interrupts (5-bits)
  DCMI->ICR = 0x1f;
}
