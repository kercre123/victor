#include <mv_types.h>
#include <isaac_registers.h>
#include <stdio.h>
#include "DrvI2c.h"
#include "stdlib.h"
#include "DrvTimer.h"
#include <swcLeonUtils.h>

u32 icRxflr;
u32 icRawIntrStat;

/*************************************************************************
I2C general config
@{
----------------------------------------------------------------------------
@}
@param
        i2cBase - base address of the i2c module
@param
        Ms - set master/disbale slave
@param
        adr10 - master and slave 10 bit address modes
@param
        speed - 1 max 100k, 2 max 400k, 3 max 3.4M
@param
        rstEn - restart enable
@return
@{
info:
@}
 ************************************************************************* */
void DrvIicCon(u32 i2cBase, u32 ms, u32 adr10, u32 speed, u32 rstEn)
{
    u32 temp;
//    printf("\nbase %d, ms %d, adr10 %d, speed %d, rstEn %d",i2cBase,Ms, ADR10,speed,rstEn);
    temp= (ms & 1) | ((ms & 1) << 6);                                               /* master enable/disable, slave disable/enable */
    temp |= ((adr10 & 1) << 3) | ((adr10 & 1) << 4);                                /* 10 bit addr for slave and master */
    temp |= (rstEn & 1) << 5;                                                      /* allows restart condition to be send */
    temp |= (speed & 3)  << 1;                                                      /* sets the speed mode low, normal , hi */
    SET_REG_WORD(i2cBase + IIC_CON, temp);
}

/* ***********************************************************************//**
   *************************************************************************
I2C enable/disable
@{
----------------------------------------------------------------------------
@}
@param
        i2cBase - base address of the i2c module
@param
        en - 1 enables iic module
           -  0 disables
           .
@return
@{
info:
        Module should be disabled before timing configuration
@}
 ************************************************************************* */
inline void DrvIicEn(u32 i2cBase, u32 en)
{
    SET_REG_WORD(i2cBase + IIC_ENABLE, en & 1);
}


/* ***********************************************************************//**
   *************************************************************************
I2C
@{
----------------------------------------------------------------------------
@}
@param
      i2cBase - base address of the i2c module
@param
      tar - target address
@return
@{
info:
     direct acces to TAR
@}
 ************************************************************************* */
inline void DrvIicSetTar(u32 i2cBase, u32 tar)
{
    SET_REG_WORD(i2cBase + IIC_TAR, tar);
}

 /* ***********************************************************************//**
   *************************************************************************
I2C
@{
----------------------------------------------------------------------------
@}
@param
      i2cBase - base address of the i2c module
@param
      sar - slave address of the IIC module in slave mode
@return
@{
info:
@}
 ************************************************************************* */
inline void DrvIicSetSar(u32 i2cBase, u32 sar)
{
    SET_REG_WORD(i2cBase + IIC_SAR, sar);
}

 /* ***********************************************************************//**
   *************************************************************************
I2C
@{
----------------------------------------------------------------------------
@}
@param
       i2cBase - base address of the i2c module
@param
       hCnt - high count register
@param
       lCnt - low count register
@return
@{
info:
@}
 ************************************************************************* */
inline void DrvIicSetSpeedSlow(u32 i2cBase, u32 hCnt, u32 lCnt)
{
    SET_REG_WORD(i2cBase + IIC_SS_HCNT, hCnt);
    SET_REG_WORD(i2cBase + IIC_SS_LCNT, lCnt);
}

 /* ***********************************************************************//**
   *************************************************************************
I2C
@{
----------------------------------------------------------------------------
@}
@param
       i2cBase - base address of the i2c module
@param
       hCnt - high count register
@param
       lCnt - low count register
@return
@{
info:
@}
 ************************************************************************* */
inline void DrvIicSetSpeedFast(u32 i2cBase, u32 hCnt, u32 lCnt)
{
    SET_REG_WORD(i2cBase + IIC_FS_HCNT, hCnt);
    SET_REG_WORD(i2cBase + IIC_FS_LCNT, lCnt);
}

 /* ***********************************************************************//**
   *************************************************************************
I2C
@{
----------------------------------------------------------------------------
@}
@param
       i2cBase - base address of the i2c module
@param
       hCnt - high count register
@param
       lCnt - low count register
@return
@{
info:
@}
 ************************************************************************* */
inline void DrvIicSetSpeedHigh(u32 i2cBase, u32 hCnt, u32 lCnt)
{
    SET_REG_WORD(i2cBase + IIC_HS_HCNT, hCnt);
    SET_REG_WORD(i2cBase + IIC_HS_LCNT, lCnt);
}

 /* ***********************************************************************//**
   *************************************************************************
I2C FIFO depth
@{
----------------------------------------------------------------------------
@}
@param
      i2cBase - base address of the i2c module
@param
      ptxDepth - tx fifo depth will be stored at that pointer
@param
      prxDepth - rx fifo depth will be stored at that pointer
@return
@{
info:
@}
************************************************************************* */
inline void DrvIicFifoDepth(u32 i2cBase, u32 *ptxDepth,u32 *prxDepth)
{
     u32 depth;
     depth = GET_REG_WORD_VAL(i2cBase + IIC_COMP_PARAM_1);
     *prxDepth = (depth >> 8) & 0xFF;
     *ptxDepth = (depth >> 16) & 0xFF;
}

/* ***********************************************************************//**
   *************************************************************************
I2C write
@{
----------------------------------------------------------------------------
@}
@param
     i2cBase - base address of the i2c module
@param
     dat1
@param
     dat2 - data to be written
@return
@{
info:
     - can't be used for continuous writing
     - this can be used to send an addressa and a data
     - waits for the fifo to be empty, forces a STOP condition
@}
************************************************************************* */
u32 DrvIicWr2(u32 i2cBase, u32 dat1, u32 dat2) {
  u32 txAbrtSrc;
  volatile u32 clr;

  while (!(GET_REG_WORD_VAL(i2cBase + IIC_STATUS) & IIC_STATUS_TX_FIFO_EMPTY))
    ;
  SET_REG_WORD(i2cBase + IIC_DATA_CMD, dat1 & 0xFF);
  SET_REG_WORD(i2cBase + IIC_DATA_CMD, dat2 & 0xFF);
  while (!(GET_REG_WORD_VAL(i2cBase + IIC_STATUS) & IIC_STATUS_TX_FIFO_EMPTY))
    ;
  while ((GET_REG_WORD_VAL(i2cBase + IIC_STATUS) & IIC_STATUS_ACTIVITY_MASTER))
    ; // wait for the master to go idle
  if((GET_REG_WORD_VAL(i2cBase + IIC_INTR_STAT) & IIC_IRQ_TX_ABRT)) {
    txAbrtSrc = GET_REG_WORD_VAL(i2cBase + IIC_TX_ABRT_SOURCE);
    clr = GET_REG_WORD_VAL(i2cBase + IIC_CLR_INTR);
    return txAbrtSrc;
  }
  return 0;
}
/* ***********************************************************************//**
   *************************************************************************
I2C write timeout
@{
----------------------------------------------------------------------------
@}
@param
     i2cBase - base address of the i2c module
@param
     dat1
@param
     dat2 - data to be written
@param
     timeout - relative timeout
@return
@{
info:
     - can't be used for continuous writing
     - this can be used to send an addressa and a data
     - waits for the fifo to be empty, forces a STOP condition
@}
************************************************************************* */
int DrvIicWr2Timeout(u32 i2cBase, u32 dat1, u32 dat2, u32 timeout) {
  u32 txAbrtSrc;
  volatile u32 clr;
  volatile int localTimeout;

  localTimeout = timeout;
  while ((!(GET_REG_WORD_VAL(i2cBase + IIC_STATUS) & IIC_STATUS_TX_FIFO_EMPTY)) && --localTimeout )
    ;
  if(!localTimeout)
    goto err_timeout;

  SET_REG_WORD(i2cBase + IIC_DATA_CMD, dat1 & 0xFF);
  SET_REG_WORD(i2cBase + IIC_DATA_CMD, dat2 & 0xFF);
  localTimeout = timeout;
  while (!(GET_REG_WORD_VAL(i2cBase + IIC_STATUS) & IIC_STATUS_TX_FIFO_EMPTY) && --localTimeout )
    ;
  if(!localTimeout)
    goto err_timeout;
  localTimeout = timeout;
  while ((GET_REG_WORD_VAL(i2cBase + IIC_STATUS) & IIC_STATUS_ACTIVITY_MASTER) && --localTimeout )
    ; // wait for the master to go idle
  if(!localTimeout)
    goto err_timeout;
  if((GET_REG_WORD_VAL(i2cBase + IIC_INTR_STAT) & IIC_IRQ_TX_ABRT)) {
    txAbrtSrc = GET_REG_WORD_VAL(i2cBase + IIC_TX_ABRT_SOURCE);
    clr = GET_REG_WORD_VAL(i2cBase + IIC_CLR_INTR);
    return txAbrtSrc;
  }
  return 0;
err_timeout:
  return -1;
}

