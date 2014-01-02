///  
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved 
///            For License Warranty see: common/license.txt   
///
/// @brief     API for the Clock Power Reset Module
/// 
/// CPR Driver Core Functions
/// - DrvCprSetupClocks function which provides a table driven mechanism for configuring all clocks in the system
///   This API call replaces the previously used drv_sys_config
/// - Input Osc clock config is handled using callback functionality which will be routed to the appropriate board driver
///   function on boards which support variable input clocks (NO \#define OSC_IN!!)
/// - Optional Callback mechanism to notifiy applications of system clock changes so that user actions can be performed on clock change
/// - Automatic update of DDR timings on ddr clock frequency change (Not implemented yet)
/// - Unified API for sysdevice action (BLOCK reset, enabling clocks, disable clocks)
/// - Function to query clock frequency
/// 
/// 

#ifndef DRV_CPR_H
#define DRV_CPR_H 

// 1: Includes
// ----------------------------------------------------------------------------
#include "DrvCprDefines.h"

// 2:  Exported Global Data (generally better to avoid)
// ----------------------------------------------------------------------------
// Justification for this global is to have a performant way of 
// accessing this
///Global pointer to system clock
extern u32 * pGlobalSysclockKhz; // Setup to point to cprState.currentSysClockKhz

// 3:  Exported Functions (non-inline)
// ----------------------------------------------------------------------------

/// Initialise the Clock Power Reset Controller
/// 
/// This function has 2 optional function callback parameters both of which may be NULL if not needed
/// This function must be called prior to using any other CPR function
/// @param[in] pfGetSetOscFreqKhzCb - Callback to support changing system Osc In frequency on boards that support it 
/// @param[in] pfSysClockFreqChange - Callback which is used to notify the application when system clock frequency has changed. 
/// @return    0 on success, non-zero otherwise  
int DrvCprInit(tyGetSetOscFreqKhzCb pfGetSetOscFreqKhzCb,tySysClkFreqChangeCb pfSysClockFreqChange);

/// Table driven configuration of all device clocks
///
/// This function may be used to configure the entire clock tree for the system
/// In many applications calling DrvCprInit and this function is all that will be needed
/// @param[in] pSocClockConfig - pointer to clock configuration structure
/// @return    0 on success, non-zero otherwise  
int DrvCprSetupClocks(const tySocClockConfig * pSocClockConfig);

/// Update the system frequency by modifiying the PLL value and the system clock divider
///
/// Most applications will not need to call this function as it will be handled by DrvCprSetupClocks
/// The resultant system clock will be
/// achievedPllKhz * clockDivNumerator / clockDivDenominator
/// Note: this may not be the same as pllTargetFreqKhz * clockDivNumerator / clockDivDenominator  
/// The function will return the actual system clock in Khz or a negative value on error
/// @param[in] pllTargetFreqKhz    - desired PLL frequency in Khz
/// @param[in] clockDivNumerator   - numerator for system clock divider
/// @param[in] clockDivDenominator - denominator for system clock divider
/// @return    negative value on error or returns the achieved system clock if positive  
int DrvCprConfigureSystemClk(int pllTargetFreqKhz, int clockDivNumerator, int clockDivDenominator);


/// Enable one or more auxillary clocks with a specific divider value
/// 
/// @param[in] auxClksMask - 32 bit mask where set bits enable the corresponding clock
/// @param[in] numerator   - clock divider numerator
/// @param[in] denominator - clock divider denominator
/// @return    0 on success, non-zero otherwise  
int DrvCprAuxClocksEnable(u32 auxClksMask,u16 numerator, u16 denominator);

/// Configure the auxilary clocks using a null terminated array of 
/// Auxilary clock configurations
/// 
/// Normally this is handled by DrvCprConfigureSystemClk for most users
/// @param[in] pAuxClkCfg[] - null terminated array of tyAuxClkDividerCfg
/// @return    0 on success, non-zero otherwise  
int DrvCprAuxClockArrayConfig(const tyAuxClkDividerCfg  pAuxClkCfg[]);

/// Get clock frequency in KHz
/// @param[in] clockType - type of clock
/// @param[out] clkCfg - clock config structure
/// @return clock frequency
/// @todo This function needs to optionally update the clkCfg Structure
// Passed an Enum of different clock types, SYSTEM, AUX clocks
int DrvCprGetClockFreqKhz(tyClockType clockType,tyClockConfig * clkCfg);

/// Bypass PLL or re-enable it
/// 
/// It is not expected that this function would be needed in normal
/// Operation. 
/// @param[in] bypass - TRUE to bypass PLL, FALSE to re-engage
/// @return    0 on success, non-zero otherwise  
int DrvCprPllBypass(int bypass);

/// Enable, Disable or reset one or more system devices
/// @param[in] action  - type of action ENABLE_CLKS, DISABLE_CLKS or  RESET_DEVICES
/// @param[in] devices - 64 bit bitfield representing the devices on which the action is applied
/// @return    void
void DrvCprSysDeviceAction(tyCprDeviceAction action,u64 devices);

/// Fast function to get the current system clock in Khz
/// @return system clock frequency in Khz
static inline u32 DrvCprGetSysClockKhz(void)
{
    return *pGlobalSysclockKhz;
}

// Returns the number of system clocks per microsecond
/// @return number of system clocks taken for 1 microsecond
static inline u32 DrvCprGetSysClocksPerUs(void)
{
    return ((*pGlobalSysclockKhz) / 1000);
}

#endif // DRV_CPR_H  

