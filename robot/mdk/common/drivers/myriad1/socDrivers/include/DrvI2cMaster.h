///   
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved. 
///            For License Warranty see: common/license.txt   
///
/// @brief     I2C Master Driver
/// 
/// This module is a general purpose I2C master driver                       
/// It is a unified driver supporting both the Myriad I2C                    
/// hardware module AND the use of GPIOs to bit bash.                        
/// The module aims to improve on existing Myriad I2C drivers                
/// in the following ways                                                    
/// - Same API supports HW and Bitbash                                       
/// - Support for error detection and reporting                              
/// - Option for unified error handler callback                              
/// - No need to re-init to change target slave                              
/// - Protocol driven to avoid the need to have 20 different I2C functions   
///

#ifndef _BRINGUP_SABRE_IIC_MASTER_H_
#define _BRINGUP_SABRE_IIC_MASTER_H_

/**
 * @page I2CM_Usage Using the I2C Master Driver
 * One of the key features of this driver is the single api call #DrvI2cMTransaction
 * which is used to handle all I2C transactions both read and write.
 * While the I2C protocol is well defined at a hardware level, the precise
 * implementation of how the bytes on the wire are interpreted by a device is 
 * left largely up the implementation of the silicon.
 * As such there are numerous different protocols which need to be supported.
 * Previously this was handled by writing new functions for each different type 
 * of device. This time it was decided to use a simple protocol engine to specify
 * the hardware protocol and allow the caller to specify the protocol to be used for
 * each transaction.
 * This section attempts to explain how this protocol is used.
 * 
 * The proto parameter to the #DrvI2cMTransaction function expects a array of bytes which defines
 * the protocol. This array represents the sequence of bytes to be placed on(or removed from) the 
 * bus. It also supports a very simple loop construct for larger sequences of bytes.
 * The codes which are used to describe the procol are defined in the enumerated type #I2CMProtocolType
 * 
 * The best way to describe this is through the use of some simple examples.
 * @section EE_READ Example I2C EEPROM Read
 * ![AT24C04BN Random Read I2C Operation](http://timi30/svn/SABRE/bringup/software/resource/i2cEepromRead.png)
 * *AT24C04BN Random Read I2C Operation*
 * @code
static int simpleEepromReadExample(void)
{
    int ret;
    I2CM_Device i2cDevice1; // Note: Normally this would be a project global

    u8 dataBuffer[10];
    const u32 speedKhz = 300;
    const u32 slaveAddress = MV0155_I2C1_EEPROM_7B; // 7 bit slave address of the EEPROM
    const u32 eepromRegAddress = 0x0; // Starting read at address 0x0
    const u32 lengthOfRead = 10;
    const u8 eeReadProto[] =
    {
        S_ADDR_WR,      // First byte is the slaveAddress (with Write bit set)
        R_ADDR_L,       // Second byte is the lowest 8 bits of the eepromRegAddress
        S_ADDR_RD,      // Third byte transmitted will be the same slaveAddress (with read set)
        DATAR,          // Next an i2c read operation; the read byte is placed in dataBuffer[index++]
        LOOP_MINUS_1    // Loops back to DATAR and repeats until lengthOfRead bytes have been read 
    };


    // Initialise the I2C device to use the I2C1 Hardware block
    // with gpio 65,66
    ret = DrvI2cMInit(&i2cDevice1,IIC1_DEVICE,65,66,speedKhz,ADDR_7BIT);
    if (ret != I2CM_STAT_OK)
        return -1;

    // Perform I2C Read transaction
    ret = drvI2cMTransaction(&i2cDevice1,slaveAddress, eepromRegAddress ,eeReadProto, dataBuffer, lengthOfRead);
    if (ret != I2CM_STAT_OK)
        return -1;

    return 0;
}
 * @endcode
 * @section EE_WRITE Example of a simple EEPROM Write Operation
 * ![AT24C04BN Page Write I2C Operation](http://timi30/svn/SABRE/bringup/software/resource/i2cEepromWrite.png)
 *  *AT24C04BN Page Write I2C Operation*
 * @code
static int simpleEepromWriteExample(void)
{
    int ret;
    I2CM_Device i2cDevice1; // Note: Normally this would be a global

    u8 dataBuffer[] = { 1,2,3,4};
    const u32 speedKhz = 300;
    const u32 slaveAddress = MV0155_I2C1_EEPROM_7B; // 7 bit slave address of the EEPROM
    const u32 eepromRegAddress = 0x0; // Starting read at address 0x0
    const u32 lengthOfWrite = sizeof(dataBuffer);
    const u8 eeWriteProto[] =
    {
        S_ADDR_WR,      // First byte is the slaveAddress (with Write bit set)
        R_ADDR_L,       // Second byte is the lowest 8 bits of the eepromRegAddress
        DATAW,          // Next an i2c write operation; the  byte written is dataBuffer[index++]
        LOOP_MINUS_1    // Loops back to DATAW and repeats until lengthOfWrite bytes have been written 
    };


    // Initialise the I2C device to use the I2C1 Hardware block
    // with gpio 65,66
    ret = DrvI2cMInit(&i2cDevice1,IIC1_DEVICE,65,66,speedKhz,ADDR_7BIT);
    if (ret != I2CM_STAT_OK)
        return -1;

    // Perform I2C Read transaction
    ret = drvI2cMTransaction(&i2cDevice1,slaveAddress, eepromRegAddress ,eeWriteProto, dataBuffer, lengthOfWrite);
    if (ret != I2CM_STAT_OK)
        return -1;

    return 0;
}
   @endcode
*/
    
