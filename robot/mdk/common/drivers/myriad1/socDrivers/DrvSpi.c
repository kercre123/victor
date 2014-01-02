#include <mv_types.h>
#include <isaac_registers.h>
#include <stdlib.h>
#include "DrvSpi.h"
#include "DrvTimer.h"
#include <swcLeonUtils.h>
#include <stdio.h>


/* ***********************************************************************//**
   *************************************************************************
SPI enable - disable
@{
----------------------------------------------------------------------------
@}
@param
      ena
            - if set to 1 the module will be enabled
            - if set to 0 the module will be disabled
            .
@param
      spiBaseAdrLocal
                   - base address of the SPI
                            SPI1_BASE_ADR
                            SPI2_BASE_ADR
                            SPI3_BASE_ADR
                   .
@return
@{
info:
     - The module should be enabled after it was configured
     .
@}
************************************************************************* */
void DrvSpiEn(u32 spiBaseAdrLocal, u32 ena)
{
   if (ena)
     SET_REG_WORD(spiBaseAdrLocal + MODE_REG, GET_REG_WORD_VAL(spiBaseAdrLocal + MODE_REG) | (0x01 << 24) );   // enable SPI1
   else
     SET_REG_WORD(spiBaseAdrLocal + MODE_REG, GET_REG_WORD_VAL(spiBaseAdrLocal + MODE_REG) & (~(0x01 << 24)) ); // disable SPI1

}


/* ***********************************************************************//**
   *************************************************************************
SPI mode configure
@{
----------------------------------------------------------------------------
@}
@param
       mode
              - 1 master
              - 0 slave
              .
@param
       wSize
              - word size: 0000b => 32b
              -  0011b:1111b  => 4-16b
              .
@param
       rev
              - 0 LSB first
              - 1 MSB first
              .
@param
       tWen   - 1 three wire mode
@return
@{
info:
     - This function should be called first in the init chain.
     .
@}
************************************************************************* */
void DrvSpiInit(u32 spiBaseAdrLocal,u32 mode, u32 wSize, u32 rev, u32 tWen)
{
   SET_REG_WORD(spiBaseAdrLocal + MODE_REG, ((rev   & 0x01) << 26) | ((mode  & 0x01) << 25) |
                                          ((wSize & 0x0f) << 20) | ((tWen  & 0x01) << 15) );


}

/* ************************************************************************
SPI timing configure
@{
----------------------------------------------------------------------------
@}
@param
       sysClk  - system clock, (most likely the clock from CPR to SPI)
@param
       spiClk  - spi clock
@return
      - returns the division factor
      - normal values are between 4 and 1024, any other value will not affect the settings
      .
@{
info:
     - This function MUST BE CALLED ONLY AFTER drvSpixInit !
     .
@}
************************************************************************* */
static u32 drvSpiClkSpeed(u32 SpiBaseAdrLocal, u32 sysClk, u32 spiClk)
{
   u32 div16, pm;
   u32 di;

   di = (1000*((u32)sysClk / (u32)spiClk) + 999) / 1000;

   if ( (di<=64) && (di>=4) )
   {
      div16 = 0;
      pm = di >> 2;

      SET_REG_WORD(SpiBaseAdrLocal + MODE_REG, GET_REG_WORD_VAL(SpiBaseAdrLocal + MODE_REG) |
                ((div16) << 27) |
                ((pm    & 0x0F) << 16) );
   }
   else
   if ( (di<=1024) && (di>64) )
   {
      div16 = 1;
      pm = di >> 2;

      SET_REG_WORD(SpiBaseAdrLocal + MODE_REG, GET_REG_WORD_VAL(SpiBaseAdrLocal + MODE_REG) |
                ((div16) << 27) |
                ((pm    & 0x0F) << 16) );
   }
   return di;
}

/*************************************************************************
SPI timing configure
@{
----------------------------------------------------------------------------
@}
@param
       cpol    - clock polarity in idle mode
@param
       cpha    - clock phase
@param
       cg      - clock cycles between words 0000b-1111b
@param
       sysClk   - Bus Clk frequency in KHz
@param
       i2cClk   - I2C clk desired frequency in KHz
@return
@{
info:
       - This function MUST BE CALLED ONLY AFTER drvSpixInit !
       .
@}
************************************************************************* */
void DrvSpiTimingCfg(u32 spiBase,u32 cpol, u32 cpha, u32 cg, u32 sysClk, u32 spiClk)
{
    SET_REG_WORD(spiBase + MODE_REG, GET_REG_WORD_VAL(spiBase + MODE_REG) |
                ((cpol  & 0x01) << 29) | ((cpha & 0x01) << 28) | ((cg & 0x0F) << 7 ));
    if(sysClk || spiClk)
        drvSpiClkSpeed(spiBase, sysClk, spiClk);
}

// /* ***********************************************************************//**
//    *************************************************************************
// SPI status
// @{
// ----------------------------------------------------------------------------
// @}
// @return
//        - contents of the status register
//        - bit:
//            - 8  Not Full
//            - 9  not empty
//            - 10 multy masster error
//            - 11 underrun
//            - 14 last character
//            .
//         .
// @{
// info:
// @}
// ************************************************************************* */
// u32 DrvSpiStatus(u32 SpiBaseAdrLocal)
// {
//      return GET_REG_WORD_VAL(spiBaseAdrLocal + EVENT_REG);
// }

/* ***********************************************************************//**
   *************************************************************************
SPI mask register
@{
----------------------------------------------------------------------------
@}
@param  -  Seting a bit , will enable interrupt generation
         -  bit:
            - 8  Not Full enable
            - 9  not empty enable
            - 10 multy masster error enable
            - 11 underrun enable
            - 14 last character  enable
            .
         .
@return
@{
info:
@}
************************************************************************* */
void DrvSpiInMask(u32 spiBaseAdrLocal,u32 mask)
{
     SET_REG_WORD(spiBaseAdrLocal + MASK_REG, mask);
}

