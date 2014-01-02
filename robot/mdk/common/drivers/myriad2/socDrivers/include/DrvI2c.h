///  
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved 
///            For License Warranty see: common/license.txt   
///
/// @brief     Low-level driver for I2C interface
/// 
/// 
/// 
#ifndef _BRINGUP_SABRE_IIC_H_
#define _BRINGUP_SABRE_IIC_H_

#include "mv_types.h"
#include "DrvI2cDefines.h"
/*i2c_base       is one of     IIC1_BASE_ADR          and      IIC2_BASE_ADR   */

typedef enum
{
	READ,
	VERIFY
} EepromActionType;


/***********************************************************************//**
I2C general config



@param
        i2cBase - base address of the i2c module
@param
        Ms - set master/disbale slave
@param
        ADR10 - master and slave 10 bit address modes
@param
        speed - 1 max 100k, 2 max 400k, 3 max 3.4M
@param
        rstEn - restart enable
@return

info:

*/
 
void DrvIicCon(u32 i2cBase, u32 Ms, u32 ADR10, u32 speed, u32 rstEn);

/* ***********************************************************************//**
   *************************************************************************
I2C enable/disable



@param
        i2cBase - base address of the i2c module
@param
        en 
			- 1 enables iic module
			-  0 disables
			.
@return

info:
        Module should be disabled before timing configuration

 ************************************************************************* */
 
void DrvIicEn(u32 i2cBase, u32 en);

/* ***********************************************************************//**
   *************************************************************************
I2C



@param
      i2cBase - base address of the i2c module
@param
      tar - target address
@return

info:
     direct acces to TAR

 ************************************************************************* */
 
void DrvIicSetTar(u32 i2cBase, u32 tar);

 /* ***********************************************************************//**
   *************************************************************************
I2C



@param
      i2cBase - base address of the i2c module
@param
      sar - slave address of the IIC module in slave mode
@return

info:

 ************************************************************************* */
 
void DrvIicSetSar(u32 i2cBase, u32 sar);

/* ***********************************************************************//**
   *************************************************************************
I2C write timeout



@param
     i2cBase - base address of the i2c module
@param
     dat1
@param
     dat2 - data to be written
@param
     timeout - relative timeout
@return

info:
     - can't be used for continuous writing
     - this can be used to send an addressa and a data
     - waits for the fifo to be empty, forces a STOP condition

************************************************************************* */
int DrvIicWr2Timeout(u32 i2cBase, u32 dat1, u32 dat2, u32 timeout);

 /* ***********************************************************************//**
   *************************************************************************
I2C



@param
       i2cBase - base address of the i2c module
@param
       HCNT - high count register
@param
       LCNT - low count register
@return

info:

 ************************************************************************* */
void DrvIicSetSpeedSlow(u32 i2cBase, u32 HCNT, u32 LCNT);

 /* ***********************************************************************//**
   *************************************************************************
I2C



@param
       i2cBase - base address of the i2c module
@param
       HCNT - high count register
@param
       LCNT - low count register
@return

info:

 ************************************************************************* */
 
void DrvIicSetSpeedFast(u32 i2cBase, u32 HCNT, u32 LCNT);

 /* ***********************************************************************//**
   *************************************************************************
I2C



@param
       i2cBase - base address of the i2c module
@param
       HCNT - high count register
@param
       LCNT - low count register
@return

info:

 ************************************************************************* */
void DrvIicSetSpeedHigh(u32 i2cBase, u32 HCNT, u32 LCNT);

 /* ***********************************************************************//**
   *************************************************************************
I2C FIFO depth



@param
      i2cBase - base address of the i2c module
@param
      ptxDepth - tx fifo depth will be stored at that pointer
@param
      prxDepth - rx fifo depth will be stored at that pointer
@return

info:

************************************************************************* */
void DrvIicFifoDepth(u32 i2cBase, u32 *ptxDepth,u32 *prxDepth);

//!@{
/* ***********************************************************************//**
   *************************************************************************
I2C write continuous



@param
     i2cBase - base address of the i2c module
@param
     *addr - address in EEPROM where the data will be stored
@param
     *data - buffer where data is
@param
     size - size of the transfer
@return

info:
     this should be used for continous writing

************************************************************************* */
u32 DrvIicWr(u32 i2cBase, u32 addr, u8 *data, u32 size);

u32 DrvIicWr8ba(u32 i2cBase, u32 addr, u8 *data, u32 size); 
//!@}

/* ***********************************************************************//**
   *************************************************************************
I2C write



@param
     i2cBase - base address of the i2c module
@param
     dat1
@param
     dat2 - data to be written
@return

info:
     - can't be used for continuous writing
     - this can be used to send an addressa and a data
     - waits for the fifo to be empty, forces a STOP condition

************************************************************************* */

u32 DrvIicWr2(u32 i2cBase, u32 dat1, u32 dat2);

/* ***********************************************************************//**
   *************************************************************************
I2C write



@param
     i2cBase - base address of the i2c module
@param
     dat1
@param
     dat2
@param
     dat3 - data to be written
@return

info:
     - can't be used for continuous writing
     - this can be used to send an addressa and two data, or a 2 byte address and a data
     - waits for the fifo to be empty, forces a STOP condition


************************************************************************* */
u32 DrvIicWr3(u32 i2cBase, u32 dat1, u32 dat2, u32 dat3);