/* ***********************************************************************//**
   *************************************************************************
I2C write
@{
----------------------------------------------------------------------------
@}
@param
     i2cBase - base address of the i2c module
@param
     dat1
@param
     dat2 - data to be written
@return
@{
info:
     - can't be used for continuous writing
     - this can be used to send an addressa and two data, or a 2 byte address and a data
     - waits for the fifo to be empty, forces a STOP condition

@}
************************************************************************* */
u32 DrvIicWr3(u32 i2cBase, u32 dat1, u32 dat2, u32 dat3) {
  u32 txAbrtSrc;
  volatile u32 clr;

  while (!(GET_REG_WORD_VAL(i2cBase + IIC_STATUS) & IIC_STATUS_TX_FIFO_EMPTY))
    ;
  SET_REG_WORD(i2cBase + IIC_DATA_CMD, dat1 & 0xFF);
  SET_REG_WORD(i2cBase + IIC_DATA_CMD, dat2 & 0xFF);
  SET_REG_WORD(i2cBase + IIC_DATA_CMD, dat3 & 0xFF);
  while (!(GET_REG_WORD_VAL(i2cBase + IIC_STATUS) & IIC_STATUS_TX_FIFO_EMPTY))
    ;
  while ((GET_REG_WORD_VAL(i2cBase + IIC_STATUS) & IIC_STATUS_ACTIVITY_MASTER))
    ; // wait for the master to go idle
  if((GET_REG_WORD_VAL(i2cBase + IIC_INTR_STAT) & IIC_IRQ_TX_ABRT)) {
    txAbrtSrc = GET_REG_WORD_VAL(i2cBase + IIC_TX_ABRT_SOURCE);
    clr = GET_REG_WORD_VAL(i2cBase + IIC_CLR_INTR);
    return txAbrtSrc;
  }
  return 0;
}

/* ***********************************************************************//**
   *************************************************************************
I2C write
@{
----------------------------------------------------------------------------
@}
@param
     i2cBase - base address of the i2c module
@param
     dat1
@param
     dat2 - data to be written
@return
@{
info:
     - can't be used for continuous writing
     - this can be used to send a two-byte address and two bytes of data
     - waits for the fifo to be empty, forces a STOP condition

@}
************************************************************************* */
u32 DrvIicWr4(u32 i2cBase, u32 dat1, u32 dat2, u32 dat3, u32 dat4) {
  u32 txAbrtSrc;
  volatile u32 clr;

  while (!(GET_REG_WORD_VAL(i2cBase + IIC_STATUS) & IIC_STATUS_TX_FIFO_EMPTY))
    ;
  SET_REG_WORD(i2cBase + IIC_DATA_CMD, dat1 & 0xFF);
  SET_REG_WORD(i2cBase + IIC_DATA_CMD, dat2 & 0xFF);
  SET_REG_WORD(i2cBase + IIC_DATA_CMD, dat3 & 0xFF);
  SET_REG_WORD(i2cBase + IIC_DATA_CMD, dat4 & 0xFF);
  while (!(GET_REG_WORD_VAL(i2cBase + IIC_STATUS) & IIC_STATUS_TX_FIFO_EMPTY))
    ;
  while ((GET_REG_WORD_VAL(i2cBase + IIC_STATUS) & IIC_STATUS_ACTIVITY_MASTER))
    ; // wait for the master to go idle
  if((GET_REG_WORD_VAL(i2cBase + IIC_INTR_STAT) & IIC_IRQ_TX_ABRT)) {
    txAbrtSrc = GET_REG_WORD_VAL(i2cBase + IIC_TX_ABRT_SOURCE);
    clr = GET_REG_WORD_VAL(i2cBase + IIC_CLR_INTR);
    return txAbrtSrc;
  }
  return 0;
}

/* ***********************************************************************//**
   *************************************************************************
I2C write
@{
----------------------------------------------------------------------------
@}
@param
     i2cBase - base address of the i2s module
@param
     adr16 - 16 bit register address
@param
     dat - data to be written
@return
@{
info:
     - can't be used for continuous writing
     - waits for the fifo to be empty, forces a STOP condition

@}
************************************************************************* */
inline u32 DrvIicWr16( u32 i2cBase, u32 adr16, u32 dat ) {
  return DrvIicWr3( i2cBase, (adr16 >> 8) & 0xFF, (adr16) & 0xFF, dat & 0xFF);
}

/* ***********************************************************************//**
   *************************************************************************
I2C write continuous
@{
----------------------------------------------------------------------------
@}
@param
     i2cBase - base address of the i2c module
@param
     *addr - address in EEPROM where the data will be stored
@param
     *data - buffer where data is
@param
     size - size of the transfer
@return
@{
info:
     this should be used for continous writing
@}
************************************************************************* */
u32 DrvIicWr(u32 i2cBase, u32 addr16, u8 *data, u32 size) {
  u32 txAbrtSrc, i;
  volatile u32 clr;

  while (!(GET_REG_WORD_VAL(i2cBase + IIC_STATUS) & IIC_STATUS_TX_FIFO_EMPTY))
    ;
  SET_REG_WORD(i2cBase + IIC_DATA_CMD, (addr16 >> 8) & 0xFF);
  SET_REG_WORD(i2cBase + IIC_DATA_CMD, addr16 & 0xFF);

  for (i=0 ; i < size ; ++i) {
    while (( i > 9 )&& !(GET_REG_WORD_VAL(i2cBase + IIC_STATUS) & IIC_STATUS_TX_FIFO_NOT_FULL))
      ;        /* Wait while FIFO full */
    SET_REG_WORD(i2cBase + IIC_DATA_CMD, *data & 0xFF);
    ++data;
    if((GET_REG_WORD_VAL(i2cBase + IIC_INTR_STAT) & IIC_IRQ_TX_ABRT)) {
      txAbrtSrc = GET_REG_WORD_VAL(i2cBase + IIC_TX_ABRT_SOURCE);
      clr = GET_REG_WORD_VAL(i2cBase + IIC_CLR_INTR);
      return txAbrtSrc;
    }
  }
  while (!(GET_REG_WORD_VAL(i2cBase + IIC_STATUS) & IIC_STATUS_TX_FIFO_EMPTY))
    ;
  while ((GET_REG_WORD_VAL(i2cBase + IIC_STATUS) & IIC_STATUS_ACTIVITY_MASTER))
    ; // wait for the master to go idle
  return 0;
}

u32 DrvIicWr8ba(u32 i2cBase, u32 addr16, u8 *data, u32 size) {
  u32 txAbrtSrc, i;
  volatile u32 clr;

  while (!(GET_REG_WORD_VAL(i2cBase + IIC_STATUS) & IIC_STATUS_TX_FIFO_EMPTY))
    ;
  SET_REG_WORD(i2cBase + IIC_DATA_CMD, addr16 & 0xFF);

  for (i=0 ; i < size ; ++i) {
    while (( i > 9 )&& !(GET_REG_WORD_VAL(i2cBase + IIC_STATUS) & IIC_STATUS_TX_FIFO_NOT_FULL))
     ;        /* Wait while FIFO full */
    SET_REG_WORD(i2cBase + IIC_DATA_CMD, *data & 0xFF);
    ++data;
    if((GET_REG_WORD_VAL(i2cBase + IIC_INTR_STAT) & IIC_IRQ_TX_ABRT)) {
      txAbrtSrc = GET_REG_WORD_VAL(i2cBase + IIC_TX_ABRT_SOURCE);
      clr = GET_REG_WORD_VAL(i2cBase + IIC_CLR_INTR);
      return txAbrtSrc;
    }
  }
  while (!(GET_REG_WORD_VAL(i2cBase + IIC_STATUS) & IIC_STATUS_TX_FIFO_EMPTY))
    ;
  while ((GET_REG_WORD_VAL(i2cBase + IIC_STATUS) & IIC_STATUS_ACTIVITY_MASTER))
    ; // wait for the master to go idle
  return 0;
}


/* ***********************************************************************//**
   *************************************************************************
I2C read
@{
----------------------------------------------------------------------------
@}
@param
     i2cBase - base address of the i2s module
@param
     adr - data to be send to the slave device, before reading, usualy some address
           first 8 bits are used
@return
    value read at that address
@{
info:
    this waits for the tx FIFO to be empty
@}
************************************************************************* */
u32 DrvIicRd8ba(u32 i2cBase, u32 adr) {
  u32 txAbrtSrc;
  volatile u32 clr;

  while (!(GET_REG_WORD_VAL(i2cBase + IIC_STATUS) & IIC_STATUS_TX_FIFO_EMPTY))
    ;
  SET_REG_WORD(i2cBase + IIC_DATA_CMD, adr & 0xFF);
  SET_REG_WORD(i2cBase + IIC_DATA_CMD, 0x100);
  while (!(GET_REG_WORD_VAL(i2cBase + IIC_STATUS) & IIC_STATUS_TX_FIFO_EMPTY))
    ;
  while ((GET_REG_WORD_VAL(i2cBase + IIC_STATUS) & IIC_STATUS_ACTIVITY_MASTER))
    ; // wait for the master to go idle
  if((GET_REG_WORD_VAL(i2cBase + IIC_INTR_STAT) & IIC_IRQ_TX_ABRT)) {
    txAbrtSrc = GET_REG_WORD_VAL(i2cBase + IIC_TX_ABRT_SOURCE);
    clr = GET_REG_WORD_VAL(i2cBase + IIC_CLR_INTR);
    return (-1 << 16 ) | txAbrtSrc;
  }
  while (GET_REG_WORD_VAL(i2cBase + IIC_RXFLR) == 0 )
    ;      // wait while the receive fifo is empty
  return GET_REG_WORD_VAL(i2cBase + IIC_DATA_CMD);
}

