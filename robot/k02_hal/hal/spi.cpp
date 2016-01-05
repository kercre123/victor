#include <string.h>
#include <stdint.h>

#include "MK02F12810.h"

#include "anki/cozmo/robot/hal.h"
#include "anki/cozmo/robot/drop.h"
#include "hal/portable.h"

#include "spi.h"
#include "uart.h"
#include "dac.h"
#include "oled.h"
#include "i2c.h"
#include "wifi.h"

typedef uint16_t transmissionWord;
const int RX_OVERFLOW = 8;
const int TX_SIZE = DROP_TO_WIFI_SIZE / sizeof(transmissionWord);
const int RX_SIZE = DROP_TO_RTIP_SIZE / sizeof(transmissionWord) + RX_OVERFLOW;

static transmissionWord spi_backbuff[2][TX_SIZE];

static transmissionWord* spi_write_buff = spi_backbuff[0];
static transmissionWord* spi_tx_buff = spi_backbuff[1];

transmissionWord spi_rx_buff[RX_SIZE];

static int totalDrops = 0;

static bool ProcessDrop(void) {
  using namespace Anki::Cozmo::HAL;
  static int pwmCmdCounter = 0;

  // Process drop receive
  transmissionWord *target = spi_rx_buff;
  for (int i = 0; i < RX_OVERFLOW; i++, target++) {
    if (*target != TO_RTIP_PREAMBLE) continue ;
    
    DropToRTIP* drop = (DropToRTIP*)target;

    if (drop->droplet & screenDataValid) {
      OLED::FeedFace(drop->screenInd, drop->screenData);
    }
    
    DAC::Feed(drop->audioData, MAX_AUDIO_BYTES_PER_DROP);
    DAC::EnableAudio(drop->droplet & audioDataValid);

    uint8_t *payload_data = (uint8_t*) drop->payload;
    totalDrops++;
    
    if (drop->payloadLen)
    {
      uint8_t *payload_data = (uint8_t*) drop->payload;
      // Handle OTA related messages right here so it's harder to break them
      switch (*payload_data)
      {
      case DROP_EnterBootloader:
      {
        EnterBootloader ebl;
          memcpy(&ebl, payload_data+1, sizeof(ebl));
          switch (ebl.which)
          {
          case BOOTLOAD_RTIP:
            {
            SPI::EnterRecoveryMode();
              break;
            }
          case BOOTLOAD_BODY:
            {
            UART::EnterBodyRecovery();
              break;
        }
          }
        break;
      }
      case DROP_BodyUpgradeData:
      {
        BodyUpgradeData bud;
          memcpy(&bud, payload_data+1, sizeof(bud));
        UART::SendRecoveryData((uint8_t*) &bud.data, sizeof(bud.data));
        break;
      }
        default:
      {
          WiFi::ReceiveMessage(drop->payload, drop->payloadLen);
      }
    }
    }
    
    return true;
  }
  
  return false;
}

void Anki::Cozmo::HAL::SPI::StartDMA(void) {
  // Start sending out junk
  SPI0_MCR |= SPI_MCR_CLR_RXF_MASK;
  DMA_TCD3_SADDR = (uint32_t)spi_write_buff;
  DMA_ERQ |= DMA_ERQ_ERQ2_MASK | DMA_ERQ_ERQ3_MASK;
}

void Anki::Cozmo::HAL::SPI::TransmitDrop(const uint8_t* buf, int buflen, int eof) {   
  // Swap buffers
  {
    transmissionWord *tmp = spi_write_buff;
    spi_write_buff = spi_tx_buff;
    spi_tx_buff = tmp;
  }

  DropToWiFi *drop_tx = (DropToWiFi*) spi_write_buff;

  drop_tx->preamble = TO_WIFI_PREAMBLE;
  drop_tx->droplet  = JPEG_LENGTH(buflen) | (eof ? jpegEOF : 0) | ToWiFi;

  memcpy(drop_tx->payload, buf, buflen);
  
  // This is where a drop should be 
  uint8_t *drop_addr = drop_tx->payload + buflen;
  const int remainingSpace = DROP_TO_WIFI_MAX_PAYLOAD - buflen;
  if (remainingSpace > 0)
  {
    drop_tx->payloadLen = Anki::Cozmo::HAL::WiFi::GetTxData(drop_addr, remainingSpace);
    if ((drop_tx->payloadLen == 0) && (remainingSpace >= sizeof(BodyState))) // Have nothing to send so transmit body state info
    {
      BodyState bodyState;
      bodyState.state = UART::recoveryMode;
      bodyState.count = UART::RecoveryStateUpdated;
      memcpy(drop_addr, &bodyState, sizeof(BodyState));
      drop_tx->payloadLen = sizeof(BodyState);
      drop_tx->droplet |= bootloaderStatus;
    }
  }
}

void Anki::Cozmo::HAL::SPI::EnterRecoveryMode(void) {
  __disable_irq();
  static uint32_t* recovery_word = (uint32_t*) 0x20001FFC;
  static const uint32_t recovery_value = 0xCAFEBABE;
  *recovery_word = recovery_value;
  NVIC_SystemReset();
};

extern "C"
void DMA2_IRQHandler(void) {
  using namespace Anki::Cozmo::HAL;
  DMA_CDNE = DMA_CDNE_CDNE(2);
  DMA_CINT = 2;

  static bool allowReset = true;
  static int droppedDrops = 0;
  
  I2C::Disable();

  // Don't check for silence, we had a drop
  if (ProcessDrop()) {
    if (allowReset) {
      droppedDrops = 0;
      allowReset = false;
    }
    
    return ;
  } else {
    droppedDrops ++;
  }

  // Check for silence in the 
  static const int MaximumSilence = 32;
  static transmissionWord lastWord = 0;
  static int SilentDrops = 0;

  if (lastWord != spi_rx_buff[0]) {
    lastWord = spi_rx_buff[0];
    SilentDrops = 0;
  } else if (++SilentDrops == MaximumSilence) {
    switch (lastWord) {
      case 0x8001:
        NVIC_SystemReset();
        break ;
      case 0x8002:
        SPI::EnterRecoveryMode();
        break ;
      case 0x8004:
        __disable_irq();
        break ;
    }
  }
}

extern "C"
void DMA3_IRQHandler(void) {
  DMA_CDNE = DMA_CDNE_CDNE(3);
  DMA_CINT = 3;
}

void Anki::Cozmo::HAL::SPI::InitDMA(void) {
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
  Anki::Cozmo::HAL::UART::DebugPrintf("Syncing to espressif clock... ");
  
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
  
  Anki::Cozmo::HAL::UART::DebugPrintf("Done.\n\r");
  __enable_irq();
}

void Anki::Cozmo::HAL::SPI::Init(void) {
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

  SPI::InitDMA();
  SyncSPI();
}
