/**
 ****************************************************************************************
 *
 * @file spi_439.c
 *
 * @brief spi interface driver for 439. V2_0
 *        requires the regular spi.c
 *
 * Copyright (C) 2012. Dialog Semiconductor Ltd, unpublished work. This computer 
 * program includes Confidential, Proprietary Information and is a Trade Secret of 
 * Dialog Semiconductor Ltd.  All use, disclosure, and/or reproduction is prohibited 
 * unless authorized in writing. All Rights Reserved.
 *
 * <bluetooth.support@diasemi.com> and contributors.
 *
 ****************************************************************************************
 */

#ifdef CFG_SPI_439
#include <stdint.h>
#include "global_io.h"
#include "spi.h"
#include "spi_439.h"
#include <core_cm0.h>
#include "datasheet.h"
#include "uart.h"
#include <stdio.h>
#if HAS_AUDIO
    #include "app_audio439.h"
#endif


#define GetWord439(a) spi_439_getword(a)
#define SetWord439(a,b) spi_439_setword(a,b)
 
#define SC14439_DMA_LEN      160
#define SC14439_DMA_DST_ADDR 0x0CD8

#define SC14439_MAGIC_ADDR 0x0D80
#define SC14439_MAGIC_WORD 0x1A30   

#define JOH_CLK_SETTINGS 

#define VDDIO_DELAY         VDD_TURN_ON_DELAY*2+250*2   // VDDIO will be enabled 250usec after VDD is stable
#define VDD_TO_CLK_DELAY    VDD_TURN_ON_DELAY*2+2000*2  // CLK will be enabled 2000usec after VDD is stable
                                                        // reset rises above 0.8 in 1.4ms from the moment that VDD is enabled 

/**
 ****************************************************************************************
 * @brief SPI_439 delay
 *
 * Simple Delay function 
 *
 * @return void
 ****************************************************************************************
 */
 void spi439_delay (unsigned int delay)
 {
     for (volatile unsigned int j=0; j<delay;j++);  
 }
 
/**
 ****************************************************************************************
 * @brief SPI_439 initialization
 *
 * Should only be called once after power reset of 439, do not call this twice.
 *
 * @return void
 ****************************************************************************************
 */
