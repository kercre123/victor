///  
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved 
///            For License Warranty see: common/license.txt              
///
/// @brief     Definitions and types needed by MV0171 Daughtercard driver
/// 
#ifndef BRD_MV0171_DCARD_DEF_H
#define BRD_MV0171_DCARD_DEF_H 

// 1: Defines
// ----------------------------------------------------------------------------
#define MV0171_EEPROM_SIZE        (0x200)
                             
#define MV0171_I2C1_EEPROM_8BW    (0xA0)
#define MV0171_I2C1_EEPROM_8BR    (MV0171_I2C1_EEPROM_8BW |  1)
#define MV0171_I2C1_EEPROM_7B     (MV0171_I2C1_EEPROM_8BW >> 1)
               
#define MV0171_I2C1_MYRIAD_8BW    (0xC6)
#define MV0171_I2C1_MYRIAD_8BR    (MV0171_I2C1_MYRIAD_8BW |  1)
#define MV0171_I2C1_MYRIAD_7B     (MV0171_I2C1_MYRIAD_8BW >> 1)

#define MV171_LED1 57
#define MV171_LED2 12
#define MV171_LED3 56
#define MV171_LED4 52

#define MV171_CAMERA_EN 58

// 2: Typedefs (types, enums, structs)
// ----------------------------------------------------------------------------

// 3: Local const declarations     NB: ONLY const declarations go here
// ----------------------------------------------------------------------------

#endif // BRD_MV0171_DCARD_DEF_H

