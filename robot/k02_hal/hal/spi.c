#include <string.h>
#include <stdint.h>

#include "board.h"
#include "fsl_debug_console.h"

#include "spi.h"

void setup_dma(void);
  
void spi_init(void) {
  // Turn on power to DMA, PORTC and SPI0
  SIM_SCGC6 |= SIM_SCGC6_SPI0_MASK | SIM_SCGC6_DMAMUX_MASK;
  SIM_SCGC5 |= SIM_SCGC5_PORTC_MASK;
  SIM_SCGC7 |= SIM_SCGC7_DMA_MASK;

  // Configure SPI pins
  //PORTC_PCR3 = PORT_PCR_MUX(2); // SPI0_PCS1
  PORTC_PCR5 = PORT_PCR_MUX(2); // SPI0_SCK
  PORTC_PCR6 = PORT_PCR_MUX(2); // SPI0_SOUT
  PORTC_PCR7 = PORT_PCR_MUX(2); // SPI0_SIN

  setup_dma();
  
  const int size = 256;
  uint16_t spi_buff[size]; // 512 bytes
  int tx_idx = 0, rx_idx = 0;

  for(;;) {
    //while (SPI0_SR & SPI_SR_RFDF_MASK)
    {
      spi_buff[rx_idx] = SPI0_POPR;
      rx_idx = (rx_idx + 1) % size;

      SPI0_SR = SPI_SR_RFDF_MASK;
    }

    //while (SPI0_SR & SPI_SR_TFFF_MASK)
    {
      SPI0_PUSHR = 0x5500; //spi_buff[tx_idx];
      tx_idx = (tx_idx + 1) % size;

      SPI0_SR = SPI_SR_TFFF_MASK;
    }
    
    for (int i = 0; i < 4; i++) BOARD_I2C_DELAY ;
  }
}

void __attribute__((section("RW_IRAM2_PROGRAM_SCRATCH"))) setup_dma(void) {
  // Configure perf
  SPI0_SR = SPI0_SR;  // Clear all status flags
  SPI0_MCR = SPI_MCR_MSTR_MASK | SPI_MCR_DCONF(0) | SPI_MCR_SMPL_PT(0) | SPI_MCR_CLR_TXF_MASK | SPI_MCR_CLR_RXF_MASK;
  SPI0_CTAR0 = SPI_CTAR_LSBFE_MASK | SPI_CTAR_BR(7) | SPI_CTAR_FMSZ(15);

  // TODO!
}