/* ***********************************************************************//**
   *************************************************************************
SPI command register
@{
----------------------------------------------------------------------------
@}
@return
@{
info:
      - setting this bit enables possible activation of LT bit in the event register
      - self clearing
      .
@}
************************************************************************* */
void DrvSpiSetLastBit(u32 spiBaseAdrLocal)
{
     SET_REG_WORD(spiBaseAdrLocal + COMMAND_REG, LST);
}

/* ***********************************************************************//**
   *************************************************************************
SPI assert/deasert
@{
----------------------------------------------------------------------------
@}
@param
   ss - the slave number + 1,
         0 deaserts the ss signal
@return
@{
info:
@}
************************************************************************* */
void DrvSpiSs(u32 spiBaseAdrLocal, u32 ss)
{
    if (ss == 0)
        SET_REG_WORD(spiBaseAdrLocal + SLVSEL_REG, 0xFF);
    else
        SET_REG_WORD(spiBaseAdrLocal + SLVSEL_REG, ~(1 << (ss-1)));
}

/* ***********************************************************************//**
   *************************************************************************
SPI send one word
@{
----------------------------------------------------------------------------
@}
@param
   data - word, of length set previously, to be sent
@return
@{
info:
   it also adjusts the position of the MSB if the rev bit is set in the mode register,
   providing that the mode register was porogrammend corectly
@}
************************************************************************* */
void DrvSpiSendWord(u32 spiBaseAdrLocal, u32 data)
{
    u32 rev, length, reg;
    reg = GET_REG_WORD_VAL( spiBaseAdrLocal + MODE_REG );
    rev = (reg >> 26) & 1;         // get the rev bit, if rev =1 then the MSB of the data should be at bit 31
    length = (reg >> 20) & 0x0f;   // get the length of the word
    if (rev && length){
        data <<= 31 - length;      // adjust the position of the MSB
    }
    while ( !(GET_REG_WORD_VAL(spiBaseAdrLocal + EVENT_REG) & NF) ) NOP;  // wait while transmit fifo is full, no overflow condition
    SET_REG_WORD(spiBaseAdrLocal + TRANSMIT_REG, data);
}

/* ***********************************************************************//**
   *************************************************************************
SPI read one word
@{
----------------------------------------------------------------------------
@}
@param
@return
   data - word, of length set previously, received
@{
info:
   it also adjusts the position of the MSB if the rev bit is set in the mode register,
   providing that the mode register was porogrammend corectly
@}
************************************************************************* */
u32 DrvSpiRecvWord(u32 spiBaseAdrLocal)
{
    u32 rev, length, reg, data;
    reg = GET_REG_WORD_VAL( spiBaseAdrLocal + MODE_REG );
    rev = (reg >> 26) & 1;                                                              /* get the rev bit */
    length = (reg >> 20) & 0x0f;                                                        /* get the length of the word */

    GET_REG_WORD(spiBaseAdrLocal + RECEIVE_REG, data);

    data >>= rev ? 16 : 15 - length;                                                    /* adjust the position of the MSB */

    return data;
}

/* ***********************************************************************//**
   *************************************************************************
SPI send buffer
@{
----------------------------------------------------------------------------
@}
@param
   *data - pointer to a buffer of words to send
@param
   size  - size of the transfer
@return
@{
info:
  - it also adjusts the position of the MSB if the rev bit is set in the mode register,
   providing that the mode register was porogrammend corectly
   .
@}
************************************************************************* */
void DrvSpiSendBuffer(u32 spiBaseAdrLocal,u32 *data, u32 size)
{
    u32 rev, length, reg, i;

    reg = GET_REG_WORD_VAL( spiBaseAdrLocal + MODE_REG );

    rev = (reg >> 26) & 1;         // get the rev bit, if rev =1 then the MSB of the data should be at bit 31
    length = (reg >> 16) & 0x0f;   // get the length of the word
    rev = rev && length;

    for (i=0 ; i < size ; ++i)
    {
         while ( !(GET_REG_WORD_VAL(spiBaseAdrLocal + EVENT_REG) & NF) ) NOP;  // wait while transmit fifo is full, no overflow condition
         reg = *data;
         ++data;
         if (rev)
             reg <<= 31 - length;      // adjust the position of the MSB
         SET_REG_WORD(spiBaseAdrLocal + TRANSMIT_REG, reg);
    }
}


/* ***********************************************************************//**
   *************************************************************************
SPI flush receive FIFO
@{
----------------------------------------------------------------------------
@}
@return
@{
info:
      - all received data is lost
      - call this function only if the device is not receiving anymore, otherwise
      it might loop until the transmision is over (possible in slave mode)
      .
@}
 ************************************************************************* */
void DrvSpiFlushFifo(u32 spiBaseAdrLocal)
{
    u32 dat;
    while (GET_REG_WORD_VAL(spiBaseAdrLocal + EVENT_REG) & NE)    // while fifo not empty
        dat = GET_REG_WORD_VAL(spiBaseAdrLocal + RECEIVE_REG);  // get char
}


/* ***********************************************************************//**
   *************************************************************************
SPI receive word as master, send a word in the same time
@{
 ----------------------------------------------------------------------------
@}
@return
     data read
@{
Info:
   - it also adjusts the position of the MSB if the rev bit is set in the mode register,
   providing that the mode register was porogrammend corectly
   - master has to provide the clock, so it has to send data to receive
   - flush the fifo at the begining if necesary, the function assumes that the FIFOs are empty
   .
@}
 ************************************************************************* */
