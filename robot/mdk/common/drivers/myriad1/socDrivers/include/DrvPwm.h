///  
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved 
///            For License Warranty see: common/license.txt   
///
/// @brief     API to the Myriad PWM driver
/// 
/// 
/// 
/// 
/// 

#ifndef DRV_PWM_H
#define DRV_PWM_H 

// 1: Includes
// ----------------------------------------------------------------------------
#include "DrvPwmDefines.h"

// 2:  Exported Global Data (generally better to avoid)
// ----------------------------------------------------------------------------

// 3:  Exported Functions (non-inline)
// ----------------------------------------------------------------------------

/// Configure a pwm Output
/// @param[in] pwmNumber number of PWM device in range 0-5
/// @param[in] repeat count 0->0xFFFF (whereby 0 = continuous)
/// @param[in] leadIn count in the range 0->0x7FFF
/// @param[in] highCount (0->0xFFFF)
/// @param[in] lowCount (0->0xFFFF)
/// @param[in] defaultEnable 0 to disable, otherwise PWM is enabled by default
/// @return    void
/// @note It is still necessary to configure an appropriate GPIO in the correct mode before the PWM is active
void DrvPwmConfigure(u32 pwmNumber,u32 repeat, u32 leadIn, u32 highCount, u32 lowCount,u32 defaultEnable);

/// Enable or Disable a specific PWM output
/// @param[in] pwmNumber number of PWM device in range 0-5
/// @param[in] enable 0 to disable, otherwise PWM is enabled by default
/// @return    
void DrvPwmEnable(u32 pwmNumber,u32 enable);


/// Configure a PWM pin to drive out a waveform which is used to control a linear regulator
/// @param[in] pcfg Configuration Structure which defines the necessary parameters
/// @param[in] targetVoltageMv (desired voltage in mV)
/// @return    0 on Success, non-zero on failure
int DrvPwmSetupVoltageControl(tyPwmVoltageConfig * pcfg,u32  targetVoltageMv);

#endif // DRV_PWM_H  

