///   
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved. 
///            For License Warranty see: common/license.txt   
///
/// @brief     Low-level driver for SPI interface
/// 
/// 
/// 
#ifndef _BRINGUP_SABRE_SPI_H_
#define _BRINGUP_SABRE_SPI_H_

/*======================*/
/*  Include Headers     */
/*======================*/
#include "mv_types.h"
#include <DrvSpiDefines.h>

/* ---------------------------------------------------------------------------------------------
 SPI_BASE_ADR_local C { SPI1_BASE_ADR,  SPI2_BASE_ADR,  SPI3_BASE_ADR }
 --------------------------------------------------------------------------------------------- */ 
 
 /* ***********************************************************************//**
   *************************************************************************
SPI GET SPI Base

 

@param
    index   - takes values from 0 to 2
@return
      - returns the base address of an SPI

Info:

 ************************************************************************* */
int DrvSpiMssiBase(u32 index);

/* ***********************************************************************//**
   *************************************************************************
SPI enable - disable



@param
      ena
            - if set to 1 the module will be enabled
            - if set to 0 the module will be disabled
            .
@param
      SPI_BASE_ADR_local
                   - base address of the SPI
                            SPI1_BASE_ADR
                            SPI2_BASE_ADR
                            SPI3_BASE_ADR
                   .
@return

info:
     - The module should be enabled after it was configured
     .

************************************************************************* */
void DrvSpiEn(u32 SPI_BASE_ADR_local, u32 ena);

/* ***********************************************************************//**
   *************************************************************************
SPI - wait until last bit is transmitted

 

@param
    baseAddr
        - SPI Base Address
@return

Info:

 ************************************************************************* */
void DrvSpiWaitLastBit(u32 baseAddr);

/* ***********************************************************************//**
   *************************************************************************
SPI mode configure



@param
      SPI_BASE_ADR_local
                   - base address of the SPI
                            SPI1_BASE_ADR
                            SPI2_BASE_ADR
                            SPI3_BASE_ADR
                   .
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

info:
     - This function should be called first in the init chain.
     .

************************************************************************* */
void DrvSpiInit(u32 SPI_BASE_ADR_local,u32 mode, u32 wSize, u32 rev, u32 tWen);

/* **********************************************************************//**
****************************************************************************
SPI timing configure


@param
      SPI_BASE_ADR_local
                   - base address of the SPI
                            SPI1_BASE_ADR
                            SPI2_BASE_ADR
                            SPI3_BASE_ADR
                   .
@param
       cpol    - clock polarity in idle mode
@param
       cpha    - clock phase
@param
       cg      - clock cycles between words 0000b-1111b
@param
       sysClk   - Bus Clk frequency in KHz
@param
       spiClk   - SPI clk desired frequency in KHz
@return

info:
       - This function MUST BE CALLED ONLY AFTER drvSpixInit !
       .

************************************************************************* */
void DrvSpiTimingCfg(u32 SPI_BASE_ADR_local,u32 cpol, u32 cpha, u32 cg, u32 sysClk, u32 spiClk);

 /* ***********************************************************************//**
    *************************************************************************
 SPI status
 
 
 
 @return
        - contents of the status register
        - bit:
            - 8  Not Full
            - 9  not empty
            - 10 multy masster error
            - 11 underrun
            - 14 last character
            .
        .
 
 info:
 
 ************************************************************************* */
u32 DrvSpiStatus(u32 SPI_BASE_ADR_local);

/* ***********************************************************************//**
   *************************************************************************
SPI mask register


@param
      SPI_BASE_ADR_local
                   - base address of the SPI
                            SPI1_BASE_ADR
                            SPI2_BASE_ADR
                            SPI3_BASE_ADR
                   .
@param  mask -  Seting a bit , will enable interrupt generation
         -  bit:
            - 8  Not Full enable
            - 9  not empty enable
            - 10 multy masster error enable
            - 11 underrun enable
            - 14 last character  enable
            .
         .
@return

info:

************************************************************************* */
void DrvSpiInMask(u32 SPI_BASE_ADR_local,u32 mask);