u32 DrvSpiWord(u32 spiBaseAdrLocal,u32 data)
{
    u32 reg, rev, length, recData;
    reg = GET_REG_WORD_VAL( spiBaseAdrLocal + MODE_REG );

    rev = (reg >> 26) & 1;                                              // get the rev bit, if rev =1 then the MSB of the data should be at bit 31
    length = (reg >> 16) & 0x0f;                                        // get the length of the word

    // transmit a word
    while (!(GET_REG_WORD_VAL(spiBaseAdrLocal + EVENT_REG) & NF)) NOP;      // wait for tx fifo empty, the FIFO should be empty after flushing the RX fifo, just in case it's not wait for it to become available
    if (rev && length)
        data <<= 31 - length;                                           // adjust the position of the MSB
    SET_REG_WORD(spiBaseAdrLocal + TRANSMIT_REG, data );                  // send a word


    // receive a word
    while (!(GET_REG_WORD_VAL(spiBaseAdrLocal + EVENT_REG) & NE)) NOP;      // wait for receive
    recData = GET_REG_WORD_VAL(spiBaseAdrLocal + RECEIVE_REG);           // rec

    // return alligned with lsb at bit 0
    if (length == 0x00)                                                 // 32 bits words
       return recData;
    else
    if (rev)                                                            // if revearsed the LSB is always at bit 16
       return (recData >> 16);
    else                                                                // if normal the MSB is at bit 15
       return (recData >> (15-length));
}


/* ***********************************************************************//**
   *************************************************************************
SPI send and receive a buffer in the same time
@{
 ----------------------------------------------------------------------------
@}
@param
    *txd    - pointer to a buffer to be transmitted
@param
    *rxd    - pointer to a zone where received data is stored
@param
    size    - transfer size
@return
@{
Info:
      - assumes the FIFO's are empty/(old contents is not needed)
      .
@}
 ************************************************************************* */
void DrvSpiRxTx(u32 spiBaseAdrLocal,u32 *txd, u32 *rxd, u32 size)
{
   u32 FIFO_size, dat;
   u32 rev, leng;
   u32 i,j;

   FIFO_size =  (GET_REG_WORD_VAL(spiBaseAdrLocal + CAPABILITY_REG) >> 8) & 0x00FF;  // size of the FIFO from the capability reg
   dat = GET_REG_WORD_VAL( spiBaseAdrLocal + MODE_REG );
   rev = (dat >> 26) & 1;                                              // get the rev bit, if rev =1 then the MSB of the data should be at bit 31
   leng = (dat >> 16) & 0x0f;                                          // get the length of the word

    // empty fifo
   while (GET_REG_WORD_VAL(spiBaseAdrLocal + EVENT_REG) & NE)
   {    // while fifo not empty
        dat = GET_REG_WORD_VAL(spiBaseAdrLocal + RECEIVE_REG);           // discard old contents of the FIFO
   }

   while (size)
   {
        i = size;
        if (i>FIFO_size) i = FIFO_size;

        // send max FIFO_length words
        for (j=0;j<i;++j)
        {
               while (!(GET_REG_WORD_VAL(spiBaseAdrLocal + EVENT_REG) & NF)) NOP;      // wait while TX fifo full, this should not happen
               dat = *txd++;
               if (rev && leng)
                   dat <<= 31 - leng;                                              // adjust the position of the MSB
               SET_REG_WORD(spiBaseAdrLocal + TRANSMIT_REG, dat );                   // send a word
               --size;
        }

        // receive max FIFO length words
        for (j=0;j<i;++j)
        {
               while (!(GET_REG_WORD_VAL(spiBaseAdrLocal + EVENT_REG) & NE)) NOP;      // wait while RX fifo empty
               dat = GET_REG_WORD_VAL(spiBaseAdrLocal + RECEIVE_REG);
               if (leng != 0x00)                                                   // not 32 bits words
               {
                   if (rev)                                                        // if revearsed the LSB is always at bit 16
                       dat >>= 16;
                   else                                                            // if normal the MSB is at bit 15
                       dat >>= 15-leng;
               }
               *rxd++ = dat;
        }
   }
}

int SPI_TEST_HELPER_func(u32 spiBaseAdrLocal,u32 *zone1, u32 *zone2, u32 size,u8 returnCode)
{
   u32 i, len;
   u32 *p1, *p2, j;

   len = (GET_REG_WORD_VAL( spiBaseAdrLocal + MODE_REG ) >> 16) & 0x0f;     // get the length of the word
   if (len == 0) len = 31;
   ++len;

   p1 = zone1; p2 = zone2; j=0;
   for (i=0 ; i<size ; ++i)    // init zones
   {
        *p1 = j; ++p1;
        *p2 = 0; ++p2;
        ++j;
        if ( j>= ((1u<<len)-1)) j=0;// limit pattern to word length
   }

   // send a buffer trough SPI and recive it the same way
   DrvSpiRxTx(spiBaseAdrLocal, zone1, zone2, size);

   p1 = zone1; p2 = zone2;
   for (i=0 ; i<size ; ++i)  // compare buffers
   {
        if ((*p1) != (*p2)) return ((returnCode<<24) | (i & 0xFFFFFF)); //test failed
        ++p1;
        ++p2;
   }

   return 0;
}

/* ***********************************************************************//**
   *************************************************************************
Test SPI in LOOP mode, functionality test at low speed
@{
----------------------------------------------------------------------------
@}
@param
    *zone1    - pointer to a free memory zone
@param
    *zone2    - pointer to another free memory zone
@param
    size      - how big the zones are, for conducting the test
@return
    - 0 - if test passes
         else
    - 0x01xxxxxx    in 1st test comparison xxxxxx mismatch
    .
@{
info:
@}
 ************************************************************************* */