void spi_439_init()
{
//FIRST OF ALL PROVIDE POWER!!
#ifdef HAS_AUDIO_MUTE

#ifdef HAS_AUDIO_SOFT_START    
    volatile int i,j;
    for (i=0; i<45; i++) {
        GPIO_ConfigurePin( AUDIO_MUTE_PORT, AUDIO_MUTE_PIN, INPUT_PULLUP, PID_GPIO, 0 );  
        spi439_delay(50);
        GPIO_ConfigurePin( AUDIO_MUTE_PORT, AUDIO_MUTE_PIN, INPUT_PULLDOWN, PID_GPIO, 0 );  // Power on the 439, by setting MUTE_LDO = 0
        spi439_delay(5);
    }
#else
// Cannot use P0_6 and P0_7 as outputs because we use P0_5 as clock
    GPIO_ConfigurePin( AUDIO_MUTE_PORT, AUDIO_MUTE_PIN, ((AUDIO_MUTE_POLARITY)==GPIO_ACTIVE_HIGH)?INPUT_PULLUP:INPUT_PULLDOWN, PID_GPIO, 0 );  // Power on the 439, by setting MUTE_LDO active
#endif // HAS_AUDIO_SOFT_START
        
    spi439_delay(VDDIO_DELAY); 

#ifdef HAS_AUDIO_VDDIO_CONTROL   
    GPIO_ConfigurePin( AUDIO_VDDIO_CONTROL_PORT, AUDIO_VDDIO_CONTROL_PIN, ((AUDIO_VDDIO_CONTROL_POLARITY)==GPIO_ACTIVE_HIGH)?INPUT_PULLUP:INPUT_PULLDOWN, PID_GPIO, 0 );  // Enable VDDIO
#endif    
    
    spi439_delay(VDD_TO_CLK_DELAY-VDDIO_DELAY); 

#endif // HAS_AUDIO_MUTE
    
    // Enable the 16Mhz output clock for the 439
    // Note: putting the scope probe on pin 0-5 disturbed the debugger signals..
    SetWord32(TEST_CTRL_REG, 1);
    GPIO_SetPinFunction( AUDIO_CLK_PORT, AUDIO_CLK_PIN, OUTPUT, PID_GPIO );

    GPIO_ConfigurePin( AUDIO_SPI_EN_PORT,  AUDIO_SPI_EN_PIN,  OUTPUT, PID_SPI_EN, true );
    GPIO_ConfigurePin( AUDIO_SPI_CLK_PORT, AUDIO_SPI_CLK_PIN, OUTPUT, PID_SPI_CLK, false );
    GPIO_ConfigurePin( AUDIO_SPI_DO_PORT,  AUDIO_SPI_DO_PIN,  OUTPUT, PID_SPI_DO, false );    
    GPIO_ConfigurePin( AUDIO_SPI_DI_PORT,  AUDIO_SPI_DI_PIN,  INPUT,  PID_SPI_DI, false );

    spi439_delay(1200); //this disturbs with the PLL 
   
    SetBits16(SPI_CTRL_REG, SPI_ON, 0);			// switch off SPI block
    SPI_Pad_t spi_cs_pad;

    spi_cs_pad.port = AUDIO_SPI_EN_PORT;
    spi_cs_pad.pin  = AUDIO_SPI_EN_PIN;

    spi_init(&spi_cs_pad, SPI_MODE_16BIT, SPI_ROLE_MASTER,
             SPI_CLK_IDLE_POL_LOW,SPI_PHA_MODE_0,SPI_MINT_DISABLE,SPI_XTAL_DIV_14);  // SPI_XTAL_DIV_2=8MHZ, SPI_XTAL_DIV_8=2MHZ
               
    /*
    ** First try to read our magic word. It should be 1A30. If so, skip the clock initializations. 
    ** Otherwise, we are coming from cold boot.
    */
    {
#ifdef JOH_CLK_SETTINGS
        /*  
        ** clk=16 Mhz from the 580 => set 439 PLL for Fsys = 46.08 MHz
        ** This means:
        ** - SC14439_CLK_CTRL_REG=0,0x12, 0x1A 
        ** - SC14439_PLL_DIV_REG = 0x2A19, so VD/VX=(72/25)*16Mhz = 46.08 Mhz Fsys clock
        ** - SC14439_CLK_CTRL_REG  , turn on PLL
        ** - SC14439_PER_DIV_REG = 4
        ** - SC14439_CODEC_DIV_REG = 20, so ClodecClk=46.08/20=2.304 Mhz
        */

        SetWord439(SC14439_CLK_CTRL_REG, 0);    
        SetWord439(SC14439_PLL_DIV_REG,  0x2A19);    /* VD/XD=(72/25)*16MHz = 46.08 Mhz Fsys. VD=72=6x12=01.0101; XD=25=00.11001, total = 0010.1010.0001.1001 */
        SetWord439(SC14439_CLK_CTRL_REG, 0x12);    
        SetWord439(SC14439_PER_DIV_REG,  4);         /* see calculation in DS SC14439, per_div<4 */
#else
        /*  
        ** clk=16 Mhz from the 580 => set 439 PLL for Fsys = 39.183 MHz
        ** This means:
        ** - SC14439_CLK_CTRL_REG=0,0x12, 0x1A 
        ** - SC14439_PLL_DIV_REG = 0x2A19, so VD/VX=(120/49)*16Mhz = 39.183 Mhz Fsys clock
        ** - SC14439_CLK_CTRL_REG  , turn on PLL
        ** - SC14439_PER_DIV_REG = 4
        ** - SC14439_CODEC_DIV_REG = 20, so ClodecClk=46.08/20=2.304 Mhz
        */

        SetWord439(SC14439_CLK_CTRL_REG, 0);    
        SetWord439(SC14439_PLL_DIV_REG,  0x3631);    /* VD/XD=(120/49)*16MHz =  39.183 Mhz Fsys. VD=120=10x12=01.1011; XD=49=01.10001 */
        SetWord439(SC14439_CLK_CTRL_REG, 0x12);    
        SetWord439(SC14439_PER_DIV_REG,  4);         /* see calculation in DS SC14439, per_div<4 */
#endif
        SetWord439(SC14439_BAT_CTRL_REG, 2);         /* VDDIO 2.25..2.75V gr: ok up to 3.45 V  Not required - default value is valid */
        SetWord439(SC14439_CODEC_VREF_REG, 0x0000);
        SetWord439(SC14439_CODEC_ADDA_REG, 0x040e);
        
        SetWord439(SC14439_CODEC_MIC_REG,  AUDIO_MIC_AMP_GAIN);    	  

        spi439_delay(1500);                          //wait for PLL to settle
        SetWord439(SC14439_CLK_CTRL_REG, 0x1A);    
        spi439_delay(100); //wait 16 spi cycles
    }
    
    /*
    ** 439 Codec initialization
    **
    */   
#ifdef JOH_CLK_SETTINGS
    SetWord439(SC14439_CODEC_DIV_REG, 0x14);       /* codec clk divider 46.08/20 = 2.304 MHz */
#else
    SetWord439(SC14439_CODEC_DIV_REG, 0x11);       /* codec clk divider Fsys/17 = 2.304 MHz */
#endif
    spi439_delay(100); //wait a while
    SetWord439(SC14439_CODEC_ADDA_REG, 0x0006);
    SetWord439(SC14439_CODEC_VREF_REG, 0x0000); 

    /*
    ** Setup DMA on 439
    ** DMA will collect 80 samples from CODEC and put them in Shared RAM at address 0x0CD8, in circular buffer
    ** and continue indefenitely.
    ** 
    ** The DMA buffer on the 439 is set to 80 samples = 160 bytes (Note hat DMA0_LEN is in bytes)
    ** Start address is 0xCD8. Second part of buffer starts at 0xD28
    **
    */
    SetWord439(SC14439_DMA0_LEN_REG,      SC14439_DMA_LEN);
    SetWord439(SC14439_DMA0_A_STARTL_REG, SC14439_DMA_DST_ADDR);
    SetWord439(SC14439_DMA0_A_IDX_REG,    0);
    SetWord439(SC14439_DMA0_B_STARTL_REG, SC14439_CODEC_IN_OUT_REG);
    SetWord439(SC14439_DMA0_B_IDX_REG,    0);
    SetWord439(SC14439_DMA0_CTRL_REG,     0x062D);   // SYNC_SEL=0, DREQ_LEVEL=0,CIRUCLAR=1., AINC=10,BINC=00 DREQ_MODE=1, RSRV, IND=1, DIR=1, RSRVD, DMA_ON=1 = 0000.0110.0010.1101

    
    NVIC_SetPriority(SPI_IRQn,1);
    NVIC_EnableIRQ(SPI_IRQn);
}