/* ***********************************************************************//**
   *************************************************************************
SPI command register



@return

info:
      - setting this bit enables possible activation of LT bit in the event register
      - self clearing
      .

************************************************************************* */
void DrvSpiSetLastBit(u32 SPI_BASE_ADR_local);

/* ***********************************************************************//**
   *************************************************************************
SPI assert/deasert


@param
      SPI_BASE_ADR_local
                   - base address of the SPI
                            SPI1_BASE_ADR
                            SPI2_BASE_ADR
                            SPI3_BASE_ADR
                   .
@param
   ss - the slave number + 1,
         0 deaserts the ss signal
@return

info:

************************************************************************* */
void DrvSpiSs(u32 SPI_BASE_ADR_local,u32 ss);

/* ***********************************************************************//**
   *************************************************************************
SPI send one word


@param
      SPI_BASE_ADR_local
                   - base address of the SPI
                            SPI1_BASE_ADR
                            SPI2_BASE_ADR
                            SPI3_BASE_ADR
                   .
@param
   data - word, of length set previously, to be sent
@return

info:
   it also adjusts the position of the MSB if the rev bit is set in the mode register,
   providing that the mode register was programmed correctly

************************************************************************* */
void DrvSpiSendWord(u32 SPI_BASE_ADR_local,u32 data);

/* ***********************************************************************//**
   *************************************************************************
SPI send buffer


@param
      SPI_BASE_ADR_local
                   - base address of the SPI
                            SPI1_BASE_ADR
                            SPI2_BASE_ADR
                            SPI3_BASE_ADR
                   .
@param
   *data - pointer to a buffer of words to send
@param
   size  - size of the transfer
@return

info:
  - it also adjusts the position of the MSB if the rev bit is set in the mode register,
   providing that the mode register was programmed correctly

************************************************************************* */
void DrvSpiSendBuffer(u32 SPI_BASE_ADR_local,u32 *data, u32 size);

/* ***********************************************************************//**
   *************************************************************************
SPI flush receive FIFO



@return

info:
      - all received data is lost
      - call this function only if the device is not receiving anymore, otherwise
      it might loop until the transmission is over (possible in slave mode)
      .

 ************************************************************************* */
void DrvSpiFlushFifo(u32 SPI_BASE_ADR_local);

/* ***********************************************************************//**
   *************************************************************************
SPI receive word as master, send a word in the same time

 

@return
     data read

Info:
   - it also adjusts the position of the MSB if the rev bit is set in the mode register,
   providing that the mode register was programmed correctly
   - master has to provide the clock, so it has to send data to receive
   - flush the fifo at the beginning if necessary, the function assumes that the FIFOs are empty
   .

 ************************************************************************* */
u32 DrvSpiWord(u32 SPI_BASE_ADR_local,u32 data);

/* ***********************************************************************//**
   *************************************************************************
SPI send and receive a buffer in the same time

 
@param
      SPI_BASE_ADR_local
                   - base address of the SPI
                            SPI1_BASE_ADR
                            SPI2_BASE_ADR
                            SPI3_BASE_ADR
                   .
@param
    *txd    - pointer to a buffer to be transmitted
@param
    *rxd    - pointer to a zone where received data is stored
@param
    size    - transfer size
@return

Info:
      - assumes the FIFO's are empty/(old contents is not needed)
      .

 ************************************************************************* */
void DrvSpiRxTx(u32 SPI_BASE_ADR_local,u32 *txd, u32 *rxd, u32 size);

/* ***********************************************************************//**
   *************************************************************************
Test SPI in LOOP mode, functionality test at low speed


@param
      SPI_BASE_ADR_local
                   - base address of the SPI
                            SPI1_BASE_ADR
                            SPI2_BASE_ADR
                            SPI3_BASE_ADR
                   .
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

info:

 ************************************************************************* */
