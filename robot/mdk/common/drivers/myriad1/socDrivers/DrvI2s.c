#include <mv_types.h>
#include <isaac_registers.h>
#include "DrvI2s.h"

/* ***********************************************************************//**
   *************************************************************************
I2S - configure channel 0 of a i2s module
@{
----------------------------------------------------------------------------
@}
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
      fifoLvl - fifo level at which the interrupt is triggered
@param
      lenCfg - word length
              - defines in the DrvI2sDefines.h can be used for this
              .
@param
      intMask - interrupt masks
@return
@{
info:

@}
 ************************************************************************* */
void DrvI2sInitCh0(u32 base, u32 txNotRx,
                                         u32 master,
                                         u32 clkCfg,
                                         u32 fifoLvl,
                                         u32 lenCfg,
                                         u32 intMask)
{
      u32 i2sModule;
      i2sModule = ((base - LPB1_CONTROL_ADR) >> 16) & 0x0F;
      if (master)
         switch (i2sModule)
         {
           case 0x0c: SET_REG_WORD(CPR_GEN_CTRL_ADR, GET_REG_WORD_VAL(CPR_GEN_CTRL_ADR) | 0x01); break;
           case 0x0d: SET_REG_WORD(CPR_GEN_CTRL_ADR, GET_REG_WORD_VAL(CPR_GEN_CTRL_ADR) | 0x02); break;
           case 0x0e: SET_REG_WORD(CPR_GEN_CTRL_ADR, GET_REG_WORD_VAL(CPR_GEN_CTRL_ADR) | 0x04); break;
           default:
            break;
         }


      SET_REG_WORD(base + I2S_IER   , 1);        // enable I2S module
      SET_REG_WORD(base + I2S_CER   , 0);        // disable clock generation
      SET_REG_WORD(base + I2S_CCR   , clkCfg);  //clock configuration

      SET_REG_WORD(base + I2S_ITER  , 0);  // disable tx block
      SET_REG_WORD(base + I2S_TER0  , 0);  // transmit disable

      SET_REG_WORD(base + I2S_IRER  , 0);  // disable rx block
      SET_REG_WORD(base + I2S_RER0  , 0);  // receiver disable

      SET_REG_WORD(base + I2S_IMR0  , intMask); // mask interrupts

      if (txNotRx)  // set channel for tx
      {
           SET_REG_WORD(base + I2S_TFCR0 , fifoLvl); // transmit fifo lvl
           SET_REG_WORD(base + I2S_TCR0  , lenCfg);  // data length
           SET_REG_WORD(base + I2S_TXFFR , 1);        // flush the tx fifo
           SET_REG_WORD(base + I2S_ITER  , 1);        // enable tx block
           SET_REG_WORD(base + I2S_IRER  , 1);         // enable rx block
           SET_REG_WORD(base + I2S_TER0  , 1);        // enable transmition
      }
      else            // set channel for rx
      {
           SET_REG_WORD(base + I2S_RFCR0 , fifoLvl);  // fifo length triger (triggered on value +1)
           SET_REG_WORD(base + I2S_RCR0  , lenCfg);   // data length
           SET_REG_WORD(base + I2S_RXFFR , 1);         // flush the tx fifo
           SET_REG_WORD(base + I2S_IRER  , 1);         // enable rx block
           SET_REG_WORD(base + I2S_ITER  , 1);        // enable tx block
           SET_REG_WORD(base + I2S_RER0  , 1);         // enable receiver
      }

      if (master)
         SET_REG_WORD(base + I2S_CER   , 1);              // enable clock generation
}