/* ***********************************************************************//**
   *************************************************************************
I2C read with timeout
@{
----------------------------------------------------------------------------
@}
@param
     i2cBase - base address of the i2s module
@param
     adr - data to be send to the slave device, before reading, usualy some address
           first 8 bits are used
@param
     timeout - sets a timout timer after which an error is returned
@return
    value read at that address
@{
info:
    this waits for the tx FIFO to be empty
@}
************************************************************************* */
int DrvIicRd8baTimeout(u32 i2cBase, u32 adr, u32 timeout) {
  u32 txAbrtSrc;
  volatile u32 clr;
  int localTimeout;

  localTimeout = timeout;
  while (!(GET_REG_WORD_VAL(i2cBase + IIC_STATUS) & IIC_STATUS_TX_FIFO_EMPTY) && --localTimeout)
    ;
  if(!localTimeout)
    goto err_return;
  SET_REG_WORD(i2cBase + IIC_DATA_CMD, adr & 0xFF);
  SET_REG_WORD(i2cBase + IIC_DATA_CMD, 0x100);
  localTimeout = timeout;
  while (!(GET_REG_WORD_VAL(i2cBase + IIC_STATUS) & IIC_STATUS_TX_FIFO_EMPTY) && --localTimeout)
    ;
  if(!localTimeout)
    goto err_return;
  localTimeout = timeout;
  while ((GET_REG_WORD_VAL(i2cBase + IIC_STATUS) & IIC_STATUS_ACTIVITY_MASTER) && --localTimeout)
    ; // wait for the master to go idle
  if(!localTimeout)
    goto err_return;
  if((GET_REG_WORD_VAL(i2cBase + IIC_INTR_STAT) & IIC_IRQ_TX_ABRT)) {
    txAbrtSrc = GET_REG_WORD_VAL(i2cBase + IIC_TX_ABRT_SOURCE);
    clr = GET_REG_WORD_VAL(i2cBase + IIC_CLR_INTR);
    return (-1 << 16 ) | txAbrtSrc;
  }
  localTimeout = timeout;
  while ((GET_REG_WORD_VAL(i2cBase + IIC_RXFLR) == 0 ) && --localTimeout)
    ;      // wait while the receive fifo is empty
  if(!localTimeout)
    goto err_return;
  return GET_REG_WORD_VAL(i2cBase + IIC_DATA_CMD);
err_return:
    return -1;
}

/* ***********************************************************************//**
   *************************************************************************
I2C read continuous
@{
----------------------------------------------------------------------------
@}
@param
     i2cBase - base address of the i2c module
@param
     *addr16- address in EEPROM from where the data will be read
@param
     *data - buffer where data will be written
@param
     size - size of the transfer
@return
@{
info:
     this should be used for continous reading
@}
************************************************************************* */
u32 DrvIicRd(u32 i2cBase, u32 addr16, u8 *data, u32 size) {
  u32 txAbrtSrc, i;
  volatile u32 clr;

  while (!(GET_REG_WORD_VAL(i2cBase + IIC_STATUS) & IIC_STATUS_TX_FIFO_EMPTY))
    ;
  SET_REG_WORD(i2cBase + IIC_DATA_CMD, (addr16>> 8) & 0xFF);
  SET_REG_WORD(i2cBase + IIC_DATA_CMD, addr16& 0xFF);

  for (i=0 ; i < size ; ++i) {
    while (( i > 9 )&& !(GET_REG_WORD_VAL(i2cBase + IIC_STATUS) & IIC_STATUS_TX_FIFO_NOT_FULL))
      ;        // Wait while FIFO full
    SET_REG_WORD(i2cBase + IIC_DATA_CMD, 0x100);
    if((GET_REG_WORD_VAL(i2cBase + IIC_INTR_STAT) & IIC_IRQ_TX_ABRT)) {
      txAbrtSrc = GET_REG_WORD_VAL(i2cBase + IIC_TX_ABRT_SOURCE);
      clr = GET_REG_WORD_VAL(i2cBase + IIC_CLR_INTR);
      return txAbrtSrc;
    }
    while( GET_REG_WORD_VAL(i2cBase + IIC_RXFLR)> 0) {                                              /* Wait while there is no data received */
      *data = GET_REG_WORD_VAL(i2cBase + IIC_DATA_CMD);
      data++;
    }
  }
  return 0;
}


/*
u32 DRV_IIC_rd_8ba(u32 i2cBase, u32 addr8, u8 *data, u32 size) {
  u32 txAbrtSrc, i;
  volatile u32 clr;

  while (!(GET_REG_WORD_VAL(i2cBase + IIC_STATUS) & IIC_STATUS_TX_FIFO_EMPTY))
    ;
  SET_REG_WORD(i2cBase + IIC_DATA_CMD, addr8& 0xFF);

  for (i=0 ; i < size ; ++i) {
    while (( i > 9 )&& !(GET_REG_WORD_VAL(i2cBase + IIC_STATUS) & IIC_STATUS_TX_FIFO_NOT_FULL))
      ;        // Wait while FIFO full
    SET_REG_WORD(i2cBase + IIC_DATA_CMD, 0x100);
    if((GET_REG_WORD_VAL(i2cBase + IIC_INTR_STAT) & IIC_IRQ_TX_ABRT)) {
      txAbrtSrc = GET_REG_WORD_VAL(i2cBase + IIC_TX_ABRT_SOURCE);
      clr = GET_REG_WORD_VAL(i2cBase + IIC_CLR_INTR);
      return txAbrtSrc;
    }
    while( GET_REG_WORD_VAL(i2cBase + IIC_RXFLR)> 0) {                                              // Wait while there is no data received
      *data = GET_REG_WORD_VAL(i2cBase + IIC_DATA_CMD);
      data++;
    }
  }
  return 0;
}
*/

void DRV_IIC_rd_8ba(u32 i2cBase, u32 addr, u8 *data, u32 size)
{
    u32 i;
    
    SET_REG_WORD(i2cBase + IIC_DATA_CMD, addr & 0xFF);
    
    for (i=0 ; i < size ; ++i)
    {
       while (!(GET_REG_WORD_VAL(i2cBase + IIC_STATUS) & IIC_STATUS_TX_FIFO_NOT_FULL)) NOP;        /* Wait if the TX_FIFO becomes full */
       SET_REG_WORD(i2cBase + IIC_DATA_CMD, 0x100);
    }

    while(GET_REG_WORD_VAL(i2cBase + IIC_RXFLR) == 0)                                              /* Wait while there is no data received */
        ;
    for(i =0; i < size; ++i) {
         while (!(GET_REG_WORD_VAL(i2cBase + IIC_STATUS) & IIC_STATUS_RX_FIFO_NOT_EMPTY))          /* Wait if the RX_FIFO becomes empty */
                ;
        *data = GET_REG_WORD_VAL(i2cBase + IIC_DATA_CMD);
        data++;
    }
}

/* ***********************************************************************//**
   *************************************************************************
I2C read
@{
----------------------------------------------------------------------------
@}
@param
     i2cBase - base address of the i2s module
@param
     adr16 - data to be send to the slave device, before reading, usualy some address
           first 16 bits are used
@return
    value read at that address
@{
info:
    this waits for the tx FIFO to be empty
@}
************************************************************************* */
u32 DrvIicRd16ba(u32 i2cBase, u32 adr16) {
  u32 txAbrtSrc;
  volatile u32 clr;

  while (!(GET_REG_WORD_VAL(i2cBase + IIC_STATUS) & IIC_STATUS_TX_FIFO_EMPTY))
    ;
  SET_REG_WORD(i2cBase + IIC_DATA_CMD, (adr16 >> 8) & 0xFF);
  SET_REG_WORD(i2cBase + IIC_DATA_CMD, adr16 & 0xFF);
  SET_REG_WORD(i2cBase + IIC_DATA_CMD, 0x100);                  // execute the read command

  while (GET_REG_WORD_VAL(i2cBase + IIC_RXFLR) == 0 ) {
    if((GET_REG_WORD_VAL(i2cBase + IIC_INTR_STAT) & IIC_IRQ_TX_ABRT)) {
      txAbrtSrc = GET_REG_WORD_VAL(i2cBase + IIC_TX_ABRT_SOURCE);
      clr = GET_REG_WORD_VAL(i2cBase + IIC_CLR_INTR);
      return (-1 << 16 ) | txAbrtSrc;
    }
  }
  return GET_REG_WORD_VAL(i2cBase + IIC_DATA_CMD);
}