int DrvSpiTestLoop(u32 SPI_BASE_ADR_local,u32 *zone1, u32 *zone2, u32 size);

/* ***********************************************************************//**
   *************************************************************************
SPI loop mode

 

@param
      SPI_BASE_ADR_local
                   - base address of the SPI
                            SPI1_BASE_ADR
                            SPI2_BASE_ADR
                            SPI3_BASE_ADR
@param
    loop    - 1 enables loop mode, 0 disables loop mode
@return

Info:

 ************************************************************************* */
void DrvSpiLoop(u32 SPI_BASE_ADR_local, u32 loop);

/* ***********************************************************************//**
   *************************************************************************
SPI Configure GPIO

@return void

 ************************************************************************* */
void DrvSpiConfigureGpio(void);

/* ***********************************************************************//**
   *************************************************************************
SPI Wait for TX

@param
    base
@param
    initDelay
        - wait some before checking the data was transmitted
@return

Info:

 ************************************************************************* */
void DrvSpiWaitForTX(u32 base, u32 initDelay);

/* ***********************************************************************//**
   *************************************************************************
SPI Retrieve Data

 

@param
    bitW - Word length
@param
    rev - Reverse data
@param
    udata - data
@return
      - returns the data changed according to the bitW and rev

Info:

 ************************************************************************* */
u32 DrvSpiRetriveData (u32 bitW, u32 rev, u32 udata);

/* ***********************************************************************//**
   *************************************************************************
SPI EEPROM Write

 
@param
      SPI_BASE_ADR_local
                   - base address of the SPI
                            SPI1_BASE_ADR
                            SPI2_BASE_ADR
                            SPI3_BASE_ADR
                   .
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
        - Number of bytes wanted to be transferred
@param
    pageSize
        - Page size of the EEPROM
@return


 ************************************************************************* */
void DrvSpiEepromWrite(u32 SPI_BASE_ADR_local, u32 addr, u32 addrSize, u32 *data, u32 size, u32 pageSize);

/***********************************************************************//**
   *************************************************************************
SPI EEPROM Read

 
@param
      SPI_BASE_ADR_local
                   - base address of the SPI
                            SPI1_BASE_ADR
                            SPI2_BASE_ADR
                            SPI3_BASE_ADR
                   .
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


 ************************************************************************* */
void DrvSpiEepromRead(u32 SPI_BASE_ADR_local, u32 addr, u32 addrSize, u32 *data, u32 size);

/* ***********************************************************************//**
   *************************************************************************
SPI SRAM read

 
@param
      SPI_BASE_ADR_local
                   - base address of the SPI
                            SPI1_BASE_ADR
                            SPI2_BASE_ADR
                            SPI3_BASE_ADR
                   .
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
        - Number of bytes wanted to be transferred
@return


 ************************************************************************* */
void DrvSpiSramRead(u32 SPI_BASE_ADR_local, u32 addr, u32 mode, u32 *data, u32 size);

/************************************************************************//**
SPI SRAM Write

 

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
        - Number of bytes wanted to be transferred
@return


 ************************************************************************* */
void DrvSpiSramWrite(u32 SPI_BASE_ADR_local, u32 addr, u32 mode, u32 *data, u32 size);

/*************************************************************************//**
SPI SRAM read

 

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
        - Number of bytes wanted to be transferred
@return


 ************************************************************************* */
void DrvSpiSlave(u32 spiBase);

/* ***********************************************************************//**
   *************************************************************************
SPI read one word



@param
      SPI_BASE_ADR_local
                   - base address of the SPI
                            SPI1_BASE_ADR
                            SPI2_BASE_ADR
                            SPI3_BASE_ADR
                   .
info:
   it also adjusts the position of the MSB if the rev bit is set in the mode register,
   providing that the mode register was programmed correctly

************************************************************************* */
u32 DrvSpiRecvWord(u32 SPI_BASE_ADR_local);
#endif // _BRINGUP_SABRE_SPI_H_