int DrvSpiTestLoop(u32 spiBaseAdrLocal,u32 *zone1, u32 *zone2, u32 size)
{
    u32 r;

   // test 1 , 32b word, normal settings , lowest speed
   DrvSpiEn(spiBaseAdrLocal, 0);                         // disable SPI1
   DrvSpiInit(spiBaseAdrLocal, 1, 0, 0, 0);              // master, 32b word, normal order, not 3 whire mode
   DrvSpiTimingCfg(spiBaseAdrLocal, 0, 0, 1, 0x0F, 0);  // divizor is 1/(16*4*16), no cc between words
   SET_REG_WORD(spiBaseAdrLocal + MODE_REG, GET_REG_WORD_VAL(spiBaseAdrLocal + MODE_REG) | (1 << 30)); // set loop mode
   DrvSpiEn(spiBaseAdrLocal, 1);                         // enable SPI1
   r = SPI_TEST_HELPER_func(spiBaseAdrLocal, zone1, zone2, size, 1);
   if (r != 0) return r;

   // test 2, 8b words revearsed,  lowest speed
   DrvSpiEn(spiBaseAdrLocal, 0);                          // disable SPI1
   DrvSpiInit(spiBaseAdrLocal, 1, 7, 1, 0);               // master, 8b word, rev, not 3 whire mode
   DrvSpiTimingCfg(spiBaseAdrLocal, 0, 0, 1, 0x0F, 0);   // divizor is 1/(16*4*16), no cc between words
   SET_REG_WORD(spiBaseAdrLocal + MODE_REG, GET_REG_WORD_VAL(spiBaseAdrLocal + MODE_REG) | (1 << 30)); // set loop mode
   DrvSpiEn(spiBaseAdrLocal, 1);                          // enable SPI1
   r = SPI_TEST_HELPER_func(spiBaseAdrLocal, zone1, zone2, size, 2);
   if (r != 0) return r;

   // test 3, 12b words revearsed,  lowest speed
   DrvSpiEn(spiBaseAdrLocal, 0);                          // disable SPI1
   DrvSpiInit(spiBaseAdrLocal, 1, 11, 1, 0);               // master, 12b word, rev, not 3 whire mode
   DrvSpiTimingCfg(spiBaseAdrLocal, 0, 0, 0, 0x0F, 0);   // divizor is 1/(4*16), no cc between words
   SET_REG_WORD(spiBaseAdrLocal + MODE_REG, GET_REG_WORD_VAL(spiBaseAdrLocal + MODE_REG) | (1 << 30)); // set loop mode
   DrvSpiEn(spiBaseAdrLocal, 1);                          // enable SPI1
   r = SPI_TEST_HELPER_func(spiBaseAdrLocal, zone1, zone2, size, 3);
   if (r != 0) return r;

   // test 4, 6b words revearsed,  lowest speed
   DrvSpiEn(spiBaseAdrLocal, 0);                          // disable SPI1
   DrvSpiInit(spiBaseAdrLocal, 1, 5, 0, 0);               // master, 6b word,  not 3 whire mode
   DrvSpiTimingCfg(spiBaseAdrLocal, 0, 0, 0, 0x0F, 7);   // divizor is 1/(4*16), 7 cc between words
   SET_REG_WORD(spiBaseAdrLocal + MODE_REG, GET_REG_WORD_VAL(spiBaseAdrLocal + MODE_REG) | (1 << 30)); // set loop mode
   DrvSpiEn(spiBaseAdrLocal, 1);                          // enable SPI1
   r = SPI_TEST_HELPER_func(spiBaseAdrLocal, zone1, zone2, size, 4);
   if (r != 0) return r;

   // many more tests

   return 0;
}

/* ***********************************************************************//**
   *************************************************************************
SPI loop mode
@{
 ----------------------------------------------------------------------------
@}
@param
      spiBaseAdrLocal
                   - base address of the SPI
                            SPI1_BASE_ADR
                            SPI2_BASE_ADR
                            SPI3_BASE_ADR
@param
    loop    - 1 enables loop mode, 0 disables loop mode
@return
@{
Info:
@}
 ************************************************************************* */
void DrvSpiLoop(u32 spiBaseAdrLocal, u32 loop)
{
   if (loop)
       SET_REG_WORD(spiBaseAdrLocal + MODE_REG, GET_REG_WORD_VAL(spiBaseAdrLocal + MODE_REG) | (1 << 30)); // set loop mode
   else
       SET_REG_WORD(spiBaseAdrLocal + MODE_REG, GET_REG_WORD_VAL(spiBaseAdrLocal + MODE_REG) & (~(1 << 30))); // set loop mode
}

/* ***********************************************************************//**
   *************************************************************************
SPI Write Register
@{
 ----------------------------------------------------------------------------
@}
@param
      spiBaseAdrLocal
                   - base address of the SPI
                            SPI1_BASE_ADR
                            SPI2_BASE_ADR
                            SPI3_BASE_ADR
@param
    offset
@param value
@{
Info:
@}
 ************************************************************************* */
void drvSpiWrReg(u32 spiBaseAdrLocal, u32 offset, u32 data)
{
       SET_REG_WORD(spiBaseAdrLocal + offset, data);
}

/* ***********************************************************************//**
   *************************************************************************
SPI Read Register
@{
 ----------------------------------------------------------------------------
@}
@param
      spiBaseAdrLocal
                   - base address of the SPI
                            SPI1_BASE_ADR
                            SPI2_BASE_ADR
                            SPI3_BASE_ADR
@param
    offset
@return
@{
Info:
@}
 ************************************************************************* */
void drvSpiRdReg(u32 spiBaseAdrLocal, u32 offset, u32 data)
{
       GET_REG_WORD(spiBaseAdrLocal + offset, data);
}

/* ***********************************************************************//**
   *************************************************************************
SPI Configure GPIO
@{
 ----------------------------------------------------------------------------
@}
@param value
@return
@{
Info:
@}
 ************************************************************************* */