/* ***********************************************************************//**
   *************************************************************************
I2C read 16 bits
@{
----------------------------------------------------------------------------
@}
@param
     i2cBase - base address of the i2s module
@param
     adr8 - data to be send to the slave device, before reading, usualy some address
           first 8 bits are used
@return
    16 bits, read at that address
@{
info:
    this waits for the tx FIFO to be empty
@}
************************************************************************* */
u32 DrvIicRd8ba16d(u32 i2cBase, u32 adr8) {
  u32 txAbrtSrc, tmp;
 volatile u32 clr;

  while (!(GET_REG_WORD_VAL(i2cBase + IIC_STATUS) & IIC_STATUS_TX_FIFO_EMPTY))
    ;
  SET_REG_WORD(i2cBase + IIC_DATA_CMD, adr8 & 0xFF);
  SET_REG_WORD(i2cBase + IIC_DATA_CMD, 0x100);// read back two values
  SET_REG_WORD(i2cBase + IIC_DATA_CMD, 0x100);
  while (GET_REG_WORD_VAL(i2cBase + IIC_RXFLR) < 2 ) {
    if((GET_REG_WORD_VAL(i2cBase + IIC_INTR_STAT) & IIC_IRQ_TX_ABRT)) {
      txAbrtSrc = GET_REG_WORD_VAL(i2cBase + IIC_TX_ABRT_SOURCE);
      clr = GET_REG_WORD_VAL(i2cBase + IIC_CLR_INTR);
      return (-1 << 16 ) | txAbrtSrc;
    }
  }
  tmp = (GET_REG_WORD_VAL(i2cBase + IIC_DATA_CMD)) << 8;
  tmp |= (GET_REG_WORD_VAL(i2cBase + IIC_DATA_CMD)) & (~0xFF00);
  return tmp;
}

/* ***********************************************************************//**
   *************************************************************************
I2C read 16 bits
@{
----------------------------------------------------------------------------
@}
@param
     i2cBase - base address of the i2s module
@param
     adr - data to be send to the slave device, before reading, usualy some address
           first 16 bits are used
@return
    16 bits, read at that address
@{
info:
    this waits for the tx FIFO to be empty
@}
************************************************************************* */
u32 DrvIicRd16ba16d(u32 i2cBase, u32 adr) {
  u32 txAbrtSrc, tmp;
  volatile u32 clr;

  while (!(GET_REG_WORD_VAL(i2cBase + IIC_STATUS) & IIC_STATUS_TX_FIFO_EMPTY))
    ;
  SET_REG_WORD(i2cBase + IIC_DATA_CMD, (adr >> 8) & 0xFF);
  SET_REG_WORD(i2cBase + IIC_DATA_CMD, adr & 0xFF);
  SET_REG_WORD(i2cBase + IIC_DATA_CMD, 0x100);// read back two values
  SET_REG_WORD(i2cBase + IIC_DATA_CMD, 0x100);
  
  while (GET_REG_WORD_VAL(i2cBase + IIC_RXFLR) < 2 ) {
    if((GET_REG_WORD_VAL(i2cBase + IIC_INTR_STAT) & IIC_IRQ_TX_ABRT)) {
      txAbrtSrc = GET_REG_WORD_VAL(i2cBase + IIC_TX_ABRT_SOURCE);
      clr = GET_REG_WORD_VAL(i2cBase + IIC_CLR_INTR);
      return (-1 << 16 ) | txAbrtSrc;
    }
  }
  tmp = (GET_REG_WORD_VAL(i2cBase + IIC_DATA_CMD)) << 8;
  tmp |= (GET_REG_WORD_VAL(i2cBase + IIC_DATA_CMD)) & (~0xFF00);
  return tmp;
}

/* ***********************************************************************//**
   *************************************************************************
I2C test
@{
----------------------------------------------------------------------------
@{
@param
     i2cBase - base address of the i2s module
@param
     addr - address of the slave
@return
@{
info:
@}
************************************************************************* */
void DrvIicTestInit(u32 i2cBase, u32 addr)
{
     DrvIicEn(i2cBase, 0);                       // disable module
     DrvIicSetTar(i2cBase, addr);               // set target address
     DrvIicCon(i2cBase, 1, 0, 1, 1);             // configure as master, do not use 10 bit adr, slow speed, restart enable
     DrvIicSetSpeedSlow(i2cBase, 1000,2000);    // set speed registers
     DrvIicEn(i2cBase, 1);                       // enbale module
}

/* ***********************************************************************//**
   *************************************************************************
I2C test
@{
----------------------------------------------------------------------------
@{
@param
     i2cBase - base address of the i2s module
@param
     addr - address of the slave
@return
@{
info:  This function exists only to make life easier for converter.
       TODO:  remove when converter project does something more appropriate
@}
************************************************************************* */
void DrvIicTestInitFast(u32 i2cBase, u32 addr)
{
     DrvIicEn(i2cBase, 0);                       // disable module
     DrvIicSetTar(i2cBase, addr);               // set target address
     DrvIicCon(i2cBase, 1, 0, 1, 1);             // configure as master, do not use 10 bit adr, slow speed, restart enable
     DrvIicSetSpeedSlow(i2cBase, 700, 1300);    // set speed registers
     DrvIicEn(i2cBase, 1);                       // enbale module
}

/* ***********************************************************************//**
   *************************************************************************
I2C write continuous EEPROM
@{
----------------------------------------------------------------------------
@}
@param
     i2cBase - base address of the i2c module
@param
     *addr16- address in EEPROM where the data will be stored
@param
     *data - buffer where data is
@param
     size - size of the transfer
@return
@{
info:
     this should be used for continous writing
@}
************************************************************************* */
void DrvIicEepromWr(u32 i2cBase, u32 addr16, u8 *data, u32 size)
{
    u32 n;
    int pendingWrite, icBit;
    u32 i;


    while(size) {
        n = 0x20 - (addr16& 0x1F);
        if (n > size)
            n = size;

        SET_REG_WORD(i2cBase + IIC_DATA_CMD, (addr16>> 8) & 0xFF);
        SET_REG_WORD(i2cBase + IIC_DATA_CMD, addr16& 0xFF);

        for (i = 0 ; i < n ; ++i)
        {
            while (!(GET_REG_WORD_VAL(i2cBase + IIC_STATUS) & IIC_STATUS_TX_FIFO_NOT_FULL))        /* Wait while FIFO full */
                ;
            SET_REG_WORD(i2cBase + IIC_DATA_CMD, *data & 0xFF);
            ++data;


        }

        while(GET_REG_WORD_VAL(i2cBase + IIC_TXFLR) != 0)                                          /* Wait while there are still data to send */
            ;
        while( (GET_REG_WORD_VAL(i2cBase + IIC_STATUS) & IIC_STATUS_ACTIVITY_MASTER) != 0 )        /* Wait while there is activity on the master */
            ;

        do {
            SET_REG_WORD(IIC1_DATA_CMD, 0x23);                                                      /* Send a dummy word */
            while(GET_REG_WORD_VAL(i2cBase + IIC_TXFLR) != 0)                                      /* Wait while there are still data to send */
                ;
            while( (GET_REG_WORD_VAL(i2cBase + IIC_STATUS) & IIC_STATUS_ACTIVITY_MASTER) != 0 )    /* Wait while there is activity on the master */
                ;
            if(( pendingWrite = ((GET_REG_WORD_VAL(i2cBase + IIC_RAW_INTR_STAT)) & IIC_IRQ_TX_ABRT )))/* Wait for TX_ABRT int and clear it */
                GET_REG_WORD(i2cBase + IIC_CLR_TX_ABRT, icBit);
        } while( pendingWrite );
        addr16+= n;
        size-= n;
    }
}

/* ***********************************************************************//**
   *************************************************************************
I2C read continuous EEPROM
@{
----------------------------------------------------------------------------
@}
@param
     i2cBase - base address of the i2c module
@param
     *addr16- address in EEPROM from where the data will be read
@param
     *data - buffer where data will be written
@param
     size - size of the transfer
@return
@{
info:
     this should be used for continous reading
@}
************************************************************************* */
void DrvIicEepromRd(u32 i2cBase, u32 addr16, u8 *data, u32 size)
{
    u32 n;
    u32 i;

    SET_REG_WORD(i2cBase + IIC_DATA_CMD, (addr16 >> 8) & 0xFF);
    SET_REG_WORD(i2cBase + IIC_DATA_CMD, addr16 & 0xFF);

    while(size) {
        n = size;
        if (size > 32)
            n = 32;

        for (i=0 ; i < n ; ++i)
        {
            while (!(GET_REG_WORD_VAL(i2cBase + IIC_STATUS) & IIC_STATUS_TX_FIFO_NOT_FULL))        /* Wait while FIFO full */
                ;
            SET_REG_WORD(i2cBase + IIC_DATA_CMD, 0x100);
        }

        while(GET_REG_WORD_VAL(i2cBase + IIC_RXFLR) == 0)                                          /* Wait while there is no data received */
            ;
        for(i =0; i < n; ++i) {
            while (!(GET_REG_WORD_VAL(i2cBase + IIC_STATUS) & IIC_STATUS_RX_FIFO_NOT_EMPTY))       /* Wait if the RX_FIFO becomes empty */
                ;
            *data = GET_REG_WORD_VAL(i2cBase + IIC_DATA_CMD);
            data++;
        }
        size -= n;
    }
}


/*************************************************************************
I2C set fifo threshold level
@{
----------------------------------------------------------------------------
@{
@param
     i2cBase - base address of the i2c module
@param
     thrLvl - desired threshold level
@return
@{
info:
@}
************************************************************************* */

