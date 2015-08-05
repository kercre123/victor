// High speed DMA-driven face animation
#include "lib/stm32f4xx.h"
#include "anki/cozmo/robot/hal.h"
#include "anki/cozmo/shared/faceDisplayDecode.h"
#include "hal/portable.h"

#include <math.h>

// Definitions for SSD1306 controller
#define COLS                  128
#define ROWS                  64

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

#define DISPLAY_SPI             SPI3
#define DISPLAY_DMA_CHANNEL     DMA_Channel_0
#define DISPLAY_DMA_STREAM      DMA1_Stream7
#define DISPLAY_DMA_IRQ         DMA1_Stream7_IRQn
#define DISPLAY_DMA_IRQ_HANDLER DMA1_Stream7_IRQHandler


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

      enum DataCommand {
        DATA,
        COMMAND
      };

      struct DisplayCommand {
        DataCommand direction;
        int length;
        const void* data;
      };

      // Set to disable face animation - in case of FacePrintf
      static u8 m_disableAnimate = 0;

      static ONCHIP uint64_t m_frame[COLS];

      static ONCHIP const u8 InitDisplay[] = {
        DISPLAYOFF,
        SETDISPLAYCLOCKDIV, 0xF0,
        SETMULTIPLEX, 0x3F,
        SETDISPLAYOFFSET, 0x0,
        SETSTARTLINE | 0x0,
        CHARGEPUMP, 0x14,
        MEMORYMODE, 0x01,
        SEGREMAP | 0x1,
        COMSCANDEC,
        SETCOMPINS, 0x12,
        SETCONTRAST, 0xCF,
        SETPRECHARGE, 0xF1,
        SETVCOMDETECT, 0x40,
        DISPLAYALLON_RESUME,
        NORMALDISPLAY,
        DISPLAYON
      };
      static ONCHIP const u8 ResetCursor[] = {
        COLUMNADDR, 0, COLS-1,
        PAGEADDR, 0, 7
      };

      const static int FIFO_SIZE = 4;
      static DisplayCommand fifo[FIFO_SIZE];
      static volatile int fifo_entry;
      static volatile int fifo_exit;
      static volatile int fifo_count;

      static void HWInit(void)
      {
        // Clock configuration
        RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);
        RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);
        RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);

        RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI3, ENABLE);
        RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA1, ENABLE);

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
        SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_8;
        SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
        SPI_InitStructure.SPI_CRCPolynomial = 7;
        SPI_Init(SPI3, &SPI_InitStructure);
        SPI_Cmd(SPI3, ENABLE);

        // Initalize the dma
        DMA_InitTypeDef DMA_InitStructure;
        DMA_InitStructure.DMA_Channel = DISPLAY_DMA_CHANNEL;
        DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&DISPLAY_SPI->DR;
        DMA_InitStructure.DMA_DIR = DMA_DIR_MemoryToPeripheral;
        DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
        DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
        DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
        DMA_InitStructure.DMA_MemoryDataSize = DMA_PeripheralDataSize_Byte;
        DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
        DMA_InitStructure.DMA_Priority = DMA_Priority_High;
        DMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Disable;
        DMA_InitStructure.DMA_FIFOThreshold = DMA_FIFOThreshold_1QuarterFull;
        DMA_InitStructure.DMA_MemoryBurst = DMA_MemoryBurst_Single;
        DMA_InitStructure.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;
        DMA_Init(DISPLAY_DMA_STREAM, &DMA_InitStructure);

        // Configure interrupts
        NVIC_InitTypeDef NVIC_InitStructure;
        NVIC_InitStructure.NVIC_IRQChannel = DISPLAY_DMA_IRQ;
        NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
        NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
        NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
        NVIC_Init(&NVIC_InitStructure);

        // Enable transfer complete
        DMA_ITConfig(DISPLAY_DMA_STREAM, DMA_IT_TC, ENABLE);
        NVIC_EnableIRQ(DISPLAY_DMA_IRQ);

        SPI_I2S_DMACmd(DISPLAY_SPI, SPI_I2S_DMAReq_Tx, ENABLE);

        // Exit reset after 10ms
        MicroWait(10000);
        GPIO_SET(GPIO_RES, PIN_RES);

        fifo_entry = 0;
        fifo_exit = 0;
        fifo_count = 0;

        SPI3->SR = 0;
      }

      static void DequeueDMA()
      {
        if (!fifo_count) {

          GPIO_SET(GPIO_CS, PIN_CS);
          return ;
        }

        DisplayCommand *in = &fifo[fifo_entry];
        fifo_entry = (fifo_entry + 1) % FIFO_SIZE;
        fifo_count--;

        switch(in->direction) {
          case DATA:
            GPIO_SET(GPIO_CMD, PIN_CMD);
            break ;
          case COMMAND:
            GPIO_RESET(GPIO_CMD, PIN_CMD);
            break ;
        }

        // Select chip
        GPIO_RESET(GPIO_CS, PIN_CS);
        MicroWait(1);

        DISPLAY_DMA_STREAM->NDTR = in->length;    // Buffer size
        DISPLAY_DMA_STREAM->M0AR = (u32)in->data; // Buffer address
        DISPLAY_DMA_STREAM->CR |= DMA_SxCR_EN;    // Enable DMA
      }

      static bool EnqueueWrite(DataCommand mode, const void* data, int length) {
        // Fifo is full
        if (fifo_count >= FIFO_SIZE) {
          return false;
        }

        // Enqueue data in SPI buffer
        __disable_irq();
        DisplayCommand *out = &fifo[fifo_exit];
        fifo_exit = (fifo_exit + 1) % FIFO_SIZE;
        fifo_count++;

        out->direction = mode;
        out->length = length;
        out->data = data;
        __enable_irq();

        if (DMA_GetCmdStatus(DISPLAY_DMA_STREAM) == DISABLE) {
          DequeueDMA();
        }

        return true;
      }

      static void SendFrame(void)
      {
        EnqueueWrite(COMMAND, ResetCursor, sizeof(ResetCursor));
        EnqueueWrite(DATA, m_frame, sizeof(m_frame));
      }

      #define CLEAR_ROW(x)      (0x00 | x)
      #define COPY_ROW(x)       (0x40 | x)
      #define RLE_PATTERN(c, p) (0x80 | (c << 2) | p)
      
      void OLEDInit(void)
      {
        HWInit();

        // Init sequence for 128x64 OLED module
        GPIO_RESET(GPIO_CMD, PIN_CMD);
        EnqueueWrite(COMMAND, InitDisplay, sizeof(InitDisplay));

        // Draw "programmer art" face until we get real assets
        u8 face[] = { 
          CLEAR_ROW(24),
          RLE_PATTERN(12, 0),
          RLE_PATTERN(8, 3),
          COPY_ROW(15),
          CLEAR_ROW(48),
          RLE_PATTERN(12, 0),
          RLE_PATTERN(8, 3),
          COPY_ROW(15),
          CLEAR_ROW(24)
        };
        FaceAnimate(face);
      }

      
      // Update the face to the next frame of an animation
      // @param frame - a pointer to a variable length frame of face animation data
      // Frame is in 8-bit RLE format:
      // 00xxxxxx     CLEAR row (blank)
      // 01xxxxxx     COPY PREVIOUS ROW (repeat)
      // 1xxxxxyy     RLE 2-bit block
      void FaceAnimate(u8* src)
      {
        if (m_disableAnimate)
          return;

        FaceDisplayDecode(src, ROWS, COLS, m_frame);

        SendFrame();
      }

      // 96 characters from ASCII 32 to 127, each 5x8 pixels in 5 bytes oriented vertically
      const int CHAR_WIDTH = 5, CHAR_HEIGHT = 8, CHAR_START = 32, CHAR_END = 127;
      static const u8 FONT[] = {
         0x00,0x00,0x00,0x00,0x00, 0x00,0x00,0x5F,0x00,0x00, 0x00,0x07,0x00,0x07,0x00, 0x14,0x7F,0x14,0x7F,0x14,
         0x24,0x2A,0x7F,0x2A,0x12, 0x23,0x13,0x08,0x64,0x62, 0x36,0x49,0x56,0x20,0x50, 0x00,0x08,0x07,0x03,0x00,
         0x00,0x1C,0x22,0x41,0x00, 0x00,0x41,0x22,0x1C,0x00, 0x2A,0x1C,0x7F,0x1C,0x2A, 0x08,0x08,0x3E,0x08,0x08,
         0x00,0x80,0x70,0x30,0x00, 0x08,0x08,0x08,0x08,0x08, 0x00,0x00,0x60,0x60,0x00, 0x20,0x10,0x08,0x04,0x02,
         0x3E,0x51,0x49,0x45,0x3E, 0x00,0x42,0x7F,0x40,0x00, 0x72,0x49,0x49,0x49,0x46, 0x21,0x41,0x49,0x4D,0x33,
         0x18,0x14,0x12,0x7F,0x10, 0x27,0x45,0x45,0x45,0x39, 0x3C,0x4A,0x49,0x49,0x31, 0x41,0x21,0x11,0x09,0x07,
         0x36,0x49,0x49,0x49,0x36, 0x46,0x49,0x49,0x29,0x1E, 0x00,0x00,0x14,0x00,0x00, 0x00,0x40,0x34,0x00,0x00,
         0x00,0x08,0x14,0x22,0x41, 0x14,0x14,0x14,0x14,0x14, 0x00,0x41,0x22,0x14,0x08, 0x02,0x01,0x59,0x09,0x06,
         0x3E,0x41,0x5D,0x59,0x4E, 0x7C,0x12,0x11,0x12,0x7C, 0x7F,0x49,0x49,0x49,0x36, 0x3E,0x41,0x41,0x41,0x22,
         0x7F,0x41,0x41,0x41,0x3E, 0x7F,0x49,0x49,0x49,0x41, 0x7F,0x09,0x09,0x09,0x01, 0x3E,0x41,0x41,0x51,0x73,
         0x7F,0x08,0x08,0x08,0x7F, 0x00,0x41,0x7F,0x41,0x00, 0x20,0x40,0x41,0x3F,0x01, 0x7F,0x08,0x14,0x22,0x41,
         0x7F,0x40,0x40,0x40,0x40, 0x7F,0x02,0x1C,0x02,0x7F, 0x7F,0x04,0x08,0x10,0x7F, 0x3E,0x41,0x41,0x41,0x3E,
         0x7F,0x09,0x09,0x09,0x06, 0x3E,0x41,0x51,0x21,0x5E, 0x7F,0x09,0x19,0x29,0x46, 0x26,0x49,0x49,0x49,0x32,
         0x03,0x01,0x7F,0x01,0x03, 0x3F,0x40,0x40,0x40,0x3F, 0x1F,0x20,0x40,0x20,0x1F, 0x3F,0x40,0x38,0x40,0x3F,
         0x63,0x14,0x08,0x14,0x63, 0x03,0x04,0x78,0x04,0x03, 0x61,0x59,0x49,0x4D,0x43, 0x00,0x7F,0x41,0x41,0x41,
         0x02,0x04,0x08,0x10,0x20, 0x00,0x41,0x41,0x41,0x7F, 0x04,0x02,0x01,0x02,0x04, 0x40,0x40,0x40,0x40,0x40,
         0x00,0x03,0x07,0x08,0x00, 0x20,0x54,0x54,0x78,0x40, 0x7F,0x28,0x44,0x44,0x38, 0x38,0x44,0x44,0x44,0x28,
         0x38,0x44,0x44,0x28,0x7F, 0x38,0x54,0x54,0x54,0x18, 0x00,0x08,0x7E,0x09,0x02, 0x18,0xA4,0xA4,0x9C,0x78,
         0x7F,0x08,0x04,0x04,0x78, 0x00,0x44,0x7D,0x40,0x00, 0x20,0x40,0x40,0x3D,0x00, 0x7F,0x10,0x28,0x44,0x00,
         0x00,0x41,0x7F,0x40,0x00, 0x7C,0x04,0x78,0x04,0x78, 0x7C,0x08,0x04,0x04,0x78, 0x38,0x44,0x44,0x44,0x38,
         0xFC,0x18,0x24,0x24,0x18, 0x18,0x24,0x24,0x18,0xFC, 0x7C,0x08,0x04,0x04,0x08, 0x48,0x54,0x54,0x54,0x24,
         0x04,0x04,0x3F,0x44,0x24, 0x3C,0x40,0x40,0x20,0x7C, 0x1C,0x20,0x40,0x20,0x1C, 0x3C,0x40,0x30,0x40,0x3C,
         0x44,0x28,0x10,0x28,0x44, 0x4C,0x90,0x90,0x90,0x7C, 0x44,0x64,0x54,0x4C,0x44, 0x00,0x08,0x36,0x41,0x00,
         0x00,0x00,0x77,0x00,0x00, 0x00,0x41,0x36,0x08,0x00, 0x02,0x01,0x02,0x04,0x02, 0x3C,0x26,0x23,0x26,0x3C
      };

      // Print a message to the face - this will permanently replace the face
      extern "C" void FacePrintf(const char *format, ...)
      {
        // Build the printf into a local buffer and zero-terminate it
        char buffer[256];
        va_list argptr;
        va_start(argptr, format);
        vsnprintf(buffer, sizeof(buffer)-1, format, argptr);
        va_end(argptr);
        buffer[sizeof(buffer)-1] = '\0';

        // Build the result into the framebuffer
        int x = 0, y = 0;
        char* cptr = buffer;
        memset(m_frame, 0, sizeof(m_frame));

        // Go character by character until we hit the end
        while (*cptr)
        {
          // Wrap to next row, and bail out past bottom row
          int c = *cptr++;
          if (c == '\n' || x >= COLS / (CHAR_WIDTH+1))
          {
            x = 0;
            y++;
          }
          if (y >= ROWS / CHAR_HEIGHT)
            break;

          // Skip unrecognized chars
          if (c < CHAR_START || c > CHAR_END)
            continue;

          // Copy the character from the font buffer to the display buffer
          const u8* fptr = FONT + (c - CHAR_START) * CHAR_WIDTH;
          u8* gptr = (u8*)(m_frame) + y + x * (CHAR_WIDTH + 1) * (ROWS / CHAR_HEIGHT);
          for (int i = 0; i < CHAR_WIDTH; i++)
          {
            *gptr = *fptr++;
            gptr += ROWS / CHAR_HEIGHT;
          }
          x++;
        }

        // Display it, and halt all further face animation
        SendFrame();
        m_disableAnimate = 1;
      }
    }
  }
}

extern "C"
void DISPLAY_DMA_IRQ_HANDLER(void)
{
  using namespace Anki::Cozmo::HAL;

  if (DMA_GetFlagStatus(DISPLAY_DMA_STREAM, DMA_FLAG_TCIF7)) {
    while (!(SPI3->SR & SPI_FLAG_TXE)) ;
    while (SPI3->SR & SPI_FLAG_BSY) ;
    DMA_ClearFlag(DISPLAY_DMA_STREAM, DMA_FLAG_TCIF7);
    DequeueDMA();
  }
}
