///   
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved. 
///            For License Warranty see: common/license.txt   
///
/// @brief     Driver for simple 1 byte address EEPROMs such as AT24C04B
/// 
/// This driver supports simple EEPROMS with 1 byte address such as AT24C04B           
/// It relies on the I2C Master Driver and takes a handle to an already established   
/// I2C Device                                                                        
/// Key Features:                                                                     
/// - Support for Acknowledge polling                                                 
/// - Support for EEPROMS which use multiple banks                                    
/// - API which handles crossing page boundaries and bank boundaries                  
///

// 1: Includes
// ----------------------------------------------------------------------------
#include <isaac_registers.h>
#include <mv_types.h>
#include <DrvGpio.h>
#include <icEeprom.h>
#include <DrvI2cMaster.h>

// 2:  Source Specific #defines and types  (typedef,enum,struct)
// ----------------------------------------------------------------------------
#define NACK_TIMEOUT    (1200)  // Based on estimated worst case of 1Mhz SCL and 1.2ms PAGE_WRITE time
#define PAGE_SIZE       (16)
#define BANK_SIZE       (256)

#ifndef MIN
#   define MIN(a,b) (a>b?b:a)
#endif

#ifndef MAX
#   define MAX(a,b) (a>b?a:b)
#endif
//#define DRV_EEPROM_DEBUG1  // Level 1: Debug Messages (Highest priority, e.g. Errors)

#ifdef DRV_EEPROM_DEBUG1
#include <stdio.h>
#define DPRINTF1(...)  printf(__VA_ARGS__)
#else
#define DPRINTF1(...)  {}
#endif

// 3: Global Data (Only if absolutely necessary)
// ----------------------------------------------------------------------------

// 4: Static Local Data 
// ----------------------------------------------------------------------------
static u8 protoEepromWrite[] = 
    {
        S_ADDR_WR, 
        R_ADDR_L, 
        DATAW,
        LOOP_MINUS_1
    };

static u8 protoEepromRead[] = 
    {
        S_ADDR_WR, 
        R_ADDR_L, 
        S_ADDR_RD, 
        DATAR,
        LOOP_MINUS_1
    };   

static u8 protoEepromVerify[] = 
    {
        S_ADDR_WR, 
        R_ADDR_L, 
        S_ADDR_RD, 
        DATAV,              // Verify
        LOOP_MINUS_1
    };

// 5: Static Function Prototypes
// ----------------------------------------------------------------------------

// 6: Functions Implementation
// ----------------------------------------------------------------------------

int icEepromWrite(I2CM_Device * dev, u32 slaveAddress, u32 startAddress, u32 len, u8 * data)
{
    u32 i;
    int retVal=I2CM_STAT_OK;
    u32 currentAddress;
    u32 pages;
    u32 bankNumber = 0;
    u32 firstWrite;
    u32 numNACKS = 0;
    // AT24C04B EEPROMS have 8 bit addressing which can address 256 bytes directly
    // They comed in two varieties:
    // 4K Parts (512 * 8) 
    //   Uses Slave Address, and Slave Address + 1 to fully address the 512 bytes
    // 8K Parts (1024 * 8)
    //   Uses Slave Address + 0, 1, 2, 3 to address all 4 banks of 256 bytes

    DPRINTF1("\neepromWrite DEV:%04X  SA:%08X  L:%08X",slaveAddress,startAddress,len);

    currentAddress = startAddress;

    // First we detect if we start on an non-page aligned boundary and write that runt first
    firstWrite = MIN(PAGE_SIZE - (currentAddress % PAGE_SIZE),len);
    if (firstWrite != 0) 
    {
        bankNumber = currentAddress / BANK_SIZE;
        do  // This loop handles the acknowledge polling feature of EEPROMs
        {
            retVal = DrvI2cMTransaction(dev,slaveAddress + bankNumber, currentAddress ,protoEepromWrite,data, firstWrite);
        } while ((retVal == I2CM_STAT_ABRT_7B_ADDR_NOACK) && (numNACKS++ < NACK_TIMEOUT));
        currentAddress += firstWrite;
        data += firstWrite;
        len -= firstWrite;
    }

    if (retVal != I2CM_STAT_OK)
        goto eewrite_error;

    pages = len / PAGE_SIZE;

    for (i=0;i<pages;i++)
    {
        bankNumber = currentAddress / BANK_SIZE;
        numNACKS = 0;
        do  // This loop handles the acknowledge polling feature of EEPROMs
        {
            retVal = DrvI2cMTransaction(dev,slaveAddress + bankNumber, currentAddress ,protoEepromWrite,data, PAGE_SIZE);
        } while ((retVal == I2CM_STAT_ABRT_7B_ADDR_NOACK) && (numNACKS++ < NACK_TIMEOUT));
        currentAddress += PAGE_SIZE;
        data += PAGE_SIZE;
        len -= PAGE_SIZE;
    }

    if ((retVal == I2CM_STAT_OK) && len != 0)
    {
        // Write the remaining bytes
        numNACKS = 0;
        bankNumber = currentAddress / BANK_SIZE;              
        do  // This loop handles the acknowledge polling feature of EEPROMs
        {
            retVal = DrvI2cMTransaction(dev,slaveAddress + bankNumber, currentAddress ,protoEepromWrite,data , len);
        } while ((retVal == I2CM_STAT_ABRT_7B_ADDR_NOACK) && (numNACKS++ < NACK_TIMEOUT));
    }


eewrite_error:
    // In the event of a NACK Timeout the reported error will be I2CM_STAT_ABRT_7B_ADDR_NOACK
    return retVal;
}