/* ***********************************************************************//**
   *************************************************************************
I2C write



@param
     i2cBase - base address of the i2c module
@param
     dat1
@param
     dat2
@param
     dat3
@param
     dat4 - data to be written
@return

info:
     - can't be used for continuous writing
     - this can be used to send a two-byte address and two bytes of data
     - waits for the fifo to be empty, forces a STOP condition


************************************************************************* */
u32 DrvIicWr4(u32 i2cBase, u32 dat1, u32 dat2, u32 dat3, u32 dat4);

/* ***********************************************************************//**
   *************************************************************************
I2C write



@param
     i2cBase - base address of the i2s module
@param
     adr16 - 16 bit register address
@param
     dat - data to be written
@return

info:
     - can't be used for continuous writing
     - waits for the fifo to be empty, forces a STOP condition


************************************************************************* */
u32 DrvIicWr16( u32 i2cBase, u32 adr16, u32 dat );

/**
Write 2 bytes to the bit-bang I2C interface
@param dat1 first byte to be written
@param dat2 second byte to be written
@return errno
*/
u32 DrvIicBitbangWr2( u32 dat1, u32 dat2 );

/* ***********************************************************************//**
   *************************************************************************
I2C read



@param
     i2cBase - base address of the i2s module
@param
     adr - data to be send to the slave device, before reading, usualy some address
           first 8 bits are used
@return
    value read at that address

info:
    this waits for the tx FIFO to be empty

************************************************************************* */
u32 DrvIicRd8ba(u32 i2cBase, u32 adr);

/* ***********************************************************************//**
   *************************************************************************
I2C read



@param
     i2cBase - base address of the i2s module
@param
     addr - data to be send to the slave device, before reading, usualy some address
           first 8 bits are used
@param
     *data - buffer with stored data
@param
     size - size of the transfer
@return
    value read at that address

info:
    this waits for the tx FIFO to be empty

************************************************************************* */
u32 DrvIicRd(u32 i2cBase, u32 addr, u8 *data, u32 size);

/* ***********************************************************************//**
   *************************************************************************
I2C read



@param
     i2cBase - base address of the i2s module
@param
     addr - data to be send to the slave device, before reading, usualy some address
           first 8 bits are used
@param
     *data - buffer with stored data
@param
     size - size of the transfer
@return
    value read at that address

info:
    this waits for the tx FIFO to be empty. Address is on 8 bits, usefull on devices

************************************************************************* */
void DRV_IIC_rd_8ba(u32 i2cBase, u32 addr, u8 *data, u32 size); // address is on 8 bits, usefull on devices
//u32 DRV_IIC_rd_8ba(u32 i2cBase, u32 addr, u8 *data, u32 size); // address is on 8 bits, usefull on devices

/* ***********************************************************************//**
   *************************************************************************
I2C read with timeout



@param
     i2cBase - base address of the i2s module
@param
     adr - data to be send to the slave device, before reading, usualy some address
           first 8 bits are used
@param
     timeout - sets a timout timer after which an error is returned
@return
    value read at that address

info:
    this waits for the tx FIFO to be empty

************************************************************************* */
int DrvIicRd8baTimeout(u32 i2cBase, u32 adr, u32 timeout);

/* ***********************************************************************//**
   *************************************************************************
I2C read



@param
     i2cBase - base address of the i2s module
@param
     adr - data to be send to the slave device, before reading, usualy some address
           first 16 bits are used
@return
    value read at that address

info:
    this waits for the tx FIFO to be empty

************************************************************************* */
u32 DrvIicRd16ba(u32 i2cBase, u32 adr);

/* ***********************************************************************//**
   *************************************************************************
I2C read 16 bits



@param
     i2cBase - base address of the i2s module
@param
     adr - data to be send to the slave device, before reading, usualy some address
           first 8 bits are used
@return
    16 bits, read at that address

info:
    this waits for the tx FIFO to be empty

************************************************************************* */
u32 DrvIicRd8ba16d(u32 i2cBase, u32 adr);


/* ***********************************************************************//**
   *************************************************************************
Bit-bang I2C read 16 bits



@param
     adr - data to be send to the slave device, before reading, usualy some address
           first 8 bits are used
@return
    16 bits, read at that address

************************************************************************* */
u32 DrvIicBitbangRd8ba16d( u32 adr );

/* ***********************************************************************//**
   *************************************************************************
I2C read 16 bits



@param
     i2cBase - base address of the i2s module
@param
     adr - data to be send to the slave device, before reading, usualy some address
           first 16 bits are used
@return
    16 bits, read at that address

info:
    this waits for the tx FIFO to be empty

************************************************************************* */
u32 DrvIicRd16ba16d(u32 i2cBase, u32 adr);

