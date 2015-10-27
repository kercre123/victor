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
  
  ProgramCube();
  for(;;);
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

static inline void CubeCommand(CUBE_COMMANDS cmd, uint32_t arg, int arg_size, uint8_t *reply, int reply_size) {
  GPIO_ResetBits(GPIOB, GPIO_Pin_7); // #CS
  MicroWait(1);
  
  CubeWrite(cmd);
  for (int i = 8 * (arg_size - 1); i >= 0; i -= 8) {
    CubeWrite(arg >> i);
  }

  while (reply_size-- > 0) {
    *(reply++) = CubeWrite(CUBE_DUMMY);
  }

  MicroWait(1);  
  GPIO_SetBits(GPIOB, GPIO_Pin_7); // #CS
}

static void CubeWriteEn() {
  CubeCommand(CUBE_WREN, NULL, 0, NULL, 0);
}

static uint8_t CubeReadFSR() {
  uint8_t reply;
  CubeCommand(CUBE_WREN, NULL, 0, &reply, sizeof(reply));
  return reply;
}

static void CubeErasePage(uint8_t page) {
  CubeCommand(CUBE_WREN, page, sizeof(page), NULL, 0);
}

CUBE_PROGRAM_ERROR ProgramCube(void) {
  DisplayClear();
  DisplayPrintf("Programming Cube");

  EnableBAT();

  GPIO_SetBits(GPIOA, GPIO_Pin_8);  // PROG
  MicroWait(2000);
  GPIO_ResetBits(GPIOC, GPIO_Pin_5);  // #Reset
  MicroWait(2000);
  GPIO_SetBits(GPIOC, GPIO_Pin_5);  // #Reset
  MicroWait(100000);

  CubeWriteEn();
  uint8_t status = CubeReadFSR();
  
  GPIO_ResetBits(GPIOA, GPIO_Pin_8);  // PROG
  DisableBAT();

  return CUBE_ERROR_NONE;
}
