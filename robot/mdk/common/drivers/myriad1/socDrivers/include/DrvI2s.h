///  
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved 
///            For License Warranty see: common/license.txt   
///
/// @brief     Low-level driver for I2S interface
/// 
/// 
/// 

#ifndef _BRINGUP_SABRE_I2S_H_
#define _BRINGUP_SABRE_I2S_H_

#include "mv_types.h"
#include "DrvI2sDefines.h"

/* ***********************************************************************//**
   *************************************************************************
I2S structure for interrupt handling

@param
   addrX - address where the data is to be written or read from
@param
   addrX_shadow - address which will be reloaded in the prevoius address counter
@param
   sizeX - size of zone X,
@param
   chX_type - tx(1) or rx(0)
@param
   chX_int_mask - look up defines in the DrvI2sDefines.h for interrupt mask
@param
   baseAddress - base address of the i2s module
info:
   X coresponds to the channel number of the I2S module
 ************************************************************************* */
typedef struct drvStructI2sHand
{
   volatile u32 addr1;          // addresses used in the interrupt rutine;
   volatile u32 addr2;
   volatile u32 addr3;
   volatile u32 addr11;         // added in the case  the chanel uses both rx and tx, these are for tx
   volatile u32 addr21;      

   volatile u32 addr1Shadow;   // address to be reloaded when the address counter reaches the end of the zone
   volatile u32 addr2Shadow;
   volatile u32 addr3Shadow;
   volatile u32 addr1Shadow1;  // added in the case  the chanel uses both rx and tx, this are for tx
   volatile u32 addr2Shadow1;

   u32 size1;                   // size of the zone in memory
   u32 size2;
   u32 size3;
   u32 size11;                  // size of the zone in memory
   u32 size21;

   
   u32 baseAddress; // address of the i2s module
} DrvI2sStructH;


extern DrvI2sStructH *sI2sHandler;
/* ***********************************************************************//**
   *************************************************************************
I2S pointer to structures that would configure the i2s module during interrupts



@param
    sI2sHandlerRxTx - pointer to a DrvI2sStructH structure used in interrupt handling of I2S module

info:
    - has to be initilized before using
    - fields have to be initialized with a call to : DrvI2sHandlerInitRxTx(...)
    .

 ************************************************************************* */
extern DrvI2sStructH *sI2sHandlerRxTx;

/* ***********************************************************************//**
   *************************************************************************
I2S - configure channel 0 of a i2s module



@param
      base  - base address of the i2s module, one of the following could be used: I2S1_BASE_ADR, I2S2_BASE_ADR, I2S3_BASE_ADR
@param
      txNotRx - configure channel as rx or tx
@param
      master  - 1 module is master
              - 0 module is slave
              .
@param
      clkCfg - word to be written in the I2S_TFCR or I2S_RFCR registers
              - defines in the DrvI2sDefines.h can be used for this
              - this sets the number of cc per word and the clock gating
              .
@param
      fifLvl - fifo level at which the interrupt is triggered
@param
      lenCfg - word length
              - defines in the DrvI2sDefines.h can be used for this
              .
@param
      intMask - interrupt masks

info:


 ************************************************************************* */
void DrvI2sInitCh0(u32 base, u32 txNotRx,
                                         u32 master,
                                         u32 clkCfg,
                                         u32 fifLvl,
                                         u32 lenCfg,
                                         u32 intMask);

/* ***********************************************************************//**
   *************************************************************************
I2S - configure channel 0 of a i2s module



@param
      base  - base address of the i2s module, one of the following could be used: I2S1_BASE_ADR, I2S2_BASE_ADR, I2S3_BASE_ADR
@param
      master  - 1 module is master
              - 0 module is slave
              .
@param
      clkCfg - word to be written in the I2S_TFCR or I2S_RFCR registers
              - defines in the DrvI2sDefines.h can be used for this
              - this sets the number of cc per word and the clock gating
              .
@param
      fifLvl - fifo level at which the interrupt is triggered
@param
      lenCfg - word length
              - defines in the DrvI2sDefines.h can be used for this
              .
@param
      intMask - interrupt masks

info:


 ************************************************************************* */
void DrvI2sInitCh0RxTx(u32 base,     u32 master,
                                         u32 clkCfg,
                                         u32 fifLvl,
                                         u32 lenCfg,
                                         u32 intMask);
                                         
/* ***********************************************************************//**
   *************************************************************************
I2S - configure channel 1 of a i2s module



@param
      base  - base address of the i2s module, one of the following could be used: I2S1_BASE_ADR, I2S2_BASE_ADR, I2S3_BASE_ADR
@param
      txNotRx - configure channel as rx or tx
@param
      master  - 1 module is master
              - 0 module is slave
              .
@param
      clkCfg - word to be written in the I2S_TFCR or I2S_RFCR registers
              - defines in the DrvI2sDefines.h can be used for this
              - this sets the number of cc per word and the clock gating
              .
@param
      fifLvl - fifo level at which the interrupt is triggered
@param
      lenCfg - word length
              - defines in the DrvI2sDefines.h can be used for this
              .
@param
      intMask - interrupt masks

info:


 ************************************************************************* */
void DrvI2sInitCh1(u32 base, u32 txNotRx,
                                         u32 master,
                                         u32 clkCfg,
                                         u32 fifLvl,
                                         u32 lenCfg,
                                         u32 intMask);



