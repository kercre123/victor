#include <string.h>
#include "hal/board.h"
#include "hal/portable.h"

#include "hal/timers.h"
#include "../app/fixture.h"
#include "../app/binaries.h"

#include "hal/espressif.h"
#include "hal/uart.h"

#define TESTPORT_TX       UART5
#define TESTPORT_RX       USART3

// Espressif baudrate detection only knows 'common' baudrates (57600 * 2^n)
// Note:  BAUD_RATE is unstable beyond 230400
#define BAUD_RATE         230400

#define PINC_TX           12
#define GPIOC_TX          (1 << PINC_TX)
#define PINC_RX           10
#define GPIOC_RX          (1 << PINC_RX)

// Uncomment the below to see why programming is failing
//#define ESP_DEBUG

static const int MAX_TIMEOUT = 100000; // 100ms - will retry if needed

// These are the currently known commands supported by the ROM
static const uint8_t ESP_FLASH_BEGIN = 0x02;
static const uint8_t ESP_FLASH_DATA  = 0x03;
// static const uint8_t ESP_FLASH_END   = 0x04;
static const uint8_t ESP_MEM_BEGIN   = 0x05;
static const uint8_t ESP_MEM_END     = 0x06;
// static const uint8_t ESP_MEM_DATA    = 0x07;
static const uint8_t ESP_SYNC        = 0x08;
// static const uint8_t ESP_WRITE_REG   = 0x09;
// static const uint8_t ESP_READ_REG    = 0x0a;

// Maximum block sized for RAM and Flash writes, respectively.
// static const uint16_t ESP_RAM_BLOCK   = 0x1800;
static const uint16_t ESP_FLASH_BLOCK = 0x400;

static const uint16_t SECTOR_SIZE = 4096;

// First byte of the application image
// static const uint8_t ESP_IMAGE_MAGIC = 0xe9;

// Initial state for the checksum routine
static const uint8_t ESP_CHECKSUM_MAGIC = 0xef;

// OTP ROM addresses
// static const uint32_t ESP_OTP_MAC0    = 0x3ff00050;
// static const uint32_t ESP_OTP_MAC1    = 0x3ff00054;

/* Sflash stub: an assembly routine to read from spi flash and send to host
static const uint8_t SFLASH_STUB[]    = {
    0x80,0x3c,0x00,0x40,0x1c,0x4b,0x00,0x40,0x21,0x11,0x00,0x40,0x00,0x80,0xfe,
    0x3f,0xc1,0xfb,0xff,0xd1,0xf8,0xff,0x2d,0x0d,0x31,0xfd,0xff,0x41,0xf7,0xff,
    0x4a,0xdd,0x51,0xf9,0xff,0xc0,0x05,0x00,0x21,0xf9,0xff,0x31,0xf3,0xff,0x41,
    0xf5,0xff,0xc0,0x04,0x00,0x0b,0xcc,0x56,0xec,0xfd,0x06,0xff,0xff,0x00,0x00
};
*/

enum ESP_SOURCE {
  ESP_FIXTURE = 0,
  ESP_TARGET = 1
};

struct ESPHeader {
  uint8_t   source;
  uint8_t   opcode;
  uint16_t  length;
  uint32_t  checksum;   // Only on "flash/memory" data, not on whole data section including header
};

struct FlashLoadLocation {
  const unsigned char* name;
  uint32_t addr;
  int length;
  const uint8_t* data;
};
  
static const FlashLoadLocation ESPRESSIF_ROMS[] = {
  { "BOOT", 0x000000, g_EspBootEnd - g_EspBoot, g_EspBoot },
  { "USER",  0x003000, g_EspUserEnd - g_EspUser, g_EspUser },
  { "INIT", 0x1fc000, g_EspInitEnd - g_EspInit, g_EspInit },
  { "BLANK", 0x1fe000, g_EspBlankEnd - g_EspBlank, g_EspBlank },
  { 0, 0, NULL }
};

void InitEspressif(void) {
  // Clock configuration
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE);
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_UART5, ENABLE);

  // Pull PB7 (CS#) low.  This MUST happen before 
  GPIO_InitTypeDef GPIO_InitStructure;
  GPIO_ResetBits(GPIOB, GPIO_Pin_7);
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
  GPIO_Init(GPIOB, &GPIO_InitStructure);

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
}