static void drvIicSetFifoTl(u32 i2cBase, u32 thrLvl)
{
    SET_REG_WORD(i2cBase + IIC_RX_TL, thrLvl);
    SET_REG_WORD(i2cBase + IIC_TX_TL, thrLvl);
}

/* ************************************************************************
I2C - Scaler of the SCL
@{
----------------------------------------------------------------------------
@{
@param
     timeNs - time period(ns) the SCL has to be LOW/HIGH
@return
    Scaler number that will be written in the corresponding register
@{
info:
@}
************************************************************************* */

static u32 iicScaler( u32 timeNs ) {
  return ( timeNs * DrvCprGetSysClockKhz()  + 500000 )/ 1000000;
}

/*************************************************************************
I2C - initialize as master
@{
----------------------------------------------------------------------------
@{
@param
     i2cBase - base address of the i2c module
@param
     addr - address of the slave
@param
     thl - threshold level
@param
     speed - speed of the transfer: 1 - Standard; 2-Fast
@return
@{
info:
@}
************************************************************************* */
void DrvIicMasterInit(u32 i2cBase, u32 addr, u8 thl, u32 speed)
{
    DrvIicEn(i2cBase, 0);                                                        /* Disbale module */
    DrvIicSetTar(i2cBase, addr);                                                /* Set target address */
    DrvIicCon(i2cBase, 1, 0, speed, 1);                                          /* Master, 7 bit adr, Sr enable */
    drvIicSetFifoTl(i2cBase, thl);                                             /* Set the 2 fifo's threshold level */
    if(speed == 0x2)                                                                /* Fast mode */
        DrvIicSetSpeedFast(i2cBase, iicScaler(600), iicScaler(1300));
    else // 0x1                                                                     /* Standard mode */
        DrvIicSetSpeedSlow(i2cBase, iicScaler(4000), iicScaler(4700));
    DrvIicEn(i2cBase, 1);                                                        /* Enable module */
}

/*************************************************************************
I2C - initialize as slave
@{
----------------------------------------------------------------------------
@{
@param
     i2cBase - base address of the i2c module
@param
     addr - address of the slave
@param
     thl - threshold level
@param
     thl - speed of the transfer
@return
@{
info:
@}
************************************************************************* */
void DrvIicSlaveInit(u32 i2cBase, u32 addr, u8 thl, u32 speed)
{
    DrvIicEn(i2cBase, 0);                                                        /* disbale module */
    DrvIicSetSar(i2cBase, addr);                                                /* set target address */
    DrvIicCon(i2cBase, 0, 0, speed, 1);                                          /* configure as slave, do not use 10 bit adr, slow speed, restart enable */
    drvIicSetFifoTl(i2cBase, thl);                                             /* set the 2 fifo's threshold level */
    if(speed == 0x2)
        DrvIicSetSpeedFast(i2cBase, iicScaler(600), iicScaler(1300));
    else // 0x1
        DrvIicSetSpeedSlow(i2cBase,  iicScaler(4000), iicScaler(4700));
    DrvIicEn(i2cBase, 1);                                                        /* enable module */
}

/* ************************************************************************
I2C assemble bytes in a word
@{
----------------------------------------------------------------------------
@{
@param
     i2cBase - base address of the i2c module
@param
     size - number of bytes to be assembled
@return
@{
info:
@}
************************************************************************* */

static u32 drvIicWordAsm (u32 i2cBase, u32 size)
{
    u32 i;
    int data = 0;
    u32 icDataCmd;
    
    for(i = 0; i < size; i++) {
        GET_REG_WORD(i2cBase + IIC_DATA_CMD, icDataCmd);
        data |= ((icDataCmd & 0xFF) << (8*i));
    }
    return data;
}

/* ************************************************************************
I2C disassemble a word in bytes
@{
----------------------------------------------------------------------------
@{
@param
     i2cBase - base address of the i2c module
@param
     size - number of bytes to be disassembled
@return
@{
info:
@}
**************************************************************************/

static void drvIicWordDisasm (u32 i2cBase, u32 data, u32 size)
{
    u32 i;
    u32 icDataCmd;
    for(i = 0; i < size; i++) {
        icDataCmd = (data >> (8*i)) & 0xFF;
        SET_REG_WORD(i2cBase + IIC_DATA_CMD, icDataCmd);
    }
}

/* **************************************************************************
I2C Tx Abort Interrupt Routine
@{
----------------------------------------------------------------------------
@{
@param
     i2cBase - base address of the i2c module
@return
@{
info:
@}
************************************************************************* */

static void txAbrtIsr(u32 i2cBase)
{
/*
   MAIN GOAL OF THE FUNCTION :
        Prints to the tester that the TX ABRT INT
        asserted and tries to clear it.
        OPT : Prints the source of the Interrupt(OFF)
*/
    u32 icBit;
    //printf("TX_ABRT Interrupt asserted\n");

    GET_REG_WORD(i2cBase + IIC_TX_ABRT_SOURCE, icRawIntrStat);

    /* Check to see what caused it and clear it (Decomment the next to lines:)
    DEBUG_INFO("The source of TX_ABRT interupt is:");
    ACTUAL_DATA(icRawIntrStat); */

    GET_REG_WORD(i2cBase + IIC_CLR_TX_ABRT, icBit);                               /* Clear the TX_ABRT Interrupt */
    GET_REG_WORD(i2cBase + IIC_RAW_INTR_STAT, icRawIntrStat);
    if ((GET_REG_WORD_VAL(i2cBase + IIC_RAW_INTR_STAT) & IIC_IRQ_TX_ABRT) != 0) {
        //printf("The source of TX_ABRT Interrupt could not be cleared\n");
        //printf("Exiting Test ...\n");
        exit(4);
    }
}

/* ************************************************************************
I2C - Recovery from error functions(resets the I2C module)
@{
----------------------------------------------------------------------------
@{
@param
     i2cBase - base address of the i2c module
@return
@{
info:
@}
************************************************************************* */
static void i2cRecovery(u32 i2cBase)
{
    u32 data;

    DrvIicEn(i2cBase, 0x0);                                                      /* Disable the Slave */
    GET_REG_WORD (CPR_BLK_RST0_ADR, data);                                          /* Reset I2C */
    SET_REG_WORD (CPR_BLK_RST0_ADR, (data & 0xFFFFFB7F));
    SleepTicks(100);
    DrvIicSlaveInit (i2cBase, 0x63, 0xF, 0x2);                                  /* Initialize I2C as slave */

}

/* **************************************************************************
I2C Read Request Interrupt Routine
@{
----------------------------------------------------------------------------
@{
@param
     i2cBase - base address of the i2c module
@param
     srcAddr - Start address from where the master want to read
@param
     size - nr of bytes the master wants to
@return
@{
info:
@}
************************************************************************* */

static void rdReqIsr(u32 i2cBase, u32 srcAddr, u32 size)
{
    u32 data;
    u32 icBit;
    u32 intSize;
    u32 i;

/* MAIN GOAL OF THE FUNCTION :
        Read data from memory, deassemble it and
        write it in the I2C SLAVE's TX_FIFO */

    GET_REG_WORD(i2cBase + IIC_CLR_RD_REQ, icBit);                                            /* Clear RD_REQ interrupt */
    if(((GET_REG_WORD_VAL(i2cBase + IIC_RAW_INTR_STAT)) & IIC_IRQ_RD_REQ))                         /* Check to see if RD_REQ Int cleared */
        exit(5); // Error state
        //printf("ERROR: RD_REQ Int NOT CLEARED cleared\n");
    intSize = size/4;                                                                          /* Compute the nr of WD requested by master */
    for(i = 0; i < intSize; i++) {
        GET_REG_WORD(srcAddr, data);                                                           /* Get a WD from memory */
        while((GET_REG_WORD_VAL(i2cBase + IIC_TXFLR)) > 0xC)                                     /* Wait while fifo level is bigger than 12 */
            ;
        drvIicWordDisasm(i2cBase, data, 4);                                                 /* Dissassemble the WD into B and WR them in the TX FIFO */
        srcAddr += 4;                                                                          /* Increment the read address */
    }
    if((intSize = size % 4)){
        GET_REG_WORD (srcAddr, data);                                                          /* Read the last word that contains the needed bytes */
        while(GET_REG_WORD_VAL(i2cBase + IIC_TXFLR) > 0xC)                                     /* Wait while fifo level bigger than 12 */
            ;
        drvIicWordDisasm(i2cBase, data, intSize);                                          /* Dissassemble the WD into B and WR them in the TX FIFO */
    }
}

