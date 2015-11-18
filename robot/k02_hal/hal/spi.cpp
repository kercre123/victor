#include <string.h>
#include <stdint.h>

#include "MK02F12810.h"

#include "anki/cozmo/robot/hal.h"
#include "anki/cozmo/robot/drop.h"
#include "hal/portable.h"

#include "spi.h"
#include "uart.h"

typedef uint16_t transmissionWord;
const int TX_SIZE = DROP_TO_WIFI_SIZE / sizeof(transmissionWord);
const int RX_SIZE = DROP_TO_RTIP_SIZE / sizeof(transmissionWord);

static transmissionWord spi_tx_buff[TX_SIZE];
static union {
  transmissionWord spi_tx_side[TX_SIZE];
  DropToWiFi drop_tx ;
};

static union {
  transmissionWord spi_rx_buff[RX_SIZE];
  DropToRTIP drop_rx ;
};

void Anki::Cozmo::HAL::TransmitDrop(const uint8_t* buf, int buflen, int eof) { 
  static int eoftime = 0;
  eoftime++;
  drop_tx.preamble = TO_WIFI_PREAMBLE;
  memcpy(drop_tx.payload, "0123456789ABCDEF", 16);
  drop_tx.payloadLen  = 0;
  drop_tx.droplet = JPEG_LENGTH(16) | (eoftime & 63 ? 0 : jpegEOF);
}

extern "C"
void DMA2_IRQHandler(void) {
  // Process drop receive

  DMA_CDNE = DMA_CDNE_CDNE(2);
  DMA_CINT = 2;
}

extern "C"
void DMA3_IRQHandler(void) {
  memcpy(spi_tx_buff, spi_tx_side, sizeof(spi_tx_side));

  DMA_CDNE = DMA_CDNE_CDNE(3);
  DMA_CINT = 3;
}

void Anki::Cozmo::HAL::SPIInitDMA(void) {
  // Disable DMA
  DMA_ERQ &= ~DMA_ERQ_ERQ3_MASK & ~DMA_ERQ_ERQ2_MASK;

  // Configure receive buffer
  DMAMUX_CHCFG2 = (DMAMUX_CHCFG_ENBL_MASK | DMAMUX_CHCFG_SOURCE(14)); 

  DMA_TCD2_SADDR          = (uint32_t)&(SPI0_POPR);
  DMA_TCD2_SOFF           = 0;
  DMA_TCD2_SLAST          = 0;

  DMA_TCD2_DADDR          = (uint32_t)spi_rx_buff;
  DMA_TCD2_DOFF           = sizeof(transmissionWord);
  DMA_TCD2_DLASTSGA       = -sizeof(spi_rx_buff);

  DMA_TCD2_NBYTES_MLNO    = sizeof(transmissionWord);                   // The minor loop moves 32 bytes per transfer
  DMA_TCD2_BITER_ELINKNO  = RX_SIZE;                          // Major loop iterations
  DMA_TCD2_CITER_ELINKNO  = RX_SIZE;                          // Set current interation count  
  DMA_TCD2_ATTR           = (DMA_ATTR_SSIZE(1) | DMA_ATTR_DSIZE(1));    // Source/destination size (8bit)
 
  DMA_TCD2_CSR            = DMA_CSR_DREQ_MASK | DMA_CSR_INTMAJOR_MASK;  // clear ERQ @ end of major iteration               

  // Configure transfer buffer
  DMAMUX_CHCFG3 = (DMAMUX_CHCFG_ENBL_MASK | DMAMUX_CHCFG_SOURCE(15)); 

  DMA_TCD3_SADDR          = (uint32_t)spi_tx_buff;
  DMA_TCD3_SOFF           = sizeof(transmissionWord);
  DMA_TCD3_SLAST          = -sizeof(spi_tx_buff);

  DMA_TCD3_DADDR          = (uint32_t)&(SPI0_PUSHR_SLAVE);
  DMA_TCD3_DOFF           = 0;
  DMA_TCD3_DLASTSGA       = 0;

  DMA_TCD3_NBYTES_MLNO    = sizeof(transmissionWord);                   // The minor loop moves 32 bytes per transfer
  DMA_TCD3_BITER_ELINKNO  = TX_SIZE;                          // Major loop iterations
  DMA_TCD3_CITER_ELINKNO  = TX_SIZE;                          // Set current interation count
  DMA_TCD3_ATTR           = (DMA_ATTR_SSIZE(1) | DMA_ATTR_DSIZE(1));    // Source/destination size (8bit)
 
  DMA_TCD3_CSR            = DMA_CSR_DREQ_MASK | DMA_CSR_INTMAJOR_MASK;  // clear ERQ @ end of major iteration               

  NVIC_EnableIRQ(DMA2_IRQn);
  NVIC_EnableIRQ(DMA3_IRQn);
}