/**
 ****************************************************************************************
 * @brief SPI_439 release
 *
 * Should be called for powering-down 439 and releasing SPI interface.
 *
 * @return void
 ****************************************************************************************
 */
void spi_439_release(void)
{
#ifdef HAS_AUDIO_VDDIO_CONTROL    
    GPIO_ConfigurePin(AUDIO_VDDIO_CONTROL_PORT, AUDIO_VDDIO_CONTROL_PIN, ((AUDIO_VDDIO_CONTROL_POLARITY)==GPIO_ACTIVE_HIGH)?INPUT_PULLDOWN:INPUT_PULLUP, PID_GPIO, 0 );  // Power down the 439
#endif

#ifdef HAS_AUDIO_MUTE
    GPIO_ConfigurePin(AUDIO_MUTE_PORT, AUDIO_MUTE_PIN, ((AUDIO_MUTE_POLARITY)==GPIO_ACTIVE_HIGH)?INPUT_PULLDOWN:INPUT_PULLUP, PID_GPIO, 0 );  // Power down the 439 by setting MUTE_LDO inactive
#endif

    GPIO_SetPinFunction( AUDIO_SPI_EN_PORT,  AUDIO_SPI_EN_PIN,  INPUT_PULLDOWN, PID_GPIO);
    GPIO_SetPinFunction( AUDIO_SPI_CLK_PORT, AUDIO_SPI_CLK_PIN, INPUT_PULLDOWN, PID_GPIO);
    GPIO_SetPinFunction( AUDIO_SPI_DO_PORT,  AUDIO_SPI_DO_PIN,  INPUT_PULLDOWN, PID_GPIO);    
    GPIO_SetPinFunction( AUDIO_SPI_DI_PORT,  AUDIO_SPI_DI_PIN,  INPUT_PULLDOWN, PID_GPIO);

    SetWord32(TEST_CTRL_REG, 0);
    GPIO_SetPinFunction( AUDIO_CLK_PORT, AUDIO_CLK_PIN, INPUT_PULLDOWN, PID_GPIO );    
}


