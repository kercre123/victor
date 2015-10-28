#include <string.h>
#include "hal/board.h"
#include "hal/portable.h"

#include "hal/display.h"
#include "hal/timers.h"
#include "../app/fixture.h"
#include "../app/binaries.h"

#include "hal/cube.h"

// PROG = PA8
// RESET = PC5
// VDD = PB1

void InitCube(void) {
  // Clock configuration
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);

  GPIO_InitTypeDef GPIO_InitStructure;
  
  // Pull PA8 (PROG) low. 
  GPIO_ResetBits(GPIOA, GPIO_Pin_8);
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
  GPIO_Init(GPIOA, &GPIO_InitStructure);

  // Pull PB7 (CS#) high.
  GPIO_SetBits(GPIOB, GPIO_Pin_7);
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7;
  GPIO_Init(GPIOB, &GPIO_InitStructure);
  
  // Pull PC5 (Reset) low. 
  GPIO_ResetBits(GPIOC, GPIO_Pin_5);
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5;
  GPIO_Init(GPIOC, &GPIO_InitStructure);
}

static inline int PageCount(int length) {
  return (length + CUBE_PAGE_SIZE - 1) / CUBE_PAGE_SIZE;
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
    GPIO_ResetBits(GPIOB, GPIO_Pin_7); // #CS
  } else {
    GPIO_SetBits(GPIOB, GPIO_Pin_7); // #CS
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

static void CubeErasePage(uint8_t page) {
  CubeAssert(true);
  CubeWrite(CUBE_ERASE_PAGE);
  CubeWrite(page);
  CubeAssert(false);

  CubeBlockBusy(); // Wait for page to be erased
}

static inline void CubeProgram(int address, const uint8_t *data, int length) {
  CubeAssert(true);
  CubeWrite(CUBE_PROGRAM);
  CubeWrite(address >> 8);
  CubeWrite(address);
  CubeSend(data, length);
  CubeAssert(false);
  
  CubeBlockBusy(); // Wait for page to be erased
}

static inline void CubeRead(int address, uint8_t *data, int length) {
  CubeAssert(true);
  CubeWrite(CUBE_READ);
  CubeWrite(address >> 8);
  CubeWrite(address);
  CubeRecv(data, length);
  CubeAssert(false);
}

CUBE_PROGRAM_ERROR LoadRom(const uint8_t *rom, int length) {
  DisplayClear();
  DisplayPrintf("Programming Cube");

  if (length > CUBE_MAX_PROGRAM_SIZE) {
    return CUBE_ERROR_ROM_OVERSIZE;
  }

  for (int i = 0; i < PageCount(length); i++) {
    DisplayPrintf("\nErasing %i", i);

    CubeWriteEn();
    if (~CubeReadFSR() & CUBE_FSR_WEN) { return CUBE_ERROR_CANNOT_WRITE; }

    CubeErasePage(i);
  }

  const uint8_t *mem = rom;
  for (int addr = 0; addr < length; addr += CUBE_PAGE_SIZE, mem += CUBE_PAGE_SIZE) {
    int left =  length - addr;
    DisplayPrintf("\nWritting %i", addr / CUBE_PAGE_SIZE);

    CubeWriteEn();
    if (~CubeReadFSR() & CUBE_FSR_WEN) { return CUBE_ERROR_CANNOT_WRITE; }

    CubeProgram(addr, mem, (left > CUBE_PAGE_SIZE) ? CUBE_PAGE_SIZE : left);
  }

  mem = rom;
  for (int addr = 0; addr < length; addr += CUBE_PAGE_SIZE, mem += CUBE_PAGE_SIZE) {
    int left =  length - addr;
    int send = (left > CUBE_PAGE_SIZE) ? CUBE_PAGE_SIZE : left;
      
    DisplayPrintf("\nVerifying %i", addr / CUBE_PAGE_SIZE);

    uint8_t verify[CUBE_PAGE_SIZE];
    CubeRead(addr, verify, send);
    
    for (int i = 0; i < send; i++) {
      if (verify[i] != mem[i]) {
        return CUBE_ERROR_VERIFY_FAILED;
      }
    }
  }
  
  DisplayPrintf("\nDone         ");
  
  return CUBE_ERROR_NONE;
}

CUBE_PROGRAM_ERROR ProgramCube(void) {

  EnableBAT();

  GPIO_SetBits(GPIOA, GPIO_Pin_8);  // PROG
  MicroWait(2000);
  GPIO_ResetBits(GPIOC, GPIO_Pin_5);  // #Reset
  MicroWait(2000);
  GPIO_SetBits(GPIOC, GPIO_Pin_5);  // #Reset
  MicroWait(100000);

  CUBE_PROGRAM_ERROR result = LoadRom(g_Block, g_BlockEnd - g_Block);

  if (result != CUBE_ERROR_NONE) {
    DisplayPrintf("\nFailed      ");
  }
  
  GPIO_ResetBits(GPIOA, GPIO_Pin_8);  // PROG
  DisableBAT();

  return result;
}
