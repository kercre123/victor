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

//We need some defines that aren't in some RTOS SDK versions. Define them here if we can't find them.
#ifndef i2c_bbpll
#define i2c_bbpll                                 0x67
#define i2c_bbpll_en_audio_clock_out            4
#define i2c_bbpll_en_audio_clock_out_msb        7
#define i2c_bbpll_en_audio_clock_out_lsb        7
#define i2c_bbpll_hostid                           4

#define i2c_writeReg_Mask(block, host_id, reg_add, Msb, Lsb, indata)  rom_i2c_writeReg_Mask(block, host_id, reg_add, Msb, Lsb, indata)
#define i2c_readReg_Mask(block, host_id, reg_add, Msb, Lsb)  rom_i2c_readReg_Mask(block, host_id, reg_add, Msb, Lsb)
#define i2c_writeReg_Mask_def(block, reg_add, indata) \
      i2c_writeReg_Mask(block, block##_hostid,  reg_add,  reg_add##_msb,  reg_add##_lsb,  indata)
#define i2c_readReg_Mask_def(block, reg_add) \
      i2c_readReg_Mask(block, block##_hostid,  reg_add,  reg_add##_msb,  reg_add##_lsb)
#endif
#ifndef ETS_SLC_INUM
#define ETS_SLC_INUM       1
#endif

//Pointer to the I2S DMA buffer data
static unsigned int i2sBuf[I2SDMABUFCNT][I2SDMABUFLEN];
//I2S DMA buffer descriptors
static struct sdio_queue i2sBufDesc[I2SDMABUFCNT];
// Index of the buffer the I2S peripherial is reading from
static unsigned int i2sRdInd = 0;
// Index of the buffer we are writing samples into
static unsigned int i2sWrInd = 0;
//DMA underrun counter
static long underrunCnt;

//This routine is called as soon as the DMA routine has something to tell us. All we
//handle here is the RX_EOF_INT status, which indicate the DMA has sent a buffer whose
//descriptor has the 'EOF' field set to 1.
LOCAL void slc_isr(void) {
	struct sdio_queue *finishedDesc;
	uint32 slc_intr_status;
	int dummy;

	//Grab int status
	slc_intr_status = READ_PERI_REG(SLC_INT_STATUS);
	//clear all intr flags
	WRITE_PERI_REG(SLC_INT_CLR, 0xffffffff);//slc_intr_status);
	if (slc_intr_status & SLC_RX_EOF_INT_ST) {
		//The DMA subsystem is done with this block: Push it on the queue so it can be re-used.
		finishedDesc=(struct sdio_queue*)READ_PERI_REG(SLC_RX_EOF_DES_ADDR);
    if (i2sRdInd == i2sWrInd) {
			//All buffers are empty. This means we have an underflow on our hands.
			underrunCnt++;
			//Pop the top off the queue; it's invalid now anyway.
			i2sWrInd = (i2sWrInd + 1) % I2SDMABUFCNT;
		}

		//Dump the buffer on the queue so the rest of the software can fill it.
    i2sRdInd = (i2sRdInd + 1) % I2SDMABUFCNT;
	}
}


//Initialize I2S subsystem for DMA circular buffer use
void ICACHE_FLASH_ATTR i2sInit() {
	int i, j;

	underrunCnt=0;

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


	//Initialize DMA buffer descriptors in such a way that they will form a circular
	//buffer.
	for (i=0; i<I2SDMABUFCNT; i++) {
		i2sBufDesc[i].owner=1;
		i2sBufDesc[i].eof=1;
		i2sBufDesc[i].sub_sof=0;
		i2sBufDesc[i].datalen=I2SDMABUFLEN*4;
		i2sBufDesc[i].blocksize=I2SDMABUFLEN*4;
		i2sBufDesc[i].buf_ptr=(uint32_t)&i2sBuf[i][0];
		i2sBufDesc[i].unused=0;
		i2sBufDesc[i].next_link_ptr=(int)((i<(I2SDMABUFCNT-1))?(&i2sBufDesc[i+1]):(&i2sBufDesc[0]));
    for (j=0; j<I2SDMABUFLEN; j++) {
      i2sBuf[i][j] = i * j;
    }
	}

	//Feed dma the 1st buffer desc addr
	//To send data to the I2S subsystem, counter-intuitively we use the RXLINK part, not the TXLINK as you might
	//expect. The TXLINK part still needs a valid DMA descriptor, even if it's unused: the DMA engine will throw
	//an error at us otherwise. Just feed it any random descriptor.
	CLEAR_PERI_REG_MASK(SLC_TX_LINK,SLC_TXLINK_DESCADDR_MASK);
	SET_PERI_REG_MASK(SLC_TX_LINK, ((uint32)&i2sBufDesc[1]) & SLC_TXLINK_DESCADDR_MASK); //any random desc is OK, we don't use TX but it needs something valid
	CLEAR_PERI_REG_MASK(SLC_RX_LINK,SLC_RXLINK_DESCADDR_MASK);
	SET_PERI_REG_MASK(SLC_RX_LINK, ((uint32)&i2sBufDesc[0]) & SLC_RXLINK_DESCADDR_MASK);

	//Attach the DMA interrupt
	ets_isr_attach(ETS_SLC_INUM, slc_isr);
	//Enable DMA operation intr
	WRITE_PERI_REG(SLC_INT_ENA,  SLC_RX_EOF_INT_ENA);
	//clear any interrupt flags that are set
	WRITE_PERI_REG(SLC_INT_CLR, 0xffffffff);
	///enable DMA intr in cpu
	ets_isr_unmask(1<<ETS_SLC_INUM);

  // Keep track of read / write queues
  i2sRdInd = 0;
  i2sWrInd = 0;

	//Start transmission
	SET_PERI_REG_MASK(SLC_TX_LINK, SLC_TXLINK_START);
	SET_PERI_REG_MASK(SLC_RX_LINK, SLC_RXLINK_START);

//----

	//Init pins to i2s functions
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_U0RXD_U, FUNC_I2SO_DATA);
	//PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO2_U, FUNC_I2SO_WS); // No more word selects
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDO_U, FUNC_I2SO_BCK);

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
	CLEAR_PERI_REG_MASK(I2SINT_CLR, I2S_I2S_TX_REMPTY_INT_CLR|I2S_I2S_TX_WFULL_INT_CLR|
			I2S_I2S_RX_WFULL_INT_CLR|I2S_I2S_PUT_DATA_INT_CLR|I2S_I2S_TAKE_DATA_INT_CLR);

	//trans master&rece slave,MSB shift,right_first,msb right
	CLEAR_PERI_REG_MASK(I2SCONF, I2S_TRANS_SLAVE_MOD|
						(I2S_BITS_MOD<<I2S_BITS_MOD_S)|
						(I2S_BCK_DIV_NUM <<I2S_BCK_DIV_NUM_S)|
						(I2S_CLKM_DIV_NUM<<I2S_CLKM_DIV_NUM_S));
	SET_PERI_REG_MASK(I2SCONF, I2S_RIGHT_FIRST|I2S_MSB_RIGHT|I2S_RECE_SLAVE_MOD|
						I2S_RECE_MSB_SHIFT|I2S_TRANS_MSB_SHIFT|
						((16&I2S_BCK_DIV_NUM )<<I2S_BCK_DIV_NUM_S)|
						((7&I2S_CLKM_DIV_NUM)<<I2S_CLKM_DIV_NUM_S));


	//No idea if ints are needed...
	//clear int
	SET_PERI_REG_MASK(I2SINT_CLR,   I2S_I2S_TX_REMPTY_INT_CLR|I2S_I2S_TX_WFULL_INT_CLR|
			I2S_I2S_RX_WFULL_INT_CLR|I2S_I2S_PUT_DATA_INT_CLR|I2S_I2S_TAKE_DATA_INT_CLR);
	CLEAR_PERI_REG_MASK(I2SINT_CLR,   I2S_I2S_TX_REMPTY_INT_CLR|I2S_I2S_TX_WFULL_INT_CLR|
			I2S_I2S_RX_WFULL_INT_CLR|I2S_I2S_PUT_DATA_INT_CLR|I2S_I2S_TAKE_DATA_INT_CLR);
	//enable int
	SET_PERI_REG_MASK(I2SINT_ENA,   I2S_I2S_TX_REMPTY_INT_ENA|I2S_I2S_TX_WFULL_INT_ENA|
	I2S_I2S_RX_REMPTY_INT_ENA|I2S_I2S_TX_PUT_DATA_INT_ENA|I2S_I2S_RX_TAKE_DATA_INT_ENA);

	//Start transmission
	SET_PERI_REG_MASK(I2SCONF,I2S_I2S_TX_START);
}


