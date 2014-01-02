///   
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved. 
///            For License Warranty see: common/license.txt   
///
/// @brief     Driver for Timer module
/// 
/// 
/// 


// 1: Includes
// ----------------------------------------------------------------------------
#include "DrvCpr.h" 
#include "DrvTimer.h"
#include "registersMyriad.h"
#include "stdio.h"
#include "DrvIcb.h"
#include "swcWhoAmI.h"
#include "assert.h" // for now

// 2:  Source Specific #defines and types  (typedef,enum,struct)
// ----------------------------------------------------------------------------
#define LAST_AVAILABLE_TIMER (NUM_TIMERS)


//#define DRV_TIMER_DEBUG 

#ifdef DRV_TIMER_DEBUG
#define DPRINTF1(...) printf(__VA_ARGS__)
#else
#define DPRINTF1(...)
#endif

// Private Register Bitfield descriptions
#define D_TIMER_GENCFG_FREE_CNT_EN         (1 << 0)
#define D_TIMER_GENCFG_WATCHDOG_PRESCALLER (1 << 1)
#define D_TIMER_GENCFG_WATCHDOG_SYSTEM_CLK (0x0000)
#define D_TIMER_GENCFG_PRESCALER_ENABLE    (1 << 2)
#define D_TIMER_GENCFG_CLEAR_WATCHDOG_INT  (0x0000)

#define D_TIMER_CFG_ENABLE          (1 << 0)
#define D_TIMER_CFG_RESTART         (1 << 1)
#define D_TIMER_CFG_EN_INT          (1 << 2)
#define D_TIMER_CFG_CHAIN           (1 << 3)
#define D_TIMER_CFG_IRQ_PENDING     (1 << 4)
#define D_TIMER_CFG_FORCE_RELOAD    (1 << 5) 



typedef enum  
{
    TIMER_FREE,
    TIMER_NORMAL,
    TIMER_CHAINED_HIGH,
    TIMER_CHAINED_LOW
} tyTimerType;

typedef struct  
{
  tyTimerType           type;
  tyTimerCallback       callbackFunc;
  u32                   repeatCount;
  volatile u32        * pDataVal;
}tyTimerConfig;

typedef struct 
{
    u32 timerBaseAddress;
  // no need for this
  // u32 lastTicks;                          // Last recorded lower 32 bits of tick counter
  // u32 upperTicks32;                       // Software record of the upper 32 bits of the tick counter
  
  tyTimerConfig sysTimers[NUM_TIMERS+1];  // configuration state for each timer
  u8  initialised;                        // So we don't re-init
  
  // no need for this 
  //u8  ticksTimer;                         // Used to store the global timer we use for monitoring ticks
} tyTimerState;


// 3: Global Data (Only if absolutely necessary)
// ----------------------------------------------------------------------------


// 4: Static Local Data 
// ----------------------------------------------------------------------------
static tyTimerState timCfg;                   

// 5: Static Function Prototypes
// ----------------------------------------------------------------------------

static int timerCallAfterMicro(u32 timer,u32 numMicroSeconds, tyTimerCallback callback, int repeatCount, int priority);
static int getFreeTimer(u32 numMicroSeconds);
static u32 updateDataValue(u32 timer, u32 param0);
static inline void timerIrqHandler(u32 irq_source);

// 6: Functions Implementation
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// API Function Implementations
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

#ifndef DONT_CLEAR_BSS
// ^^ This is for safety, to make sure that people get a compile error if they try to use this function
// without BSS initialized.

int DrvTimerInit(void)
{
    int timer;

    if (timCfg.initialised)
        return ERR_TIM_ALREADY_INTIALISED; // The timer driver should only be initialised once at startup by CPR Init

    // Select between base address for LeonOS timer and LeonRT timer
    if (swcWhoAmI() == PROCESS_LEON_OS)
        timCfg.timerBaseAddress = TIM0_BASE_ADR;
    else
        timCfg.timerBaseAddress = TIM1_BASE_ADR;

    // Enable 64 bit mode of free running clock
    for (timer=0;timer<=LAST_AVAILABLE_TIMER;timer++)
    {
        timCfg.sysTimers[timer].type           = TIMER_FREE;
        timCfg.sysTimers[timer].callbackFunc   = NULL;
        timCfg.sysTimers[timer].pDataVal       = NULL;
        timCfg.sysTimers[timer].repeatCount    = 0;
    }

    // Enable the free running clock, and the scaler
    SET_REG_BITS_MASK(timCfg.timerBaseAddress + TIM_GEN_CONFIG_OFFSET ,   D_TIMER_GENCFG_FREE_CNT_EN | D_TIMER_GENCFG_PRESCALER_ENABLE); 

    // Set the prescaler to 0 so that we use the system clock direct for the timers
    // This gives us maximum resolution and we can chain registers to get larger counts
    // It is slightly more power hungry, but I think the tradeoff is worth it
    SET_REG_WORD(timCfg.timerBaseAddress + TIM_SCALER_RELOAD_OFFSET,0);

    
    timCfg.initialised = TRUE; // This function should only be run once

    return 0;
}

