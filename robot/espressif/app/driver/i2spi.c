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
#include "driver/uart.h"
#include "driver/i2s_reg.h"
#include "driver/slc_register.h"
#include "driver/sdio_slv.h"
#include "driver/i2spi.h"
#include "driver/i2s_ets.h"

/// SLC can only move 256 byte chunks so that's what we use
#define DMA_BUF_SIZE (256)
/// How often we will garuntee servicing the DMA buffers
#defien DMA_SERVICE_INTERVAL_MS (5)
/// How many buffers are required given the above constraints. + 1 for ceiling function
#define DMA_BUF_COUNT ((DROP_SIZE * DROPS_PER_SECOND * DMA_SERVICE_INTERVAL_MS / 1000 / DMA_BUF_SIZE) + 1)

/// Signals to the I2SPI task
enum 
{
  TASK_SIG_I2SPI_RX,
  TASK_SIG_I2SPI_TX
};

#define I2SPI_TASK_QUEUE_LEN (DMA_BUF_COUNT * 2)
/// Queue for I2SPI tasks. 2x the buffer count because have tasks for send and receive
os_event_t i2spiTaskQ[I2SPI_TASK_QUEUE_LEN];

/// DMA I2SPI transmit buffers
static unsigned int txBufs[DMA_BUF_COUNT][DMA_BUF_SIZE/4];
/// DMA transmit queue control structure
static struct sdio_queue txQueue[DMA_BUF_COUNT];
/// DMA I2SPI transmit buffers
static unsigned int rxBufs[DMA_BUF_COUNT][DMA_BUF_SIZE/4];
/// DMA transmit queue control structure
static struct sdio_queue rxQueue[DMA_BUF_COUNT];

/// Prep an sdio_queue structure (DMA descriptor) for (re)use
static void inline prepSdioQueue(struct sdio_queue* desc)
{
  desc->owner     = 1;
  desc->eof       = 1;
  desc->sub_sof   = 0;
  desc->datalen   = DMA_BUF_SIZE;
  desc->blocksize = DMA_BUF_SIZE;
  desc->unused    = 0;
}


LOCAL void i2spiTask(os_event_t *event)
{
  struct sdio_queue* desc = (struct sdio_queue*)(event->par);
  
  if (desc == NULL)
  {
    os_printf("ERROR: I2SPI task got null descriptor with signal %d\r\n", event->sig);
    return;
  }
  
  switch (event->sig)
  {
    case TASK_SIG_I2SPI_RX:
    {
      XXX Handle RXing data
      break;
    }
    case TASK_SIG_I2SPI_TX:
    {
      XXX Handle returned TX buffer
      prepSdioQueue(desc);
      break;
    }
    default:
    {
      os_printf("ERROR: Unexpected I2SPI task signal signal: %d, %08x\r\n", event->sig, event->par);
    }
  }
}


/** DMA buffer complete ISR
 * Called for both completed SLC_TX (I2SPI receive) and SLC_RX (I2SPI transmit) events
 * Everything is passed off to tasks to be handled
 * @warnings ISRs cannot call printf or any radio related functions and must return in under 10us
 */
LOCAL void dmaisr(void* arg) {
	//Grab int status
	const uint32 slc_intr_status = READ_PERI_REG(SLC_INT_STATUS);
	//clear all intr flags
	WRITE_PERI_REG(SLC_INT_CLR, slc_intr_status);

  //os_put_hex(slc_intr_status, 8); os_put_char('\r'); os_put_char('\n');

  if (slc_intr_status & SLC_TX_EOF_INT_ST) { // Receive complete interrupt
    if (system_os_post(I2SPI_PRIO, TASK_SIG_I2SPI_RX, READ_PERI_REG(SLC_TX_EOF_DES_ADDR)) == false)
    {
      os_put_char('!'); os_put_char('I'); os_put_char('R');
    }
  }
  if (slc_intr_status & SLC_RX_EOF_INT_ST)
  { // Transmit complete interrupt
    if (system_os_post(I2SPI_PRIO, TASK_SIG_I2SPI_TX, READ_PERI_REG(SLC_RX_EOF_DES_ADDR)) == false)
    {
      os_put_char('!'); os_put_char('I'); os_put_char('T');
    }
  }
}


