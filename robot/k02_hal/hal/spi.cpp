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

static void start_tx(uint32_t num_bytes) {
  void* source_addr = spi_tx_buff;
  void* dest_addr = (void*)&(SPI0_PUSHR);
  
  // Disable DMA
  DMA_ERQ &= ~DMA_ERQ_ERQ1_MASK;

  // DMA source DMA Mux (i2c0)
  DMAMUX_CHCFG1 = (DMAMUX_CHCFG_ENBL_MASK | DMAMUX_CHCFG_SOURCE(18)); 
  
  // Configure source address
  DMA_TCD2_SADDR          = (uint32_t)source_addr;
  DMA_TCD2_SOFF           = 1;
  DMA_TCD2_SLAST          = -num_bytes;

  // Configure source address
  DMA_TCD2_DADDR          = (uint32_t)dest_addr;
  DMA_TCD2_DOFF           = 0;
  DMA_TCD2_DLASTSGA       = 0;
  
  DMA_TCD2_NBYTES_MLNO    = 1;                                          // The minor loop moves 32 bytes per transfer
  DMA_TCD2_BITER_ELINKNO  = num_bytes;                                  // Major loop iterations
  DMA_TCD2_CITER_ELINKNO  = num_bytes;                                  // Set current interation count  
  DMA_TCD2_ATTR           = (DMA_ATTR_SSIZE(0) | DMA_ATTR_DSIZE(0));    // Source/destination size (8bit)
 
  DMA_TCD1_CSR            = DMA_CSR_DREQ_MASK;                          // Enable end of loop DMA interrupt; clear ERQ @ end of major iteration               

  // Enable DMA
  DMA_ERQ |= DMA_ERQ_ERQ1_MASK;
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
             SPI_MCR_CLR_RXF_MASK;

  SPI0_CTAR0 = SPI_CTAR_BR(0) |
               SPI_CTAR_CPOL_MASK |
               SPI_CTAR_CPHA_MASK |
               SPI_CTAR_FMSZ(7);

  // Clear all status flags
  SPI0_SR = SPI0_SR;

  for (int i = 0; i < size; i++) {
    int k = i >> 1;

    spi_tx_buff[i] = SPI_PUSHR_CONT_MASK | SPI_PUSHR_PCS((~i & 1) ? ~0: 0);
  }

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
