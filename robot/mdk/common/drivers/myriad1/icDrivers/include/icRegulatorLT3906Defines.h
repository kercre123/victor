///  
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved 
///            For License Warranty see: common/license.txt              
///
/// @brief     Definitions and types needed by the LT3906 Driver
/// 
#ifndef REG_LT3906_DEF_H
#define REG_LT3906_DEF_H 

// 1: Defines
// ----------------------------------------------------------------------------
#define LT3906_ADDR_I2C (0xC0>>1)         /// This address is fixed for the regulator


#define REGULATOR_POLL_COUNT        (100) // Max times to poll before giving up on regulator settling 
										  // Emperical evidence suggests that only one poll will be necessary, so margin is great

// 2: Typedefs (types, enums, structs)
// ----------------------------------------------------------------------------
typedef struct
{
    u32 switchedVoltage1; // Valid Range: 0.8V -> 2.0V in 50mV  increments (Set 120 for 1.2V; 125 for 1.25V etc.) iMax=1500mA
    u32 switchedVoltage2; // Valid Range: 1.0V -> 3.5V in 100mV increments (Set 12 for 1.2V; 13 for 1.3V etc.)    iMax=1500mA 
    u32 ldoVoltage1;      // Valid Range: 1.0V -> 3.5V in 100mV increments (set 12 for 1.2V; 25 for 2.5V etc.)    iMax= 300mA 
    u32 ldoVoltage2;      // Valid Range: 1.0V -> 3.5V in 100mV increments (set 12 for 1.2V; 25 for 2.5V etc.)    iMax= 300mA 
    u8  voltRegCache[5];  // Set to 0 initially, Used internally as storage for a register value cache)  // Caller should set to 0
} tyLT3906Config;


// 3: Local const declarations     NB: ONLY const declarations go here
// ----------------------------------------------------------------------------

#endif // REG_LT3906_DEF_H

