#include <string.h>
#include <stdint.h>

#include "MK02F12810.h"

#include "anki/cozmo/robot/hal.h"
#include "anki/cozmo/robot/drop.h"
#include "hal/portable.h"

#include "spi.h"

const int MAX_JPEG_DATA = 128;
const int TRANSMISSION_SIZE = DROP_SIZE;
static uint32_t spi_tx_buff[TRANSMISSION_SIZE];
static uint8_t  spi_rx_buff[TRANSMISSION_SIZE];

void Anki::Cozmo::HAL::TransmitDrop(const uint8_t* buf, int buflen, int eof) {
  static uint8_t jb = 0;
	DropToWiFi drop;
  uint8_t* txSrc = (uint8_t*) &drop;
  uint8_t* txDst = (uint8_t*) spi_tx_buff;

  // This should be altered to 
#if 1
  memcpy(drop.payload, buf, buflen);  
  drop.droplet = JPEG_LENGTH(buflen) | (eof ? jpegEOF : 0);
#else
	for (int i=0; i<72; i++) drop.payload[i] = jb++;
	drop.msgLen  = 0;
	drop.droplet = JPEG_LENGTH(72) | (eof ? jpegEOF : 0);
	drop.tail[0] = 0x90;
	drop.tail[1] = 0x91;
	drop.tail[2] = 0x92;
	if (eof) jb = 0;
#endif
	
  for (int i = 0; i < sizeof(drop); i++, txDst += sizeof(uint32_t))
    *(txDst) = *(txSrc++);

  // Clear done flags for DMA
  DMA_CDNE = DMA_CDNE_CDNE(2);
  DMA_CDNE = DMA_CDNE_CDNE(3);

  // Enable DMA
  SPIInitDMA();
  DMA_ERQ |= DMA_ERQ_ERQ2_MASK | DMA_ERQ_ERQ3_MASK;
}

void Anki::Cozmo::HAL::SPIInitDMA(void) {
  const uint32_t num_words = TRANSMISSION_SIZE;
  const int transferSize = sizeof(uint32_t);

  // Disable DMA
  DMA_ERQ &= ~DMA_ERQ_ERQ3_MASK & ~DMA_ERQ_ERQ2_MASK;

  // Configure receive buffer
  DMAMUX_CHCFG2 = (DMAMUX_CHCFG_ENBL_MASK | DMAMUX_CHCFG_SOURCE(14)); 

  DMA_TCD2_SADDR          = (uint32_t)&(SPI0_POPR);
  DMA_TCD2_SOFF           = 0;
  DMA_TCD2_SLAST          = 0;

  DMA_TCD2_DADDR          = (uint32_t)spi_rx_buff;
  DMA_TCD2_DOFF           = 1;
  DMA_TCD2_DLASTSGA       = -num_words;

  DMA_TCD2_NBYTES_MLNO    = transferSize;                               // The minor loop moves 32 bytes per transfer
  DMA_TCD2_BITER_ELINKNO  = num_words;                                  // Major loop iterations
  DMA_TCD2_CITER_ELINKNO  = num_words;                                  // Set current interation count  
  DMA_TCD2_ATTR           = (DMA_ATTR_SSIZE(0) | DMA_ATTR_DSIZE(0));    // Source/destination size (8bit)
 
  DMA_TCD2_CSR            = DMA_CSR_DREQ_MASK;                          // clear ERQ @ end of major iteration               

  // Configure transfer buffer
  DMAMUX_CHCFG3 = (DMAMUX_CHCFG_ENBL_MASK | DMAMUX_CHCFG_SOURCE(15)); 

  DMA_TCD3_SADDR          = (uint32_t)spi_tx_buff;
  DMA_TCD3_SOFF           = transferSize;
  DMA_TCD3_SLAST          = -num_words * transferSize;

  DMA_TCD3_DADDR          = (uint32_t)&(SPI0_PUSHR);
  DMA_TCD3_DOFF           = 0;
  DMA_TCD3_DLASTSGA       = 0;

  DMA_TCD3_NBYTES_MLNO    = transferSize;                               // The minor loop moves 32 bytes per transfer
  DMA_TCD3_BITER_ELINKNO  = num_words;                                  // Major loop iterations
  DMA_TCD3_CITER_ELINKNO  = num_words;                                  // Set current interation count  
  DMA_TCD3_ATTR           = (DMA_ATTR_SSIZE(2) | DMA_ATTR_DSIZE(2));    // Source/destination size (8bit)
 
  DMA_TCD3_CSR            = DMA_CSR_DREQ_MASK;                          // clear ERQ @ end of major iteration               
}

void Anki::Cozmo::HAL::SPIInit(void) {
  // Turn on power to DMA, PORTC and SPI0
  SIM_SCGC6 |= SIM_SCGC6_SPI0_MASK | SIM_SCGC6_DMAMUX_MASK;
  SIM_SCGC5 |= SIM_SCGC5_PORTE_MASK | SIM_SCGC5_PORTD_MASK;
  SIM_SCGC7 |= SIM_SCGC7_DMA_MASK;

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

    spi_tx_buff[i] = 
      (~i & 0xFF) |
      SPI_PUSHR_CONT_MASK | 
      SPI_PUSHR_PCS((~i & 1) ? ~0: 0);
  }
}
