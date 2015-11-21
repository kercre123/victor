/*
 * @file I2S driver for Espressif chip.
 * @author Daniel Casner <daniel@anki.com>
 *
 * Original code taken from Espressif MP3 decoder project. Heavily reworked to liberate it from FreeRTOS and enable
 * bidirectional communication.
 */

#include "ets_sys.h"
#include "osapi.h"
#include "driver/uart.h"
#include "driver/i2s_reg.h"
#include "driver/slc_register.h"
#include "driver/sdio_slv.h"
#include "driver/i2spi.h" 
#include "driver/i2s_ets.h"
#include "imageSender.h"

// Forward declaration
uint32 PumpAudioData(void);
uint32 PumpScreenData(void);

#define min(a, b) (a < b ? a : b)
#define asDesc(x) ((struct sdio_queue*)(x))

/// Flags to control the DMA mode
enum {
  PHASE_FLAGS = -32000, ///< Anything below this value is a flag, not a valid phase value
  UNINITALIZED_PHASE    = PHASE_FLAGS-1, ///< Haven't achieved sync
  BOOTLOADER_SYNC_PHASE = PHASE_FLAGS-2, ///< Synchronizing with RTIP bootloader
  BOOTLOADER_XFER_PHASE = PHASE_FLAGS-3, ///< Bootloader mode
  PAUSED_PHASE          = PHASE_FLAGS-4, ///< I2SPI communication paused, sending 0xFFFFffff
};

uint32_t i2spiTxUnderflowCount;
uint32_t i2spiTxOverflowCount;
uint32_t i2spiRxOverflowCount;
uint32_t i2spiPhaseErrorCount;
int32_t  i2spiIntegralDrift;

/// Signals to the I2SPI task
enum
{
  TASK_SIG_I2SPI_RX,
  TASK_SIG_I2SPI_TX,
  TASK_SIG_I2SPI_BL_RX,
  TASK_SIG_I2SPI_BL_TX,
};

#define I2SPI_TASK_QUEUE_LEN (DMA_BUF_COUNT * 2)
/// Queue for I2SPI tasks. 2x the buffer count because have tasks for send and receive
os_event_t i2spiTaskQ[I2SPI_TASK_QUEUE_LEN];

/// DMA I2SPI transmit buffers
static unsigned int txBufs[DMA_BUF_COUNT][DMA_BUF_SIZE/4];
/// DMA transmit queue control structure
static struct sdio_queue txQueue[DMA_BUF_COUNT];
/// Number of TX DMA buffers we've filled but not yet transmitted
static uint8_t txFillCount;
/// DMA I2SPI transmit buffers
static unsigned int rxBufs[DMA_BUF_COUNT][DMA_BUF_SIZE/4];
/// DMA transmit queue control structure
static struct sdio_queue rxQueue[DMA_BUF_COUNT];
/// A pointer to the next txQueue we can fill
static struct sdio_queue* nextOutgoingDesc;
/// Stores the alignment for outgoing drops.
static int16_t outgoingPhase;
/// Phase relationship between incoming drops and outgoing drops, determined through experimentation
#define DROP_TX_PHASE_ADJUST ((DROP_SPACING-24)/2)
/// The last state we've received from the RTIP bootloader regarding it's state
static int16_t rtipBootloaderState;
/// The last state we've received from the RTIP updating the Body bootloader state
static int16_t bodyBootloaderState;

/// Prep an sdio_queue structure (DMA descriptor) for (re)use
void prepSdioQueue(struct sdio_queue* desc, uint8 eof)
{
  desc->owner     = 1;
  desc->eof       = eof;
  desc->sub_sof   = 0;
  desc->datalen   = DMA_BUF_SIZE;
  desc->blocksize = DMA_BUF_SIZE;
  desc->unused    = 0;
}

/** 16-bit half word wise copy function
 * Copies num words from src to dest
 */