int16_t *spi_439_buf_ptr;

#ifdef SPI439_TEST
/**
 ****************************************************************************************
 * @brief Test and debugging function for 439 SPI interface
 *
 * @return void
 ****************************************************************************************
 */
void spi_439_test(void)
{
    int i,j;
    SetBits16(SPI_CTRL_REG, SPI_MINT, SPI_MINT_DISABLE);
        
    int sc_addr = 0x0CD8;
    SetWord439(sc_addr,0xAAAA); sc_addr+=2;
    SetWord439(sc_addr,0xBBBB); sc_addr+=2;
    SetWord439(sc_addr,0xCCCC); sc_addr+=2;
    SetWord439(sc_addr,0xDDDD); sc_addr+=2;
    SetWord439(sc_addr,0xEEEE); sc_addr+=2;
    SetWord439(sc_addr,0xFFFF); sc_addr+=2;
    SetWord439(sc_addr,0x1111); sc_addr+=2;
    SetWord439(sc_addr,0x2222); sc_addr+=2;
    SetWord439(sc_addr,0x3333); sc_addr+=2;
    SetWord439(sc_addr,0x4444); sc_addr+=2;
    SetWord439(sc_addr,0x5555); sc_addr+=2;
    SetWord439(sc_addr,0x6666); sc_addr+=2;
    SetWord439(sc_addr,0x7777); sc_addr+=2;
    SetWord439(sc_addr,0x8888); sc_addr+=2;
    
    sc_addr = 0x0CD8;
    for (i=0;i<160;i++) {
       SetWord439(sc_addr,i); sc_addr+=2;
    }
        
    for (j=0;j<30000;j++) {
        uint32_t d;
        char s;
        if (j&0x04) {
            d = GetWord439(SC14439_DMA0_A_IDX_REG); // SC14439_PLL_DIV_REG); // SC14439_CLK_CTRL_REG) ; // SC14439_VERSION_REG1);
            s='i';
        } else {
            d = GetWord439(SC14439_DMA0_CTRL_REG); // SC14439_PLL_DIV_REG); // SC14439_CLK_CTRL_REG) ; // SC14439_VERSION_REG1);
            s = 'c';
        }
        char msg[15];
        sprintf(msg,"v %c=%x \r\n",s,d);
        uart_write((uint8_t*)msg,sizeof(msg),NULL);
        spi439_delay (0x7FFF);
    }
}
#endif

/**
****************************************************************************************
* @brief SPI 439 Read Audio Sample from 439 CODEC
*
* This functions uses POLLING and is time consuming. 
*
* @return audio data read from the 439
****************************************************************************************
*/
int32_t spi_439_getsample(void)
{
    int32_t d = GetWord439(SC14439_CODEC_IN_OUT_REG);
    return d;
}