#define BASEFREQ (12000000L)
#define ABS(x) (((x)>0)?(x):(-(x)))

//Set the I2S sample rate, in HZ
void i2sSetRate(int rate) {
	//Find closest divider
	int bestbck=0, bestfreq=0;
	int tstfreq;
	int i;
	//Calculate the base divider for 16 bits of data
	int div=(BASEFREQ/(rate*32));
	//The base divider can be off by as much as <1 Compensate by trying to make the amount of bytes in the
	//i2s cycle more than 16. Do this by trying the amounts from 16 to 32 and keeping the one that fits best.
	for (i=16; i<32; i++) {
		tstfreq=BASEFREQ/(div*i*2);
//		printf("Best (%d,%d) cur (%d,%d) div %d\n", bestbck, bestfreq, i, tstfreq, ABS(rate-tstfreq));
		if (ABS(rate-tstfreq)<ABS(rate-bestfreq)) {
			bestbck=i;
			bestfreq=tstfreq;
		}
	}

//	printf("ReqRate %d Div %d Bck %d Frq %d\n", rate, div, bestbck, BASEFREQ/(div*bestbck*2));

	CLEAR_PERI_REG_MASK(I2SCONF, I2S_TRANS_SLAVE_MOD|
						(I2S_BITS_MOD<<I2S_BITS_MOD_S)|
						(I2S_BCK_DIV_NUM <<I2S_BCK_DIV_NUM_S)|
						(I2S_CLKM_DIV_NUM<<I2S_CLKM_DIV_NUM_S));
	SET_PERI_REG_MASK(I2SCONF, I2S_RIGHT_FIRST|I2S_MSB_RIGHT|I2S_RECE_SLAVE_MOD|
						I2S_RECE_MSB_SHIFT|I2S_TRANS_MSB_SHIFT|
						(((bestbck-1)&I2S_BCK_DIV_NUM )<<I2S_BCK_DIV_NUM_S)|
						(((div-1)&I2S_CLKM_DIV_NUM)<<I2S_CLKM_DIV_NUM_S));
}

//Current DMA buffer we're writing to
static unsigned int *currDMABuff=NULL;
//Current position in that DMA buffer
static int currDMABuffPos=0;


//This routine pushes a single, 32-bit sample to the I2S buffers. Call this at (on average)
//at least the current sample rate. You can also call it quicker: it will suspend the calling
//thread if the buffer is full and resume when there's room again.
void i2sPushSample(unsigned int sample) {
	//Check if current DMA buffer is full.
	if (currDMABuffPos==I2SDMABUFLEN || currDMABuff==NULL) {
		//We need a new buffer. Pop one from the queue.
    currDMABuff = i2sBuf[i2sWrInd];
    i2sWrInd = (i2sWrInd + 1) % I2SDMABUFCNT;
		currDMABuffPos=0;
	}
	currDMABuff[currDMABuffPos++]=sample;
}


long ICACHE_FLASH_ATTR i2sGetUnderrunCnt() {
	return underrunCnt;
}