//is the function below commented correctly?
/* ***********************************************************************//**
   *************************************************************************
I2S - configure channel 1 of a i2s module



@param
      base  - base address of the i2s module, one of the following could be used: I2S1_BASE_ADR, I2S2_BASE_ADR, I2S3_BASE_ADR

@param
      master  - 1 module is master
              - 0 module is slave
              .
@param
      clkCfg - word to be written in the I2S_TFCR or I2S_RFCR registers
              - defines in the DrvI2sDefines.h can be used for this
              - this sets the number of cc per word and the clock gating
              .
@param
      fifLvl - fifo level at which the interrupt is triggered
@param
      lenCfg - word length
              - defines in the DrvI2sDefines.h can be used for this
              .
@param
      intMask - interrupt masks

info:


 ************************************************************************* */
void DrvI2sInitCh1RxTx(u32 base,     u32 master,
                                         u32 clkCfg,
                                         u32 fifLvl,
                                         u32 lenCfg,
                                         u32 intMask);
                                         

/* ***********************************************************************//**
   *************************************************************************
I2S - configure channel 2 of a i2s module



@param
      base  - base address of the i2s module, one of the following could be used: I2S1_BASE_ADR, I2S2_BASE_ADR, I2S3_BASE_ADR
@param
      master  - 1 module is master
              - 0 module is slave
              .
@param
      clkCfg - word to be written in the I2S_TFCR or I2S_RFCR registers
              - defines in the DrvI2sDefines.h can be used for this
              - this sets the number of cc per word and the clock gating
              .
@param
      fifLvl - fifo level at which the interrupt is triggered
@param
      lenCfg - word length
              - defines in the DrvI2sDefines.h can be used for this
              .
@param
      intMask - interrupt masks

info:
    this channel works only as tx

 ************************************************************************* */
void DrvI2sInitCh2(u32 base, u32 master,
                                         u32 clkCfg,
                                         u32 fifLvl,
                                         u32 lenCfg,
                                         u32 intMask);

/* ***********************************************************************//**
   *************************************************************************
I2S config example


 ************************************************************************* */
void DrvI2sConfigExample();

/* ***********************************************************************//**
   *************************************************************************
I2S interrupt handler  structure pointer init



@param
         *i2sModulePntr - pointer to a DrvI2sStructH structure which will be initialized
@param
         address1 - address of the memory zone used for channel 0 of the module
@param
         address2 - address of the memory zone used for channel 1 of the module
@param
         address3 - address of the memory zone used for channel 2 of the module
@param
         address1sh - address of the memory zone used for channel 0 rx of the module
@param
         address2sh - address of the memory zone used for channel 1 rx of the module
@param
         address3sh - address of the memory zone used for channel 2 tx of the module
@param
         size1 - size of the first zone
@param
         size2 - size of the second zone
@param
         size3 - size of the third zone
@param
         moduleBase - base address of the i2s module

@return
        pointer to the variable initilaized

info:  use this function to init the coresponding pointer for the coresponding i2s module

 ************************************************************************* */
DrvI2sStructH * DrvI2sHandlerInit(DrvI2sStructH *i2sModulePntr,
                          u32 address1,u32 address2,u32 address3,
                          u32 address1sh,u32 address2sh,u32 address3sh,
                          u32 size1, u32 size2, u32 size3,
                          u32 moduleBase);
                          
                          
                          // addresses for                           
                          //ch0             ch1             ch0             ch1              ch2   
                          //rx              rx              tx              tx               tx   

/* ***********************************************************************//**
   *************************************************************************
I2S interrupt handler  structure pointer init



@param
         *i2sModulePntr - pointer to a DrvI2sStructH structure which will be initialized
@param
         address1 - address of the memory zone used for channel 0 of the module
@param
         address2 - address of the memory zone used for channel 1 of the module
@param
         address11 - address of the memory zone used for channel 0 of the module
@param
         address21 - address of the memory zone used for channel 1 of the module
@param
         address3 - address of the memory zone used for channel 2 of the module
@param
         address1sh - address of the memory zone used for channel 0 rx of the module
@param
         address2sh - address of the memory zone used for channel 1 rx of the module
@param
         address1sh1 - address of the memory zone used for channel 0 tx of the module
@param
         address2sh1 - address of the memory zone used for channel 1 tx of the module
@param
         address3sh - address of the memory zone used for channel 2 tx of the module
@param
         size1 - size of memory referenced by address1
@param
         size2 - size of memory referenced by address2
@param
         size11 - size of memory referenced by address11
@param
         size21 - size of memory referenced by address21
@param
         size3 - size of memory referenced by address3
@param
         moduleBase - base address of the i2s module

@return
        pointer to the variable initilaized

info:  use this function to init the coresponding pointer for the coresponding i2s module

 ************************************************************************* */
DrvI2sStructH * DrvI2sHandlerInitRxTx(DrvI2sStructH *i2sModulePntr,
                          u32 address1  , u32 address2  , u32 address11  , u32 address21  , u32 address3,
                          u32 address1sh, u32 address2sh, u32 address1sh1, u32 address2sh1, u32 address3sh,
                          u32 size1     , u32 size2     , u32 size11     , u32 size21     , u32 size3,
                          u32 moduleBase);

//!@{
/* ***********************************************************************//**
   *************************************************************************
I2S interrupt handler



@param
     unusedParam - parameter is not used, but given to comply with the handler function pointer

info:

 ************************************************************************* */
void DrvI2sHandlerTwo(u32 unusedParam);
void DrvI2sHandler(u32 unusedParam);
//!@}

/* ***********************************************************************//**
   *************************************************************************
I2S interrupt handler



@param
     unusedParam - parameter is not used, but given to comply with the handler function pointer

info:

 ************************************************************************* */

void DrvI2sHandlerRxTx(u32 unusedParam); // used when both rx and tx are used from a channel
#endif // _BRINGUP_SABRE_I2S_H_