void halfWordCopy(uint16_t* dest, uint16_t* src, int num)
{
  int w;
  for (w=0; w<num; ++w)
  {
    dest[w] = src[w];
  }
}

/** Processes an incomming drop from the RTIP to the WiFi
 * @param drop A pointer to a complete drop
 * @warning Call from a task not an ISR.
 */
void processDrop(DropToWiFi* drop)
{
  const uint8 rxJpegLen = (drop->droplet & jpegLenMask) * 4;
  if (rxJpegLen > 0) imageSenderQueueData(drop->payload, rxJpegLen, drop->droplet & jpegEOF);
  if (drop->droplet & bootloaderStatus) os_memcpy(&bodyBootloaderState, drop->payload, sizeof(bootloaderStatus));
  // XXX do stuff with the rest of the payload
}

/** Fills a drop into the outgoing DMA buffers
 * Uses the outgoingPhase and nextOutgoingDesc module variables and advances them as nessisary
 * @param payload A pointer to the payload data to fill the drop with or NULL
 * @param length The number of bytes of payload to be put into this drop
 */
bool makeDrop(uint8_t* payload, uint8_t length)
{
  if (DMA_BUF_SIZE/2 - outgoingPhase < DROP_TO_RTIP_SIZE/2) // Don't have room for the whole drop in this buffer
  {
    if (txFillCount > (DMA_BUF_COUNT-2)) // Leave room for the one the I2S peripheral owns
    {
      i2spiTxOverflowCount++;
      return false;
    }
  }
  DropToRTIP drop;
  uint32 isocData;
  os_memset(&drop, 0, DROP_TO_RTIP_SIZE);
  drop.preamble = TO_RTIP_PREAMBLE;
  // Fill in the drop itself
  isocData = PumpAudioData();
  os_memcpy(&(drop.audioData), &isocData, MAX_AUDIO_BYTES_PER_DROP);
  isocData = PumpScreenData();
  os_memcpy(&drop.screenData, &isocData, MAX_SCREEN_BYTES_PER_DROP);
  drop.droplet = screenDataValid | audioDataValid;
  drop.payloadLen = length;
  if (payload != NULL) os_memcpy(&(drop.payload), payload, length);
  // Copy into the DMA buffer
  ets_intr_lock();
  uint16_t* txBuf = (uint16_t*)(nextOutgoingDesc->buf_ptr);
  if (DMA_BUF_SIZE/2 - outgoingPhase >= DROP_TO_RTIP_SIZE/2) // Whole drop fits here
  {
    halfWordCopy(txBuf + outgoingPhase, (uint16_t*)&drop, DROP_TO_RTIP_SIZE/2);
    outgoingPhase += DROP_SPACING/2;
    if (outgoingPhase > DMA_BUF_SIZE/2) // Have rolled over into next buffer
    {
      txFillCount++;
      nextOutgoingDesc = asDesc(nextOutgoingDesc->next_link_ptr);
      txBuf = (uint16_t*)(nextOutgoingDesc->buf_ptr);
      os_memset(txBuf, 0, DMA_BUF_SIZE);
      outgoingPhase -= DMA_BUF_SIZE/2;
    }
  }
  else // Split across two buffers
  {
    const int16_t halfWordsWritten = DMA_BUF_SIZE/2 - outgoingPhase;
    halfWordCopy(txBuf + outgoingPhase, (uint16_t*)&drop, halfWordsWritten);
    txFillCount++;
    nextOutgoingDesc = asDesc(nextOutgoingDesc->next_link_ptr);
    txBuf = (uint16_t*)(nextOutgoingDesc->buf_ptr);
    os_memset(txBuf, 0, DMA_BUF_SIZE);
    halfWordCopy(txBuf, ((uint16_t*)&drop) + halfWordsWritten, DROP_TO_RTIP_SIZE/2 - halfWordsWritten);
    outgoingPhase += DROP_SPACING/2 - DMA_BUF_SIZE/2;
  }
  ets_intr_unlock();
  return true;
}

