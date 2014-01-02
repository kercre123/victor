///  
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved 
///            For License Warranty see: common/license.txt              
///
/// @brief     Definitions and types needed by MV0163 Daughtercard driver
/// 
#ifndef BRD_MV0163_DCARD_DEF_H
#define BRD_MV0163_DCARD_DEF_H 

// 1: Defines
// ----------------------------------------------------------------------------
#define MV0163_EEPROM_SIZE        (0x200)
                             
#define MV0163_I2C1_EEPROM_8BW    (0xA0)
#define MV0163_I2C1_EEPROM_8BR    (MV0163_I2C1_EEPROM_8BW |  1)
#define MV0163_I2C1_EEPROM_7B     (MV0163_I2C1_EEPROM_8BW >> 1)
                 
#define MV0163_I2C1_MYRIAD_8BW    (0xC6)
#define MV0163_I2C1_MYRIAD_8BR    (MV0163_I2C1_MYRIAD_8BW |  1)
#define MV0163_I2C1_MYRIAD_7B     (MV0163_I2C1_MYRIAD_8BW >> 1)

// 2: Typedefs (types, enums, structs)
// ----------------------------------------------------------------------------

// 3: Local const declarations     NB: ONLY const declarations go here
// ----------------------------------------------------------------------------

#endif // BRD_MV0163_DCARD_DEF_H