/******************************************************************************
 1: Included types first then APIs from other modules
******************************************************************************/

#include "DrvI2cMasterDefines.h"
                        
/******************************************************************************
 2:  Exported variables --- these should be avoided at all costs
******************************************************************************/

/******************************************************************************
 3:  Exported Functions
******************************************************************************/

/**  Initialise the I2C Driver Module and configure its parameters

 @param     dev Pointer to an #I2CM_Device structure which will be used to store the device state
 @param     module Enum #I2CMModuleOption to specify choice of hardware block or bitbash
 @param     sclPin GPIO number of the pin to be used for the SCL line
 @param     sdaPin GPIO number of the pin to be used for the SDA line
 @param     speedKhz Target speed of the I2C bus in Khz
 @param     addrType Enum #I2CMAddrType to specify 7 or 10 bit addressing
 @return    #I2CM_StatusType to report either success (0) or give error code.
 @note      The #I2CM_Device structure is to be treated by the programmer as an opaque structure
            No access should be made to its members except via recognised I2CM driver API calls
*/
I2CM_StatusType DrvI2cMInit(I2CM_Device * dev,I2CMModuleOption module, u32 sclPin,u32 sdaPin,u32 speedKhz,I2CMAddrType addrType);


/** Initialise the I2C Driver Module and configure its parameters basesd on a tyI2cConfig structure

 @param     dev Pointer to an #I2CM_Device structure which will be used to store the device state
 @param     config pointer to a tyI2cConfig structure that contains the configuration parameters
 @return    #I2CM_StatusType to report either success (0) or give error code.
 @note      The #I2CM_Device structure is to be treated by the programmer as an opaque structure
            No access should be made to its members except via recognised I2CM driver API calls
*/

I2CM_StatusType DrvI2cMInitFromConfig(I2CM_Device *dev, const tyI2cConfig *config);

/**  Perform an I2C Master Transaction to either Read or Write data to an I2C bus

 @param     devHandle Pointer to a preinitialised #I2CM_Device handle 
 @param     slaveAddr I2C Slave address of the target device in **7-BIT** Format 
 @param     regAddr NOT COMMENTED!
 @param     proto Array of u8 values which defines the target protocol to be used for this transaction.
 See \#I2CM_Usage for a description of how this array is used. 
 @param     dataBuffer u8 * to dataBuffer which is either source or target of operation depending on proto
 @param     dataBufferSize Size of data to be read/written in bytes. (Read/Write depends on proto)
 @return    #I2CM_StatusType to report either success (0) or give error code. 
*/
I2CM_StatusType DrvI2cMTransaction(I2CM_Device * devHandle, u32 slaveAddr,u32 regAddr,u8 * proto, u8 * dataBuffer, u32 dataBufferSize);

/**  Specify a callback function which is to be used to collect all errors returned.
 By default the I2CMaster driver Transaction will return an error code
 In order to support legacy code which does not handle such errors
 or code which is unable to handle such errors, it is possible to specify
 a callback function which will be used when an error occurs.
 Typical usage might include:
 - Raising of an assert in debug builds.
 - Keeping a simple error count. 
 - Handling the error, by reportin it to the user.
 @note The callback function is of the type #I2CErrorHandler. This function also returns an #I2CM_StatusType which 
 may either be the original error value or can be overridden to #I2CM_STAT_OK so that the callee doesn't see the error.
 @param    dev     devHandle Pointer to a preinitialised I2CM_Device handle 
 @param    handler Function pointer of type #I2CErrorHandler for callback to be used 
 @return    I2CM_StatusType to report either success (0) or give error code.
 @note   Only the #DrvI2cMTransaction function will trigger the callback as DrvI2cMInit cannot do so.
*/
I2CM_StatusType DrvI2cMSetErrorHandler(I2CM_Device * dev,I2CErrorHandler handler);


/// Simple wrapper for I2C transaction that reads 1 byte from a slave using
/// default 8 bit read
/// @param[in] dev Handle to target I2C device
/// @param[in] slaveAddr 7 bit I2C Slave Address of the target device
/// @param[in] regAddr address if the register to read within the slave device 
/// @return    
u8   DrvI2cMReadByte(I2CM_Device * dev,u32 slaveAddr,int regAddr);

/// Simple wrapper for I2C transaction that writes 1 byte to a slave using
/// default 8 bit write
/// @param[in] dev Handle to target I2C device
/// @param[in] slaveAddr 7 bit I2C Slave Address of the target device
/// @param[in] regAddr address if the register to read within the slave device 
/// @param[in] value 8 bit value to be written to the register
/// @return    
void DrvI2cMWriteByte(I2CM_Device * dev,u32 slaveAddr,u32 regAddr, u8 value);

/// Retrieve a copy of the settings which were used to configure this I2C device
///
/// This can be useful if there is a need to temporarily change I2C config and 
/// restore a backup of the config.
///
/// @param[in] dev - device handle pointer 
/// @param[in] currentConfig - pointer to storage into which the results will be placed
/// @return    
void DrvI2cMGetCurrentConfig(I2CM_Device * dev,tyI2cConfig * currentConfig);


#endif // _BRINGUP_SABRE_IIC_MASTER_H_
