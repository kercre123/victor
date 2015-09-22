#include <string.h>
#include <stdint.h>

#include "board.h"
#include "fsl_debug_console.h"
#include "anki/cozmo/robot/hal.h"
#include "hal/portable.h"

#include "spi.h"


/*
  In the final version, we will use the EOQ flag to mark when
  the buffer has finished, and then we will clear the CONT_SCKE flag from MCR
  so it doesn't keep clocking the Espressif.
*/

void bit_bang_test(void) {
  const int DELAY = 100;
  const int EARLY_CLOCKS = 10;
  const int TRANSFER_SIZE = 96;
  const int LATE_CLOCKS = 12;
  const int PACKET_DELAY = 1000000;
  
  GPIO_PIN_SOURCE(PCS1, PTD,  4);
  GPIO_PIN_SOURCE( SCK, PTE, 17);
  GPIO_PIN_SOURCE(SOUT, PTE, 18);
  GPIO_PIN_SOURCE( SIN, PTE, 19);

  GPIO_OUT(GPIO_PCS1, PIN_PCS1);
  GPIO_OUT(GPIO_SCK, PIN_SCK);
  GPIO_OUT(GPIO_SOUT, PIN_SOUT);
  GPIO_IN(GPIO_SIN, PIN_SIN);

  PORTD_PCR4  = PORT_PCR_MUX(1);
  PORTE_PCR17 = PORT_PCR_MUX(1);
  PORTE_PCR18 = PORT_PCR_MUX(1);
  PORTE_PCR19 = PORT_PCR_MUX(1);

  bool WS = false;

  for(;;) {
    for(int ec = 0; ec < EARLY_CLOCKS; ec++) {
      GPIO_RESET(GPIO_SCK, PIN_SCK);
      Anki::Cozmo::HAL::MicroWait(DELAY);
      GPIO_SET(GPIO_SCK, PIN_SCK);
      Anki::Cozmo::HAL::MicroWait(DELAY);
    }
    for(uint16_t w = 0; w < TRANSFER_SIZE;w++) {
      if (WS) {
        GPIO_SET(GPIO_PCS1, PIN_PCS1);
      } else {
        GPIO_RESET(GPIO_PCS1, PIN_PCS1);
      }
      WS = !WS;

      for(int b = 0; b < 16; b++) {
        if ((w >> b) & 1) {
          GPIO_SET(GPIO_SOUT, PIN_SOUT);
        } else {
          GPIO_RESET(GPIO_SOUT, PIN_SOUT);
        }
        
        GPIO_RESET(GPIO_SCK, PIN_SCK);
        Anki::Cozmo::HAL::MicroWait(DELAY);
        GPIO_SET(GPIO_SCK, PIN_SCK);
        Anki::Cozmo::HAL::MicroWait(DELAY);
      }
    }
    for(int ec = 0; ec < LATE_CLOCKS; ec++) {
      GPIO_RESET(GPIO_SCK, PIN_SCK);
      Anki::Cozmo::HAL::MicroWait(DELAY);
      GPIO_SET(GPIO_SCK, PIN_SCK);
      Anki::Cozmo::HAL::MicroWait(DELAY);
    }
    Anki::Cozmo::HAL::MicroWait(PACKET_DELAY);
  }
}

void spi_init(void) {
  Anki::Cozmo::HAL::MicroWait(1000000);

  const int size = 192;
  uint32_t spi_buff[size]; // 512 bytes

  // Turn on power to DMA, PORTC and SPI0
  SIM_SCGC6 |= SIM_SCGC6_SPI0_MASK | SIM_SCGC6_DMAMUX_MASK;
  SIM_SCGC5 |= SIM_SCGC5_PORTE_MASK | SIM_SCGC5_PORTD_MASK;
  SIM_SCGC7 |= SIM_SCGC7_DMA_MASK;

  //bit_bang_test();
  
  // Configure SPI pins
  PORTD_PCR4  = PORT_PCR_MUX(2); // SPI0_PCS1
  PORTE_PCR17 = PORT_PCR_MUX(2); // SPI0_SCK
  PORTE_PCR18 = PORT_PCR_MUX(2); // SPI0_SOUT
  PORTE_PCR19 = PORT_PCR_MUX(2); // SPI0_SIN

  // Configure SPI perf to the magical value of magicalness
  SPI0_MCR = SPI_MCR_MSTR_MASK |
             SPI_MCR_DCONF(0) |
             SPI_MCR_SMPL_PT(0) |
             SPI_MCR_CLR_TXF_MASK |
             SPI_MCR_CLR_RXF_MASK;

  SPI0_CTAR0 = SPI_CTAR_BR(0) |
               SPI_CTAR_CPOL_MASK |
               SPI_CTAR_CPHA_MASK |
               SPI_CTAR_FMSZ(7);
  
  SPI0_RSER = SPI_RSER_EOQF_RE_MASK;

  // Clear all status flags
  SPI0_SR = SPI0_SR;

  // Enable IRQs
  NVIC_SetPriority(SPI0_IRQn, 0);
  NVIC_EnableIRQ(SPI0_IRQn);

  for (int i = 0; i < size; i++) {
    int k = i >> 1;

    spi_buff[i] = 
        ((~i & 1) ? k : (1 << (k & 7))) |
        SPI_PUSHR_CONT_MASK | 
        SPI_PUSHR_PCS((i & 1) ? ~0: 0);// |
        //(i == (size-1) ? SPI_PUSHR_EOQ_MASK : 0);
  }
  
  int rx_idx = 0, tx_idx = 0;
  
  while(true) {
    for (int i = 0; i < 96; i++) {
      while (false && SPI0_SR & SPI_SR_RFDF_MASK)
      {
        spi_buff[rx_idx] = (spi_buff[rx_idx] & ~ 0xFFFF) | SPI0_POPR;
        rx_idx = (rx_idx + 1) % size;
        SPI0_SR = SPI_SR_RFDF_MASK;
      }

      while (SPI0_SR & SPI_SR_TFFF_MASK)
      {
        SPI0_MCR |= SPI_MCR_CONT_SCKE_MASK;
        SPI0_PUSHR = spi_buff[tx_idx];
        tx_idx = (tx_idx + 1) % size;
        SPI0_SR = SPI_SR_TFFF_MASK;

        if (!tx_idx) Anki::Cozmo::HAL::MicroWait(10000);
      }
    }
  }
}

void SPI0_IRQHandler(void) {
    // When the EOQ flag is set, clear our continuous clock mode.
    while (SPI0_SR & SPI_SR_EOQF_MASK)
    {
      //SPI0_MCR &= ~SPI_MCR_CONT_SCKE_MASK;
      SPI0_SR = SPI_SR_EOQF_MASK;
    }
}

