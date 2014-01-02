///   
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved. 
///            For License Warranty see: common/license.txt   
///
/// @brief     Driver to control Clocks, Power and Reset
/// 
/// 
/// 
// Module Goals:
// 1. PLL Configuration, reporting actual PLL frequency
// 2. Handling different Osc_in sources
//    -- Change Osc In source
// 3. Sys Config??
// 4. Setup Tick count timer
// 5. Control of auxilary clocks
// 6. Enable/ disable clcoks
// 7. Report a given clock frequency including breakdown of "funny clocks"
// 8. 

// 1: Includes
// ----------------------------------------------------------------------------
#include "DrvCpr.h"
#include "DrvTimer.h"
#include "isaac_registers.h"
#include "stdio.h"
#include "DrvIcb.h"
#include "assert.h"


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

#define SYSTEM_TICKS_TIMER              (8)             // Timer 8 always used
#ifndef SYSTEM_TICKS_TIMER_PRIORITY
#define SYSTEM_TICKS_TIMER_PRIORITY     (1)             // Set for low priority
#endif
#define SYSTEM_TICKS_TIMER_CLKS         (0xE0000000)    // Towards the end of the 32 bit word, but not too close for comfort

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
  u32 lastTicks;                          // Last recorded lower 32 bits of tick counter
  u32 upperTicks32;                       // Software record of the upper 32 bits of the tick counter
  tyTimerConfig sysTimers[NUM_TIMERS+1];  // configuration state for each timer
  u8  initialised;                        // So we don't re-init
  u8  ticksTimer;                         // Used to store the global timer we use for monitoring ticks
} tyTimerState;


// 3: Global Data (Only if absolutely necessary)
// ----------------------------------------------------------------------------


// 4: Static Local Data 
// ----------------------------------------------------------------------------
static tyTimerState timCfg;                   

// 5: Static Function Prototypes
// ----------------------------------------------------------------------------

static void setupTickCounterIrq(void);
static int timerCallAfterMicro(u32 timer,u32 numMicroSeconds, tyTimerCallback callback, int repeatCount, int priority);
static void update64BitTicksCounter(u32 source);
static int getFreeTimer(u32 numMicroSeconds);
static u32 updateDataValue(u32 timer, u32 param0);
static inline void timerIrqHandler(u32 irqSource);

// 6: Functions Implementation
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// API Function Implementations
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

int DrvTimerInit(void)
{
    int timer;

    if (timCfg.initialised)
        return ERR_TIM_ALREADY_INTIALISED; // The timer driver should only be initialised once at startup by CPR Init

    // Enable 64 bit mode of free running clock
    for (timer=0;timer<=LAST_AVAILABLE_TIMER;timer++)
    {
        timCfg.sysTimers[timer].type           = TIMER_FREE;
        timCfg.sysTimers[timer].callbackFunc   = NULL;
        timCfg.sysTimers[timer].pDataVal       = NULL;
        timCfg.sysTimers[timer].repeatCount    = 0;
    }

    // Enable the free running clock, and the scaler
    SET_REG_BITS_MASK(TIM_GEN_CONFIG_ADR    ,   D_TIMER_GENCFG_FREE_CNT_EN | D_TIMER_GENCFG_PRESCALER_ENABLE); 

    // Set the prescaler to 0 so that we use the system clock direct for the timers
    // This gives us maximum resolution and we can chain registers to get larger counts
    // It is slightly more power hungry, but I think the tradeoff is worth it
    SET_REG_WORD(TIM_SCALER_RELOAD_ADR,0);

    // Setup an IRQ to automatically update the free counter before it overflows
    setupTickCounterIrq();

    swcLeonSetPIL(0);     // Enable interrupts
    timCfg.initialised = TRUE; // This function should only be run once

    return 0;
}

u64 DrvTimerGetSysTicks64(void)
{
    u32 currentTicks;

    /// TODO: Review performance of this function
    swcLeonDisableTraps(); // The following operations need to be atomic
    currentTicks = GET_REG_WORD_VAL(TIM_FREE_CNT_ADR);
    if (currentTicks < timCfg.lastTicks)
        timCfg.upperTicks32++;
    timCfg.lastTicks = currentTicks; 
    swcLeonEnableTraps();

    return ((u64)timCfg.upperTicks32 << 32) | ((u64)currentTicks);   
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
    SET_REG_WORD ((TIM0_RELOAD_VAL_ADR  - 0x10) + (0x10 * timer), 0 );
    SET_REG_WORD ((TIM0_CONFIG_ADR      - 0x10) + (0x10 * timer), 0 );

    // Then clear any outstanding IRQ and disable
    DrvIcbIrqClear(IRQ_TIMER + timer);
	DrvIcbDisableIrq(IRQ_TIMER + timer);

    if (timCfg.sysTimers[timer].type == TIMER_CHAINED_HIGH)
    {
        // Disble the timer and IRQ source
        SET_REG_WORD ((TIM0_RELOAD_VAL_ADR  - 0x10) + (0x10 * timer-1), 0 );
        SET_REG_WORD ((TIM0_CONFIG_ADR      - 0x10) + (0x10 * timer-1), 0 );
        timCfg.sysTimers[timer-1].type     = TIMER_FREE;
        DPRINTF1("\nDisable Chained Timer: %d\n",timer-1);
    }

    // Last step, mark it free
    timCfg.sysTimers[timer].type           = TIMER_FREE;

    return timer;
}

