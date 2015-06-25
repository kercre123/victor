// Simple (slow) OLED driver to check out the display
// TODO:  Implement DMA before attempting animation
#include "lib/stm32f4xx.h"
#include "anki/cozmo/robot/hal.h"
#include "hal/portable.h"

// Definitions for SSD1306 controller
#define LCDWIDTH                  128
#define LCDHEIGHT                 64

#define SETCONTRAST 0x81
#define DISPLAYALLON_RESUME 0xA4
#define DISPLAYALLON 0xA5
#define NORMALDISPLAY 0xA6
#define INVERTDISPLAY 0xA7
#define DISPLAYOFF 0xAE
#define DISPLAYON 0xAF

#define SETDISPLAYOFFSET 0xD3
#define SETCOMPINS 0xDA

#define SETVCOMDETECT 0xDB

#define SETDISPLAYCLOCKDIV 0xD5
#define SETPRECHARGE 0xD9

#define SETMULTIPLEX 0xA8

#define SETLOWCOLUMN 0x00
#define SETHIGHCOLUMN 0x10

#define SETSTARTLINE 0x40

#define MEMORYMODE 0x20
#define COLUMNADDR 0x21
#define PAGEADDR   0x22

#define COMSCANINC 0xC0
#define COMSCANDEC 0xC8

#define SEGREMAP 0xA0

#define CHARGEPUMP 0x8D

#define EXTERNALVCC 0x1
#define SWITCHCAPVCC 0x2

#define ACTIVATE_SCROLL 0x2F
#define DEACTIVATE_SCROLL 0x2E
#define SET_VERTICAL_SCROLL_AREA 0xA3
#define RIGHT_HORIZONTAL_SCROLL 0x26
#define LEFT_HORIZONTAL_SCROLL 0x27
#define VERTICAL_AND_RIGHT_HORIZONTAL_SCROLL 0x29
#define VERTICAL_AND_LEFT_HORIZONTAL_SCROLL 0x2A

namespace Anki
{
  namespace Cozmo
  {
    namespace HAL
    {       
      GPIO_PIN_SOURCE(SCK, GPIOB, 3);
      GPIO_PIN_SOURCE(MOSI, GPIOB, 5);
      
      GPIO_PIN_SOURCE(CS, GPIOC, 12);   
      GPIO_PIN_SOURCE(CMD, GPIOD, 6);
      GPIO_PIN_SOURCE(RES, GPIOD, 7);
      
      static void HWInit(void)
      {
        // Clock configuration
        RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);
        RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);
        RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);
        
        RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI3, ENABLE);
        
        GPIO_InitTypeDef GPIO_InitStructure;
        GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
        GPIO_InitStructure.GPIO_Speed = GPIO_Speed_25MHz;
        GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
        GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_NOPULL;
                
        // Configure the pins for SPI in AF mode
        GPIO_PinAFConfig(GPIO_SCK, SOURCE_SCK, GPIO_AF_SPI3);
        GPIO_PinAFConfig(GPIO_MOSI, SOURCE_MOSI, GPIO_AF_SPI3);
        
        // Configure the SPI pins
        GPIO_InitStructure.GPIO_Pin = PIN_SCK | PIN_MOSI;
        GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
        GPIO_Init(GPIO_SCK, &GPIO_InitStructure);
        
        GPIO_SET(GPIO_CS, PIN_CS);  // Force CS high
        GPIO_InitStructure.GPIO_Pin = PIN_CS;
        GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
        GPIO_Init(GPIO_CS, &GPIO_InitStructure);

        GPIO_InitStructure.GPIO_Pin = PIN_CMD;
        GPIO_Init(GPIO_CMD, &GPIO_InitStructure);
        
        GPIO_RESET(GPIO_RES, PIN_RES);  // Force RESET low
        GPIO_InitStructure.GPIO_Pin = PIN_RES;
        GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
        GPIO_Init(GPIO_RES, &GPIO_InitStructure);

        // Initialize SPI in master mode
        SPI_I2S_DeInit(SPI3);
        SPI_InitTypeDef SPI_InitStructure;
        SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
        SPI_InitStructure.SPI_Mode = SPI_Mode_Master;
        SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;
        SPI_InitStructure.SPI_CPOL = SPI_CPOL_High;
        SPI_InitStructure.SPI_CPHA = SPI_CPHA_2Edge;
        SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;
        SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_128;
        SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
        SPI_InitStructure.SPI_CRCPolynomial = 7;
        SPI_Init(SPI3, &SPI_InitStructure);
        SPI_Cmd(SPI3, ENABLE);      
        
        // Exit reset after 10ms
        MicroWait(10000);
        GPIO_SET(GPIO_RES, PIN_RES);
      }
      
      // Slow write - to improve performance, switch to DMA!
      static void SPIWrite(u8 data)
      {
        MicroWait(1);
        GPIO_RESET(GPIO_CS, PIN_CS);
        MicroWait(1);
        while (!(SPI3->SR & SPI_FLAG_TXE))
            ;
        SPI3->DR = data;
        while (!(SPI3->SR & SPI_FLAG_TXE))
            ;
        while (SPI3->SR & SPI_FLAG_BSY)
          ;
        GPIO_SET(GPIO_CS, PIN_CS); 
      }
      
      static void SendCommand(u8 cmd)
      {
        GPIO_RESET(GPIO_CMD, PIN_CMD);
        SPIWrite(cmd);    
      }

      static u8 m_frame[LCDHEIGHT/8][LCDWIDTH];
      static void SendFrame(void)
      {
        SendCommand(COLUMNADDR);
        SendCommand(0);   // Column start address (0 = reset)
        SendCommand(LCDWIDTH-1); // Column end address (127 = reset)

        SendCommand(PAGEADDR);
        SendCommand(0); // Page start address (0 = reset)
        SendCommand(7); // Page end address

        GPIO_SET(GPIO_CMD, PIN_CMD);
        u8* p = m_frame[0];
        for (u16 i=0; i<sizeof(m_frame); i++)
          SPIWrite(*p++);
      }
      
      void OLEDInit(void)
      {
        HWInit();
  
        // Init sequence for 128x64 OLED module
        SendCommand(DISPLAYOFF);                    // 0xAE
        SendCommand(SETDISPLAYCLOCKDIV);            // 0xD5
        SendCommand(0xF0);                                  // set max clock rate for most stable image (suggested is 0x80)
        SendCommand(SETMULTIPLEX);                  // 0xA8
        SendCommand(0x3F);
        SendCommand(SETDISPLAYOFFSET);              // 0xD3
        SendCommand(0x0);                                   // no offset
        SendCommand(SETSTARTLINE | 0x0);            // line #0
        SendCommand(CHARGEPUMP);                    // 0x8D
        SendCommand(0x14);                  // No external VCC - charge pump ON
        SendCommand(MEMORYMODE);                    // 0x20
        SendCommand(0x00);                                  // 0x0 act like ks0108
        SendCommand(SEGREMAP | 0x1);
        SendCommand(COMSCANDEC);
        SendCommand(SETCOMPINS);                    // 0xDA
        SendCommand(0x12);
        SendCommand(SETCONTRAST);                   // 0x81
        SendCommand(0xCF);                  // No external VCC
        SendCommand(SETPRECHARGE);                  // 0xd9
        SendCommand(0xF1);                  // No external VCC
        SendCommand(SETVCOMDETECT);                 // 0xDB
        SendCommand(0x40);
        SendCommand(DISPLAYALLON_RESUME);           // 0xA4
        SendCommand(NORMALDISPLAY);                 // 0xA6
        
        // Now light it up
        SendCommand(DISPLAYON);
        
        // Draw "programmer art" face until we get real assets
        memset(m_frame, 0, sizeof(m_frame));
        for (int x = 24; x < 40; x++)
        {
          m_frame[3][x] = 0xaa;
          m_frame[4][x] = 0xaa;
          m_frame[3][x+64] = 0xaa;
          m_frame[4][x+64] = 0xaa;
        }
        SendFrame();
      }
    }
  }
}
