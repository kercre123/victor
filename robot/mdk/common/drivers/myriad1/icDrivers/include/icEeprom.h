///  
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved 
///            For License Warranty see: common/license.txt   
///
/// @brief     Driver for simple 1 byte address EEPROMs such as AT24C04B 
/// 
/// 
/// This driver supports simple EEPROMS with 1 byte address such as AT24C04B           
/// It relies on the I2C Master Driver and takes a handle to an already established   
/// I2C Device                                                                        
/// Key Features:                                                                     
/// - Support for Acknowledge polling                                                 
/// - Support for EEPROMS which use multiple banks                                    
/// - API which handles crossing page boundaries and bank boundaries                  
/// 

#ifndef IC_EEPROM_H
#define IC_EEPROM_H 

// 1: Includes
// ----------------------------------------------------------------------------
#include "icEepromDefines.h"
#include "DrvI2cMaster.h"

// 2:  Exported Global Data (generally better to avoid)
// ----------------------------------------------------------------------------

// 3:  Exported Functions (non-inline)
// ----------------------------------------------------------------------------

/// Write data buffer to the EEPROM
/// @param[in] dev Pointer to an #I2CM_Device structure which has already been initialised 
/// @param[in] slaveAddress 7-bit slave address of the target I2C EEPROM                   
/// @param[in] startAddress byte offset at which writing should start within the EEPROM    
/// @param[in] len length in bytes of the write operation                                  
/// @param[in] *data pointer to data to be written                                         
/// @param[in] 0 for Success; otherwise failure code                                       
/// @return    0 on success, non-zero otherwise  
///  @note     This function handles page accesses and acknowledge polling transparently.
///            It support EEPROMS up to 512 bytes in size using the convention of upper back at slaveAddress+1
int icEepromWrite(I2CM_Device * dev, u32 slaveAddress, u32 startAddress, u32 len, u8 * data);


/// Read or Verify EEPROM Data contents
/// @param[in] dev Pointer to an #I2CM_Device structure which has already been initialised 
/// @param[in] slaveAddress 7-bit slave address of the target I2C EEPROM                   
/// @param[in] startAddress byte offset at which reading should start within the EEPROM    
/// @param[in] len length in bytes of the read operation                                   
/// @param[in] *data pointer to storage for the data which is read or verified against     
/// @param[in] action enum of type #ComEepromAction to select either EE_READ or EE_VERIFY  
/// @return    0 for Success; otherwise failure code                                        
/// @note      This function handles page accesses and acknowledge polling transparently.                      
///            It support EEPROMS up to 512 bytes in size using the convention of upper back at slaveAddress+1 
int icEepromRead(I2CM_Device * dev, u32 slaveAddress, u32 startAddress, u32 len, u8 * data,ComEepromAction action);

#endif // IC_EEPROM_H  