void DrvI2sInitCh0RxTx(u32 base,     u32 master,
                                         u32 clkCfg,
                                         u32 fifoLvl,
                                         u32 lenCfg,
                                         u32 intMask)
{
      u32 i2sModule;
      i2sModule = ((base - LPB1_CONTROL_ADR) >> 16) & 0x0F;
      if (master)
         switch (i2sModule)
         {
           case 0x0c: SET_REG_WORD(CPR_GEN_CTRL_ADR, GET_REG_WORD_VAL(CPR_GEN_CTRL_ADR) | 0x01); break;
           case 0x0d: SET_REG_WORD(CPR_GEN_CTRL_ADR, GET_REG_WORD_VAL(CPR_GEN_CTRL_ADR) | 0x02); break;
           case 0x0e: SET_REG_WORD(CPR_GEN_CTRL_ADR, GET_REG_WORD_VAL(CPR_GEN_CTRL_ADR) | 0x04); break;
           default:
            break;
         }


      SET_REG_WORD(base + I2S_IER   , 1);        // enable I2S module
      SET_REG_WORD(base + I2S_CER   , 0);        // disable clock generation
      SET_REG_WORD(base + I2S_CCR   , clkCfg);  //clock configuration

      SET_REG_WORD(base + I2S_ITER  , 0);  // disable tx block
      SET_REG_WORD(base + I2S_TER0  , 0);  // transmit disable

      SET_REG_WORD(base + I2S_IRER  , 0);  // disable rx block
      SET_REG_WORD(base + I2S_RER0  , 0);  // receiver disable

      SET_REG_WORD(base + I2S_IMR0  , intMask); // mask interrupts

      // set channel for tx
           SET_REG_WORD(base + I2S_TFCR0 , fifoLvl); // transmit fifo lvl
           SET_REG_WORD(base + I2S_TCR0  , lenCfg);  // data length
           SET_REG_WORD(base + I2S_TXFFR , 1);        // flush the tx fifo

      // set channel for rx
           SET_REG_WORD(base + I2S_RFCR0 , fifoLvl);  // fifo length triger (triggered on value +1)
           SET_REG_WORD(base + I2S_RCR0  , lenCfg);   // data length
           SET_REG_WORD(base + I2S_RXFFR , 1);         // flush the tx fifo

      // enable
           SET_REG_WORD(base + I2S_IRER  , 1);         // enable rx block
           SET_REG_WORD(base + I2S_ITER  , 1);         // enable tx block
           SET_REG_WORD(base + I2S_TER0  , 1);         // enable transmition
           SET_REG_WORD(base + I2S_RER0  , 1);         // enable receiver

      if (master)
         SET_REG_WORD(base + I2S_CER   , 1);              // enable clock generation
}


/* ***********************************************************************//**
   *************************************************************************
I2S - configure channel 1 of a i2s module
@{
----------------------------------------------------------------------------
@}
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
      fifoLvl - fifo level at which the interrupt is triggered
@param
      lenCfg - word length
              - defines in the DrvI2sDefines.h can be used for this
              .
@param
      intMask - interrupt masks
@return
@{
info:

@}
 ************************************************************************* */
void DrvI2sInitCh1(u32 base, u32 txNotRx,
                                         u32 master,
                                         u32 clkCfg,
                                         u32 fifoLvl,
                                         u32 lenCfg,
                                         u32 intMask)
{
      u32 i2sModule;
      i2sModule = ((base - LPB1_CONTROL_ADR) >> 16) & 0x0F;
      if (master)
         switch (i2sModule)
         {
           case 0x0c: SET_REG_WORD(CPR_GEN_CTRL_ADR, GET_REG_WORD_VAL(CPR_GEN_CTRL_ADR) | 0x01); break;
           case 0x0d: SET_REG_WORD(CPR_GEN_CTRL_ADR, GET_REG_WORD_VAL(CPR_GEN_CTRL_ADR) | 0x02); break;
           case 0x0e: SET_REG_WORD(CPR_GEN_CTRL_ADR, GET_REG_WORD_VAL(CPR_GEN_CTRL_ADR) | 0x04); break;
           default:
            break;
         }


      SET_REG_WORD(base + I2S_IER   , 1);        // enable I2S module
      SET_REG_WORD(base + I2S_CER   , 0);        // disable clock generation
      SET_REG_WORD(base + I2S_CCR   , clkCfg);  //clock configuration

      SET_REG_WORD(base + I2S_ITER  , 0);  // disable tx block
      SET_REG_WORD(base + I2S_TER1  , 0);  // transmit disable

      SET_REG_WORD(base + I2S_IRER  , 0);  // disable rx block
      SET_REG_WORD(base + I2S_RER1  , 0);  // receiver disable

      SET_REG_WORD(base + I2S_IMR1  , intMask); // mask interrupts

      if (txNotRx)  // set channel for tx
      {
           SET_REG_WORD(base + I2S_TFCR1 , fifoLvl);  // transmit fifo lvl
           SET_REG_WORD(base + I2S_TCR1  , lenCfg);   // data length
           SET_REG_WORD(base + I2S_TXFFR , 1);         // flush the tx fifo
           SET_REG_WORD(base + I2S_ITER  , 1);         // enable tx block
           SET_REG_WORD(base + I2S_IRER  , 1);         // enable rx block
           SET_REG_WORD(base + I2S_TER1  , 1);         // enable transmition
      }
      else            // set channel for rx
      {
           SET_REG_WORD(base + I2S_RFCR1 , fifoLvl);  // fifo length triger (triggered on value +1)
           SET_REG_WORD(base + I2S_RCR1  , lenCfg);   // data length
           SET_REG_WORD(base + I2S_RXFFR , 1);         // flush the tx fifo
           SET_REG_WORD(base + I2S_IRER  , 1);         // enable rx block
           SET_REG_WORD(base + I2S_ITER  , 1);         // enable tx block
           SET_REG_WORD(base + I2S_RER1  , 1);         // enable receiver
      }

      if (master)
         SET_REG_WORD(base + I2S_CER   , 1);              // enable clock generation
}