static inline int BlockCount(int length) {
  return (length + ESP_FLASH_BLOCK - 1) / ESP_FLASH_BLOCK;
};

// Send a character over the test port
static inline void ESPPutChar(u8 c)
{    
  while (!(TESTPORT_TX->SR & USART_FLAG_TXE)) ;
  TESTPORT_TX->DR = c;
}

static inline int ESPGetChar(int timeout = -1)
{
  int countdown = timeout + getMicroCounter();
  
  TESTPORT_RX->SR = 0;

  while (timeout < 0 || countdown > getMicroCounter()) {
    if (TESTPORT_RX->SR & USART_SR_RXNE) {
      return TESTPORT_RX->DR & 0xFF;
    }
  }

  return -1;
}

static void ESPWrite(const u8 *data, int length) { 
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
}

static int ESPRead(u8 *target, int length, int timeout = -1) {
  int read = 0;

  for(;;) {
    int ch = ESPGetChar(timeout);
    if (ch < 0) return -1;  // Timeout
    if (ch == 0xC0) break ;
  }

  while (length-- > 0) {
    int ch = ESPGetChar(timeout);
    if (ch < 0) return -1;  // Timeout

    if (ch == 0xC0) {
      if (read <= 0) continue ;
      return read;
    } else if (ch == 0xDB) {
      int ch = ESPGetChar(timeout);
      if (ch < 0) return -1;  // Timeout
      
      if (ch == 0xDC) {
        *(target++) = 0xC0;
      } else if (ch == 0xDD) {
        *(target++) = 0xDB;
      } else {
        return -2;  // Escape error
      }
    } else {
      *(target++) = ch;
    }
    
    read++;
  }
  
  return -3; // Buffer overflow
}

static uint8_t checksum(const uint8_t *data, int length) {
  uint8_t state = ESP_CHECKSUM_MAGIC;
  while(length-- > 0) {
    state ^= *(data++);
  }
  return state;
}

static inline void ESPCommand(uint8_t command, const uint8_t* data, int length, int checksum) {
  ESPHeader header = { ESP_FIXTURE, command, length, checksum } ;
  
#ifdef ESP_DEBUG
  for (int i = 0; i < sizeof(header); i++)
    SlowPrintf("%02X ", ((char*)&header)[i]);
  SlowPrintf("  ");
  for (int i = 0; i < length && i < 16; i++)
    SlowPrintf("%02X ", data[i]);
#endif
  
  ESPPutChar(0xC0);
  ESPWrite((u8*)&header, sizeof(header));
  ESPWrite(data, length);
  ESPPutChar(0xC0);
}

