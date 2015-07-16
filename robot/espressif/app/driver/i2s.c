/**
 * @file I2S driver for Espressif chip.
 * @author Daniel Casner <daniel@anki.com>
 *
 * Original code taken from Espressif MP3 decoder project. Heavily reworked to liberate it from FreeRTOS and enable
 * bidirectional communication.
 */

/*
How does this work? Basically, to get sound, you need to:
- Connect an I2S codec to the I2S pins on the ESP.
- Start up a thread that's going to do the sound output
- Call I2sInit()
- Call I2sSetRate() with the sample rate you want.
- Generate sound and call i2sPushSample() with 32-bit samples.
The 32bit samples basically are 2 16-bit signed values (the analog values for
the left and right channel) concatenated as (Rout<<16)+Lout

I2sPushSample will block when you're sending data too quickly, so you can just
generate and push data as fast as you can and I2sPushSample will regulate the
speed.
*/

#include "ets_sys.h"
#include "osapi.h"
#include "os_type.h"
#include "driver/i2s_reg.h"
#include "driver/slc_register.h"
#include "driver/sdio_slv.h"
#include "driver/i2s.h"
#include "driver/i2s_ets.h"

#define DMA_BUF_COUNT (8) // 512bytes per buffer * 8 buffers = 4k.
ct_assert(DMA_BUF_COUNT % 2 == 0) // Must be a power of two for masking to work
#define DMA_BUF_COUNT_MASK (DMA_BUF_COUNT-1)

/** DMA I2SPI transfer buffers
 * The buffers are used for transfers in both directions with the last transmitted buffer being used for the next
 * incoming transfer.
 */
static I2SPITransfer xferBufs[DMA_BUF_COUNT];
/// DMA queue control structure
static struct sdio_queue xferQueue[DMA_BUF_COUNT];
/// Index of the next buffer to transmit (i.e. where we should write data, not what's currently being transmitted)
static uint8 transmitInd;
/// Index of the buffer which is currently receiving DMA_BUF_COUNT_MASK
static uint8 receiveInd;
/// The sequence number for the next transmission
static uint16_t transmitSeqNo;
/// Last received sequence number from other side
static uint16_t lastSeqNo;
/// Error count for missed transmits
static uint32_t missedTransmits;
/// Out of sequence count
static uint32_t outOfSequence;

/// Prep an sdio_queue structure (DMA descriptor) for (re)use
static void inline prepSdioQueue(struct sdio_queue* desc)
{
  desc->owner     = 1;
  desc->eof       = 1;
  desc->sub_sof   = 0;
  desc->datalen   = I2SPI_TRANSFER_SIZE;
  desc->blocksize = I2SPI_TRANSFER_SIZE;
  desc->unused    = 0;
}

/** General DMA ISR
 * Primarily handles RX_EOF_INT (DMA sent buffer out) and TX_EOF_INT (DMA receiverd buffer in). In addition to clearing
 * the interrupt flags, the sdio_queue returned must be reset so it can be reused. Finally when data is received from
 * DMA (TX) the i2sRecvCallback is called.
 */
LOCAL void slcIsr(void) {
	uint32 slc_intr_status;

	//Grab int status
	slc_intr_status = READ_PERI_REG(SLC_INT_STATUS);
	//clear all intr flags
	WRITE_PERI_REG(SLC_INT_CLR, 0xffffffff);

	if (slc_intr_status & SLC_RX_EOF_INT_ST) {
    // Nothing to do here, pointers will be advanced by TX_EOF interrupt
	}
  if (slc_intr_status & SLC_TX_EOF_INT_ST) {
    struct sdio_queue* desc = (struct sdio_queue*)READ_PERI_REG(SLC_TX_EOF_DES_ADDR);
    I2SPITransfer* xfer     = (I2SPITransfer*)(desc->buf_ptr);
    receiveInd = (receiveInd + 1) & DMA_BUF_COUNT_MASK;
    if (xfer->from == fromRTIP) // Is received data
    {
      if (i2spiSequential(xfer->seqNo, lastSeqNo)) // Is sequential
      {
        i2sRecvCallback(xfer->payload, xfer->tag);
        lastSeqNo = xfer->seqNo;
      }
      else // Not in sequence
      {
        outOfSequence++;
      }
    }
    else // Not received data
    {
      missedTransmits++;
    }

    prepSdioQueue(desc); // Reset the DMA descriptor for reuse
  }
}