/* **************************************************************************
I2C Read from memory function
@{
----------------------------------------------------------------------------
@{
@param
     i2cBase - base address of the i2c module
@param
     srcAddr - Start address from where the master want to read
@param
     size - nr of bytes the master wants to
@return
@{
info:
@}
************************************************************************* */
static int drvIicBulkRead(u32 i2cBase, u32 addr, u32 size)
{
    int i;
/* MAIN GOAL OF THE FUNCTION :
        a. Calls the TX ABORT Int Service Routine
        b. Calls the RD REQUEST Int Service Routine
        c. Waits for transfer to complete
*/
    if(GET_REG_WORD_VAL(i2cBase + IIC_RAW_INTR_STAT) & IIC_IRQ_TX_ABRT)                            /* Check to see if the ABRT Interrupt is asserted */
        txAbrtIsr(i2cBase);
    rdReqIsr(i2cBase, addr, size);                                                               /* Service the RD_REQ Interrupt */

    icRawIntrStat = 0;
    i = 0;
    while(GET_REG_WORD_VAL(i2cBase + IIC_STATUS) & IIC_STATUS_ACTIVITY_SLAVE){                     /* Wait for Slave to enter IDLE state */
        if(GET_REG_WORD_VAL(i2cBase + IIC_STATUS) & IIC_STATUS_TX_FIFO_EMPTY){                     /* If RX_FIFO Empty */
            icRawIntrStat |= (GET_REG_WORD_VAL(i2cBase + IIC_RAW_INTR_STAT) & IIC_IRQ_RX_DONE);
            NOP;
            i++;
            if((i == 100) && !(icRawIntrStat)){                                                   /* If RX_DONE did not assert in 100 NOPS */
                i2cRecovery(i2cBase);
                exit(5);
                return -1;
            }
        }
    }
    return 0;
/*  If we reach this point data was transfered successfuly to the master */
}

/* **************************************************************************
I2C Write to memory function
@{
----------------------------------------------------------------------------
@{
@param
     i2cBase - base address of the i2c module
@param
     srcAddr - Start address from where the master want to read
@param
     size - nr of bytes the master wants to
@return
@{
info:
@}
************************************************************************* */

static void drvIicBulkWrite(u32 i2cBase, u32 destAddr, u32 size)
{
    u32 intSize;
    u32 i;

/* MAIN GOAL OF THE FUNCTION :
        Gets data from SLAVE's RX_FIFO assemble
        it and writes it in the memory */

    intSize = size/4;                                                              /* Compute the number of words to be transferred to the master */

    for(i = 0; i < intSize; i++){
        while(GET_REG_WORD_VAL(i2cBase + IIC_RXFLR) < 4)                           /* Wait for Slave's TX FIFO to contain 4 bytes. */
            ;
        SET_REG_WORD(destAddr, drvIicWordAsm(i2cBase, 4));                     /* Write the word in the memory */
        destAddr += 4;                                                             /* Increment address */
    }

    /*
    At this point we only can have less than 4 bytes
    so we can wait for the transaction to complete
    because there is no risk of loosing data
    */

    while ((GET_REG_WORD_VAL(i2cBase + IIC_STATUS) & IIC_STATUS_ACTIVITY_SLAVE))                   /* Wait for all the DATA bytes to arrive */
            ;
    intSize = size % 4;                                                                            /* Compute the number of bytes remanined to be transferred */
    if (intSize) {                                                                                 /* More bytes that can not form a word? */
        SET_REG_WORD(destAddr, drvIicWordAsm(i2cBase, intSize));                              /* Write the word in the memory */
    }
    printf("Data was transfered successfuly to the slave\n");
}

/***************************************************************************
I2C Functions that serves the I2C Interrupt
@{
----------------------------------------------------------------------------
@{
@param
     i2cBase - base address of the i2c module
@return
@{
info:
@}
************************************************************************* */

void DrvI2cSlvIsr(u32 i2cBase)
{
    u32 addr;
    u32 size; // 16 bits

/*  MAIN GOAL OF THE FUNCTION :
        1. Gets the first 6 bytes written by the master
        and assembles them into address and lenght.
        2. Waits to see what transfer will be required
        from the master and calls the for the apprpiate
        function
*/
    while(GET_REG_WORD_VAL(i2cBase + IIC_RXFLR) < 6)                                               /* Wait for the first 6 bytes to arrive */
        ;
    addr = drvIicWordAsm(i2cBase, 4);                                                           /* The first four bytes will represent the destination address */
    size = drvIicWordAsm(i2cBase, 2);                                                           /* The next 2 bytes represents the size of the transfer */

    do {                                                                                            /* Check if it is a RD or WR request waiting for: */
        GET_REG_WORD(i2cBase + IIC_RAW_INTR_STAT, icRawIntrStat);                               /*      a. RD_REQ interrupt assertion or */
        GET_REG_WORD(i2cBase + IIC_RXFLR, icRxflr);                                               /*      b. The Receive FIFO Level changes */
    } while((icRxflr == 0) && ((icRawIntrStat & IIC_IRQ_RD_REQ) == 0));

    if (icRawIntrStat & IIC_IRQ_RD_REQ) {                                                        /* At this poit one condition is fufilled. Check to see which one */
        drvIicBulkRead(i2cBase, addr, size);                                                    /* A read transfer was requested by the master */
    } else {
        drvIicBulkWrite(i2cBase, addr, size);                                                   /* A write transfer was requested by the master */
    }
}

/**
 * @fn u32 DrvIicProgramDev( u32 i2cBase, u32 sclFreqKhz, u32 i2cAddr, u32 *setupArray16[2], u32 items );
 *     Initializes an i2c on the board
 * @param i2cBase - bus addr of the i2c module to use
 * @param sclFreqKhz - i2c freq(usualy 50..400 in khz)\n
 *                       it's tipical of the slave spec\n
 * @param i2cAddr - slave's device address on the i2c bus
 * @param setupArray16 - an table of N items each being an array of 2 words\n
 *                        16bit addr and 8bit data
 * @param items - number of items in the above array
 * @return Function returns 0 on success or Abort Source on error
 * Note. User must treat the return value, as it means peripheral setup has failed\n
 *       the i2c addr was wrong or another master was doing transactions on the bus
 */
u32 DrvIicProgramDev( u32 i2cBase, u32 sclFreqKhz, u32 i2cAddr, u32 *setupArray16[2], u32 items ) {
  u32 i, speed = 1;

  DrvIicEn(i2cBase, 0);                       // disable module
  DrvIicSetTar(i2cBase, i2cAddr);               // set target address
  if( sclFreqKhz > 99 ) {// Fast mode
    DrvIicSetSpeedFast(i2cBase, iicScaler(600), iicScaler(1300));
    speed++;
  } else // 0x1               // Standard mode
    DrvIicSetSpeedSlow(i2cBase, iicScaler(4000), iicScaler(4700));
    
  DrvIicCon(i2cBase, 1, 0, speed, 1);             // configure as master, do not use 10 bit adr, slow speed, restart enable
  DrvIicEn(i2cBase, 1);                       // enbale module

  for( i=0; i < items; ++i) {
    if(DrvIicWr16(i2cBase, setupArray16[i][0],setupArray16[i][1])) {  // if it failed once
      if(DrvIicWr16(i2cBase, setupArray16[i][0],setupArray16[i][1]))  // try again
        return -1;                                           // than report error
    }
  }
  return 0;
}
/**
 * @fn u32 DrvIicProgramDev2( u32 sclFreqKhz, u32 i2cAddr, u32 *setupArray16[2], u32 items );
 *     Same as DrvIicProgramDev but programs 2 identical devices using both i2c modules
 * @param sclFreqKhz - i2c freq(usualy 50..400 in khz)\n
 *                       it's tipical of the slave spec\n
 * @param i2cAddr - slave's device address on the i2c bus
 * @param setupArray16 - an table of N items each being an array of 2 words\n
 *                        16bit addr and 8bit data
 * @param items - number of items in the above array
 * @return Function returns 0 on success or Abort Source on error
 * Note. User must treat the return value, as it means peripheral setup has failed\n
 *       the i2c addr was wrong or another master was doing transactions on the bus
 */