bool ESPSync(void) {
  static const uint8_t SYNC_DATA[] = {
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
  uint8_t read[256];

  // Attempt 16 times to get sync
  for (int i = 0; i < 0x40; i++) {
    ESPCommand(ESP_SYNC, SYNC_DATA, sizeof(SYNC_DATA), 0);
    int size = ESPRead(read, sizeof(read), MAX_TIMEOUT);
    
    if (size > 0) {
      // Flush ESP buffer
      while (ESPGetChar(1000) != -1);

      return true ;
    }
    
    MicroWait(1000);
  }
  
  return false;
}

// From esptool.py:
//  Responses consist of at least 8 bytes:  u8 response, u8 req_cmd, u16 datalen, u32 'value' - followed by datalen of 'body'
//  The 'response' is always 1, the req_cmd echoes back the command, and the 'value' is only used to return read_reg (so far)
//  The 'body' is generally a 2 byte error code - body[0] = 0 for success, 1 for fail - body[1] = last error code (not reset)
//  Error code 7 means bad checksum.. doh
static int Command(const char* debug, uint8_t cmd, const uint8_t* data, int length, uint8_t *reply, int max, int timeout = -1, int checksum = 0) {
  SlowPrintf("%s=%02X {", debug, cmd);
  ESPCommand(cmd, data, length, checksum);
  
  for (int retry = 0 ; retry < 100; retry++)
  {    
    int replySize = ESPRead(reply, max, timeout);

    // esptool says retry a command until it at least acks the command
    // NOTE:  This check is hardcoded to deal with flashing commands and will fail on readback commands
    if (replySize < 10 || reply[1] != cmd || reply[8] != 0) {
      SlowPrintf("retry..");
      continue;
    }
    
    SlowPrintf("..} <- %d { ", replySize);
#ifdef ESP_DEBUG
    for (int i = 0; i < replySize; i++)
      SlowPrintf("%02X ", reply[i]);
#endif
    SlowPrintf("}\n");

    return replySize;
  }
  SlowPrintf("\nGAVE UP\n");
  return -1;
}

static bool ESPFlashLoad(uint32_t address, int length, const uint8_t *data) {
  union {
    uint8_t reply_buffer[0x100];
  };

  struct FlashBegin {
    uint32_t size;
    uint32_t blocks;
    uint32_t blocksize;
    uint32_t address;
  };

  struct FlashWrite {
    uint32_t len;
    uint32_t seq;
    uint32_t _[2];
    uint8_t data[ESP_FLASH_BLOCK];
  };

  // This locks the flash chip - not really what we want!
  struct FlashEnd {
    uint32_t not_reboot;
  };
  
  int blockCount = BlockCount(length);
  
  // esptool.py does bizarre calculations with eraseSize - trying to align erases to 64KB boundaries?
  // I'm just aligning it to 4KB boundaries and crossing my fingers
  int eraseSize = ((length + SECTOR_SIZE - 1) / SECTOR_SIZE) * SECTOR_SIZE;
  const FlashBegin begin = { eraseSize, blockCount, ESP_FLASH_BLOCK, address };
  // const FlashEnd finish = { 0 };  // 0 means reboot, 1 means don't - DANGER: flash end locks the chip, making further writes fail
  FlashWrite block = { ESP_FLASH_BLOCK, 0, 0, 0 };
  
  // This is copied wholesale from esptool.py - a useful hack to leave chip unlocked
  const FlashBegin mem_begin = { 0, 0, 0, 0x40100000 };
  const FlashEnd mem_end = { 0x40000080 };

  // Write BEGIN command (erase, wait for flash to blank)
  int replySize = Command("Flash Begin", ESP_FLASH_BEGIN, (uint8_t*) &begin, sizeof(begin), reply_buffer, sizeof(reply_buffer), MAX_TIMEOUT);
  
  while (length > 0) {
    int copyLength = (length > ESP_FLASH_BLOCK) ? ESP_FLASH_BLOCK : length;
    if (copyLength < ESP_FLASH_BLOCK) {
      memset(block.data, 0xFF, ESP_FLASH_BLOCK);
    }
    memcpy(block.data, data, copyLength);

    int check = checksum(block.data, ESP_FLASH_BLOCK);
    replySize = Command("Flash Block", ESP_FLASH_DATA, (uint8_t*) &block, sizeof(block), reply_buffer, sizeof(reply_buffer), MAX_TIMEOUT, check);
    
    data += ESP_FLASH_BLOCK;
    length -= ESP_FLASH_BLOCK;
    block.seq++;
  }

  // FINISH with this hack from esptool.py - to prevent chip locking
  replySize = Command("Mem   Begin", ESP_MEM_BEGIN, (uint8_t*) &mem_begin, sizeof(mem_begin), reply_buffer, sizeof(reply_buffer), MAX_TIMEOUT);
  replySize = Command("Mem   End  ", ESP_MEM_END, (uint8_t*) &mem_end, sizeof(mem_end), reply_buffer, sizeof(reply_buffer), MAX_TIMEOUT);
  
  return true;
}

ESP_PROGRAM_ERROR ProgramEspressif(void) {
  SlowPrintf("ESP Syncronizing...");

  // Note:  This is sensitive to delay - we must start sync immediately after power up
  EnableBAT();
  if (!ESPSync()) { 
    SlowPrintf("Sync Failed.\n");
    return ESP_ERROR_NO_COMMUNICATION;
  }
  
  for(const FlashLoadLocation* rom = &ESPRESSIF_ROMS[0]; rom->length; rom++) {
    SlowPrintf("Load ROM %s\n", rom->name);

    if (!ESPFlashLoad(rom->addr, rom->length, rom->data)) {
      return ESP_ERROR_BLOCK_FAILED;
    }
  }
  
  return ESP_ERROR_NONE;
}
