#include <string.h>
#include <stdint.h>

#include "board.h"
#include "fsl_debug_console.h"

#include "spi.h"


/*
  In the final version, we will use the EOQ flag to mark when
  the buffer has finished, and then we will clear the CONT_SCKE flag from MCR
  so it doesn't keep clocking the Espressif.
*/


void spi_init(void) {
  const int size = 96;
  uint32_t spi_buff[size]; // 512 bytes

  // Turn on power to DMA, PORTC and SPI0
  SIM_SCGC6 |= SIM_SCGC6_SPI0_MASK | SIM_SCGC6_DMAMUX_MASK;
  SIM_SCGC5 |= SIM_SCGC5_PORTC_MASK;
  SIM_SCGC7 |= SIM_SCGC7_DMA_MASK;

  // Configure SPI pins
  PORTC_PCR3 = PORT_PCR_MUX(2); // SPI0_PCS1
  PORTC_PCR5 = PORT_PCR_MUX(2); // SPI0_SCK
  PORTC_PCR6 = PORT_PCR_MUX(2); // SPI0_SOUT
  PORTC_PCR7 = PORT_PCR_MUX(2); // SPI0_SIN

  // Configure SPI perf to the magical value of magicalness
  SPI0_MCR = SPI_MCR_MSTR_MASK |
             SPI_MCR_DCONF(0) | 
             SPI_MCR_SMPL_PT(0) | 
             SPI_MCR_CLR_TXF_MASK | 
             SPI_MCR_CLR_RXF_MASK;
  SPI0_CTAR0 = SPI_CTAR_BR(3) | 
               SPI_CTAR_CPOL_MASK |
               SPI_CTAR_CPHA_MASK |
               SPI_CTAR_FMSZ(15);
  int x = 0;
  x = x;
  
  SPI0_RSER = SPI_RSER_EOQF_RE_MASK;

  // Clear all status flags
  SPI0_SR = SPI0_SR;

  // Enable IRQs
  NVIC_SetPriority(SPI0_IRQn, 0);
  NVIC_EnableIRQ(SPI0_IRQn);

  for (int i = 0; i < size; i++) {
    spi_buff[i] = 
        SPI_PUSHR_CONT_MASK | 
        SPI_PUSHR_PCS(~i & 1 ? ~0: 0) | 
        (i == (size-1) ? SPI_PUSHR_EOQ_MASK : 0) |
        (~i & 0xFFFF);
  }

  while(true) {
    // attempt to silence early clocking
    for(int i = 0; i < size; ) {
      while (SPI0_SR & SPI_SR_RFDF_MASK)
      {
        SPI0_POPR;
        SPI0_SR = SPI_SR_RFDF_MASK;
      }

      while (i < size && SPI0_SR & SPI_SR_TFFF_MASK)
      {
        SPI0_MCR |= SPI_MCR_CONT_SCKE_MASK;
        SPI0_PUSHR = spi_buff[i++];
        SPI0_SR = SPI_SR_TFFF_MASK;
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

