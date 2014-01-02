///  
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved 
///            For License Warranty see: common/license.txt              
///
/// @brief     Definitions and types needed by 
/// 
#ifndef DRV_PWM_DEF_H
#define DRV_PWM_DEF_H 

// 1: Defines
// ----------------------------------------------------------------------------

// 2: Typedefs (types, enums, structs)
// ----------------------------------------------------------------------------

typedef struct 
{
    u8   gpioNum;
    u8   gpioMode;
    u8   pwmDevice;
    u16  vminHardwareMv;
    u16  vmaxHardwareMv;
    u16  vminSoftwareClampMv;
    u16  vmaxSoftwareClampMv;
    u32  sysFreqHz;
    u32  pwmFreqHz;
    u32  postCfgDelayMs;
} tyPwmVoltageConfig;
         
// 3: Local const declarations     NB: ONLY const declarations go here
// ----------------------------------------------------------------------------

#endif // DRV_PWM_DEF_H
