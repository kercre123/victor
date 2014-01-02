///  
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved 
///            For License Warranty see: common/license.txt   
///
/// @brief     API for the Clock Power Reset Module
/// 
/// The Timer Driver provides the following key functionality       
/// - Full 64 bit system tick counter which is never reset.                                                                   
/// - Timer Async callback functionality to allow the user to request a callback after specified num microseconds             
///   The callback can be requested; once, x times or infinitely.                                                             
///   There is a pool of 7 hardware timers available.                                                                         
/// - Timer Async variable update. (Same as callback only simpler case where the users variable gets incremented after a time)
/// - New Sleep API with units of microseconds, ms, or ticks. Is aware of current CPU frequency.                              
///   It has no dependence of RTC and it does not reset the the system tick counter                                           
/// - Timeout API to support a simple design pattern for timeout on register access etc.                                      
/// 

#ifndef DRV_TIMER_H
#define DRV_TIMER_H 

// 1: Includes
// ----------------------------------------------------------------------------
#include "DrvCpr.h"
#include "DrvTimerDefines.h"
#include "swcLeonMath.h"

// 2:  Exported Global Data (generally better to avoid)
// ----------------------------------------------------------------------------

// 3:  Exported Functions (non-inline)
// ----------------------------------------------------------------------------


/// Initialise the timer driver
/// 
/// Initialise config structures, enable 64 bit free running clock
/// This function should only be called once from DrvCprInit so users generally
/// should not have to call this function assuming CPR has been properly intialised
/// Mark all timers as available
/// @return    0 on success, non-zero otherwise  
int DrvTimerInit(void);

/// Returns the number of clock cycles since DrvTimerInit() called
/// 
/// This is a full 64 bit free running counter so it only overflows 
/// every 3249 years @ 180Mhz
/// @return    Number of clock cycles since DrvCprInit() [64 bit value]
u64 DrvTimerGetSysTicks64(void);

/// Converts a number of ticks into a number of ms in FP64 representation
///
/// Floating point representation is used to give maximum useful range for this function
/// @return float containing Number of milliseconds corresponding to the specified tick count
static inline double DrvTimerTicksToMs(u64 ticks)
{
    double timeMs;
    timeMs = swcLongLongToDouble(ticks);
    timeMs = timeMs / DrvCprGetSysClocksPerUs() / 1000;
    return timeMs;
}

/// Request a callback after numMicroSeconds
/// 
/// Optionally have the callback for repeatCount times
/// @param[in] numMicroSeconds number of micro-seconds before the callback is called
/// @param[in] callback pointer to function to callback
/// @param[in] repeatCount Times to callback (0-> forever until DrvTimerDisable), 1 once, x -> x times)
/// @param[in] irqPriority priority level of the IRQ
/// @return    Number of the timer used on success <0 on fail. 
int DrvTimerCallAfterMicro(u32 numMicroSeconds, tyTimerCallback callback, int repeatCount, int irqPriority); // 0 is forever


/// Trigger timer even to increment a static 32 bit word
///
/// Optionally do this for repeatCount times
/// @param[in] numMicroSeconds number of micro-seconds before the timer increments the variable
/// @param[in] pValueToIncrement pointer to 32 bit word to be incremented
/// @param[in] repeatCount Times to callback (0-> forever until DrvTimerDisable), 1 once, x -> x times)
/// @return    Number of the timer used on success <0 on fail. 
int DrvTimerIncrementAfterMicro(u32 numMicroSeconds, volatile u32 * pValueToIncrement, int repeatCount);

/// Adjust a currently running timer to a new time value
///
/// This function may be used in the callback routine of an infinite count timer
/// @param[in] timer handle returned from timer setup function
/// @param[in] numMicroSeconds set adjusted number of micro-seconds until the next timer callback
void DrvTimerAdjustTimerValue(u32 timer, u32 numMicroSeconds);

/// Disable the specified timer using the handle returned by other timer functions
/// 
/// This is necesary if timer was configured as REPEAT_FOREVER or if it needs to be stopped early
/// @param[in] timer handle returned from timer setup function
/// @return    0 on success, non-zero otherwise  
int DrvTimerDisable(int timer);

/// Sleep for numMs milli-seconds
/// 
/// Uses knowledge of the system clock to calculate the time
/// Doesn't rely on RTC
/// @param[in] numMs number of milliseconds to stall for
/// @return    void
void SleepMs(unsigned int numMs);

/// Sleep for numMicroSeconds micro-seconds
/// 
/// Uses knowledge of the system clock to calculate the time
/// Doesn't rely on RTC
/// When running with 180Mhz system clock and cache enabled, this function can 
/// achieve accurate delays down to 1 micro-second
/// Note: Accurate => error < 1uS
/// @param[in] numMicroSeconds number of microseconds to stall for
/// @return    void
void SleepMicro(unsigned int numMicroSeconds);

/// Return system uptime in milli-seconds
///
/// @return    number of ms since DrvTimerInit as FP64
double DrvTimerUptimeMs(void);

/// Sleep for numSystemTicks system clock cycles
/// 
/// Attempts to define a delay based on system clock cycles.
/// For small values this function will not be accurate due to 
/// function overhead.
/// Function is inline for performance
/// This function is not advised in most applications as delays should normally be in ms or us.
/// @param[in] numSystemTicks number of system clocks to stall for
/// @return    void
static inline void SleepTicks(u64 numSystemTicks)
{
    u64 startTime = DrvTimerGetSysTicks64();
    u64 endTime   = startTime + numSystemTicks;
    while (DrvTimerGetSysTicks64() < endTime)
        ; // Deliberatly empty
    return;
}

/// Initialise a timeout 
///
/// Creates a timestamp which is used to check if a timeout has occurred
/// Uses storage passed by the user to make the function re-entrant
/// @param[in] pTimeStamp pointer to storage for calculated timestamp
/// @param[in] intervalUs Interval for the timestamp in microseconds
/// @return    void
void DrvTimerTimeoutInitUs(tyTimeStamp * pTimeStamp,u32 intervalUs);


/// Reports if a timeout has occurred based on previously initialised timestamp
/// 
/// @param[in] pTimeStamp pointer to timestamp which after which the function will return TRUE
/// @return    FALSE if current time is < timestamp; TRUE if timeout has occurred
int DrvTimerTimeout(tyTimeStamp * pTimeStamp);


/// Update pTimestamp with the current tickcount
///
/// This function is part of a pair of functions used for event timing
/// Call this function with a pointer to a tyTimeStamp which is used to 
/// record the current tick count.Later this timestamp will be used to 
/// report the elapsed ticks since the original timestamp
/// @param[in] pTimeStamp pointer to storage for calculated timestamp
/// @return    void
void DrvTimerStart(tyTimeStamp * pTimeStamp);

/// Report the ticks elapsed since DrvTimerStart was called on this timestamp
///
/// @param[in] pTimeStamp pointer to the previsouly initialised timeStamp (using DrvTimerStart)
/// @return    64 bit tick count of elapsed time since start
u64 DrvTimerElapsedTicks(tyTimeStamp * pTimeStamp);

#endif // DRV_TIMER_H  