#endif

u64 DrvTimerGetSysTicks64(void)
{
    u32 currentTicksL, currentTicksH;

    // Note: The following 2 reads perform an atomic read of the internal 64 bit timer
    currentTicksH = GET_REG_WORD_VAL(timCfg.timerBaseAddress + TIM_FREE_CNT1_OFFSET);  // On this read the low value is latched
    currentTicksL = GET_REG_WORD_VAL(timCfg.timerBaseAddress + TIM_FREE_CNT0_OFFSET);  // previously latched low value read here

    return ((u64)currentTicksH << 32) | ((u64)currentTicksL);   
}

int DrvTimerDisable(int timer)
{
    // Disable the IRQ before we wipe out the callback
    DrvIcbDisableIrq(IRQ_TIMER + timer); 

    timCfg.sysTimers[timer].callbackFunc   = NULL;
    timCfg.sysTimers[timer].pDataVal       = NULL;
    timCfg.sysTimers[timer].repeatCount    = 0;

    DPRINTF1("\nDisable Timer: %d\n",timer);

    // Disble the timer and IRQ source
    SET_REG_WORD ((timCfg.timerBaseAddress + TIM0_RELOAD_VAL_OFFSET  - 0x10) + (0x10 * timer), 0 );
    SET_REG_WORD ((timCfg.timerBaseAddress + TIM0_CONFIG_OFFSET      - 0x10) + (0x10 * timer), 0 );

    // Then clear any outstanding IRQ and disable
    DrvIcbIrqClear(IRQ_TIMER + timer);  
	DrvIcbDisableIrq(IRQ_TIMER + timer);

    if (timCfg.sysTimers[timer].type == TIMER_CHAINED_HIGH)     
    {
        // Disble the timer and IRQ source
        SET_REG_WORD ((timCfg.timerBaseAddress + TIM0_RELOAD_VAL_OFFSET  - 0x10) + (0x10 * (timer-1)), 0 );
        SET_REG_WORD ((timCfg.timerBaseAddress + TIM0_CONFIG_OFFSET      - 0x10) + (0x10 * (timer-1)), 0 );
        timCfg.sysTimers[timer-1].type     = TIMER_FREE;
        DPRINTF1("\nDisable Chained Timer: %d\n",timer-1);
    }

    // Last step, mark it free
    timCfg.sysTimers[timer].type           = TIMER_FREE;

    return timer;
}

void DrvTimerAdjustTimerValue(u32 timer, u32 numMicroSeconds)
{
    u32 configAddr = (timCfg.timerBaseAddress + TIM0_CONFIG_OFFSET  - 0x10) + (0x10 * (timer-0));
    SET_REG_WORD ((timCfg.timerBaseAddress + TIM0_RELOAD_VAL_OFFSET  - 0x10) + (0x10 * (timer-0)), DrvCprGetSysClocksPerUs()*numMicroSeconds);
    SET_REG_WORD (configAddr, GET_REG_WORD_VAL(configAddr)| D_TIMER_CFG_FORCE_RELOAD);

	return;
}

int DrvTimerIncrementAfterMicro(u32 numMicroSeconds,volatile u32 * pValueToIncrement, int repeatCount)
{
    int retVal;
    int timer = getFreeTimer(numMicroSeconds);
    timCfg.sysTimers[timer].pDataVal = pValueToIncrement;
    retVal = timerCallAfterMicro(timer,numMicroSeconds,&updateDataValue,repeatCount,DEFAULT_TIMER_PRIORITY); 
    assert(retVal > 0);
    return retVal;
}