void DrvSpiConfigureGpio(){

// SPI1 :  gpio_76	spi1_txd        O       mode 3      MSPI1_MOSI
//         gpio_77	spi1_rxd	    I       mode 3      MSPI1_MISO
//         gpio_78	spi1_sclk_out	I/O     mode 3      MSPI1_SCK_VAR
//         gpio_79	spi1_ss_out_0	I/O     mode 3      MSPI1_SLVSEL[0]

    u32 address, data;
//  Configure SPI1 GPIO
    for (address = GPIO_MODE69_ADR; address <= GPIO_MODE71_ADR; address += 4) {
        GET_REG_WORD (address,  data);
        SET_REG_WORD (address, (data & 0xfffffff8) | 0x0);
    }
//      MSPI1_SLVSEL must be set as an output so the 11th bit must be 0
        GET_REG_WORD (GPIO_MODE72_ADR,  data);
        SET_REG_WORD (GPIO_MODE72_ADR, (data & 0xfffff7f8) | 0x0);
}

/* ***********************************************************************//**
   *************************************************************************
SPI Retrieve Data
@{
 ----------------------------------------------------------------------------
@}
@param
    bitW - Word length
@param
    rev - Reverse data
@param
    udata - data
@return
      - returns the data changed according to the bitW and rev
@{
Info:
@}
 ************************************************************************* */

u32 DrvSpiRetriveData (u32 bitW, u32 rev, u32 udata)
{
    u32 tmp;
    tmp = 0;
    if (rev)
        switch (bitW){
            case  3: tmp = ((udata >> 28) & 0xF   ) << 16; break;
            case  4: tmp = ((udata >> 27) & 0x1F  ) << 16; break;
            case  5: tmp = ((udata >> 26) & 0x3F  ) << 16; break;
            case  6: tmp = ((udata >> 25) & 0x7F  ) << 16; break;
            case  7: tmp = ((udata >> 24) & 0xFF  ) << 16; break;
            case  8: tmp = ((udata >> 23) & 0x1FF ) << 16; break;
            case  9: tmp = ((udata >> 22) & 0x3FF ) << 16; break;
            case 10: tmp = ((udata >> 21) & 0x7FF ) << 16; break;
            case 11: tmp = ((udata >> 20) & 0xFFF ) << 16; break;
            case 12: tmp = ((udata >> 19) & 0x1FFF) << 16; break;
            case 13: tmp = ((udata >> 18) & 0x3FFF) << 16; break;
            case 14: tmp = ((udata >> 17) & 0x7FFF) << 16; break;
            case 15: tmp = ((udata >> 16) & 0xFFFF) << 16; break;
            default: tmp = 0x69;
        }
    else
        switch (bitW) {
            case  3: tmp = (udata & 0xF   ) << 12; break;
            case  4: tmp = (udata & 0x1F  ) << 11; break;
            case  5: tmp = (udata & 0x3F  ) << 10; break;
            case  6: tmp = (udata & 0x7F  ) <<  9; break;
            case  7: tmp = (udata & 0xFF  ) <<  8; break;
            case  8: tmp = (udata & 0x1FF ) <<  7; break;
            case  9: tmp = (udata & 0x3FF ) <<  6; break;
            case 10: tmp = (udata & 0x7FF ) <<  5; break;
            case 11: tmp = (udata & 0xFFF ) <<  4; break;
            case 12: tmp = (udata & 0x1FFF) <<  3; break;
            case 13: tmp = (udata & 0x3FFF) <<  2; break;
            case 14: tmp = (udata & 0x7FFF) <<  1; break;
            case 15: tmp = (udata & 0xFFFF)      ; break;
            default: tmp   = 0x69;
        }
    return tmp;
}

/* ***********************************************************************//**
   *************************************************************************
SPI GET SPI Base
@{
 ----------------------------------------------------------------------------
@}
@param
    index   - takes values from 0 to 2
@return
      - returns the base address of an SPI
@{
Info:
@}
 ************************************************************************* */

 int DrvSpiMssiBase(u32 index)
 {
    switch (index){
        case 1: return SPI1_BASE_ADR; //LEON's MSSI
        case 2: return SPI2_BASE_ADR; //WPP's  MSSI
        case 3: return SPI3_BASE_ADR; //WPP's  MSSI
    }
    return 0;
 }

/**************************************************************************
SPI Wait for TX
@{
 ----------------------------------------------------------------------------
@}
@param
    spiBaseAdrLocal
@param
    init_delay
        - wait saome before checking the data was transmited
@return
@{
Info:
@}
 ************************************************************************* */

void DrvSpiWaitForTX(u32 baseAddr, u32 initDelay)
{
    SleepTicks(initDelay);
    while(!(GET_REG_WORD_VAL(baseAddr + EVENT_REG) & NE))                               /* Wait while we receive the word */
        ;
    SleepTicks(20);
}

/**************************************************************************
SPI - wait until last bit is transmitted
@{
 ----------------------------------------------------------------------------
@}
@param
    baseAddr
        - SPI Base Address
@return
@{
Info:
@}
 ************************************************************************* */
void DrvSpiWaitLastBit(u32 baseAddr)
{
    while (!(GET_REG_WORD_VAL(baseAddr + EVENT_REG) & LT))
        ;
    SET_REG_WORD(baseAddr + EVENT_REG, LT);                                            /* Clear LT Int */
    DrvSpiSetLastBit(baseAddr);                                                    /* Set LST bit */
}

/**************************************************************************
SPI - check if EEPROM is busy writting data internally
@{
 ----------------------------------------------------------------------------
@}
@param
    base - SPI Base Address
@return
    -  1 if EEPROM busy; 0 otherwise.
@{
Info:
@}
 ************************************************************************* */