/**
****************************************************************************************
* @brief SPI 439 Read
* @param[in] address: 12 bits register address on 439
* @param[in] data: 
*
* To read one data word from the SPI, 3 transactions are needed.
* - First transaction: send command&address
* - Second transaction: send dummy 0
* - Third transacation: read returned data
* This functions uses POLLING to check if SPI transaction has completed.
*
* @return  data read from the 439
****************************************************************************************
*/
uint32_t spi_439_getword(uint32_t address)  // IZP named changed
{
    uint32_t dataRead = 0;
    
    spi_cs_low();

    uint32_t dataToSend = ((SC14439_MEM_RD<<13) | (address&0x1FFF ));
    SetWord16(SPI_RX_TX_REG0, (uint16_t)dataToSend);      // write (low part of) dataToSend
    while (GetBits16(SPI_CTRL_REG, SPI_INT_BIT) == 0);    // polling to wait for spi transmission
    
    SetWord16(SPI_CLEAR_INT_REG, 0x01);                   // clear pending flag
    dataToSend = 0;
    SetWord16(SPI_RX_TX_REG0, (uint16_t)dataToSend);      // write (low part of) dataToSend
    while (GetBits16(SPI_CTRL_REG, SPI_INT_BIT) == 0);    // polling to wait for spi transmission
    
    SetWord16(SPI_CLEAR_INT_REG, 0x01);                   // clear pending flag
    SetWord16(SPI_RX_TX_REG0, (uint16_t)dataToSend);      // write (low part of) dataToSend
    while (GetBits16(SPI_CTRL_REG, SPI_INT_BIT) == 0);    // polling to wait for spi transmission
    
    SetWord16(SPI_CLEAR_INT_REG, 0x01);                   // clear pending flag
    dataRead  = GetWord16(SPI_RX_TX_REG0);                //read (low part of) data from spi slave
    spi_cs_high();

    return dataRead;                                      // return data read from spi slave
}
    

/**
****************************************************************************************
* @brief SPI 439  Write
* @param[in] address: 12 bits address on 439
* @param[in] data: 16 bits value for the register
*
* To write one data word to 439 with the SPI, 2 transactions are needed.
* - First transaction: send command&address
* - Second transaction: write data
* This functions uses POLLING to check if SPI transaction has completed.
*
* @return  data read 
****************************************************************************************
*/
void spi_439_setword(uint32_t address, uint32_t data)  // IZP named changed
{
    spi_cs_low();
    
    uint32_t dataToSend = ((SC14439_MEM_WR<<13) | (address&0x1FFF ));
    
    SetWord16(SPI_RX_TX_REG0, (uint16_t)dataToSend);           // write address
    while (GetBits16(SPI_CTRL_REG, SPI_INT_BIT) == 0);       // polling to wait for spi transmission
    
    SetWord16(SPI_CLEAR_INT_REG, 0x01);                        // clear pending flag
    SetWord16(SPI_RX_TX_REG0, (uint16_t)data);                 // write (low part of) dataToSend
    while (GetBits16(SPI_CTRL_REG, SPI_INT_BIT) == 0);       // polling to wait for spi transmission
    
    SetWord16(SPI_CLEAR_INT_REG, 0x01);                        // clear pending flag

    spi_cs_high();
}


/**
 ****************************************************************************************
 * @brief Start or Restart the 439 SPI to get it ready for fetching Audio Data
 *
 * @return void
 ****************************************************************************************
 */