//Initialize I2S subsystem for DMA circular buffer use
void ICACHE_FLASH_ATTR i2sInit() {
	int i;

  transmitInd     = 0;
  receiveInd      = DMA_BUF_COUNT-1;
  transmitSeqNo   = 0;
  lastSeqNo       = 0;
  missedTransmits = 0;
  outOfSequence   = 0;

  // Setup the buffers and descriptors
  for (i=0; i<DMA_BUF_COUNT; ++i)
  {
    os_memset(&xferBufs[i], 0, I2SPI_TRANSFER_SIZE);
    prepSdioQueue(&xferQueue[i]);
    xferQueue[i].buf_ptr = (uint32_t)&xferBufs[i];
    xferQueue[i].next_link_ptr = (int)((i<(DMA_BUF_COUNT-1))?(&xferQueue[i+1]):(&xferQueue[0]));
  }

	//Reset DMA
	SET_PERI_REG_MASK(SLC_CONF0, SLC_RXLINK_RST|SLC_TXLINK_RST);
	CLEAR_PERI_REG_MASK(SLC_CONF0, SLC_RXLINK_RST|SLC_TXLINK_RST);

	//Clear DMA int flags
	SET_PERI_REG_MASK(SLC_INT_CLR,  0xffffffff);
	CLEAR_PERI_REG_MASK(SLC_INT_CLR,  0xffffffff);

	//Enable and configure DMA
	CLEAR_PERI_REG_MASK(SLC_CONF0, (SLC_MODE<<SLC_MODE_S));
	SET_PERI_REG_MASK(SLC_CONF0,(1<<SLC_MODE_S));
	SET_PERI_REG_MASK(SLC_RX_DSCR_CONF,SLC_INFOR_NO_REPLACE|SLC_TOKEN_NO_REPLACE);
	CLEAR_PERI_REG_MASK(SLC_RX_DSCR_CONF, SLC_RX_FILL_EN|SLC_RX_EOF_MODE | SLC_RX_FILL_MODE);

	/* Feed DMA the buffer descriptors
    TX and RX are from the DMA's perspective so we use the TX LINK part to receive data from the I2S DMA and the RX LINK
    part to send data out the DMA to I2S.
    Both RX and TX are using the same circular buffer TX (I2S receive) running just behind RX (I2S transmit). As long
    as we process the incoming data before the buffer loops around there is no competition.
  */
	CLEAR_PERI_REG_MASK(SLC_TX_LINK,SLC_TXLINK_DESCADDR_MASK);
	SET_PERI_REG_MASK(SLC_TX_LINK, ((uint32)&xferQueue[receiveInd])  & SLC_TXLINK_DESCADDR_MASK);
	CLEAR_PERI_REG_MASK(SLC_RX_LINK,SLC_RXLINK_DESCADDR_MASK);
	SET_PERI_REG_MASK(SLC_RX_LINK, ((uint32)&xferQueue[transmitInd]) & SLC_RXLINK_DESCADDR_MASK);

	//Attach the DMA interrupt
	ets_isr_attach(ETS_SLC_INUM, slc_isr);
	//Enable DMA operation intr Want to get interrupts for both end of transmits
	WRITE_PERI_REG(SLC_INT_ENA,  SLC_TX_EOF_INT_ENA); // SLC_RX_EOF_INT_ENA // only interrupting on TX (receive)
	//clear any interrupt flags that are set
	WRITE_PERI_REG(SLC_INT_CLR, 0xffffffff);
	///enable DMA intr in cpu
	ets_isr_unmask(1<<ETS_SLC_INUM);

//----

	//Init pins to i2s functions
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_U0RXD_U, FUNC_I2SO_DATA);
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO2_U, FUNC_I2SO_WS);
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDO_U, FUNC_I2SO_BCK);
  PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDI_U, FUNC_I2SI_DATA);
  PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTCK_U, FUNC_I2SI_BCK);
  PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTMS_U, FUNC_I2SI_WS);

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

	//tx/rx binaureal
	CLEAR_PERI_REG_MASK(I2SCONF_CHAN, (I2S_TX_CHAN_MOD<<I2S_TX_CHAN_MOD_S)|(I2S_RX_CHAN_MOD<<I2S_RX_CHAN_MOD_S));

	//Clear int
	SET_PERI_REG_MASK(I2SINT_CLR,   I2S_I2S_TX_REMPTY_INT_CLR|I2S_I2S_TX_WFULL_INT_CLR|
			I2S_I2S_RX_WFULL_INT_CLR|I2S_I2S_PUT_DATA_INT_CLR|I2S_I2S_TAKE_DATA_INT_CLR);
	CLEAR_PERI_REG_MASK(I2SINT_CLR,   I2S_I2S_TX_REMPTY_INT_CLR|I2S_I2S_TX_WFULL_INT_CLR|
			I2S_I2S_RX_WFULL_INT_CLR|I2S_I2S_PUT_DATA_INT_CLR|I2S_I2S_TAKE_DATA_INT_CLR);
	//enable int
	SET_PERI_REG_MASK(I2SINT_ENA,   I2S_I2S_TX_REMPTY_INT_ENA|I2S_I2S_TX_WFULL_INT_ENA|
	I2S_I2S_RX_REMPTY_INT_ENA|I2S_I2S_TX_PUT_DATA_INT_ENA|I2S_I2S_RX_TAKE_DATA_INT_ENA);

  CLEAR_PERI_REG_MASK(I2SCONF, I2S_TRANS_SLAVE_MOD|I2S_RECE_SLAVE_MOD|
						(I2S_BITS_MOD<<I2S_BITS_MOD_S)|
						(I2S_BCK_DIV_NUM <<I2S_BCK_DIV_NUM_S)|
						(I2S_CLKM_DIV_NUM<<I2S_CLKM_DIV_NUM_S));
	SET_PERI_REG_MASK(I2SCONF, I2S_RIGHT_FIRST|I2S_MSB_RIGHT|//I2S_RECE_SLAVE_MOD|
						I2S_RECE_MSB_SHIFT|I2S_TRANS_MSB_SHIFT|
						(((1)&I2S_BCK_DIV_NUM )<<I2S_BCK_DIV_NUM_S)|
						(((1)&I2S_CLKM_DIV_NUM)<<I2S_CLKM_DIV_NUM_S));
}