inline uint16_t WaitForByte(void) {
  while(~SPI0_SR & SPI_SR_RFDF_MASK) ;  // Wait for a byte    
  uint16_t ret = SPI0_POPR;
  SPI0_SR = SPI0_SR;
  return ret;
}

void SyncSPI(void) {
  // Syncronize SPI to WS
  __disable_irq();
  Anki::Cozmo::HAL::DebugPrintf("Syncing to espressif clock... ");
  
  for (;;) {
    // Flush SPI
    SPI0_MCR = SPI_MCR_CLR_TXF_MASK |
               SPI_MCR_CLR_RXF_MASK;
    SPI0_SR = SPI0_SR;

    SPI0_PUSHR_SLAVE = 0xAAA0;
    PORTE_PCR17 = PORT_PCR_MUX(2);    // SPI0_SCK (enabled)

    WaitForByte();
    
    // Make sure we are talking to the perf
    {
      const int loops = 3;
      bool success = true;

      for(int i = 0; i < loops; i++) {
        WaitForByte();
        if (WaitForByte() != 0x8000) success = false ;
      }

      if (success) break ;
    }
    
    PORTE_PCR17 = PORT_PCR_MUX(0);    // SPI0_SCK (disabled)
  }
  
  Anki::Cozmo::HAL::DebugPrintf("Done.\n\r");
  __enable_irq();
}

void Anki::Cozmo::HAL::SPIInit(void) {
  // Turn on power to DMA, PORTC and SPI0
  SIM_SCGC6 |= SIM_SCGC6_SPI0_MASK | SIM_SCGC6_DMAMUX_MASK;
  SIM_SCGC5 |= SIM_SCGC5_PORTD_MASK | SIM_SCGC5_PORTE_MASK;
  SIM_SCGC7 |= SIM_SCGC7_DMA_MASK;

  // Configure SPI pins
  PORTD_PCR0  = PORT_PCR_MUX(2) |  // SPI0_PCS0 (internal)
                PORT_PCR_PE_MASK;

  PORTD_PCR4  = PORT_PCR_MUX(1);
  GPIOD_PDDR &= ~(1 << 4);

  PORTE_PCR18 = PORT_PCR_MUX(2); // SPI0_SOUT
  PORTE_PCR19 = PORT_PCR_MUX(2); // SPI0_SIN

  // Configure SPI perf to the magical value of magicalness
  SPI0_MCR = SPI_MCR_DCONF(0) |
             SPI_MCR_SMPL_PT(0) |
             SPI_MCR_CLR_TXF_MASK |
             SPI_MCR_CLR_RXF_MASK;

  SPI0_CTAR0_SLAVE = SPI_CTAR_FMSZ(15);

  SPI0_RSER = SPI_RSER_TFFF_RE_MASK |
              SPI_RSER_TFFF_DIRS_MASK |
              SPI_RSER_RFDF_RE_MASK |
              SPI_RSER_RFDF_DIRS_MASK;

  // Clear all status flags
  SPI0_SR = SPI0_SR;

  SPIInitDMA();
  SyncSPI();
}
