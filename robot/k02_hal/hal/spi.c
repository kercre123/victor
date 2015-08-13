#include <string.h>
#include <stdint.h>

#include "board.h"

#include "spi.h"

void spi_init(void) {
  // Turn on power to DMA, PORTC and SPI0
  SIM_SCGC6 |= SIM_SCGC6_SPI0_MASK | SIM_SCGC6_DMAMUX_MASK;
  SIM_SCGC5 |= SIM_SCGC5_PORTC_MASK;
  SIM_SCGC7 |= SIM_SCGC7_DMA_MASK;

  // Configure SPI pins
  PORTC_PCR3 = PORT_PCR_MUX(2); // SPI0_PCS1
  PORTC_PCR5 = PORT_PCR_MUX(2); // SPI0_SCK
  PORTC_PCR6 = PORT_PCR_MUX(2); // SPI0_SOUT
  PORTC_PCR7 = PORT_PCR_MUX(2) | PORT_PCR_PE_MASK | PORT_PCR_PS_MASK ; // SPI0_SIN (Temporary pull-up)

  
  // Configure perf
  SPI0_MCR = SPI_MCR_MSTR_MASK | SPI_MCR_DCONF(0) | SPI_MCR_SMPL_PT(0);
  SPI0_CTAR0 = SPI_CTAR_LSBFE_MASK | SPI_CTAR_BR(4) | SPI_CTAR_FMSZ(15);

  for(;;) {
    SPI0_PUSHR = SPI_PUSHR_CTAS(0) | SPI_PUSHR_PCS(2) | 0x55;
    BOARD_I2C_DELAY ;
    SPI0_PUSHR = SPI_PUSHR_CTAS(0) | SPI_PUSHR_PCS(0) | 0x55;
    BOARD_I2C_DELAY ;
  }
}
