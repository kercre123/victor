#include <string.h>
#include <stdint.h>

#include "board.h"
#include "fsl_debug_console.h"
#include "anki/cozmo/robot/hal.h"
#include "hal/portable.h"

#include "spi.h"

const int TRANSMISSION_SIZE = 192;
static uint32_t spi_tx_buff[TRANSMISSION_SIZE];
static uint8_t  spi_rx_buff[TRANSMISSION_SIZE];

// 14 = SPI0 RX
// 15 = SPI0_TX

static void start_tx() {
  uint32_t num_words = TRANSMISSION_SIZE;
  
  // Disable DMA
  DMA_ERQ &= ~DMA_ERQ_ERQ3_MASK & ~DMA_ERQ_ERQ2_MASK;

  // DMA source DMA Mux (spi rx/tx)
  DMAMUX_CHCFG2 = (DMAMUX_CHCFG_ENBL_MASK | DMAMUX_CHCFG_SOURCE(14)); 
  DMAMUX_CHCFG3 = (DMAMUX_CHCFG_ENBL_MASK | DMAMUX_CHCFG_SOURCE(15)); 
  
  // Configure source address
  DMA_TCD3_SADDR          = (uint32_t)spi_tx_buff;
  DMA_TCD3_SOFF           = 1;
  DMA_TCD3_SLAST          = -num_words;

  // Configure source address
  DMA_TCD3_DADDR          = (uint32_t)&(SPI0_PUSHR);
  DMA_TCD3_DOFF           = 0;
  DMA_TCD3_DLASTSGA       = 0;

  DMA_TCD3_NBYTES_MLNO    = 1;                                          // The minor loop moves 32 bytes per transfer
  DMA_TCD3_BITER_ELINKNO  = num_words;                                  // Major loop iterations
  DMA_TCD3_CITER_ELINKNO  = num_words;                                  // Set current interation count  
  DMA_TCD3_ATTR           = (DMA_ATTR_SSIZE(2) | DMA_ATTR_DSIZE(2));    // Source/destination size (8bit)
 
  DMA_TCD3_CSR            = DMA_CSR_DREQ_MASK;                          // clear ERQ @ end of major iteration               

  // Enable DMA
  DMA_ERQ |= DMA_ERQ_ERQ3_MASK;
}

void spi_init(void) {
  Anki::Cozmo::HAL::MicroWait(1000000);

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
             SPI_MCR_CLR_RXF_MASK |
             SPI_MCR_CONT_SCKE_MASK;

  SPI0_CTAR0 = SPI_CTAR_BR(0) |
               SPI_CTAR_CPOL_MASK |
               SPI_CTAR_CPHA_MASK |
               SPI_CTAR_FMSZ(7);

  SPI0_RSER = SPI_RSER_TFFF_RE_MASK | 
              SPI_RSER_TFFF_DIRS_MASK |
              SPI_RSER_RFDF_RE_MASK |
              SPI_RSER_RFDF_DIRS_MASK;
              

  // Clear all status flags
  SPI0_SR = SPI0_SR;

  for (int i = 0; i < TRANSMISSION_SIZE; i++) {
    int k = i >> 1;

    spi_tx_buff[i] = SPI_PUSHR_CONT_MASK | SPI_PUSHR_PCS((~i & 1) ? ~0: 0);
  }

  start_tx();
}