void DrvI2sInitCh1RxTx(u32 base,     u32 master,
                                         u32 clkCfg,
                                         u32 fifoLvl,
                                         u32 lenCfg,
                                         u32 intMask)
{
      u32 i2sModule;
      i2sModule = ((base - LPB1_CONTROL_ADR) >> 16) & 0x0F;
      if (master)
         switch (i2sModule)
         {
           case 0x0c: SET_REG_WORD(CPR_GEN_CTRL_ADR, GET_REG_WORD_VAL(CPR_GEN_CTRL_ADR) | 0x01); break;
           case 0x0d: SET_REG_WORD(CPR_GEN_CTRL_ADR, GET_REG_WORD_VAL(CPR_GEN_CTRL_ADR) | 0x02); break;
           case 0x0e: SET_REG_WORD(CPR_GEN_CTRL_ADR, GET_REG_WORD_VAL(CPR_GEN_CTRL_ADR) | 0x04); break;
           default:
            break;
         }


      SET_REG_WORD(base + I2S_IER   , 1);        // enable I2S module
      SET_REG_WORD(base + I2S_CER   , 0);        // disable clock generation
      SET_REG_WORD(base + I2S_CCR   , clkCfg);  //clock configuration

      SET_REG_WORD(base + I2S_ITER  , 0);  // disable tx block
      SET_REG_WORD(base + I2S_TER1  , 0);  // transmit disable

      SET_REG_WORD(base + I2S_IRER  , 0);  // disable rx block
      SET_REG_WORD(base + I2S_RER1  , 0);  // receiver disable

      SET_REG_WORD(base + I2S_IMR1  , intMask); // mask interrupts

      // set channel for tx
           SET_REG_WORD(base + I2S_TFCR1 , fifoLvl);  // transmit fifo lvl
           SET_REG_WORD(base + I2S_TCR1  , lenCfg);   // data length
           SET_REG_WORD(base + I2S_TXFFR , 1);         // flush the tx fifo

      // set channel for rx
           SET_REG_WORD(base + I2S_RFCR1 , fifoLvl);  // fifo length triger (triggered on value +1)
           SET_REG_WORD(base + I2S_RCR1  , lenCfg);   // data length
           SET_REG_WORD(base + I2S_RXFFR , 1);         // flush the tx fifo

      // enable
           SET_REG_WORD(base + I2S_IRER  , 1);         // enable rx block
           SET_REG_WORD(base + I2S_ITER  , 1);         // enable tx block
           SET_REG_WORD(base + I2S_RER1  , 1);         // enable receiver
           SET_REG_WORD(base + I2S_TER1  , 1);         // enable transmition
      if (master)
         SET_REG_WORD(base + I2S_CER   , 1);              // enable clock generation
}



/* ***********************************************************************//**
   *************************************************************************
I2S - configure channel 2 of a i2s module
@{
----------------------------------------------------------------------------
@}
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
      fifoLvl - fifo level at which the interrupt is triggered
@param
      lenCfg - word length
              - defines in the DrvI2sDefines.h can be used for this
              .
@param
      intMask - interrupt masks
@return
@{
info:
    this channel works only as tx
@}
 ************************************************************************* */
void DrvI2sInitCh2(u32 base, u32 master,
                                         u32 clkCfg,
                                         u32 fifoLvl,
                                         u32 lenCfg,
                                         u32 intMask)
{
      u32 i2sModule;
      i2sModule = ((base - LPB1_CONTROL_ADR) >> 16) & 0x0F;
      if (master)
         switch (i2sModule)
         {
           case 0x0c: SET_REG_WORD(CPR_GEN_CTRL_ADR, GET_REG_WORD_VAL(CPR_GEN_CTRL_ADR) | 0x01); break;
           case 0x0d: SET_REG_WORD(CPR_GEN_CTRL_ADR, GET_REG_WORD_VAL(CPR_GEN_CTRL_ADR) | 0x02); break;
           case 0x0e: SET_REG_WORD(CPR_GEN_CTRL_ADR, GET_REG_WORD_VAL(CPR_GEN_CTRL_ADR) | 0x04); break;
           default:
            break;
         }


      SET_REG_WORD(base + I2S_IER   , 1);        // enable I2S module
      SET_REG_WORD(base + I2S_CER   , 0);        // disable clock generation
      SET_REG_WORD(base + I2S_CCR   , clkCfg);  //clock configuration

      SET_REG_WORD(base + I2S_ITER  , 0);  // disable tx block
      SET_REG_WORD(base + I2S_TER2  , 0);  // transmit disable

      SET_REG_WORD(base + I2S_IRER  , 0);  // disable rx block

      SET_REG_WORD(base + I2S_IMR2  , intMask); // mask interrupts

           SET_REG_WORD(base + I2S_TFCR2 , fifoLvl); // transmit fifo lvl
           SET_REG_WORD(base + I2S_TCR2  , lenCfg);  // data length
           SET_REG_WORD(base + I2S_TXFFR , 1);        // flush the tx fifo
           SET_REG_WORD(base + I2S_ITER  , 1);        // enable tx block
           SET_REG_WORD(base + I2S_IRER  , 1);        // enable rx block
           SET_REG_WORD(base + I2S_TER2  , 1);        // enable transmition

      if (master)
         SET_REG_WORD(base + I2S_CER   , 1);              // enable clock generation
}