ct_assert(DMA_BUF_SIZE == 512); // We assume that the DMA buff size is 128 32bit words in a lot of logic below.
#define DRIFT_MARGIN 2

void i2spiTask(os_event_t *event)
{
  struct sdio_queue* desc = asDesc(event->par);

  if (desc == NULL)
  {
    os_printf("ERROR: I2SPI task got null descriptor with signal %u\r\n", (unsigned int)event->sig);
    return;
  }

  switch (event->sig)
  {
    case TASK_SIG_I2SPI_RX:
    {
      static DropToWiFi drop;
      static uint8 dropWrInd = 0; // In 16bit half-words
      static int16_t dropPhase = 0; ///< Stores the estiamted alightment of drops in the DMA buffer.
      static int16_t drift = 0;
      uint16_t* buf = (uint16_t*)(desc->buf_ptr);
      while(true)
      {
        while((dropPhase < DMA_BUF_SIZE/2) && (dropWrInd == 0)) // Search for preamble
        {
          if (buf[dropPhase] == TO_WIFI_PREAMBLE)
          {
            if (unlikely(drift > DRIFT_MARGIN)) 
            {
              i2spiRxOverflowCount++;
              os_printf("!I2SPI too much drift: %d, RX overflow count: %d\r\n", drift, i2spiRxOverflowCount);
            }
            if (unlikely(outgoingPhase == UNINITALIZED_PHASE)) // Haven't established outgoing phase yet
            {
              // Going past the end is OKAY as that will be used to increment the buffer
              outgoingPhase = dropPhase + DROP_TX_PHASE_ADJUST;
            }
            else // Have a phase, just adjust for drift
            {
              outgoingPhase += drift;
            }
            i2spiIntegralDrift += drift;
            drift = -DRIFT_MARGIN;
            break;
          }
          dropPhase++;
          drift++;
        }
        if (likely(dropPhase < DMA_BUF_SIZE/2)) // If we found a header
        {
          const int8 halfWordsToRead = min(DROP_TO_WIFI_SIZE-(dropWrInd*2), DMA_BUF_SIZE - (dropPhase*2))/2;
          halfWordCopy(((uint16_t*)(&drop)) + dropWrInd, buf + dropPhase + dropWrInd, halfWordsToRead);
          dropWrInd += halfWordsToRead;
          if (dropWrInd*2 == DROP_TO_WIFI_SIZE) // The end of the drop was in this buffer
          {
            processDrop(&drop);
            dropWrInd = 0;
            dropPhase += (DROP_SPACING/2) - DRIFT_MARGIN;
          }
          else break; // The end of the drop wasn't in this buffer, go on to the next one
        }
        else break; // Didn't find the next header in this one
      }
      dropPhase -= DMA_BUF_SIZE/2; // Now looking in next buffer
      prepSdioQueue(desc, 0);
      break;
    }
    case TASK_SIG_I2SPI_TX:
    {
      if (unlikely(outgoingPhase < 0)) // Durring startup
      {
        if (outgoingPhase == UNINITALIZED_PHASE)
        {
          uint32_t* txBuf = (uint32_t*)desc->buf_ptr;
          int w;
          for (w=0; w<DMA_BUF_SIZE/4; w++) txBuf[w] = 0x80000000;
          ets_intr_lock();
          nextOutgoingDesc = asDesc(nextOutgoingDesc->next_link_ptr);
          ets_intr_unlock();
        }
      }
      else // When running
      {
        if (txFillCount > 0) txFillCount--; // We just transmitted one 
        struct sdio_queue* newOut = asDesc(asDesc(desc->next_link_ptr)->next_link_ptr);
        while (newOut == nextOutgoingDesc) // Fill in drops until we are as far ahead as we want to be
        {
          i2spiTxUnderflowCount++; // Keep track of the underflow
          makeDrop(NULL, 0);
        }
      }
      break;
    }
    case TASK_SIG_I2SPI_BL_RX:
    {
      uint16_t* buf = (uint16_t*)(desc->buf_ptr);
      rtipBootloaderState = buf[DMA_BUF_SIZE/2 - 1]; // Last half-word in buffer is latest state we know
      if (rtipBootloaderState == STATE_IDLE) outgoingPhase = BOOTLOADER_XFER_PHASE;
      prepSdioQueue(desc, 0);
      break;
    }
    case TASK_SIG_I2SPI_BL_TX:
    {
      if (outgoingPhase == BOOTLOADER_SYNC_PHASE)
      {
        uint32_t* txBuf = (uint32_t*)desc->buf_ptr;
        int w;
        for (w=0; w<DMA_BUF_SIZE/4; w++) txBuf[w] = 0x80000000;
        ets_intr_lock();
        nextOutgoingDesc = asDesc(nextOutgoingDesc->next_link_ptr);
        ets_intr_unlock();
      }
      else // When running
      {
        if (txFillCount > 0) txFillCount--; // We just transmitted one
        ets_intr_lock();
        struct sdio_queue* newOut = asDesc(asDesc(desc->next_link_ptr)->next_link_ptr);
        if (newOut == nextOutgoingDesc) // Fill in drops until we are as far ahead as we want to be
        {
          int w;
          uint32_t* txBuf = (uint32_t*)newOut->buf_ptr;
          for (w=0; w<DMA_BUF_SIZE/4; w++) txBuf[w] = 0;
          i2spiTxUnderflowCount++; // Keep track of the underflow
          nextOutgoingDesc = asDesc(nextOutgoingDesc->next_link_ptr);
        }
        ets_intr_unlock();
      }
      break;
    }
    default:
    {
      os_printf("ERROR: Unexpected I2SPI task signal signal: %u, %08x\r\n",
                (unsigned int)event->sig, (unsigned int)event->par);
    }
  }
}


