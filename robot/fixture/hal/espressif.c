#include "hal/board.h"
#include "hal/portable.h"

#include "hal/display.h"
#include "hal/timers.h"
#include "../app/fixture.h"

#include "hal/espressif.h"

#define TESTPORT_TX       UART5
#define TESTPORT_RX       USART3
#define BAUD_RATE         230400

#define PINC_TX           12
#define GPIOC_TX          (1 << PINC_TX)
#define PINC_RX           10
#define GPIOC_RX          (1 << PINC_RX)

// These are the currently known commands supported by the ROM
static const uint8_t ESP_FLASH_BEGIN = 0x02;
static const uint8_t ESP_FLASH_DATA  = 0x03;
static const uint8_t ESP_FLASH_END   = 0x04;
static const uint8_t ESP_MEM_BEGIN   = 0x05;
static const uint8_t ESP_MEM_END     = 0x06;
static const uint8_t ESP_MEM_DATA    = 0x07;
static const uint8_t ESP_SYNC        = 0x08;
static const uint8_t ESP_WRITE_REG   = 0x09;
static const uint8_t ESP_READ_REG    = 0x0a;

// Maximum block sized for RAM and Flash writes, respectively.
static const uint16_t ESP_RAM_BLOCK   = 0x1800;
static const uint16_t ESP_FLASH_BLOCK = 0x400;

// Default baudrate. The ROM auto-bauds, so we can use more or less whatever we want.
static const int ESP_ROM_BAUD    = 230400;

// First byte of the application image
static const uint8_t ESP_IMAGE_MAGIC = 0xe9;

// Initial state for the checksum routine
static const uint8_t ESP_CHECKSUM_MAGIC = 0xef;

// OTP ROM addresses
static const uint32_t ESP_OTP_MAC0    = 0x3ff00050;
static const uint32_t ESP_OTP_MAC1    = 0x3ff00054;

// Sflash stub: an assembly routine to read from spi flash and send to host
static const uint8_t SFLASH_STUB[]    = {
    0x80,0x3c,0x00,0x40,0x1c,0x4b,0x00,0x40,0x21,0x11,0x00,0x40,0x00,0x80,0xfe,
    0x3f,0xc1,0xfb,0xff,0xd1,0xf8,0xff,0x2d,0x0d,0x31,0xfd,0xff,0x41,0xf7,0xff,
    0x4a,0xdd,0x51,0xf9,0xff,0xc0,0x05,0x00,0x21,0xf9,0xff,0x31,0xf3,0xff,0x41,
    0xf5,0xff,0xc0,0x04,0x00,0x0b,0xcc,0x56,0xec,0xfd,0x06,0xff,0xff,0x00,0x00
};

void InitEspressif(void) {
  // Clock configuration
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE);
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_UART5, ENABLE);
  

  // Pull PB7 (CS#) low.  This MUST happen before 
  GPIO_InitTypeDef GPIO_InitStructure;
  GPIO_SetBits(GPIOB, GPIO_Pin_7);
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
  GPIO_Init(GPIOC, &GPIO_InitStructure);


  USART_InitTypeDef USART_InitStructure;
  
  USART_HalfDuplexCmd(TESTPORT_RX, ENABLE);  // Enable the pin for transmitting and receiving
  
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_Pin =  GPIOC_TX;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_Init(GPIOC, &GPIO_InitStructure);
  GPIO_PinAFConfig(GPIOC, PINC_TX, GPIO_AF_UART5);
  
  GPIO_InitStructure.GPIO_Pin =  GPIOC_RX;
  GPIO_Init(GPIOC, &GPIO_InitStructure);
  GPIO_PinAFConfig(GPIOC, PINC_RX, GPIO_AF_USART3);
  
  // TESTPORT_TX config
  USART_Cmd(TESTPORT_TX, DISABLE);
  USART_InitStructure.USART_BaudRate = BAUD_RATE;
  USART_InitStructure.USART_WordLength = USART_WordLength_8b;
  USART_InitStructure.USART_StopBits = USART_StopBits_1;
  USART_InitStructure.USART_Parity = USART_Parity_No;
  USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
  USART_InitStructure.USART_Mode = USART_Mode_Tx;
  USART_Init(TESTPORT_TX, &USART_InitStructure);
  USART_Cmd(TESTPORT_TX, ENABLE);
  
  // TESTPORT_RX config
  USART_Cmd(TESTPORT_RX, DISABLE);
  
  USART_InitStructure.USART_Mode = USART_Mode_Rx;
  USART_Init(TESTPORT_RX, &USART_InitStructure);  
  USART_Cmd(TESTPORT_RX, ENABLE);

  PIN_AF(GPIOC, PINC_TX);
  PIN_AF(GPIOC, PINC_RX);

  // TEMPORARY CODE
  ProgramEspressif();
}