/* ***********************************************************************//**
   *************************************************************************
I2S config example
@{
----------------------------------------------------------------------------
@}
@param
@return
@{
info:
@}
 ************************************************************************* */
void DrvI2sConfigExample()
{
  DrvI2sInitCh0(I2S1_BASE_ADR /*module 1*/,
                     D_I2S_TX /*set channel as transmitter*/,
                     D_I2S_MASTER /*module is master*/,
                     D_I2S_SCLKG_NO | D_I2S_WSS_16CC /*clk gen*/,
                     7 /* fifo level*/,
                     D_I2S_TCRRCR_WLEN_16BIT /*word length*/,
                     ~D_I2S_IMR_TXFEM /*mask all but TXFEM = fifo empty*/);

  DrvI2sInitCh1(I2S1_BASE_ADR /*module 1*/,
                     D_I2S_RX /*set channel as receiver*/,
                     D_I2S_MASTER /*module is master*/,
                     D_I2S_SCLKG_NO | D_I2S_WSS_16CC /*clk gen*/,
                     7 /* fifo level*/,
                     D_I2S_TCRRCR_WLEN_16BIT /*word length*/,
                     ~D_I2S_IMR_RXDAM /*mask all but RXDAM = data available*/);
}


/* ***********************************************************************//**
   *************************************************************************
I2S interrupt handler  structure pointer init
@{
----------------------------------------------------------------------------
@}
@param
         *i2sModulePntr - pointer to a DrvI2sStructH structure which will be initialized
@param
         address1 - address of the momory zone used for channel 0 of the module
@param
         address2 - address of the momory zone used for channel 1 of the module
@param
         address3 - address of the momory zone used for channel 2 of the module
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
@{
info:  use this function to init the coresponding pointer for the coresponding i2s module
@}
 ************************************************************************* */
DrvI2sStructH * DrvI2sHandlerInit(DrvI2sStructH *i2sModulePntr,
                          u32 address1,u32 address2,u32 address3,
                          u32 address1sh,u32 address2sh,u32 address3sh,
                          u32 size1, u32 size2, u32 size3,
                          u32 moduleBase)
{
    i2sModulePntr->addr1 = address1;
    i2sModulePntr->addr2 = address2;
    i2sModulePntr->addr3 = address3;

    i2sModulePntr->addr1Shadow = address1sh;
    i2sModulePntr->addr2Shadow = address2sh;
    i2sModulePntr->addr3Shadow = address3sh;

    i2sModulePntr->size1 = size1;
    i2sModulePntr->size2 = size2;
    i2sModulePntr->size3 = size3;

    i2sModulePntr->baseAddress = moduleBase;

    return i2sModulePntr;
}

                         // addresses for
                         //ch0                 ch1                 ch0                   ch1                   ch2
                         //rx                  rx                  tx                    tx                    tx
DrvI2sStructH * DrvI2sHandlerInitRxTx(DrvI2sStructH *i2sModulePntr,
                          u32 address1  , u32 address2  , u32 address11  , u32 address21  , u32 address3,
                          u32 address1sh, u32 address2sh, u32 address1sh1, u32 address2sh1, u32 address3sh,
                          u32 size1     , u32 size2     , u32 size11     , u32 size21     , u32 size3,
                          u32 moduleBase)
{
    i2sModulePntr->addr1 = address1;
    i2sModulePntr->addr2 = address2;
    i2sModulePntr->addr3 = address3;
    i2sModulePntr->addr11 = address11;
    i2sModulePntr->addr21 = address21;

    i2sModulePntr->addr1Shadow = address1sh;
    i2sModulePntr->addr2Shadow = address2sh;
    i2sModulePntr->addr3Shadow = address3sh;
    i2sModulePntr->addr1Shadow1 = address1sh1;
    i2sModulePntr->addr2Shadow1 = address2sh1;

    i2sModulePntr->size1 = size1;
    i2sModulePntr->size2 = size2;
    i2sModulePntr->size3 = size3;
    i2sModulePntr->size11 = size11;
    i2sModulePntr->size21 = size21;


    i2sModulePntr->baseAddress = moduleBase;

    return i2sModulePntr;
}