u32 DrvIicProgramDev2( u32 sclFreqKhz, u32 i2cAddr, u32 *setupArray16[2], u32 items ) {
  u32 i, speed = 1, txAbrtSrc;
  volatile u32 clr;

  DrvIicEn(IIC1_BASE_ADR, 0);                       // disable module
  DrvIicEn(IIC2_BASE_ADR, 0);                       // disable module
  DrvIicSetTar(IIC1_BASE_ADR, i2cAddr);           // set target address
  DrvIicSetTar(IIC2_BASE_ADR, i2cAddr);           // set target address
  if( sclFreqKhz > 99 ) {// Fast mode
    DrvIicSetSpeedFast(IIC1_BASE_ADR, iicScaler(600), iicScaler(1300));
    DrvIicSetSpeedFast(IIC2_BASE_ADR, iicScaler(600), iicScaler(1300));
    speed++;
  } else {                 // Standard mode
    DrvIicSetSpeedSlow(IIC1_BASE_ADR, iicScaler(4000), iicScaler(4700));
    DrvIicSetSpeedSlow(IIC2_BASE_ADR, iicScaler(4000), iicScaler(4700));
  }
  DrvIicCon(IIC1_BASE_ADR, 1, 0, speed, 1);         // configure as master, do not use 10 bit adr, slow speed, restart enable
  DrvIicCon(IIC2_BASE_ADR, 1, 0, speed, 1);         // configure as master, do not use 10 bit adr, slow speed, restart enable
  DrvIicEn(IIC1_BASE_ADR, 1);                       // enbale module
  DrvIicEn(IIC2_BASE_ADR, 1);                       // enbale module

  while (!(GET_REG_WORD_VAL(IIC1_BASE_ADR + IIC_STATUS) & IIC_STATUS_TX_FIFO_EMPTY))
    ;
  while (!(GET_REG_WORD_VAL(IIC2_BASE_ADR + IIC_STATUS) & IIC_STATUS_TX_FIFO_EMPTY))
    ;
  for( i=0; i < items; ++i) {
    SET_REG_WORD(IIC1_BASE_ADR + IIC_DATA_CMD, (setupArray16[i][0]>>8) & 0xFF);
    SET_REG_WORD(IIC1_BASE_ADR + IIC_DATA_CMD, setupArray16[i][0]      & 0xFF);
    SET_REG_WORD(IIC1_BASE_ADR + IIC_DATA_CMD, setupArray16[i][1]      & 0xFF);
    SET_REG_WORD(IIC2_BASE_ADR + IIC_DATA_CMD, (setupArray16[i][0]>>8) & 0xFF);
    SET_REG_WORD(IIC2_BASE_ADR + IIC_DATA_CMD, setupArray16[i][0]      & 0xFF);
    SET_REG_WORD(IIC2_BASE_ADR + IIC_DATA_CMD, setupArray16[i][1]      & 0xFF);
    while (!(GET_REG_WORD_VAL(IIC1_BASE_ADR + IIC_STATUS) & IIC_STATUS_TX_FIFO_EMPTY) ||
           !(GET_REG_WORD_VAL(IIC2_BASE_ADR + IIC_STATUS) & IIC_STATUS_TX_FIFO_EMPTY)||
           (GET_REG_WORD_VAL(IIC1_BASE_ADR + IIC_STATUS) & IIC_STATUS_ACTIVITY)||
           (GET_REG_WORD_VAL(IIC2_BASE_ADR + IIC_STATUS) & IIC_STATUS_ACTIVITY))
      ;
    if((GET_REG_WORD_VAL(IIC1_BASE_ADR + IIC_INTR_STAT) & IIC_IRQ_TX_ABRT)) {
      txAbrtSrc = GET_REG_WORD_VAL(IIC1_BASE_ADR + IIC_TX_ABRT_SOURCE);
      clr = GET_REG_WORD_VAL(IIC1_BASE_ADR + IIC_CLR_INTR);
      return txAbrtSrc;
    } else if((GET_REG_WORD_VAL(IIC2_BASE_ADR + IIC_INTR_STAT) & IIC_IRQ_TX_ABRT)) {
      txAbrtSrc = GET_REG_WORD_VAL(IIC2_BASE_ADR + IIC_TX_ABRT_SOURCE);
      clr = GET_REG_WORD_VAL(IIC2_BASE_ADR + IIC_CLR_INTR);
      return txAbrtSrc;
    }
  }
  return 0;
}

//===[ IIC BIT BANG FUNCTIONS ]================================================
#include "DrvGpio.h"
static u32 gSdaPin,
           gSclPin,
           gTar,
           gHCntl, gLCntl;

static  u32 delay1,
            delay2,
            delay3;

void DrvIicBitbangInit( u32 sdaPin, u32 sclPin, u32 tar,  u32 hCntl, u32 lCntl ) {

  gSdaPin = sdaPin;
  gSclPin = sclPin;
  gTar     = tar;
  gHCntl   = hCntl;
  gLCntl   = lCntl;
  delay1 = gLCntl/ 21 + gHCntl / 22;
  delay2 = ( gLCntl - gHCntl) / 6; // lCntl-hCntl-fixed_delay
  delay3 = gHCntl / 6;

  DrvGpioMode( gSdaPin, D_GPIO_MODE_7 | D_GPIO_DIR_IN );
  DrvGpioMode( gSclPin, D_GPIO_MODE_7 | D_GPIO_DIR_IN );
  DrvGpioSetPinLo( gSdaPin );
  DrvGpioSetPinLo( gSclPin );
  //both pins are input @ this point( REN pad setting is presumed )
  //line toggling will be done by setting OE bit
}

#define setIicPinHi( pinr ) DrvGpioMode( pinr, D_GPIO_MODE_7 | D_GPIO_DIR_IN )
#define setIicPinLo( pinr ) DrvGpioMode( pinr, D_GPIO_MODE_7 )

static inline void drvIicBitbangWr( u32 byte ) {
  int i;

  for(i=0; i<8; i++,byte <<= 1 ) {
    SleepTicks( delay1 ); // wait fixed_delay
    DrvGpioMode( gSdaPin, D_GPIO_MODE_7 | ((byte & 0x80 )<< 4 )); //set data [hi]
    SleepTicks( delay2 );// wait lcntl-hcntl-fixed_delay
    setIicPinHi( gSclPin ); //set clk hi
    SleepTicks( delay3 ); //wait hcnt
    setIicPinLo( gSclPin ); //set clk lo
    //SleepTicks( delay1 ); // wait fixed_delay
    //set data [low]
  }
  SleepTicks( delay2 ); // wait fixed_delay
  setIicPinHi( gSdaPin ); // free sda pin for ACK
}

static inline u32 drvIicBitbangRd() {
  volatile u32 data;
  int i;

  data = 0;
  for(i=0; i<8; i++ ) {
    SleepTicks( delay1 ); // wait fixed_delay
    SleepTicks( delay2 );// wait lcntl-hcntl-fixed_delay
    setIicPinHi( gSclPin ); //set clk hi
    data |= DrvGpioGet( gSdaPin ); //read data
    SleepTicks( delay3 ); //wait hcnt
    setIicPinLo( gSclPin ); //set clk lo
    //SleepTicks( delay1 ); // wait fixed_delay
    //set data [low]
    if( i < 7 )
      data <<= 1;
  }
  SleepTicks( delay2 ); // wait fixed_delay
  //*data >>= 1;
  return data;
}

static inline u32 drvIicBitbangCkAck() {
  u32 dataLine;
  //toggle SCL one
  SleepTicks( delay2 );// wait lcntl-hcntl-fixed_delay
  setIicPinHi( gSclPin ); //set clk hi
  dataLine = DrvGpioGet( gSdaPin ); //read data
  SleepTicks( delay3 ); //wait hcnt
  setIicPinLo( gSclPin ); //set clk lo
  SleepTicks( delay2 ); // wait fixed_delay

  return dataLine;  //return the value on SDA line( aka SDA==1 means NACK )
}

static inline void drvIicBitbangS() {
  // SCL=1, SDA=0
  setIicPinHi( gSclPin );  // Clk must be high before doing a start
  SleepTicks( delay3 ); // insert one CLK delay here

  setIicPinLo( gSdaPin );
  SleepTicks( delay3 ); // insert one CLK delay here
  setIicPinLo( gSclPin );
}

static inline void drvIicBitbangP() {

  setIicPinLo( gSdaPin ); // SDA must be low before we bring SCL high for Stop bit
  SleepTicks( delay3 ); // insert one CLK delay here
  setIicPinHi( gSclPin );
  SleepTicks( delay3 ); // insert one CLK delay here
  setIicPinHi( gSdaPin );
}


u32 drvIicBitbangData( u32 addr, u8 * dat, u32 size) {
  register u32 state = 0;
  u32 i;

  drvIicBitbangS();

  drvIicBitbangWr( gTar << 1 ); // send TAR + rwbit( == 0 for write)
  if( drvIicBitbangCkAck()) {// check ACK here
    state++;       // errno
    goto STOP_PKG; // NACK no sense to continue
  }
  drvIicBitbangWr( addr ); //send byte 1
  if( drvIicBitbangCkAck()) {// check ACK here
    state++;       // errno
    goto STOP_PKG; // NACK no sense to continue
  }

  for (i=0;i<size;i++)
	  {
	  drvIicBitbangWr( dat[i] );
	  if( drvIicBitbangCkAck()) {// check ACK here
		state++;       // errno
		goto STOP_PKG; // NACK no sense to continue
	  }
  }
STOP_PKG:
  drvIicBitbangP();
  SleepTicks( delay2 );// wait lcntl-hcntl-fixed_delay
  SleepTicks( delay2 );// wait lcntl-hcntl-fixed_delay
  return state;
}

static inline void drvIicBitbangSendAck() {
  // Set SDA Low to acknowlege
  setIicPinLo( gSdaPin );
  //toggle SCL one
  SleepTicks( delay2 );// wait lcntl-hcntl-fixed_delay
  setIicPinHi( gSclPin ); //set clk hi
  SleepTicks( delay3 ); //wait hcnt
  setIicPinLo( gSclPin ); //set clk lo
  SleepTicks( delay2 ); // wait fixed_delay
  setIicPinHi( gSdaPin );  // Release SDA
  SleepTicks( delay2 ); // wait fixed_delay

  return;
}


inline int DrvIicBitbangRead (u32 addr, u8* data, u32 size) {
   return DrvIicBitbang(READ, addr, data, size);
}