/** DMA buffer complete ISR
 * Called for both completed SLC_TX (I2SPI receive) and SLC_RX (I2SPI transmit) events
 * Everything is passed off to tasks to be handled
 * @warnings ISRs cannot call printf or any radio related functions and must return in under 10us
 */
void dmaisr(void* arg) {
	//Grab int status
	const uint32 slc_intr_status = READ_PERI_REG(SLC_INT_STATUS);
	//clear all intr flags
	WRITE_PERI_REG(SLC_INT_CLR, slc_intr_status);

  if (slc_intr_status & SLC_TX_EOF_INT_ST) // Receive complete interrupt
  {
    uint32_t eofDesAddr = READ_PERI_REG(SLC_TX_EOF_DES_ADDR);
    switch (outgoingPhase)
    {
      case PAUSED_PHASE:
      {
        prepSdioQueue(asDesc(eofDesAddr), 0);
        break;
      }
      case BOOTLOADER_SYNC_PHASE:
      case BOOTLOADER_XFER_PHASE:
      {
        if (system_os_post(I2SPI_PRIO, TASK_SIG_I2SPI_BL_RX, eofDesAddr) == false)
        {
          os_put_char('!'); os_put_char('I'); os_put_char('F');
        }
        break;
      }
      default:
      {
        if (system_os_post(I2SPI_PRIO, TASK_SIG_I2SPI_RX, eofDesAddr) == false)
        {
          os_put_char('!'); os_put_char('I'); os_put_char('R');
        }
        break;
      }
    }
  }
  
  if (slc_intr_status & SLC_RX_EOF_INT_ST) // Transmit complete interrupt
  {
    const uint32_t eofDesAddr = READ_PERI_REG(SLC_RX_EOF_DES_ADDR);
    switch (outgoingPhase)
    {
      case PAUSED_PHASE:
      {
        uint32_t* buf = (uint32_t*)(asDesc(eofDesAddr)->buf_ptr);
        int w;
        for (w=0; w<DMA_BUF_SIZE/4; w++) buf[w] = 0xFFFFffff;
        nextOutgoingDesc = asDesc(nextOutgoingDesc->next_link_ptr);
        break;
      }
      case BOOTLOADER_SYNC_PHASE:
      case BOOTLOADER_XFER_PHASE:
      {
        if (system_os_post(I2SPI_PRIO, TASK_SIG_I2SPI_BL_TX, eofDesAddr) == false)
        {
          os_put_char('!'); os_put_char('I'); os_put_char('G');
        }
        break;
      }
      default:
      {
        if (system_os_post(I2SPI_PRIO, TASK_SIG_I2SPI_TX, eofDesAddr) == false)
        {
          os_put_char('!'); os_put_char('I'); os_put_char('T');
        }
        break;
      }
    }
  }
}