/* ***********************************************************************//**
   *************************************************************************
I2S pointer to structures that would configure the i2s module during interrupts
@{
----------------------------------------------------------------------------
@}
@param
    sI2sHandler - pointer to a DrvI2sStructH structure used in interrupt handling of I2S module
@{
info:
    - has to be initilized before using
    - fields have to be initialized with a call to : DrvI2sHandlerInit(...)
    .
@}
 ************************************************************************* */
DrvI2sStructH *sI2sHandler;


/* ***********************************************************************//**
   *************************************************************************
I2S interrupt handler
@{
----------------------------------------------------------------------------
@}
@param
     unusedParam - parameter is not used, but given to comply with the handler function pointer
@return
@{
info:
@}
 ************************************************************************* */
unsigned int i2sInt2, i2sInt1, i2sInt3, i2sInt3r, i2sInt3t;

void DrvI2sHandler(u32 unusedParam)
{
    int j,unused;
    u32 state;
    // channel 0
    unusedParam = unusedParam;

    ++i2sInt1;
    state = GET_REG_WORD_VAL(sI2sHandler->baseAddress + I2S_ISR0);
    if ((state & 0x01) == 0x01)   // receive interrupt
    {
                for (j=0 ; j<8 ; j+=1)
                {
                        *((u32*)(sI2sHandler->addr1)) = GET_REG_WORD_VAL(sI2sHandler->baseAddress + I2S_LRBR0);
                                                           unused = GET_REG_WORD_VAL(sI2sHandler->baseAddress + I2S_RRBR0); // right channel is not used
                        sI2sHandler->addr1 += 4; // next word
                        if ( sI2sHandler->addr1 - sI2sHandler->addr1Shadow > sI2sHandler->size1 )
                             sI2sHandler->addr1 = sI2sHandler->addr1Shadow;
                }
    }
    else
    if ((state & 0x10) == 0x10) // transmit interrupt
    {
                for (j=0 ; j<8 ; j+=1)
                {
                        SET_REG_WORD(sI2sHandler->baseAddress + I2S_LTHR0, *((u32*)(sI2sHandler->addr1)));
                        SET_REG_WORD(sI2sHandler->baseAddress + I2S_RTHR0, *((u32*)(sI2sHandler->addr1)));
                        sI2sHandler->addr1 += 4; // next word
                        if ( sI2sHandler->addr1 - sI2sHandler->addr1Shadow > sI2sHandler->size1 )
                             sI2sHandler->addr1 = sI2sHandler->addr1Shadow;
                }
                SET_REG_WORD(sI2sHandler->baseAddress + I2S_ITER, 1);   // enable transmision
                SET_REG_WORD(sI2sHandler->baseAddress + I2S_CER,  1);   // enable clock generation
    }


    // channel 1
    state = GET_REG_WORD_VAL(sI2sHandler->baseAddress + I2S_ISR1);
    if ((state & 0x01) == 0x01)   // receive interrupt
    {
                for (j=0 ; j<8 ; j+=1)
                {
                        *((u32*)(sI2sHandler->addr2)) = GET_REG_WORD_VAL(sI2sHandler->baseAddress + I2S_LRBR1);
                                                           unused = GET_REG_WORD_VAL(sI2sHandler->baseAddress + I2S_RRBR1); // right channel is not used
                        sI2sHandler->addr2 += 4; // next word
                        if ( sI2sHandler->addr2 - sI2sHandler->addr2Shadow > sI2sHandler->size2 )
                             sI2sHandler->addr2 = sI2sHandler->addr2Shadow;
                }
    }
    else if ((state & 0x10) == 0x10) // transmit interrupt
    {
                for (j=0 ; j<8 ; j+=1)
                {
                        SET_REG_WORD(sI2sHandler->baseAddress + I2S_LTHR1, *((u32*)(sI2sHandler->addr2)));
                        SET_REG_WORD(sI2sHandler->baseAddress + I2S_RTHR1, *((u32*)(sI2sHandler->addr2)));
                        sI2sHandler->addr2 += 4; // next word
                        if ( sI2sHandler->addr2 - sI2sHandler->addr2Shadow > sI2sHandler->size2 )
                             sI2sHandler->addr2 = sI2sHandler->addr2Shadow;
                }
                SET_REG_WORD(sI2sHandler->baseAddress + I2S_ITER, 1);   // enable transmision
                SET_REG_WORD(sI2sHandler->baseAddress + I2S_CER,  1);   // enable clock generation
    }

    // channel 2
    state = GET_REG_WORD_VAL(sI2sHandler->baseAddress + I2S_ISR2);
    if ((state & 0x10) == 0x10) // transmit interrupt
    {
                for (j=0 ; j<8 ; j+=1)
                {
                        SET_REG_WORD(sI2sHandler->baseAddress + I2S_LTHR2, *((u32*)(sI2sHandler->addr3)));
                        SET_REG_WORD(sI2sHandler->baseAddress + I2S_RTHR2, *((u32*)(sI2sHandler->addr3)));
                        sI2sHandler->addr3 += 4; // next word
                        if ( sI2sHandler->addr3 - sI2sHandler->addr3Shadow > sI2sHandler->size3 )
                             sI2sHandler->addr3 = sI2sHandler->addr3Shadow;
                }
                SET_REG_WORD(sI2sHandler->baseAddress + I2S_ITER, 1);   // enable transmision
                SET_REG_WORD(sI2sHandler->baseAddress + I2S_CER,  1);   // enable clock generation
    }
}


