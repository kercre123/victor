#include <string.h>
#include "hal/board.h"
#include "hal/portable.h"
#include "hal/uart.h"
#include "hal/testport.h"
#include "hal/display.h"
#include "hal/timers.h"
#include "../app/fixture.h"
#include "hal/cube.h"
#include "hal/flash.h"

#include "../app/binaries.h"
#include <string.h>

// PROG = PA9 (was PA8)
// RESET = PC5
// CS = PA4 (was PB7)

// SPI pins
static GPIO_TypeDef* MOSI_PORT = GPIOA;
static GPIO_TypeDef* MISO_PORT = GPIOA;
static GPIO_TypeDef* SCK_PORT = GPIOA;
static const uint32_t MOSI_PIN = GPIO_Pin_7;
static const uint32_t MISO_PIN = GPIO_Pin_6;
static const uint32_t SCK_PIN = GPIO_Pin_5;
static const uint32_t MOSI_SOURCE = GPIO_PinSource7;
static const uint32_t MISO_SOURCE = GPIO_PinSource6;
static const uint32_t SCK_SOURCE = GPIO_PinSource5;

static const int CUBE_PAGE_SIZE = 128;

enum CUBE_FSR_FLAGS {
  CUBE_FSR_ENDEBUG = 0x80,
  CUBE_FSR_STP = 0x40,
  CUBE_FSR_WEN = 0x20,
  CUBE_FSR_RDYN = 0x10,
  CUBE_FSR_INFEN = 0x08,
  CUBE_FSR_RDISMB = 0x04
};

enum CUBE_COMMANDS {
  CUBE_WREN = 0x06,
  CUBE_WRDIS = 0x04,
  CUBE_RDSR = 0x05,
  CUBE_WRSR = 0x01,
  CUBE_READ = 0x03,
  CUBE_PROGRAM = 0x02,
  CUBE_ERASE_PAGE = 0x52,
  CUBE_ERASE_ALL = 0x62,
  CUBE_RDFPCR = 0x89,
  CUBE_RDISMB = 0x85,
  CUBE_ENDEBUG = 0x86,

  CUBE_DUMMY = 0x00
};

void InitCube(void) {
  // Clock configuration
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1, ENABLE);

  GPIO_InitTypeDef GPIO_InitStructure;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_25MHz;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_NOPULL;

  // Configure the pins for SPI in AF mode
  GPIO_PinAFConfig(MOSI_PORT, MOSI_SOURCE, GPIO_AF_SPI1);
  GPIO_PinAFConfig(MISO_PORT, MISO_SOURCE, GPIO_AF_SPI1);
  GPIO_PinAFConfig(SCK_PORT, SCK_SOURCE, GPIO_AF_SPI1);

  // Configure the SPI pins
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
  GPIO_InitStructure.GPIO_Pin = MOSI_PIN | MISO_PIN; 
  GPIO_Init(SCK_PORT, &GPIO_InitStructure);
  GPIO_InitStructure.GPIO_Pin = SCK_PIN;
  GPIO_Init(SCK_PORT, &GPIO_InitStructure);
  
  // Setup outputs
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
  
  // VDD pin - PB0
  GPIO_ResetBits(GPIOB, GPIO_Pin_0);
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
  GPIO_Init(GPIOB, &GPIO_InitStructure);
  
  // Pull PA4 (CS#) high.
  GPIO_SetBits(GPIOA, GPIO_Pin_4);
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;
  GPIO_Init(GPIOA, &GPIO_InitStructure);
  
  // Pull PC5 (Reset) low. 
  GPIO_ResetBits(GPIOC, GPIO_Pin_5);
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5;
  GPIO_Init(GPIOC, &GPIO_InitStructure);

  // High-voltage PROG is off (floating) - low voltage PROG doesn't work
  GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;
  GPIO_SetBits(GPIOA, GPIO_Pin_9);
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
  GPIO_Init(GPIOA, &GPIO_InitStructure);
  
  // Initialize SPI in master mode
  SPI_I2S_DeInit(SPI1);
  SPI_InitTypeDef SPI_InitStructure;
  SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
  SPI_InitStructure.SPI_Mode = SPI_Mode_Master;
  SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;
  SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low;
  SPI_InitStructure.SPI_CPHA = SPI_CPHA_1Edge;
  SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;
  SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_256;
  SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
  SPI_InitStructure.SPI_CRCPolynomial = 7;
  SPI_Init(SPI1, &SPI_InitStructure);
  SPI_Cmd(SPI1, ENABLE);

  SPI1->SR = 0;
}