static u32 drvEepBusy(u32 baseAddr)
{
    u32 data=0;

    DrvSpiFlushFifo(baseAddr);
    DrvSpiSs (baseAddr, SLV_SEL_1);                                                           /* Enable EEPROM /CS */
    DrvSpiSendWord (baseAddr, RDSR);                                                         /* Send Read Status Command */
    DrvSpiSendWord (baseAddr, 0xFF);                                                         /* Send a dummy word */
    DrvSpiWaitLastBit(baseAddr);                                                                     /* Wait 'till last bit is transmited */

    while (GET_REG_WORD_VAL(baseAddr + EVENT_REG) & NE)                                         /* While receive fifo not empty */
        GET_REG_WORD(baseAddr + RECEIVE_REG, data);                                             /* Read word */

    DrvSpiSs(baseAddr, SLV_DES_ALL);                                                          /* Disable EEPROM /CS */
    data = (data >> 16) & 0x1;
    return (data);
}

/**************************************************************************
SPI - enable writes to the EEPROM
@{
 ----------------------------------------------------------------------------
@}
@param
    base - SPI Base Address
@{
Info:
@}
 ************************************************************************* */
static void drvEepEnWr(u32 baseAddr)
{
    DrvSpiSs(baseAddr, SLV_SEL_1);                                                            /* Enable EEPROM /CS */
    DrvSpiSendWord(baseAddr, WREN);
    DrvSpiWaitLastBit(baseAddr);                                                                     /* Wait while the instruction byte is sent */
    DrvSpiSs(baseAddr, SLV_DES_ALL);                                                          /* Disable EEPROM /CS */
}

/* ***********************************************************************//**
   *************************************************************************
SPI EEPROM Write
@{
 ----------------------------------------------------------------------------
@}
@param
    addr
        - Start address of the transaction
@param
    addrSize
        - 16 or 24 bits addressing mode
@param
    data
        - Start address of buffer wanted to be written
@param
    size
        - Number of bytes wanted to be transfered
@param
    pageSize
        - Page size of the EEPROM
@return
@{
@}
 ************************************************************************* */

void DrvSpiEepromWrite(u32 baseAddr, u32 addr, u32 addrSize, u32 *data, u32 size, u32 pageSize)
{
    u32 i;
    u32 n;

    while (size > 0) {
        n = pageSize - (addr & (pageSize - 1));
        if (n > size) n = size;
        drvEepEnWr(baseAddr);
        DrvSpiSs (baseAddr, SLV_SEL_1);                                              /* Enable EEPROM /CS */

        DrvSpiSendWord (baseAddr, WRITE);                                           /* Send Write Command */
        DrvSpiWaitLastBit(baseAddr);                                                        /* Wait until the last byte is sent */
        if (addrSize == 24) {                                                          /* If EEPROM is 24 bits addressing */
            DrvSpiSendWord (baseAddr, ((addr >> 16) & 0xFF) );                      /* Send High Byte Address */
        }
        DrvSpiWaitLastBit(baseAddr);                                                        /* Wait until the last byte is sent */
        DrvSpiSendWord (baseAddr, ((addr >> 8) & 0xFF) );                           /* Send Middle/High Byte Address */
        DrvSpiWaitLastBit(baseAddr);                                                        /* Wait until the last byte is sent */
        DrvSpiSendWord(baseAddr, (addr & 0xFF) );                                   /* Send Low Byte Address */
        DrvSpiWaitLastBit(baseAddr);                                                        /* Wait until the last byte is sent */

        for (i = 0 ; i < n ; ++i){
            DrvSpiSendWord(baseAddr, *data);
            data++;
        }
        DrvSpiWaitLastBit(baseAddr);                                                        /* Wait until the last byte is sent */
        DrvSpiSs(baseAddr, SLV_DES_ALL);                                             /* Disable EEPROM /CS */
        while(drvEepBusy(baseAddr))                                                  /* Wait while the EEPROM internal write is in progress */
            ;
        addr += n;
        size -= n;
    }
}

/***************************************************************************
SPI EEPROM Read
@{
 ----------------------------------------------------------------------------
@}
@param
    addr
        - Start address of the transaction
@param
    addrSize
        - 16 or 24 bits addressing mode
@param
    data
        - Start address of buffer wanted to be read
@param
    size
        - Number of bytes wnated to be transfered
@return
@{
@}
 ************************************************************************* */

void DrvSpiEepromRead(u32 baseAddr, u32 addr, u32 addrSize, u32 *data, u32 size)
{
    u32 i;
    u32 nSendBytes;

    DrvSpiFlushFifo(baseAddr);
    DrvSpiSs (baseAddr, SLV_SEL_1);                                                  /* Enable EEPROM /CS */

        DrvSpiSendWord (baseAddr, READ);                                            /* Send Write Command */
        DrvSpiWaitLastBit(baseAddr);                                                        /* Wait until the last byte is sent */
        if (addrSize == 24) {                                                          /* If EEPROM is 24 bits addressing */
            DrvSpiSendWord (baseAddr, ((addr >> 16) & 0xFF) );                      /* Send High Byte Address */
        }
        DrvSpiWaitLastBit(baseAddr);                                                        /* Wait until the last byte is sent */
        DrvSpiSendWord (baseAddr, ((addr >> 8) & 0xFF) );                           /* Send Middle/High Byte Address */
        DrvSpiWaitLastBit(baseAddr);                                                        /* Wait until the last byte is sent */
        DrvSpiSendWord (baseAddr, (addr & 0xFF) );                                  /* Send Low Byte Address */
        DrvSpiWaitLastBit(baseAddr);                                                        /* Wait until the last byte is sent */
    DrvSpiFlushFifo(baseAddr);

    nSendBytes = size;

    while (nSendBytes >= 32) {
        for (i = 0; i < 32; i++){
            DrvSpiSendWord(baseAddr, 0xFF);
        }
        DrvSpiWaitLastBit(baseAddr);                                                        /* Wait until the last bit is sent */
        while (GET_REG_WORD_VAL(baseAddr + EVENT_REG) & NE) {                          /* While fifo not empty */
            SET_REG_WORD(data, (DrvSpiRecvWord(baseAddr)) );
            data++;
        }
        nSendBytes-=32;
    }
    if (nSendBytes > 0) {
        for (i = 0; i < nSendBytes; i++)
            DrvSpiSendWord(baseAddr, 0xFF);

        DrvSpiWaitLastBit(baseAddr);                                                        /* Wait until the last bit is sent */

        while (GET_REG_WORD_VAL(baseAddr + EVENT_REG) & NE) {                          /* While fifo not empty */
            SET_REG_WORD(data, (DrvSpiRecvWord(baseAddr)) );
            data++;
        }
    }

    DrvSpiSs(baseAddr, SLV_DES_ALL);                                                 /* Disable EEPROM /CS */
}