void ICACHE_FLASH_ATTR i2sStart(void)
{
  // Start DMA transmitting
  SET_PERI_REG_MASK(SLC_TX_LINK, SLC_TXLINK_START);
  // Start DMA receiving
  SET_PERI_REG_MASK(SLC_RX_LINK, SLC_RXLINK_START);
  //Start transmission
  SET_PERI_REG_MASK(I2SCONF,I2S_I2S_TX_START);
  // Start receive
  SET_PERI_REG_MASK(I2SCONF,I2S_I2S_RX_START);
}

void ICACHE_FLASH_ATTR i2sStart(void)
{
  // Stop DMA transmitting
  SET_PERI_REG_MASK(SLC_TX_LINK, SLC_TXLINK_START);
  // Stop DMA receiving
  SET_PERI_REG_MASK(SLC_RX_LINK, SLC_RXLINK_START);
  // @TODO How do we stop the I2S peripheral? Does halting DMA do the trick?
}

bool i2sQueueTx(I2SPIPayload* payload, I2SPIPayloadTag tag)
{
  if (transmitInd == receiveInd) // Head has reached tail
  {
    // No room to queue this
    return false;
  }
  else
  {
    // Buffer the data to transmit
    os_memcpy(&xferBufs[transmitInd].payload, payload, sizeof(I2SPI_MAX_PAYLOAD));
    xferBufs[transmitInt].seqNo = transmitSeqNo;
    xferBufs[transmitInt].tag   = tag;
    xferBufs[transmitInd].from  = fromWiFi;
    // Update indecies
    transmitInd = (transmitInd + 1) & DMA_BUF_COUNT_MASK;
    transmitSeqNo += 1;
  }
}

uint32_t ICACHE_FLASH_ATTR i2sGetMissedTransmits(void) { return missedTransmits; }
uint32_t ICACHE_FLASH_ATTR i2sGetOutOfSequence(void)   { return outOfSequence;   }