void DrvTimerAdjustTimerValue(u32 timer, u32 numMicroSeconds)
{
    u32 configAddr = (TIM0_CONFIG_ADR      - 0x10) + (0x10 * (timer-0));
    SET_REG_WORD ((TIM0_RELOAD_VAL_ADR  - 0x10) + (0x10 * (timer-0)), DrvCprGetSysClocksPerUs()*numMicroSeconds);
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
    CLR_REG_BITS_MASK((TIM0_CONFIG_ADR     - 0x10) + (0x10 * timer), D_TIMER_CFG_IRQ_PENDING);

    DrvIcbSetupIrq(IRQ_TIMER + timer,priority,POS_LEVEL_INT,&timerIrqHandler);

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
        SET_REG_WORD ((TIM0_RELOAD_VAL_ADR  - 0x10) + (0x10 * timer), (clocksRequired >> 32));
        // Chain the timer with the next lower timer
        SET_REG_WORD ((TIM0_CONFIG_ADR      - 0x10) + (0x10 * timer), timerConfig | D_TIMER_CFG_CHAIN );

        // Lower timer
        // First we load the timer with the remainder value
        SET_REG_WORD ((TIM0_RELOAD_VAL_ADR  - 0x10) + (0x10 * (timer-1)), (clocksRequired & 0xFFFFFFFF) );
        SET_REG_WORD ((TIM0_CONFIG_ADR      - 0x10) + (0x10 * (timer-1)), D_TIMER_CFG_FORCE_RELOAD);
        // Then we set the reload value to 0xFFFFFFFF so it continues to count down from there
        // and enable it to auto restart
        SET_REG_WORD ((TIM0_RELOAD_VAL_ADR  - 0x10) + (0x10 * (timer-1)), 0xFFFFFFFF);
        SET_REG_WORD ((TIM0_CONFIG_ADR      - 0x10) + (0x10 * (timer-1)), D_TIMER_CFG_RESTART    |   
                                                                          D_TIMER_CFG_ENABLE);
    }
    else
    {
        DPRINTF1("\nUsing Timer : %d \n",timer);
        SET_REG_WORD ((TIM0_RELOAD_VAL_ADR  - 0x10) + (0x10 * timer), DrvCprGetSysClocksPerUs()*numMicroSeconds );
        // As last step we enable the timer
        SET_REG_WORD ((TIM0_CONFIG_ADR      - 0x10) + (0x10 * timer), timerConfig );
    }


    return timer;  // Return the timer index to the caller so that they can disable if necessary
}

static void setupTickCounterIrq(void) 
{
    // Ensure there is no IRQ Pending before enabling Irqs
    CLR_REG_BITS_MASK((TIM0_CONFIG_ADR     - 0x10) + (0x10 * SYSTEM_TICKS_TIMER), D_TIMER_CFG_IRQ_PENDING);

    DrvIcbSetupIrq(IRQ_TIMER + SYSTEM_TICKS_TIMER,SYSTEM_TICKS_TIMER_PRIORITY,POS_LEVEL_INT,&update64BitTicksCounter);

    SET_REG_WORD ((TIM0_RELOAD_VAL_ADR  - 0x10) + (0x10 * SYSTEM_TICKS_TIMER), SYSTEM_TICKS_TIMER_CLKS);

    // As last step we enable the timer
    SET_REG_WORD ((TIM0_CONFIG_ADR      - 0x10) + (0x10 * SYSTEM_TICKS_TIMER),(D_TIMER_CFG_FORCE_RELOAD    |  
                                                                               D_TIMER_CFG_ENABLE          |  
                                                                               D_TIMER_CFG_EN_INT          |
                                                                               D_TIMER_CFG_RESTART));            
    return;
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
    return 0;
}


static void timerIrqHandler(u32 irqSource)
{
    u32 timer = irqSource - IRQ_TIMER;

    // Call the user specified callback function
    timCfg.sysTimers[timer].callbackFunc(timer,0);

    // Clear interrupt source
    CLR_REG_BITS_MASK((TIM0_CONFIG_ADR     - 0x10) + (0x10 * timer), D_TIMER_CFG_IRQ_PENDING);
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

// This happens on an IRQ triggered to update at least once
// every 32 bit cycle
static void update64BitTicksCounter(u32 irqSource)
{
    u32 currentTicks;

    irqSource = irqSource; // Deliberately unused.

    // Note This procedure is inherently atomic as it only
    // happens in an IRQ handler
    currentTicks = GET_REG_WORD_VAL(TIM_FREE_CNT_ADR);
    if (currentTicks < timCfg.lastTicks)
        timCfg.upperTicks32++;

    timCfg.lastTicks = currentTicks; 

    // Clear interrupt source
    CLR_REG_BITS_MASK((TIM0_CONFIG_ADR     - 0x10) + (0x10 * SYSTEM_TICKS_TIMER), D_TIMER_CFG_IRQ_PENDING);
	DrvIcbIrqClear(IRQ_TIMER + SYSTEM_TICKS_TIMER); // Clear the irq in ICB

    return;
}