/***************************************************************************
SPI - SRAM functioning mode
@{
 ----------------------------------------------------------------------------
@}
@param
    baseAddr
        - SPI base address
@param
    mode
        - MBYTE, MPAGE or MSEQ
@return
@{
@}
 ************************************************************************* */
void drvSpiSramMode(u32 baseAddr, u32 mode)
{
    DrvSpiSs (baseAddr, SLV_SEL_1);                                                  /* Enable EEPROM /CS */
    DrvSpiSendWord(baseAddr, WRSR);                                                 /* Send Write Status Command */
    DrvSpiSendWord(baseAddr, mode | HOLD_DIS);                                      /* Mode and disable Hold pin functionality */
    DrvSpiWaitLastBit(baseAddr);                                                            /* Wait 'till last bit is transmited */
    DrvSpiSs(baseAddr, SLV_DES_ALL);                                                 /* Disable EEPROM /CS */
}
/**************************************************************************
SPI SRAM Write
@{
 ----------------------------------------------------------------------------
@}
@param
    addr
        - Start address of the transaction
@param
    mode
        - BYTE, PAGE or SEQUENTIAL mode
@param
    data
        - Start address of buffer wanted to be read
@param
    size
        - Number of bytes wnated to be transfered
@return
@{
@}
 ************************************************************************* */
void DrvSpiSramWrite(u32 baseAddr, u32 addr, u32 mode, u32 *data, u32 size)
{
    u32 i;
    drvSpiSramMode(baseAddr, mode);

    DrvSpiSs (baseAddr, SLV_SEL_1);                                                  /* Enable EEPROM /CS */
    DrvSpiSendWord (baseAddr, WRITE);                                               /* Send Write Command */
    DrvSpiWaitLastBit(baseAddr);                                                            /* Wait until the last byte is sent */
    DrvSpiSendWord (baseAddr, ((addr >> 8) & 0xFF) );                               /* Send High Byte Address */
    DrvSpiSendWord (baseAddr, (addr & 0xFF) );                                      /* Send Low Byte Address */

    if (mode != MBYTE){
        for (i = 0 ; i < size ; ++i){
            DrvSpiSendWord(baseAddr, *data);
            data++;
        }
    }
    else
        DrvSpiSendWord(baseAddr, *data);

    DrvSpiWaitLastBit(baseAddr);                                                            /* Wait until the last byte is sent */
    DrvSpiSs(baseAddr, SLV_DES_ALL);                                                 /* Disable EEPROM /CS */
}

/* ***********************************************************************//**
   *************************************************************************
SPI SRAM read
@{
 ----------------------------------------------------------------------------
@}
@param
    addr
        - Start address of the transaction
@param
    mode
        - BYTE, PAGE or SEQUENTIAL mode
@param
    data
        - Start address of buffer wanted to be read
@param
    size
        - Number of bytes wnated to be transfered
@return
@{
@}
 ************************************************************************* */
void DrvSpiSramRead(u32 baseAddr, u32 addr, u32 mode, u32 *data, u32 size)
{
    u32 nSendBytes;
    u32 i;

    DrvSpiSs (baseAddr, SLV_SEL_1);                                                  /* Enable EEPROM /CS */
    DrvSpiSendWord (baseAddr, READ);                                                /* Send READ Command */
    DrvSpiWaitLastBit(baseAddr);                                                            /* Wait until the last byte is sent */
    DrvSpiSendWord (baseAddr, ((addr >> 8) & 0xFF) );                               /* Send High Byte Address */
    DrvSpiSendWord (baseAddr, (addr & 0xFF) );                                      /* Send Low Byte Address */

    DrvSpiWaitLastBit(baseAddr);                                                            /* Wait until the last bit is sent */
    DrvSpiFlushFifo(baseAddr);

    nSendBytes = size;

    if (mode != MBYTE){
        while (nSendBytes >= 32) {                                                      /* If we still have bytes to send */
            for (i = 0; i < 32; i++){
                DrvSpiSendWord(baseAddr, 0xFF);
            }
            DrvSpiWaitLastBit(baseAddr);                                                    /* Wait until the last bit is sent */
            while (GET_REG_WORD_VAL(baseAddr + EVENT_REG) & NE) {                      /* While fifo not empty */
                SET_REG_WORD(data, (DrvSpiRecvWord(baseAddr)) );
                data++;
            }
            nSendBytes-=32;
        }
        if (nSendBytes > 0) {                                                           /* If we still have bytes to send */
            for (i = 0; i < nSendBytes; i++)
                DrvSpiSendWord(baseAddr, 0xFF);

            DrvSpiWaitLastBit(baseAddr);                                                    /* Wait until the last bit is sent */

            while (GET_REG_WORD_VAL(baseAddr + EVENT_REG) & NE) {                      /* While fifo not empty */
                SET_REG_WORD(data, (DrvSpiRecvWord(baseAddr)) );
                data++;
            }
        }
    }
    else {
        DrvSpiSendWord(baseAddr, 0xFF);
        SET_REG_WORD(data, (DrvSpiRecvWord(baseAddr)) );
    }

    DrvSpiSs(baseAddr, SLV_DES_ALL);                                                 /* Disable EEPROM /CS */
}