//Initialize I2S subsystem for DMA circular buffer use
int8_t ICACHE_FLASH_ATTR i2spiInit() {
  int i;

  transmitInd     = 0;
  receiveInd      = 0;
  transmitSeqNo   = 0;
  lastSeqNo       = 0;

  system_os_task(i2spiTask, I2SPI_PRIO, i2spiTaskQ, I2SPI_TASK_QUEUE_LEN);

  // Setup the buffers and descriptors
  for (i=0; i<DMA_BUF_COUNT; ++i)
  {
    os_memset(txBufs, 0x00, DMA_BUF_COUNT*DMA_BUF_SIZE);
    os_memset(rxBufs, 0x00, DMA_BUF_COUNT*DMA_BUF_SIZE);
    prepSdioQueue(&txQueue[i]);
    prepSdioQueue(&rxQueue[i]);
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
	WRITE_PERI_REG(SLC_INT_ENA,  SLC_TX_EOF_INT_ENA | SLC_RX_EOF_INT_ENA);
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
	CLEAR_PERI_REG_MASK(I2S_FIFO_CONF, I2S_I2S_DSCR_EN | (I2S_I2S_RX_FIFO_MOD<<I2S_I2S_RX_FIFO_MOD_S) |
                                     (I2S_I2S_TX_FIFO_MOD<<I2S_I2S_TX_FIFO_MOD_S));
	//Enable DMA in i2s subsystem
	SET_PERI_REG_MASK(I2S_FIFO_CONF, I2S_I2S_DSCR_EN);

	//tx/rx binaureal
	CLEAR_PERI_REG_MASK(I2SCONF_CHAN, (I2S_TX_CHAN_MOD<<I2S_TX_CHAN_MOD_S)|(I2S_RX_CHAN_MOD<<I2S_RX_CHAN_MOD_S));

	//Clear int
	SET_PERI_REG_MASK(I2SINT_CLR,   I2S_I2S_TX_REMPTY_INT_CLR|I2S_I2S_TX_WFULL_INT_CLR|
			I2S_I2S_RX_WFULL_INT_CLR|I2S_I2S_PUT_DATA_INT_CLR|I2S_I2S_TAKE_DATA_INT_CLR);
	CLEAR_PERI_REG_MASK(I2SINT_CLR,   I2S_I2S_TX_REMPTY_INT_CLR|I2S_I2S_TX_WFULL_INT_CLR|
			I2S_I2S_RX_WFULL_INT_CLR|I2S_I2S_PUT_DATA_INT_CLR|I2S_I2S_TAKE_DATA_INT_CLR);

	//trans master&rece slave,MSB shift,right_first,msb right
	CLEAR_PERI_REG_MASK(I2SCONF, I2S_RECE_SLAVE_MOD|I2S_TRANS_SLAVE_MOD|
						(I2S_BITS_MOD<<I2S_BITS_MOD_S)|
						(I2S_BCK_DIV_NUM <<I2S_BCK_DIV_NUM_S)|
						(I2S_CLKM_DIV_NUM<<I2S_CLKM_DIV_NUM_S));
	SET_PERI_REG_MASK(I2SCONF, I2S_RIGHT_FIRST|I2S_MSB_RIGHT|I2S_RECE_SLAVE_MOD|I2S_TRANS_SLAVE_MOD|
						//I2S_RECE_MSB_SHIFT|I2S_TRANS_MSB_SHIFT|
						((8   &I2S_BCK_DIV_NUM )<<I2S_BCK_DIV_NUM_S)|
						((1&I2S_CLKM_DIV_NUM)<<I2S_CLKM_DIV_NUM_S));


	//No idea if ints are needed...
	//clear int
	SET_PERI_REG_MASK(I2SINT_CLR,   I2S_I2S_TX_REMPTY_INT_CLR|I2S_I2S_TX_WFULL_INT_CLR|
			I2S_I2S_RX_WFULL_INT_CLR|I2S_I2S_PUT_DATA_INT_CLR|I2S_I2S_TAKE_DATA_INT_CLR);
	CLEAR_PERI_REG_MASK(I2SINT_CLR,   I2S_I2S_TX_REMPTY_INT_CLR|I2S_I2S_TX_WFULL_INT_CLR|
			I2S_I2S_RX_WFULL_INT_CLR|I2S_I2S_PUT_DATA_INT_CLR|I2S_I2S_TAKE_DATA_INT_CLR);
	//enable int
	SET_PERI_REG_MASK(I2SINT_ENA,   I2S_I2S_TX_REMPTY_INT_ENA|I2S_I2S_TX_WFULL_INT_ENA|
	I2S_I2S_RX_REMPTY_INT_ENA|I2S_I2S_RX_WFULL_INT_ENA|I2S_I2S_TX_PUT_DATA_INT_ENA|I2S_I2S_RX_TAKE_DATA_INT_ENA);


	return 0;
}

void ICACHE_FLASH_ATTR i2spiStart(void)
{
  //Start i2s start and receive
  SET_PERI_REG_MASK(I2SCONF,I2S_I2S_TX_START | I2S_I2S_RX_START);
}

void ICACHE_FLASH_ATTR i2spiStop(void)
{
  // Stop DMA transmitting
  SET_PERI_REG_MASK(SLC_TX_LINK, SLC_TXLINK_STOP);
  // Stop DMA receiving
  SET_PERI_REG_MASK(SLC_RX_LINK, SLC_RXLINK_STOP);
  // @TODO How do we stop the I2S peripheral? Does halting DMA do the trick?
}

bool i2spiQueueMessage(uint8_t* msgData, uint8_t msgLen, ToRTIPPayloadTag tag)
{
  XXX Implement queing message data
}