//Initialize I2S subsystem for DMA circular buffer use
int8_t ICACHE_FLASH_ATTR i2spiInit() {
  int i;

  outgoingPhase = UNINITALIZED_PHASE; // Not established yet
  nextOutgoingDesc = &txQueue[2]; // 0th entry will imeediately be going out and we want to stay one ahead
  i2spiTxUnderflowCount = 0;
  i2spiTxOverflowCount  = 0;
  i2spiRxOverflowCount  = 0;
  i2spiPhaseErrorCount  = 0;
  i2spiIntegralDrift    = 0;
  txFillCount           = 0;
  rtipBootloaderState   = STATE_UNKNOWN;
  bodyBootloaderState   = STATE_UNKNOWN;

  system_os_task(i2spiTask, I2SPI_PRIO, i2spiTaskQ, I2SPI_TASK_QUEUE_LEN);

  // Setup the buffers and descriptors
  for (i=0; i<DMA_BUF_COUNT; ++i)
  {
    os_memset(txBufs, 0xff, DMA_BUF_COUNT*DMA_BUF_SIZE);
    os_memset(rxBufs, 0xff, DMA_BUF_COUNT*DMA_BUF_SIZE);
    prepSdioQueue(&txQueue[i], 1);
    prepSdioQueue(&rxQueue[i], 0);
    txQueue[i].buf_ptr = (uint32_t)&txBufs[i];
    txQueue[i].next_link_ptr = (int)((i<(DMA_BUF_COUNT-1))?(&txQueue[i+1]):(&txQueue[0]));
    //os_printf("TX Q 0x%08x\tB 0x%08x\t",  (unsigned int)&txQueue[i], txQueue[i].buf_ptr);
    rxQueue[i].buf_ptr = (uint32_t)&rxBufs[i];
    rxQueue[i].next_link_ptr = (int)((i<(DMA_BUF_COUNT-1))?(&rxQueue[i+1]):(&rxQueue[0]));
    //os_printf("RX Q 0x%08x\tB 0x%08x\r\n",  (unsigned int)&rxQueue[i], rxQueue[i].buf_ptr);
  }

  //Reset DMA
  SET_PERI_REG_MASK(  SLC_CONF0, SLC_RXLINK_RST|SLC_TXLINK_RST);
  CLEAR_PERI_REG_MASK(SLC_CONF0, SLC_RXLINK_RST|SLC_TXLINK_RST);

  //Clear DMA int flags
  SET_PERI_REG_MASK(  SLC_INT_CLR,  0xffffffff);
  CLEAR_PERI_REG_MASK(SLC_INT_CLR,  0xffffffff);

  //Enable and configure DMA
  CLEAR_PERI_REG_MASK(SLC_CONF0, (SLC_MODE<<SLC_MODE_S));
  SET_PERI_REG_MASK(  SLC_CONF0, (1<<SLC_MODE_S));
  SET_PERI_REG_MASK(  SLC_RX_DSCR_CONF, SLC_INFOR_NO_REPLACE | SLC_TOKEN_NO_REPLACE);
  CLEAR_PERI_REG_MASK(SLC_RX_DSCR_CONF, SLC_RX_FILL_EN       | SLC_RX_EOF_MODE     | SLC_RX_FILL_MODE);

  /* Feed DMA the buffer descriptors
     TX and RX are from the DMA's perspective so we use the TX LINK part to receive data from the I2S DMA and the RX LINK
     part to send data out the DMA to I2S.
  */
  CLEAR_PERI_REG_MASK(SLC_TX_LINK,SLC_TXLINK_DESCADDR_MASK);
  SET_PERI_REG_MASK(SLC_TX_LINK, ((uint32)&rxQueue[0])  & SLC_TXLINK_DESCADDR_MASK);
  CLEAR_PERI_REG_MASK(SLC_RX_LINK,SLC_RXLINK_DESCADDR_MASK);
  SET_PERI_REG_MASK(SLC_RX_LINK, ((uint32)&txQueue[0]) & SLC_RXLINK_DESCADDR_MASK);

  //Attach the DMA interrupt
  ets_isr_attach(ETS_SLC_INUM, dmaisr, NULL);
  //Enable DMA operation intr Want to get interrupts for both end of transmits
  WRITE_PERI_REG(SLC_INT_ENA,  SLC_TX_EOF_INT_ENA | SLC_RX_EOF_INT_ENA | SLC_RX_UDF_INT_ENA | SLC_TX_DSCR_ERR_INT_ENA);
  //clear any interrupt flags that are set
  WRITE_PERI_REG(SLC_INT_CLR, 0xffffffff);
  ///enable DMA intr in cpu
  ets_isr_unmask(1<<ETS_SLC_INUM);

  // Start DMA transmitting
  SET_PERI_REG_MASK(SLC_TX_LINK, SLC_TXLINK_START);
  // Start DMA receiving
  SET_PERI_REG_MASK(SLC_RX_LINK, SLC_RXLINK_START);

//----

	//Init pins to i2s functions
  PIN_FUNC_SELECT(PERIPHS_IO_MUX_U0RXD_U, FUNC_I2SO_DATA);
  PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO2_U, FUNC_I2SO_WS);
  PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDO_U,  FUNC_I2SO_BCK);
  PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDI_U,  FUNC_I2SI_DATA);
  PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTCK_U,  FUNC_I2SI_BCK);
  PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTMS_U , FUNC_I2SI_WS);

  //Enable clock to i2s subsystem
  i2c_writeReg_Mask_def(i2c_bbpll, i2c_bbpll_en_audio_clock_out, 1);

  //Reset I2S subsystem
  CLEAR_PERI_REG_MASK(I2SCONF,I2S_I2S_RESET_MASK);
  SET_PERI_REG_MASK(I2SCONF,I2S_I2S_RESET_MASK);
  CLEAR_PERI_REG_MASK(I2SCONF,I2S_I2S_RESET_MASK);

  //Select 16bits per channel (FIFO_MOD=0), no DMA access (FIFO only)
	CLEAR_PERI_REG_MASK(I2S_FIFO_CONF, I2S_I2S_DSCR_EN|(I2S_I2S_RX_FIFO_MOD<<I2S_I2S_RX_FIFO_MOD_S)|(I2S_I2S_TX_FIFO_MOD<<I2S_I2S_TX_FIFO_MOD_S));
	//Enable DMA in i2s subsystem
	SET_PERI_REG_MASK(I2S_FIFO_CONF, I2S_I2S_DSCR_EN);
  
  //set I2S_CHAN 
  //set rx,tx channel mode, both are "two channel" here
  SET_PERI_REG_MASK(I2SCONF_CHAN, (I2S_TX_CHAN_MOD<<I2S_TX_CHAN_MOD_S)|(I2S_RX_CHAN_MOD<<I2S_RX_CHAN_MOD_S));

  //set RX eof num
	WRITE_PERI_REG(I2SRXEOF_NUM, 128);
  
  // Tweak the I2S timing
  CLEAR_PERI_REG_MASK(I2STIMING, (I2S_RECE_SD_IN_DELAY    << I2S_RECE_SD_IN_DELAY_S)   |
                                 (I2S_RECE_WS_IN_DELAY    << I2S_RECE_WS_IN_DELAY_S)   |
                                 (I2S_RECE_BCK_IN_DELAY   << I2S_RECE_BCK_IN_DELAY_S)  |
                                 (I2S_TRANS_SD_OUT_DELAY  << I2S_TRANS_SD_OUT_DELAY_S) |
                                 (I2S_TRANS_WS_OUT_DELAY  << I2S_TRANS_WS_IN_DELAY_S)  |
                                 (I2S_TRANS_BCK_OUT_DELAY << I2S_TRANS_BCK_IN_DELAY_S));

  SET_PERI_REG_MASK(I2STIMING, ((0 & I2S_RECE_SD_IN_DELAY)    << I2S_RECE_SD_IN_DELAY_S)   |
                               ((0 & I2S_RECE_WS_IN_DELAY)    << I2S_RECE_WS_IN_DELAY_S)   |
                               ((0 & I2S_RECE_BCK_IN_DELAY)   << I2S_RECE_BCK_IN_DELAY_S)  |
                               ((0 & I2S_TRANS_SD_OUT_DELAY)  << I2S_TRANS_SD_OUT_DELAY_S) |
                               ((0 & I2S_TRANS_WS_OUT_DELAY)  << I2S_TRANS_WS_IN_DELAY_S)  |
                               ((0 & I2S_TRANS_BCK_OUT_DELAY) << I2S_TRANS_BCK_IN_DELAY_S));// | I2S_TRANS_BCK_IN_INV);// | I2S_TRANS_DSYNC_SW);
  //trans master&rece slave,MSB shift,right_first,msb right
  CLEAR_PERI_REG_MASK(I2SCONF, I2S_RECE_SLAVE_MOD|I2S_TRANS_SLAVE_MOD|
		               (I2S_BITS_MOD<<I2S_BITS_MOD_S)|
		               (I2S_BCK_DIV_NUM <<I2S_BCK_DIV_NUM_S)|
		               (I2S_CLKM_DIV_NUM<<I2S_CLKM_DIV_NUM_S));
  SET_PERI_REG_MASK(I2SCONF, I2S_RIGHT_FIRST|/*I2S_MSB_RIGHT|*/I2S_RECE_SLAVE_MOD|//I2S_TRANS_SLAVE_MOD|
		                         I2S_RECE_MSB_SHIFT|I2S_TRANS_MSB_SHIFT |
                             (( 4&I2S_BCK_DIV_NUM )<<I2S_BCK_DIV_NUM_S)| // Clock counter, must be a multiple of 2 for 50% duty cycle
			                       (( 4&I2S_CLKM_DIV_NUM)<<I2S_CLKM_DIV_NUM_S)| // Clock prescaler
                             (  0<<I2S_BITS_MOD_S)); // Dead bit insertion?

  //clear int
  SET_PERI_REG_MASK(I2SINT_CLR,   I2S_I2S_TX_REMPTY_INT_CLR|I2S_I2S_TX_WFULL_INT_CLR|I2S_I2S_RX_REMPTY_INT_CLR|
                                  I2S_I2S_RX_WFULL_INT_CLR|I2S_I2S_PUT_DATA_INT_CLR|I2S_I2S_TAKE_DATA_INT_CLR);
  CLEAR_PERI_REG_MASK(I2SINT_CLR, I2S_I2S_TX_REMPTY_INT_CLR|I2S_I2S_TX_WFULL_INT_CLR|I2S_I2S_RX_REMPTY_INT_CLR|
                                  I2S_I2S_RX_WFULL_INT_CLR|I2S_I2S_PUT_DATA_INT_CLR|I2S_I2S_TAKE_DATA_INT_CLR);
 
  //enable int
  SET_PERI_REG_MASK(I2SINT_ENA,   I2S_I2S_TX_REMPTY_INT_ENA|I2S_I2S_TX_WFULL_INT_ENA|I2S_I2S_RX_REMPTY_INT_ENA|
  I2S_I2S_RX_WFULL_INT_ENA|I2S_I2S_TX_PUT_DATA_INT_ENA|I2S_I2S_RX_TAKE_DATA_INT_ENA);

  // Start
  SET_PERI_REG_MASK(I2SCONF,I2S_I2S_TX_START | I2S_I2S_RX_START);

  return 0;
}