int DrvTimerCallAfterMicro(u32 numMicroSeconds, tyTimerCallback callback, int repeatCount, int priority) // 0 is forever
{
    int retVal;
    int timer = getFreeTimer(numMicroSeconds);
    if (timer<0)
    {
        assert(FALSE);
        return timer;
    }

    retVal = timerCallAfterMicro(timer,numMicroSeconds,callback,repeatCount,priority);
    assert(retVal >= 0);
    DPRINTF1("\nDrvTimerCallAfterMicro()-> returned (%d)\n",retVal);
    return retVal;
}

void DrvTimerTimeoutInitUs(tyTimeStamp * pTimeStamp,u32 intervalUs)
{
    *pTimeStamp =  DrvTimerGetSysTicks64() + ((u64)intervalUs * (u64)DrvCprGetSysClocksPerUs() ); 
    return;
}                  

int DrvTimerTimeout(tyTimeStamp * pTimeStamp)
{
    if ( DrvTimerGetSysTicks64() > (*pTimeStamp) )
        return 1;
    else
        return 0;
}                  

double DrvTimerUptimeMs(void)
{
    u64 upTimeTicks;
    upTimeTicks = DrvTimerGetSysTicks64();
    return DrvTimerTicksToMs(upTimeTicks);
}

void DrvTimerStart(tyTimeStamp * pTimeStamp)
{
   *pTimeStamp = DrvTimerGetSysTicks64();
   return;
}