/* ************************************************************************
SPI assemble bytes in a word
@{
----------------------------------------------------------------------------
@{
@param
     spiBase - base address of the spi module
@param
     size - number of bytes to be assembled
@return
@{
info:
@}
************************************************************************* */

static u32 drvSpiWordAsm (u32 spiBase, u32 size)
{
    u32 i;
    int data = 0;
    for(i = 0; i < size; i++) {
        while(!(GET_REG_WORD_VAL(spiBase + EVENT_REG) & NE))
            ;
        data |= ((DrvSpiRecvWord(spiBase) & 0xFF) << (8*i));
    }
    return data;
}

/* ************************************************************************
SPI disassemble a word in bytes
@{
----------------------------------------------------------------------------
@{
@param
     spiBase - base address of the spi module
@param
     size - number of bytes to be disassembled
@return
@{
info:
@}
**************************************************************************/

static void drvSpiWordDisasm (u32 spiBase, u32 data, u32 size)
{
    u32 i;
    for(i = 0; i < size; i++) {
        while(!(GET_REG_WORD_VAL(spiBase + EVENT_REG) & NF))
            ;
        DrvSpiSendWord(spiBase, ((data >> (8*i)) & 0xFF));
    }
}

/* **************************************************************************
SPI Transmit to master SPI from memory
@{
----------------------------------------------------------------------------
@{
@param
     spiBase - base address of the spi module
@param
     srcAddr - Start address from where the master want to read
@param
     size - nr of bytes the master wants to transfer
@return
@{
info:
@}
************************************************************************* */

static void drvSpiSlaveTx(u32 spiBase, u32 srcAddr, u32 size)
{
    u32 data;
    u32 intSize;
    u32 i;

/* MAIN GOAL OF THE FUNCTION :
        Read data from memory, deassemble it and
        write it in the SPI SLAVE's TX_FIFO */

    intSize = size/4;                                                                          /* Compute the nr of WD requested by master */
    for(i = 0; i < intSize; i++) {
        GET_REG_WORD(srcAddr, data);                                                           /* Get a WD from memory */
        drvSpiWordDisasm(spiBase, data, 4);                                                 /* Dissassemble the WD into B and WR them in the TX FIFO */
        srcAddr += 4;                                                                          /* Increment the read address */
    }

    if((intSize = size % 4)){
        GET_REG_WORD (srcAddr, data);                                                          /* Read the last word that contains the needed bytes */
        drvSpiWordDisasm(spiBase, data, intSize);                                          /* Dissassemble the WD into B and WR them in the TX FIFO */
    }
}

/* **************************************************************************
SPI Receive from master SPI and write to memory
@{
----------------------------------------------------------------------------
@{
@param
     spiBase - base address of the spi module
@param
     destAddr - Start address from where the master wants to write
@param
     size - nr of bytes the master wants to transfer
@return
@{
info:
@}
************************************************************************* */

static void drvSpiSlaveRx(u32 spiBase, u32 destAddr, u32 size)
{
    u32 data;
    u32 intSize;
    u32 i;

    intSize = size/4;                                                                              /* Compute the number of words to be transferred to the master */

    for(i = 0; i < intSize; i++){
        data = drvSpiWordAsm(spiBase, 4);
        SET_REG_WORD(destAddr, data);                                                              /* Write the word in the memory */
        destAddr += 4;                                                                             /* Increment address */
    }
    intSize = size % 4;                                                                            /* Compute the number of bytes remanined to be transferred */
    if (intSize) {                                                                                 /* More bytes that can not form a word? */
        data = drvSpiWordAsm(spiBase, intSize);
        SET_REG_WORD(destAddr, data);                                                              /* Write the word in the memory */
    }
    /* Data was transfered successfuly to the slave */
}

/***************************************************************************
SPI SRAM read
@{
 ----------------------------------------------------------------------------
@}
@param
    addr
        - Start address of the transaction
@param
    mode
        - BYTE, PAGE or SEQUENTIAL mode
@param
    data
        - Start address of buffer wanted to be read
@param
    size
        - Number of bytes wnated to be transfered
@return
@{
@}
 ************************************************************************* */

void DrvSpiSlave(u32 spiBase)
{
    u32 addr, size, cmd;

/*  MAIN GOAL OF THE FUNCTION :
        1. Gets the first 6 bytes written by the master
        and assembles them into address and lenght.
        2. Waits to see what transfer will be required
        from the master and calls the for the apprpiate
        function
*/
    while(!(GET_REG_WORD_VAL(spiBase + EVENT_REG) & NE));                                      /* Wait for the WRITE cmd byte to arrive */
    if (DrvSpiRecvWord(spiBase) != WRITE) {
        printf("ERROR: The first command should be a WRITE");
        printf("Exiting test");
        exit(1);
    }
    addr = drvSpiWordAsm(spiBase, 4);                                                       /* The first four bytes will represent the destination address */
    size = drvSpiWordAsm(spiBase, 2);                                                       /* The next 2 bytes represents the size of the transfer */

    // printf("Wait for the command byte to arrive\n");
    while(!(GET_REG_WORD_VAL(spiBase + EVENT_REG) & NE));                                      /* Wait for the command byte to arrive */
    cmd = DrvSpiRecvWord(spiBase);

    if(cmd == WRITE){
        drvSpiSlaveRx(spiBase, addr, size);
    }
    else if (cmd == READ){
        drvSpiSlaveTx(spiBase, addr, size);
    }
    else {
        exit(2);
    }
}