bool i2spiQueueMessage(uint8_t* msgData, uint16_t msgLen)
{
  if (unlikely(outgoingPhase < PHASE_FLAGS))
  {
    return false;
  }
  else
  {
    uint16_t queued = 0;
    while (queued < msgLen)
    {
      uint8_t payloadLen = min(msgLen, DROP_TO_RTIP_MAX_VAR_PAYLOAD);
      if (makeDrop(msgData + queued, payloadLen) == false)
      {
        os_printf("FATAL: Couldn't queue message to RTIP\r\n");
        return false;
      }
      queued += payloadLen;
    }
    return true;
  }
}

void ICACHE_FLASH_ATTR i2spiSwitchMode(const I2SpiMode mode)
{
  switch(mode)
  {
    case I2SPI_NORMAL:
    {
      os_printf("I2Spi mode Normal\r\n");
      if (outgoingPhase == BOOTLOADER_XFER_PHASE)
      {
        ets_intr_lock();
        uint16_t* txBuf = (uint16_t*)(nextOutgoingDesc->buf_ptr);
        txBuf[0] = COMMAND_HEADER;
        txBuf[1] = COMMAND_DONE;
        nextOutgoingDesc = asDesc(nextOutgoingDesc->next_link_ptr);
        ets_intr_unlock();
      }
      outgoingPhase = UNINITALIZED_PHASE;
      txFillCount = 0;
      return;
    }
    case I2SPI_BOOTLOADER:
    {
      os_printf("I2Spi mode bootloader sync\r\n");
      outgoingPhase = BOOTLOADER_SYNC_PHASE;
      rtipBootloaderState = STATE_UNKNOWN;
      return;
    }
    case I2SPI_PAUSED:
    {
      outgoingPhase = PAUSED_PHASE;
      os_printf("I2Spi mode paused\r\n");
      return;
    }
  }
}

int16_t ICACHE_FLASH_ATTR i2spiGetRtipBootloaderState(void)
{
  return rtipBootloaderState;
}

int16_t ICACHE_FLASH_ATTR i2spiGetBodyBootloaderState(void)
{
  return bodyBootloaderState;
}

void ICACHE_FLASH_ATTR i2spiBootloaderPushChunk(FirmwareBlock* chunk)
{
  int wInd = 0;
  int rInd = 0;
  ets_intr_lock();
  uint16_t* txBuf = (uint16_t*)(nextOutgoingDesc->buf_ptr);
  uint16_t* data  = (uint16_t*)(chunk);
  txBuf[wInd++] = COMMAND_HEADER;
  txBuf[wInd++] = COMMAND_FLASH;
  while (rInd < (sizeof(FirmwareBlock)/2))
  {
    while ((wInd < (DMA_BUF_SIZE/2)) && (rInd < (sizeof(FirmwareBlock)/2)))
    {
      txBuf[wInd++] = data[rInd++];
    }
    nextOutgoingDesc = asDesc(nextOutgoingDesc->next_link_ptr);
    txBuf = (uint16_t*)(nextOutgoingDesc->buf_ptr);
    wInd = 0;
  }
  ets_intr_unlock();
}