int icEepromRead(I2CM_Device * dev, u32 slaveAddress, u32 startAddress, u32 len, u8 * data,ComEepromAction action)
{
    u32 i;
    int retVal=I2CM_STAT_OK;
    u32 currentAddress;
    u32 banks;
    u32 bankNumber = 0;
    u32 firstRead;
    u8 * protocol;
    u32 numNACKS=0;

    DPRINTF1("\neepromRead DEV:%04X  SA:%08X  L:%08X A:%d",slaveAddress,startAddress,len,action);
    if (action == EE_READ)
        protocol = protoEepromRead;
    else
        protocol = protoEepromVerify; 

    // AT24C04B EEPROMS have 8 bit addressing which can address 256 bytes directly
    // They comed in two varieties:
    // 4K Parts (512 * 8) 
    //   Uses Slave Address, and Slave Address + 1 to fully address the 512 bytes
    // 8K Parts (1024 * 8)
    //   Uses Slave Address + 0, 1, 2, 3 to address all 4 banks of 256 bytes

    currentAddress = startAddress;

    // First we detect if we start on an bank aligned boundary and read that runt first
    firstRead = MIN(BANK_SIZE - (currentAddress % BANK_SIZE),len);
    if (firstRead != 0) 
    {
        bankNumber = currentAddress / BANK_SIZE;
        do  // This loop handles the acknowledge polling feature of EEPROMs
        {
            retVal = DrvI2cMTransaction(dev,slaveAddress + bankNumber, currentAddress ,protocol, data, firstRead);
        } while ((retVal == I2CM_STAT_ABRT_7B_ADDR_NOACK) && (numNACKS++ < NACK_TIMEOUT));
        currentAddress += firstRead;
        data += firstRead;
        len -= firstRead;
    }

    if (retVal != I2CM_STAT_OK)
        goto eeread_error;

    banks = len / BANK_SIZE;

    for (i=0;i<banks;i++)
    {
        bankNumber = currentAddress / BANK_SIZE;
        numNACKS = 0;
        do  // This loop handles the acknowledge polling feature of EEPROMs
        {
            retVal = DrvI2cMTransaction(dev,slaveAddress + bankNumber, currentAddress ,protocol, data , BANK_SIZE);
        } while ((retVal == I2CM_STAT_ABRT_7B_ADDR_NOACK) && (numNACKS++ < NACK_TIMEOUT));
        currentAddress += BANK_SIZE;
        data += BANK_SIZE;
        len -= BANK_SIZE;
    }

    if ((retVal == I2CM_STAT_OK) && len != 0)
    {
        // Read the remaining bytes
        bankNumber = currentAddress / BANK_SIZE;
        numNACKS = 0;
        do  // This loop handles the acknowledge polling feature of EEPROMs
        {
            retVal = DrvI2cMTransaction(dev,slaveAddress + bankNumber, currentAddress ,protocol, data , len);
        } while ((retVal == I2CM_STAT_ABRT_7B_ADDR_NOACK) && (numNACKS++ < NACK_TIMEOUT));
    }                

    // In the event of a NACK Timeout the reported error will be I2CM_STAT_ABRT_7B_ADDR_NOACK
eeread_error:
    return retVal;
}
                           