//!@{
/* ***********************************************************************//**
   *************************************************************************
I2C test

@param
     i2cBase - base address of the i2s module
@param
     addr - address of the slave
@return void
info:  This function exists only to make life easier for converter.
@todo  Remove when converter project does something more appropriate
************************************************************************* */
void DrvIicTestInitFast(u32 i2cBase, u32 addr);
void DrvIicTestInit(u32 i2cBase, u32 addr);
//!@}

/**
Initialse a bit-bang I2C interface

@param SDA_pin GPIO pin used for Serial Data Line 
@param SCL_pin GPIO pin used for Serial Clock Line 
@param tar target address
@param hcntl high count register
@param lcntl low count register
*/
void DrvIicBitbangInit( u32 SDA_pin, u32 SCL_pin, u32 tar,  u32 hcntl, u32 lcntl );

/***********************************************************************//**
I2C - initialize as master



@param
     i2cBase - base address of the i2c module
@param
     addr - address of the slave
@param
     thl - threshold level
@param
     speed - speed of the transfer: 1 - Standard; 2-Fast
@return

info:

************************************************************************* */
void DrvIicMasterInit(u32 i2cBase, u32 addr, u8 thl, u32 speed);

/***********************************************************************//**
I2C - initialize as slave



@param
     i2cBase - base address of the i2c module
@param
     addr - address of the slave
@param
     thl - threshold level
@param
     thl - speed of the transfer
@return

info:

************************************************************************* */
void DrvIicSlaveInit(u32 i2cBase, u32 addr, u8 thl, u32 speed);

/***************************************************************************
I2C Functions that serves the I2C Interrupt



@param
     i2cBase - base address of the i2c module
@return

info:

************************************************************************* */
void DrvI2cSlvIsr(u32 i2cBase);

/* ***********************************************************************//**
   *************************************************************************
I2C read continuous EEPROM



@param
     i2cBase - base address of the i2c module
@param
     *addr- address in EEPROM from where the data will be read
@param
     *data - buffer where data will be written
@param
     size - size of the transfer
@return

info:
     this should be used for continous reading

************************************************************************* */
void DrvIicEepromRd(u32 i2cBase, u32 addr, u8 *data, u32 size);

/* ***********************************************************************//**
   *************************************************************************
I2C write continuous EEPROM



@param
     i2cBase - base address of the i2c module
@param
     *addr - address in EEPROM where the data will be stored
@param
     *data - buffer where data is
@param
     size - size of the transfer
@return

info:
     this should be used for continous writing

************************************************************************* */
void DrvIicEepromWr(u32 i2cBase, u32 addr, u8 *data, u32 size);

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
u32 DrvIicProgramDev( u32 i2cBase, u32 sclFreqKhz, u32 i2cAddr, u32 *setupArray16[2], u32 items );

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
u32 DrvIicProgramDev2( u32 sclFreqKhz, u32 i2cAddr, u32 *setupArray16[2], u32 items );

//int DrvIicRd8baTimeout(u32 i2cBase, u32 adr, u32 timeout);
//int DrvIicRd8baTimeout(u32 i2cBase, u32 adr, u32 timeout);

#define I2C_PAGE_WRITE_TIMEOUT (100)
#define I2C_READ_TIMEOUT       (100)
/**
   Bit bang paged write
   @param addr where to write the bytes
   @param data pointer to an array containing input elements
   @param size size of input array
   @param pageSize size of the memory block being written
   @return timeout or a negative value indicating byte errors
*/
u32 DrvIicBitbangWritePage(u32 addr, u8* data, u32 size, u32 pageSize);

/* ***********************************************************************//**
   *************************************************************************
Bit-bang I2C read continuous EEPROM


@param
      addr- address in EEPROM from where the data will be read
@param
      data - buffer where data will be written
@param
      size - size of the transfer
@return timeout or a negative value indicating byte errors

info:
     this should be used for continous reading

************************************************************************* */
int DrvIicBitbangReadEeprom(u32 addr, u8* data, u32 size);

/**
  Continuous read or verify the data received over the bit-bang I2C interface.
  @param action the type of action to execute(read/verify)
  @param addr address to read from
  @param data pointer to the data to verify, or the location where to store the received data
  @param size size in bytes of data array
  @return timeout or a negative value indicating byte errors
*/
int DrvIicBitbangEeprom(EepromActionType action,u32 addr, u8* data, u32 size);

/**
   Read from bit-bang I2C interface
   @param addr address to read from
   @param data pointer to the buffer where to store the data
   @param size size in bytes of data array
   @return timeout or a negative value indicating byte errors
*/
int DrvIicBitbangRead(u32 addr, u8* data, u32 size);

/**
  Read or verify the data received over the bit-bang I2C interface.
  @param action the type of action to execute(read/verify)
  @param addr address to read from
  @param data pointer to the data to verify, or the location where to store the received data
  @param size size in bytes of data array
  @return timeout or a negative value indicating byte errors
*/
int DrvIicBitbang(EepromActionType action,u32 addr, u8* data, u32 size);

/**
 Bit bang write an array of bytes
 @param addr where to write the bytes
 @param data array with data to be written
 @param size size of the array
 @return errno
*/
u32 DrvIicBitbangWrN(u32 addr, u8* data, u32 size);

#endif // _BRINGUP_SABRE_IIC_H_