static uint8_t CubeWrite(uint8_t data)
{
  while (!(SPI1->SR & SPI_FLAG_TXE)) ;
  SPI1->DR = data;
  
  // Make sure SPI is totally drained
  while (!(SPI1->SR & SPI_FLAG_TXE)) ;
  while (SPI1->SR & SPI_FLAG_BSY) ;
  
  return SPI1->DR;
}

static inline void CubeAssert(bool assert) {
  MicroWait(10);
  if (assert) {
    GPIO_ResetBits(GPIOA, GPIO_Pin_4); // #CS
  } else {
    GPIO_SetBits(GPIOA, GPIO_Pin_4); // #CS
  }
  MicroWait(10);
}

static inline void CubeSend(const uint8_t *arg, int arg_size) {
  while(arg_size-- > 0) {
    CubeWrite(*(arg++));
  }
}

static inline void CubeRecv(uint8_t *arg, int arg_size) {
  while(arg_size-- > 0) {
    *(arg++) = CubeWrite(CUBE_DUMMY);
  }
}

static uint8_t CubeReadFSR() {
  uint8_t reply;

  CubeAssert(true);
  CubeWrite(CUBE_RDSR);
  CubeRecv(&reply, sizeof(reply));
  CubeAssert(false);

  return reply;
}

static void CubeWriteEn() {
  CubeAssert(true);
  CubeWrite(CUBE_WREN);
  CubeAssert(false);
}

static inline void CubeBlockBusy(void) {
  while (CubeReadFSR() & CUBE_FSR_RDYN) ;
}

/* Only on nRF24  
static inline int PageCount(int length) {
  return (length + CUBE_PAGE_SIZE - 1) / CUBE_PAGE_SIZE;
}
static void CubeErasePage(uint8_t page) {
  CubeAssert(true);
  CubeWrite(CUBE_ERASE_PAGE);
  CubeWrite(page);
  CubeAssert(false);

  CubeBlockBusy(); // Wait for page to be erased
}
*/

static inline void CubeProgram(int address, const uint8_t *data, int length) {
  CubeAssert(true);
  CubeWrite(CUBE_PROGRAM);
  CubeWrite(address >> 8);
  CubeWrite(address);
  CubeSend(data, length);
  CubeAssert(false);
  
  CubeBlockBusy(); // Wait for page to be programmed
}

static inline void CubeRead(int address, uint8_t *data, int length) {
  CubeAssert(true);
  CubeWrite(CUBE_READ);
  CubeWrite(address >> 8);
  CubeWrite(address);
  CubeRecv(data, length);
  CubeAssert(false);
}

int IsPageBlank(const uint8_t *rom)
{
  for (int i = 0; i < CUBE_PAGE_SIZE; i++)
      if (0xFF != rom[i])
        return 0;
  return 1;
}