// Send a character over the test port
static inline void ESPPutChar(u8 c)
{    
  while (!(TESTPORT_TX->SR & USART_FLAG_TXE)) ;
  TESTPORT_TX->DR = c;
}

static inline bool ESPSentData(void) {
  return TESTPORT_RX->SR & USART_SR_RXNE;
}

static inline u8 ESPGetChar(void)
{
  while (!ESPSentData()) ;
  return TESTPORT_RX->DR & 0xFF;
}

static void ESPWrite(const u8 *data, int length) {
  ESPPutChar(0xC0);
  
  while (length-- > 0) {
    u8 out = *(data++);
    
    switch (out) {
      case 0xC0:
        ESPPutChar(0xDB);
        ESPPutChar(0xDC);
        break ;
      case 0xDB:
        ESPPutChar(0xDB);
        ESPPutChar(0xDD);
        break ;
      default:
        ESPPutChar(out);
        break ;
    }
  }
  
  ESPPutChar(0xC0);  
}

static uint8_t checksum(const uint8_t *data, int length, uint8_t state = ESP_CHECKSUM_MAGIC) {
  while(length-- > 0) {
    state ^= *(data++);
  }
  return state;
}
 
static bool ESPRead(u8 *target, int length, int timeout = -1) {
  u32 targetTime = getMicroCounter() + timeout;
  bool shift = false;

  while (length > 0 && (timeout < 0 || targetTime < getMicroCounter())) {
    if (ESPSentData()) {
      int ch = ESPGetChar() | (shift ? 0x100 : 0);

      switch (ch) {
        case 0x0DB:
          shift = true;
          break ;
        case 0x1DC:
          *(target++) = 0xC0;
          shift = false;
          break ;
        case 0x1DD:
          *(target++) = 0xDB;
          shift = false;
          break ;
        default:
          *(target++) = ch;
          shift = false;
          break ;
      }
      
      length--;
    }
  }
  
  return length > 0;
}

bool ESPSync(void) {
  static const uint8_t SYNC_DATA[] = {
    0x00, ESP_SYNC, 36, 0, 0, 0, 0, 0,
    
    0x07, 0x07, 0x12, 0x20,
    0x55, 0x55, 0x55, 0x55,
    0x55, 0x55, 0x55, 0x55,
    0x55, 0x55, 0x55, 0x55,
    0x55, 0x55, 0x55, 0x55,
    0x55, 0x55, 0x55, 0x55,
    0x55, 0x55, 0x55, 0x55,
    0x55, 0x55, 0x55, 0x55,
    0x55, 0x55, 0x55, 0x55,
  };
  
  // Attempt 16 times to get sync
  for (int i = 0; i < 0x10; i++) {
    ESPWrite(SYNC_DATA, sizeof(SYNC_DATA));
    
    
    for(;;) {
      if (!ESPSentData()) continue ;
      
      DisplayClear();
      DisplayPrintf("%2x", ESPGetChar());
    }
  }
}

void ProgramEspressif(void) {
  DisplayPrintf("Programming espressif...");

  ;

  if (ESPSync()) DisplayPrintf("Synced...");
  else DisplayPrintf("Could not sync...");
  for (;;) ;
}