/* ***********************************************************************//**
   *************************************************************************
I2S pointer to structures that would configure the i2s module during interrupts
@{
----------------------------------------------------------------------------
@}
@param
    sI2sHandlerTwo - pointer to a DrvI2sStructH structure used in interrupt handling of I2S module
@{
info:
    - has to be initilized before using
    - fields have to be initialized with a call to : DrvI2sHandlerInit(...)
    .
@}
 ************************************************************************* */
DrvI2sStructH *sI2sHandlerTwo;

/* ***********************************************************************//**
   *************************************************************************
I2S interrupt handler
@{
----------------------------------------------------------------------------
@}
@param
     unusedParam - parameter is not used, but given to comply with the handler function pointer
@return
@{
info:
@}
 ************************************************************************* */

void DrvI2sHandlerTwo(u32 unusedParam)
{
    int j,unused;
    u32 state;
    ++i2sInt2;
    unusedParam = unusedParam;

    // channel 0
    state = GET_REG_WORD_VAL(sI2sHandlerTwo->baseAddress + I2S_ISR0);
    if ((state & 0x01) == 0x01)   // receive interrupt
    {
                for (j=0 ; j<8 ; j+=1)
                {
                        *((u32*)(sI2sHandlerTwo->addr1)) = GET_REG_WORD_VAL(sI2sHandlerTwo->baseAddress + I2S_LRBR0);
                                                           unused = GET_REG_WORD_VAL(sI2sHandlerTwo->baseAddress + I2S_RRBR0); // right channel is not used
                        sI2sHandlerTwo->addr1 += 4; // next word
                        if ( sI2sHandlerTwo->addr1 - sI2sHandlerTwo->addr1Shadow > sI2sHandlerTwo->size1 )
                             sI2sHandlerTwo->addr1 = sI2sHandlerTwo->addr1Shadow;
                }
    }
    else
    if ((state & 0x10) == 0x10) // transmit interrupt
    {
                for (j=0 ; j<8 ; j+=1)
                {
                        SET_REG_WORD(sI2sHandlerTwo->baseAddress + I2S_LTHR0, *((u32*)(sI2sHandlerTwo->addr1)));
                        SET_REG_WORD(sI2sHandlerTwo->baseAddress + I2S_RTHR0, *((u32*)(sI2sHandlerTwo->addr1)));
                        sI2sHandlerTwo->addr1 += 4; // next word
                        if ( sI2sHandlerTwo->addr1 - sI2sHandlerTwo->addr1Shadow > sI2sHandlerTwo->size1 )
                             sI2sHandlerTwo->addr1 = sI2sHandlerTwo->addr1Shadow;
                }
                SET_REG_WORD(sI2sHandlerTwo->baseAddress + I2S_ITER, 1);   // enable transmision
                SET_REG_WORD(sI2sHandlerTwo->baseAddress + I2S_CER,  1);   // enable clock generation
    }


    // channel 1
    state = GET_REG_WORD_VAL(sI2sHandlerTwo->baseAddress + I2S_ISR1);
    if ((state & 0x01) == 0x01)   // receive interrupt
    {
                for (j=0 ; j<8 ; j+=1)
                {
                        *((u32*)(sI2sHandlerTwo->addr2)) = GET_REG_WORD_VAL(sI2sHandlerTwo->baseAddress + I2S_LRBR1);
                                                           unused = GET_REG_WORD_VAL(sI2sHandlerTwo->baseAddress + I2S_RRBR1); // right channel is not used
                        sI2sHandlerTwo->addr2 += 4; // next word
                        if ( sI2sHandlerTwo->addr2 - sI2sHandlerTwo->addr2Shadow > sI2sHandlerTwo->size2 )
                             sI2sHandlerTwo->addr2 = sI2sHandlerTwo->addr2Shadow;
                }
    }
    else if ((state & 0x10) == 0x10) // transmit interrupt
    {
                for (j=0 ; j<8 ; j+=1)
                {
                        SET_REG_WORD(sI2sHandlerTwo->baseAddress + I2S_LTHR1, *((u32*)(sI2sHandlerTwo->addr2)));
                        SET_REG_WORD(sI2sHandlerTwo->baseAddress + I2S_RTHR1, *((u32*)(sI2sHandlerTwo->addr2)));
                        sI2sHandlerTwo->addr2 += 4; // next word
                        if ( sI2sHandlerTwo->addr2 - sI2sHandlerTwo->addr2Shadow > sI2sHandlerTwo->size2 )
                             sI2sHandlerTwo->addr2 = sI2sHandlerTwo->addr2Shadow;
                }
                SET_REG_WORD(sI2sHandlerTwo->baseAddress + I2S_ITER, 1);   // enable transmision
                SET_REG_WORD(sI2sHandlerTwo->baseAddress + I2S_CER,  1);   // enable clock generation
    }

    // channel 2
    state = GET_REG_WORD_VAL(sI2sHandlerTwo->baseAddress + I2S_ISR2);
    if ((state & 0x10) == 0x10) // transmit interrupt
    {
                for (j=0 ; j<8 ; j+=1)
                {
                        SET_REG_WORD(sI2sHandlerTwo->baseAddress + I2S_LTHR2, *((u32*)(sI2sHandlerTwo->addr3)));
                        SET_REG_WORD(sI2sHandlerTwo->baseAddress + I2S_RTHR2, *((u32*)(sI2sHandlerTwo->addr3)));
                        sI2sHandlerTwo->addr3 += 4; // next word
                        if ( sI2sHandlerTwo->addr3 - sI2sHandlerTwo->addr3Shadow > sI2sHandlerTwo->size3 )
                             sI2sHandlerTwo->addr3 = sI2sHandlerTwo->addr3Shadow;
                }
                SET_REG_WORD(sI2sHandlerTwo->baseAddress + I2S_ITER, 1);   // enable transmision
                SET_REG_WORD(sI2sHandlerTwo->baseAddress + I2S_CER,  1);   // enable clock generation
    }
}