int DrvIicBitbang(EepromActionType action,u32 addr, u8* data, u32 size) {
  int state = 0;    // Positive state means timeout,negative state is due to error bytes
  unsigned char data_val;
  u32 error_bytes=0;
  u32 i;
  drvIicBitbangS();

  // Setup Read from Address
  drvIicBitbangWr( gTar << 1 ); // send TAR + rwbit( == 0 for write)
  if( drvIicBitbangCkAck()) {// check ACK here
    state++;       // errno
    goto STOP_PKG; // NACK no sense to continue
  }
  drvIicBitbangWr( addr ); //send byte 1
  if( drvIicBitbangCkAck()) {// check ACK here
    state++;       // errno
    goto STOP_PKG; // NACK no sense to continue
  }

  drvIicBitbangS();

  // Now read the data
  drvIicBitbangWr( gTar << 1 | 1); // send TAR + rwbit( == 1 for read)
  if( drvIicBitbangCkAck()) {// check ACK here
    state++;       // errno
    goto STOP_PKG; // NACK no sense to continue
  }

  for (i=0;i<size;i++){
	  data_val = drvIicBitbangRd();

	  if (action == READ)
		  data[i] = data_val;
	  else if (action == VERIFY)
		  {
		  if (data[i] != data_val)
			  error_bytes++;
		  }

	  // Acknowledge the read to enable sequential read
	  drvIicBitbangSendAck();
	  }

STOP_PKG:
  drvIicBitbangP();
  SleepTicks( delay2 );// wait lCntl-hCntl-fixed_delay
  SleepTicks( delay2 );// wait lCntl-hCntl-fixed_delay

  if (state != 0)
	  return state; // Timeout
  else
	  return -error_bytes; // Return negative value to indicate byte errors
}


u32 DrvIicBitbangWritePage (u32 addr, u8* data, u32 size, u32 pageSize) {
	register int state = 0;
    u32 n;
	u32 timeout;

    while(size)
    {
        n = pageSize ;
        if (n > size)
            n = size;

		timeout=0;
		do
			{
			state = drvIicBitbangData(addr, data,n);
			}while ((timeout++ < I2C_PAGE_WRITE_TIMEOUT) && (state != 0));

		addr += n;
		data += n;
		size -= n;
    }

	return state;
}


inline int DrvIicBitbangReadEeprom (u32 addr, u8* data, u32 size) {
	return DrvIicBitbangEeprom(READ, addr, data, size);
}

int DrvIicBitbangEeprom(EepromActionType action,u32 addr, u8* data, u32 size) {
	int state = 0;
	u32 timeout;
	timeout=0;

	do
		{
		state = DrvIicBitbang(action,addr,data,size);
		}while ((timeout++ < I2C_READ_TIMEOUT) && (state > 0)); // repeat while getting timeouts

	return state;
}


u32 DrvIicBitbangWrN(u32 addr, u8* data, u32 size) {
  register u32 state = 0;
  u32 i = 0;

  drvIicBitbangS();

  drvIicBitbangWr( gTar << 1 ); // send TAR + rwbit( == 0 for write)
  if( drvIicBitbangCkAck()) {// check ACK here
    state++;       // errno
    goto STOP_PKG; // NACK no sense to continue
  }

  drvIicBitbangWr( addr ); //send address byte
  if( drvIicBitbangCkAck()) {// check ACK here
    state++;       // errno
    goto STOP_PKG; // NACK no sense to continue
  }

  for (i=0; i<size; i++) {
    if (i>0 && (i%7==0)) {
      drvIicBitbangP();
      SleepTicks( delay2 );// wait lcntl-hcntl-fixed_delay
      SleepTicks( delay2 );// wait lcntl-hcntl-fixed_delay

      drvIicBitbangWr(addr+i); //send address byte
      if( drvIicBitbangCkAck()) {// check ACK here
        state++;       // errno
        goto STOP_PKG; // NACK no sense to continue
      }
    } else {
      drvIicBitbangWr( (u32) (data[i]) ); //send data bytes
      if( drvIicBitbangCkAck()) {// check ACK here
        state++;       // errno
        goto STOP_PKG; // NACK no sense to continue
      }
    }
  }

STOP_PKG:
  drvIicBitbangP();
  SleepTicks( delay2 );// wait lcntl-hcntl-fixed_delay
  SleepTicks( delay2 );// wait lcntl-hcntl-fixed_delay
  return state;
}

u32 DrvIicBitbangWr2( u32 dat1, u32 dat2 ) {
  register u32 state = 0;

  drvIicBitbangS();

  drvIicBitbangWr( gTar << 1 ); // send TAR + rwbit( == 0 for write)
  if( drvIicBitbangCkAck()) {// check ACK here
    state++;       // errno
    goto STOP_PKG; // NACK no sense to continue
  }
  drvIicBitbangWr( dat1 ); //send byte 1
  if( drvIicBitbangCkAck()) {// check ACK here
    state++;       // errno
    goto STOP_PKG; // NACK no sense to continue
  }

  drvIicBitbangWr( dat2 ); //send byte 2
  if( drvIicBitbangCkAck()) {// check ACK here
    state++;       // errno
    goto STOP_PKG; // NACK no sense to continue
  }
STOP_PKG:
  drvIicBitbangP();
  SleepTicks( delay2 );// wait lcntl-hcntl-fixed_delay
  SleepTicks( delay2 );// wait lcntl-hcntl-fixed_delay
  return state;
}

u32 drvIicBitbangWr16( u32 adr16, u32 dat ) {
  register u32 state = 0;
  drvIicBitbangS();

  drvIicBitbangWr( gTar << 1 ); // send TAR + rwbit( == 0 for write)
  if( drvIicBitbangCkAck()) {// check ACK here
    state++;       // errno
    goto STOP_PKG; // NACK no sense to continue
  }
  drvIicBitbangWr( adr16 >> 8 & 0xFF ); //send byte 1
  if( drvIicBitbangCkAck()) {// check ACK here
    state++;       // errno
    goto STOP_PKG; // NACK no sense to continue
  }
  drvIicBitbangWr( adr16 & 0xFF ); //send byte 1
  if( drvIicBitbangCkAck()) {// check ACK here
    state++;       // errno
    goto STOP_PKG; // NACK no sense to continue
  }
  drvIicBitbangWr( dat ); //send byte 2
  if( drvIicBitbangCkAck()) {// check ACK here
    state++;       // errno
    goto STOP_PKG; // NACK no sense to continue
  }
STOP_PKG:
  drvIicBitbangP();
  SleepTicks( delay2 );// wait lcntl-hcntl-fixed_delay
  SleepTicks( delay2 );// wait lcntl-hcntl-fixed_delay
  return state;
}


u32 drvIicBitbangRd8ba( u32 adr ) {
  register u32 state = 0;
  u8 data;
  drvIicBitbangS();

  drvIicBitbangWr( gTar << 1 ); // send TAR + rwbit( == 0 for write)
  if( drvIicBitbangCkAck()) {// check ACK here
    state++;
    goto NACK_RETURN;// NACK no sense to continue
  }
  // could build
  drvIicBitbangWr( adr ); //send byte 1
  if( drvIicBitbangCkAck()) {// check ACK here
    state++;
    goto NACK_RETURN;// NACK no sense to continue
  }
  drvIicBitbangP();
  SleepTicks( delay2 );// wait lcntl-hcntl-fixed_delay
  drvIicBitbangS();

  drvIicBitbangWr( gTar << 1 | 1 ); // send TAR + rwbit( == 1 for read)
  if( drvIicBitbangCkAck()) {// check ACK here
    state++;
    goto NACK_RETURN;// NACK no sense to continue
  }
  data = drvIicBitbangRd();
  drvIicBitbangCkAck(); // we expect NACK here
  drvIicBitbangP();
  return data;
NACK_RETURN:
  drvIicBitbangP(); // dev addr /reg addr NACK
  return (-1 << 16)| state; // error is 32 bit while data is 8 bit
}

u32 drvIicBitbangRd16ba( u32 adr ) {
  register u32 state = 0;
  u8 data;

  drvIicBitbangS();

  drvIicBitbangWr( gTar << 1 ); // send TAR + rwbit( == 0 for write)
  if( drvIicBitbangCkAck()) {// check ACK here
    state++;
    goto NACK_RETURN;// NACK no sense to continue
  }

  drvIicBitbangWr( (adr >> 8) & 0xFF ); //send byte 1
  if( drvIicBitbangCkAck()) {// check ACK here
    state++;
    goto NACK_RETURN;// NACK no sense to continue
  }
  drvIicBitbangWr( adr & 0xFF ); //send byte 1
  if( drvIicBitbangCkAck()) {// check ACK here
    state++;
    goto NACK_RETURN;// NACK no sense to continue
  }
  drvIicBitbangP();
  SleepTicks( delay2 );// wait lcntl-hcntl-fixed_delay
  drvIicBitbangS();

  drvIicBitbangWr( gTar << 1 | 1 ); // send TAR + rwbit( == 1 for read)
  if( drvIicBitbangCkAck()) { // check ACK here
    state++;
    goto NACK_RETURN;// NACK no sense to continue
  }
  data = drvIicBitbangRd();
  drvIicBitbangCkAck(); // we expect NACK here
  drvIicBitbangP();
  return data;
NACK_RETURN:
  drvIicBitbangP(); // dev addr /reg addr NACK
  return (-1 << 16)| state; // error is 32 bit while data is 8 bit
}