void LoadRom(const uint8_t *rom, int length) {
  SlowPrintf("Programming Cube");
  
  const uint8_t *mem = rom;
  for (int addr = 0; addr < length; addr += CUBE_PAGE_SIZE, mem += CUBE_PAGE_SIZE) {
    int left =  length - addr;
    if (IsPageBlank(rom + addr))
      continue;
    
    SlowPrintf("\nWriting %i", addr / CUBE_PAGE_SIZE);
    
    CubeWriteEn();
    if (~CubeReadFSR() & CUBE_FSR_WEN) { throw ERROR_CUBE_CANNOT_WRITE; }
    CubeProgram(addr, mem, (left > CUBE_PAGE_SIZE) ? CUBE_PAGE_SIZE : left);
  }

  mem = rom;
  for (int addr = 0; addr < length; addr += CUBE_PAGE_SIZE, mem += CUBE_PAGE_SIZE) {
    int left =  length - addr;
    int send = (left > CUBE_PAGE_SIZE) ? CUBE_PAGE_SIZE : left;
      
    SlowPrintf("\nVerifying %i", addr / CUBE_PAGE_SIZE);

    uint8_t verify[CUBE_PAGE_SIZE];
    CubeRead(addr, verify, send);
    
    for (int i = 0; i < send; i++) {
      if (verify[i] != mem[i]) {
        throw ERROR_CUBE_VERIFY_FAILED;
      }
    }
  }
  
  SlowPrintf("\nDone         ");
}

// Simple program routine - no brains, no serialization - for test only
void ProgramCubeTest(u8* rom, int length)
{
  GPIO_ResetBits(GPIOA, GPIO_Pin_9);  // High-voltage PROG
  MicroWait(2000);
  PIN_OUT(GPIOC, 5);
  GPIO_ResetBits(GPIOC, GPIO_Pin_5);  // #Reset
  MicroWait(2000);
  GPIO_SetBits(GPIOC, GPIO_Pin_5);  // #Reset
  MicroWait(10000);

  LoadRom(rom, length);

  GPIO_ResetBits(GPIOC, GPIO_Pin_5);  // Put in #Reset
  GPIO_SetBits(GPIOA, GPIO_Pin_9);    // Turn off high-voltage PROG
}

int GetSequence(void);
extern FixtureType g_fixtureType;

// This table is used to generate a nRF31-friendly Address for the Electronic Serial Number
// A minimum of 3 transitions occur in each byte, but never 10101 or 01010
// The first 66 entries are leading-byte-friendly (they don't start with 10 or 01)
const u8 ESN_LOOKUP[150] = {
  0x09,0x0b,0x0d,0x11,0x12,0x13,0x16,0x17,0x19,0x1a,0x1b,0x1d,
  0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x2c,0x2d,0x2e,0x2f,0x31,0x32,0x33,0x34,0x36,0x37,0x39,0x3a,0x3b,0x3d,
  0xc2,0xc4,0xc5,0xc6,0xc8,0xc9,0xcb,0xcc,0xcd,0xce,0xd0,0xd1,0xd2,0xd3,0xd8,0xd9,0xda,0xdb,0xdc,0xdd,0xde,
  0xe2,0xe4,0xe5,0xe6,0xe8,0xe9,0xec,0xed,0xee,0xf2,0xf4,0xf6,

  0x42,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0x4b,0x4c,0x4d,0x4e,0x4f,0x58,0x59,0x5a,0x5b,0x5c,0x5d,0x5e,
  0x61,0x62,0x63,0x64,0x65,0x66,0x67,0x68,0x69,0x6c,0x6d,0x6e,0x6f,0x71,0x72,0x73,0x74,0x76,0x77,0x79,0x7a,0x7b,
  0x84,0x85,0x86,0x88,0x89,0x8b,0x8c,0x8d,0x8e,0x90,0x91,0x92,0x93,0x96,0x97,0x98,0x99,0x9a,0x9b,0x9c,0x9d,0x9e,
  0xa1,0xa2,0xa3,0xa4,0xa5,0xa6,0xa7,0xb0,0xb1,0xb2,0xb3,0xb4,0xb6,0xb7,0xb8,0xb9,0xba,0xbb,0xbc,0xbd,
};