/* ***********************************************************************//**
   *************************************************************************
I2S pointer to structures that would configure the i2s module during interrupts
@{
----------------------------------------------------------------------------
@}
@param
    sI2sHandlerRxTx - pointer to a DrvI2sStructH structure used in interrupt handling of I2S module
@{
info:
    - has to be initilized before using
    - fields have to be initialized with a call to : DrvI2sHandlerInitRxTx(...)
    .
@}
 ************************************************************************* */
DrvI2sStructH *sI2sHandlerRxTx;

/* ***********************************************************************//**
   *************************************************************************
I2S interrupt handler
@{
----------------------------------------------------------------------------
@}
@param
     unusedParam - parameter is not used, but given to comply with the handler function pointer
@return
@{
info:
@}
 ************************************************************************* */

void DrvI2sHandlerRxTx(u32 unusedParam)
{
    int j,unused;
    u32 state;

    unusedParam = unusedParam;
    // channel 0 RX
    ++i2sInt3;
    state = GET_REG_WORD_VAL(sI2sHandlerRxTx->baseAddress + I2S_ISR0);
    if ((state & 0x01) == 0x01)   // receive interrupt
    {
                ++i2sInt3r;
                for (j=0 ; j<8 ; j+=1)
                {
                        *((u32*)(sI2sHandlerRxTx->addr1)) = GET_REG_WORD_VAL(sI2sHandlerRxTx->baseAddress + I2S_LRBR0);
                                                      unused = GET_REG_WORD_VAL(sI2sHandlerRxTx->baseAddress + I2S_RRBR0); // right channel is not used
                         //*((u32*)(sI2sHandlerRxTx->addr1))= GET_REG_WORD_VAL(sI2sHandlerRxTx->baseAddress + I2S_RRBR0);
                        sI2sHandlerRxTx->addr1 += 4; // next word
                        if ( sI2sHandlerRxTx->addr1 - sI2sHandlerRxTx->addr1Shadow > sI2sHandlerRxTx->size1 )
                             sI2sHandlerRxTx->addr1 = sI2sHandlerRxTx->addr1Shadow;
                }
    }

    // channel 0 TX
    state = GET_REG_WORD_VAL(sI2sHandlerRxTx->baseAddress + I2S_ISR0);
    if ((state & 0x10) == 0x10) // transmit interrupt
    {
                ++i2sInt3t;
                for (j=0 ; j<8 ; j+=1)
                {
                        SET_REG_WORD(sI2sHandlerRxTx->baseAddress + I2S_LTHR0, *((u32*)(sI2sHandlerRxTx->addr11)));
                        SET_REG_WORD(sI2sHandlerRxTx->baseAddress + I2S_RTHR0, *((u32*)(sI2sHandlerRxTx->addr11)));
                        sI2sHandlerRxTx->addr11 += 4; // next word
                        if ( sI2sHandlerRxTx->addr11 - sI2sHandlerRxTx->addr1Shadow1 > sI2sHandlerRxTx->size11 )
                             sI2sHandlerRxTx->addr11 = sI2sHandlerRxTx->addr1Shadow1;
                }
                SET_REG_WORD(sI2sHandlerRxTx->baseAddress + I2S_ITER, 1);   // enable transmision
                SET_REG_WORD(sI2sHandlerRxTx->baseAddress + I2S_CER,  1);   // enable clock generation
    }


    // channel 1 RX
    state = GET_REG_WORD_VAL(sI2sHandlerRxTx->baseAddress + I2S_ISR1);
    if ((state & 0x01) == 0x01)   // receive interrupt
    {
                for (j=0 ; j<8 ; j+=1)
                {
                        *((u32*)(sI2sHandlerRxTx->addr2)) = GET_REG_WORD_VAL(sI2sHandlerRxTx->baseAddress + I2S_LRBR1);
                                                           unused = GET_REG_WORD_VAL(sI2sHandlerRxTx->baseAddress + I2S_RRBR1); // right channel is not used
                        sI2sHandlerRxTx->addr2 += 4; // next word
                        if ( sI2sHandlerRxTx->addr2 - sI2sHandlerRxTx->addr2Shadow > sI2sHandlerRxTx->size2 )
                             sI2sHandlerRxTx->addr2 = sI2sHandlerRxTx->addr2Shadow;
                }
    }

    // channel 1 TX
    state = GET_REG_WORD_VAL(sI2sHandlerRxTx->baseAddress + I2S_ISR1);
    if ((state & 0x10) == 0x10) // transmit interrupt
    {
                for (j=0 ; j<8 ; j+=1)
                {
                        SET_REG_WORD(sI2sHandlerRxTx->baseAddress + I2S_LTHR1, *((u32*)(sI2sHandlerRxTx->addr21)));
                        SET_REG_WORD(sI2sHandlerRxTx->baseAddress + I2S_RTHR1, *((u32*)(sI2sHandlerRxTx->addr21)));
                        sI2sHandlerRxTx->addr21 += 4; // next word
                        if ( sI2sHandlerRxTx->addr21 - sI2sHandlerRxTx->addr2Shadow1 > sI2sHandlerRxTx->size21 )
                             sI2sHandlerRxTx->addr21 = sI2sHandlerRxTx->addr2Shadow1;
                }
                SET_REG_WORD(sI2sHandlerRxTx->baseAddress + I2S_ITER, 1);   // enable transmision
                SET_REG_WORD(sI2sHandlerRxTx->baseAddress + I2S_CER,  1);   // enable clock generation
    }

    // channel 2 TX
    state = GET_REG_WORD_VAL(sI2sHandlerRxTx->baseAddress + I2S_ISR2);
    if ((state & 0x10) == 0x10) // transmit interrupt
    {
                for (j=0 ; j<8 ; j+=1)
                {
                        SET_REG_WORD(sI2sHandlerRxTx->baseAddress + I2S_LTHR2, *((u32*)(sI2sHandlerRxTx->addr3)));
                        SET_REG_WORD(sI2sHandlerRxTx->baseAddress + I2S_RTHR2, *((u32*)(sI2sHandlerRxTx->addr3)));
                        sI2sHandlerRxTx->addr3 += 4; // next word
                        if ( sI2sHandlerRxTx->addr3 - sI2sHandlerRxTx->addr3Shadow > sI2sHandlerRxTx->size3 )
                             sI2sHandlerRxTx->addr3 = sI2sHandlerRxTx->addr3Shadow;
                }
                SET_REG_WORD(sI2sHandlerRxTx->baseAddress + I2S_ITER, 1);   // enable transmision
                SET_REG_WORD(sI2sHandlerRxTx->baseAddress + I2S_CER,  1);   // enable clock generation
    }
}