void spi_439_codec_restart(void)
{   
    /* disable interrupts, and use 16 bits transfers.. */
    SetBits16(SPI_CTRL_REG, SPI_MINT, SPI_MINT_DISABLE);
    SetBits16(SPI_CTRL_REG, SPI_WORD, SPI_MODE_16BIT ); 
    
    spi_cs_high();
    SetWord439(SC14439_DMA0_CTRL_REG,  0x0E2C);   // Disable, SYNC_SEL=0, DREQ_LEVEL=1,CIRUCLAR=1., AINC=10,BINC=00 DREQ_MODE=1, RSRV, IND=1, DIR=1, RSRVD, DMA_ON=0 = 0.1110.0010.1100
    SetWord439(SC14439_DMA0_A_IDX_REG, 20);
    SetWord439(SC14439_DMA0_CTRL_REG,  0x062D);   // Enable,  SYNC_SEL=0, DREQ_LEVEL=0,CIRUCLAR=1., AINC=10,BINC=00 DREQ_MODE=1, RSRV, IND=1, DIR=1, RSRVD, DMA_ON=1 = 0.0110.0010.1101

    /* Now enable interupts, use 32 bits and pull enable low */
    SetBits16(SPI_CTRL_REG, SPI_MINT, SPI_MINT_ENABLE);
    SetBits16(SPI_CTRL_REG, SPI_WORD, SPI_MODE_32BIT ); 

    spi_cs_low();    /* This enables the SPI. It can stay low for all Audio SPI transactions */  
}

    //#define TESTING_SPI
    //Enabling this test will replace the samples from 439 with a pattern (triangle)

#ifdef CFG_SPI_439_SAMPLE_BASED
/*******************************************************
**
** Sample based approach
*/

const uint32_t spi_439_firstword = ((SC14439_MEM_BRD<<13) | (SC14439_CODEC_IN_OUT_REG)); //  SC14439_DMA0_A_IDX_REG));  // 
uint32_t spi_439_first=0;

#ifdef TESTING_SPI
    static int16_t test_val = 0;
    static int16_t test_inc = 2;
#endif

/**
 ****************************************************************************************
 * @brief Get one codec sample from 439, using Interrupt to fetch result
 *
 * @return void
 ****************************************************************************************
 */
// would be better to put this inline.. it is only called from systick handler in app_audio439_task.c.
void spi_439_get_codec_sample(void)
{
    spi_439_first = 1;
    SetWord16(SPI_RX_TX_REG1, (uint16_t)spi_439_firstword);
    SetWord16(SPI_RX_TX_REG0, 2);
}

/**
 ****************************************************************************************
 * @brief The 439 SPI Interrupt handler, to copy the Audio sample to location in memory
 *
 * Note that this function is called twice per sample. Note that the SPI interface
 * needs 3 16-bits transactions to read one register. We are doing 32 bits transactions
 * and therefore need 2 to handle interrupt twice.
 * 
 * @return void
 ****************************************************************************************
 */
void SPI_Handler(void)
{ 
    // Received SPI interrupt..
    SetWord16(SPI_CLEAR_INT_REG, 0x01);  
    if (spi_439_first) {
        // Start next SPI transaction
        SetWord16(SPI_RX_TX_REG1, 0);  // write high part of dataToSend
        SetWord16(SPI_RX_TX_REG0, 0);  // write low part of dataToSend - this will trigger next SPI 
        spi_439_first = 0;
    } else {
        /** We are done */
        #ifndef TESTING_SPI
        *spi_439_buf_ptr = GetWord16(SPI_RX_TX_REG1); // tst2++; // 
        #else
        *spi_439_buf_ptr = test_val;
        test_val += test_inc;
        if (test_val == 32000) {
            test_inc = -2;
        } else if (test_val == 0) {
            test_inc = +2;
        }
        #endif
    }
}

#endif


#ifdef CFG_SPI_439_BLOCK_BASED
/*
 ****************************************************************************************
 *
 * Block based approach
 *
 * The 439 is setup with DMA that copies the CODEC_IN_OUT_REG value to circular buffer
 * in memory. 
 * Buffer starts at 0x0CDA, and is 160 samples long (2 x 80).  
 * We are reading a block of 80 samples at a time. After IMA ADPCM compression, 80 samples
 * will fit in 20 bytes, our magic number.
 * 
 ****************************************************************************************
*/

const uint32_t spi_439_bufstart0 = ((SC14439_MEM_BRD<<13) | (SC14439_DMA_DST_ADDR));
const uint32_t spi_439_bufstart1 = ((SC14439_MEM_BRD<<13) | (SC14439_DMA_DST_ADDR+(SC14439_DMA_LEN>>1))); 
uint32_t spi_439_block_idx=0;         // Global variable used by SPI interrupt handler to count number of transactions