u64 DrvTimerElapsedTicks(tyTimeStamp * pTimeStamp)
{
   return (DrvTimerGetSysTicks64() - (*pTimeStamp));
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// Static Function Implementations
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

static int getFreeTimer(u32 numMicroSeconds)
{
    int timer;
    static u64 clocksRequired;
    static u32 lowClocks;
    static u32 highClocks;

    DPRINTF1("\nNum_microSeconds: %d\n",numMicroSeconds);
    // Detect if we need timer chaining
    clocksRequired = (u64)numMicroSeconds * (u64)DrvCprGetSysClocksPerUs();
    lowClocks  = (clocksRequired & 0xFFFFFFFF);
    highClocks = (clocksRequired >> 32);

    DPRINTF1("\nCLOCKS: %08X %08X",(u32)(clocksRequired >> 32),(u32)(clocksRequired & 0xFFFFFFFF));
    if (clocksRequired > (u64) 0xFFFFFFFF )
    {
        // Get 2 consecutive free timer resources
        for (timer=LAST_AVAILABLE_TIMER;timer>=2;timer--)
        {
            if ( (timCfg.sysTimers[timer  ].type == TIMER_FREE) && 
                 (timCfg.sysTimers[timer-1].type == TIMER_FREE) 
                 )
            {  
                timCfg.sysTimers[timer  ].type = TIMER_CHAINED_HIGH;
                timCfg.sysTimers[timer-1].type = TIMER_CHAINED_LOW;
                break;
            }
        }
        if (timer < 2 ) // No Free timer available
        {
            assert(FALSE); 
            return ERR_NO_FREE_TIMER;
        }
    }
    else
    {
        // Get a single free timer resource
        for (timer=LAST_AVAILABLE_TIMER;timer>=1;timer--)
        {
            if (timCfg.sysTimers[timer].type == TIMER_FREE)
            {  
                timCfg.sysTimers[timer  ].type = TIMER_NORMAL;
                break;
            }
        }
        if (timer < 1 ) // No Free timer available
        {
            assert(FALSE); 
            return ERR_NO_FREE_TIMER;
        }
    }


    return timer;
}

static int timerCallAfterMicro(u32 timer,u32 numMicroSeconds, tyTimerCallback callback, int repeatCount, int priority) 
{
    u32 timerConfig;
    u64 clocksRequired;

    timCfg.sysTimers[timer].callbackFunc = callback;

    // Ensure there is no IRQ Pending before enabling Irqs
    CLR_REG_BITS_MASK((timCfg.timerBaseAddress + TIM0_CONFIG_OFFSET     - 0x10) + (0x10 * timer), D_TIMER_CFG_IRQ_PENDING);

    DPRINTF1("timer_no %d, in irq no %d\n", timer, IRQ_TIMER + timer);
   
    DrvIcbSetupIrq(IRQ_TIMER + timer,priority, POS_LEVEL_INT, &timerIrqHandler ); 

    // Load the current reload value into the countdown timer, enable countdown and irq
    timerConfig = D_TIMER_CFG_FORCE_RELOAD    |  
                  D_TIMER_CFG_ENABLE          |  
                  D_TIMER_CFG_EN_INT;          

    if (repeatCount != 1) // Unless oneshot, we auto-reload the timer
        timerConfig |= D_TIMER_CFG_RESTART;

    timCfg.sysTimers[timer].repeatCount = repeatCount;

    // Detect if we need timer chaining
    clocksRequired = (u64)numMicroSeconds * (u64)DrvCprGetSysClocksPerUs();

    if (clocksRequired > (u64) 0xFFFFFFFF )
    {
        DPRINTF1("\nUsing Timers: %d,%d\n",timer,timer-1);
        if (repeatCount != 1)
        {
            // Hardware doesn't accurately support repeated timer for clock counts > 32 bits
            DrvTimerDisable(timer);
            assert(FALSE);
            return ERR_SINGLE_SHOT_ONLY_LARGE_TIMER;
        }

        // Upper Timer
        SET_REG_WORD ((timCfg.timerBaseAddress + TIM0_RELOAD_VAL_OFFSET  - 0x10) + (0x10 * timer), (clocksRequired >> 32));
        // Chain the timer with the next lower timer
        SET_REG_WORD ((timCfg.timerBaseAddress + TIM0_CONFIG_OFFSET      - 0x10) + (0x10 * timer), timerConfig | D_TIMER_CFG_CHAIN );

        // Lower timer
        // First we load the timer with the remainder value
        SET_REG_WORD ((timCfg.timerBaseAddress + TIM0_RELOAD_VAL_OFFSET  - 0x10) + (0x10 * (timer-1)), (clocksRequired & 0xFFFFFFFF) );
        SET_REG_WORD ((timCfg.timerBaseAddress + TIM0_CONFIG_OFFSET      - 0x10) + (0x10 * (timer-1)), D_TIMER_CFG_FORCE_RELOAD);
        // Then we set the reload value to 0xFFFFFFFF so it continues to count down from there
        // and enable it to auto restart
        SET_REG_WORD ((timCfg.timerBaseAddress + TIM0_RELOAD_VAL_OFFSET  - 0x10) + (0x10 * (timer-1)), 0xFFFFFFFF);
        SET_REG_WORD ((timCfg.timerBaseAddress + TIM0_CONFIG_OFFSET      - 0x10) + (0x10 * (timer-1)), D_TIMER_CFG_RESTART    |   
                                                                          D_TIMER_CFG_ENABLE);
    }
    else
    {
        DPRINTF1("\nUsing Timer : %d \n",timer);
        SET_REG_WORD ((timCfg.timerBaseAddress + TIM0_RELOAD_VAL_OFFSET  - 0x10) + (0x10 * timer), DrvCprGetSysClocksPerUs()*numMicroSeconds );
        // As last step we enable the timer
        SET_REG_WORD ((timCfg.timerBaseAddress + TIM0_CONFIG_OFFSET      - 0x10) + (0x10 * timer), timerConfig );
    }

    return timer;  // Return the timer index to the caller so that they can disable if necessary
}



void SleepMs(unsigned int numMs)
{
    SleepMicro(numMs * 1000);
    return;
}

void SleepMicro(unsigned int numMicroSeconds)
{
    SleepTicks((u64)numMicroSeconds * (u64)DrvCprGetSysClocksPerUs());
    return;
}


static u32 updateDataValue(u32 timer, u32 param0)
{
    DPRINTF1("\nUpdate Data for timer %d\n",timer);

    param0 = param0; // deliberately unused. Reserved for future use.
    // Simply increment the counter in the users application
    timCfg.sysTimers[timer].pDataVal[0]++;
    DPRINTF1("\nNew Val %d\n", timCfg.sysTimers[timer].pDataVal[0]);    
    return 0;
}



static void timerIrqHandler(u32 irq_source)
{
    u32 timer = irq_source - IRQ_TIMER;

    // Call the user specified callback function
    timCfg.sysTimers[timer].callbackFunc(timer,0);

    // Clear interrupt source
    CLR_REG_BITS_MASK((timCfg.timerBaseAddress + TIM0_CONFIG_OFFSET     - 0x10) + (0x10 * timer), D_TIMER_CFG_IRQ_PENDING);
	DrvIcbIrqClear(IRQ_TIMER + timer); // Clear the irq in ICB

    if (timCfg.sysTimers[timer].repeatCount > 0) // 0 means infinite
    {
        timCfg.sysTimers[timer].repeatCount--;
        if (timCfg.sysTimers[timer].repeatCount == 0) // If this brings us to zero
        {
            DrvTimerDisable(timer);
        }
    }

    return;
}