// Get a radio-safe bit sequence
// This assumes that MSB is the leading byte - reverse it if not true!
static u32 GetRadioSequence()
{
  // Make sure both the header and footer are in-range
  u32 head = FIXTURE_SERIAL;
  u32 foot = GetSequence();
  if (head >= (1<<8) || foot >= (1<<19))
    throw ERROR_SERIAL_INVALID;
  
  // Build the 27-bit identifier
  u32 bits = (head << 19) | foot;
  
  // Expand it to a radio-safe 32-bit identifier
  u32 bytes =
    (ESN_LOOKUP[(bits>>0)  & 127] << 0) |
    (ESN_LOOKUP[(bits>>7)  & 127] << 8) |
    (ESN_LOOKUP[(bits>>14) & 127] << 16)|
    (ESN_LOOKUP[(bits>>21) & 63]  << 24);
  
  // For debug
  ConsolePrintf("cubeserial,%08x,%08x,\n", bits, bytes);
  return bytes;
}

// Patch a non-aligned 32-bit word in the cube binary
static void Patch(u8* where, u32 before, u32 after)
{
  if (where[0] != ((before>>0)  & 255) ||
      where[1] != ((before>>8)  & 255) ||
      where[2] != ((before>>16) & 255) ||
      where[3] != ((before>>24) & 255))
  {
    ConsolePrintf("Expected: %08x  Got: %02x%02x%02x%02x\n", before, where[3], where[2], where[1], where[0]);
    throw ERROR_CUBE_ROM_MISPATCH;
  }
  
  where[0] = after>>0;
  where[1] = after>>8;
  where[2] = after>>16;
  where[3] = after>>24;  
}

// Handle all the boot loader serial patching stuff
void ProgramCubeWithSerial()
{
  try {  
    // Make a copy of the cube bootloader for patching
    u8* cubeboot = GetGlobalBuffer();
    int length = g_CubeEnd - g_Cube;
    if (length > 1024 * 16)
      throw ERROR_CUBE_ROM_OVERSIZE;
    memcpy(cubeboot, g_Cube, length);

    GPIO_ResetBits(GPIOA, GPIO_Pin_9);  // High-voltage PROG
    MicroWait(2000);
    PIN_OUT(GPIOC, 5);
    GPIO_ResetBits(GPIOC, GPIO_Pin_5);  // #Reset
    MicroWait(2000);
    GPIO_SetBits(GPIOC, GPIO_Pin_5);  // #Reset
    MicroWait(10000);
         
    // Check serial number from (possibly) last time
    // We don't want to reserialize the same block
    u32 serial;
    CubeRead(0x3ff0, (u8*)&serial, 4);
    serial = __REV(serial);   // Vandiver likes it stored in reverse
    SlowPrintf("Serial was: %08x\n", serial);
    if (serial != 0xffffffff)
      SlowPrintf("Serial already set, won't set again\n");
    else
      // Generate a radio-safe serial number
      serial = GetRadioSequence();
    
    // Patch accessory type/model: 0=charger, 1=cube1, etc
    int type = (g_fixtureType - FIXTURE_CHARGER_TEST);
    cubeboot[0x3ff4] = type; 
    
    // Patch each copy of the serial number
    Patch(cubeboot+0x388d, __REV(0xca11ab1e), serial);
    Patch(cubeboot+0x38a4, __REV(0xca11ab1e), serial);
    Patch(cubeboot+0x3ff0, 0xca11ab1e, __REV(serial));  // Reversed copy (thanks Vandiver)
    
    //ConsoleWriteHex(cubeboot, 16384); 
    LoadRom(cubeboot, length);
    
    GPIO_ResetBits(GPIOC, GPIO_Pin_5);  // Put in #Reset
    GPIO_SetBits(GPIOA, GPIO_Pin_9);    // Turn off high-voltage PROG
    
  // Always clean up after yourself
  } catch (int e) {
    GPIO_ResetBits(GPIOC, GPIO_Pin_5);  // Put in #Reset
    GPIO_SetBits(GPIOA, GPIO_Pin_9);    // Turn off high-voltage PROG
    throw;
  }
}