/**
 ****************************************************************************************
 * @brief Start an SPI read block transfer of 40 audio samples from 439
 *
 * Note that we do 32 bits SPI processing, so the 580 SPI thinks it is loading 32 bits values,
 * and the 439 thinks it is doing 2 x 16 bits values.
 *
 * NOTE: it would be better to put this inline, only called from systick handler in app_audio439_task.c.
 * 
 * @return void
 ****************************************************************************************
 */

#ifdef TESTING_SPI
static int testctn=0;
static int testctnincr=2;
static int startpkt=0;
#endif
 
void spi_439_getblock(int i)
{
    uint16_t firstword = spi_439_bufstart0;
    if ((i & 0x01) == 0) {
        firstword = spi_439_bufstart1;
    }

    spi_439_block_idx = 20;  // 1 Control + 20*2 samples, 
    SetWord16(SPI_RX_TX_REG1, (uint16_t)firstword);
    SetWord16(SPI_RX_TX_REG0, 40);   /* Tell 439 it needs to send 40 samples */

#ifdef TESTING_SPI    
    startpkt=1;
#endif
}

/**
 ****************************************************************************************
 * @brief SPI interrupt handler for reading block of 40 audio samples from 439
 *
 * The destination buffer should have TWO extra dummy values at beginning of buffer, which will contain
 * the SPI result of first transaction. Although we could add check spi_439_block_idx == 21, this would
 * add more undesired cycle overhead. 
 *
 * NOTE to use SPI_Handler, uncomment the SPI_Handler entry in the linker rom_symdef.txt file,
 * see linker settings.
 * 
 * @return void
 ****************************************************************************************
 */
//#define MARK_PACKETS
//Enabling MARK_PACKETS will add a marker peridically a marker in the samples coming in through the SPI
#ifdef MARK_PACKETS
static int packet_marker=0;
#endif
void SPI_Handler(void)
{ 
    // Received SPI interrupt..
    SetWord16(SPI_CLEAR_INT_REG, 0x01);
    /* Save the words in memory */
    int16_t v1 = (int16_t)GetWord16(SPI_RX_TX_REG1);
    int16_t v2 = (int16_t)GetWord16(SPI_RX_TX_REG0);
#ifndef TESTING_SPI    
    #ifdef MARK_PACKETS
    if ((spi_439_block_idx<3) || ((spi_439_block_idx>8)&& (spi_439_block_idx<12))) {
        if ((spi_439_block_idx==0)||(spi_439_block_idx==9)) {
            packet_marker+=3000;
        }
        if (packet_marker>30000) {
            packet_marker=0;
            
        }
        v2+=packet_marker;
        v1+=packet_marker;
    }
    #endif
    *spi_439_buf_ptr++ = v1; // (int16_t)GetWord16(SPI_RX_TX_REG1);
    *spi_439_buf_ptr++ = v2; // (int16_t)GetWord16(SPI_RX_TX_REG0);  // spitst++; // 
#else
    int offs = 0;
    if (startpkt==5) {
        offs=100;
    }
    else startpkt++;
    testctn+=testctnincr;
    *spi_439_buf_ptr++ =testctn + offs;
    testctn+=testctnincr;
    *spi_439_buf_ptr++ =testctn;
    if (testctn>16000) {
        testctnincr=-2;
    }
    if (testctn<0) {
        testctnincr=+2;
    }
#endif // TESTING_SPI
    
    if (spi_439_block_idx > 0){
        // Start next SPI transaction
        SetWord16(SPI_RX_TX_REG1, 0);  // write high part of dataToSend
        SetWord16(SPI_RX_TX_REG0, 0);  // write low part of dataToSend - this will trigger next SPI 
        spi_439_block_idx--;
    }
    /** else ** We are done */
}


#endif // CFG_SPI_439_BLOCK_BASED

#endif // CFG_SPI_439

