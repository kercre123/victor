///
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved
///            For License Warranty see: common/license.txt
///
/// @brief     InfraRedSensor.c
///
/// This is the IR driver
///


// 1: Includes
// ----------------------------------------------------------------------------
#include "InfraRedSensor.h"
#include "RemoteControlApi.h"
#include <mv_types.h>
#include <stdio.h>
#include <string.h>
#include <isaac_registers.h>
#include <DrvCpr.h>
#include "DrvGpio.h"
#include "DrvIcb.h"
#include "DrvTimer.h"


// 2:  Source Specific #defines and types  (typedef,enum,struct)
// --------------------------------------------------------------------------


// 3: Global Data (Only if absolutely necessary)
// ----------------------------------------------------------------------------
volatile u8 currentRemoteKey = RCU_NULL;
extern u32 RemoteControlKeys[RCU_FUNCTIONS_NO];


// 4: Static Local Data
// ----------------------------------------------------------------------------
static          unsigned    infraRedSystemClock     = 0;
static volatile int         pendingInfraRedKey      = 0;
static volatile u32         infraRedTimestamp       = 0;
static volatile u32         startTiming             = 0;
static          float       repeatValidationMax     = 0;

static u32 priority;
static u32 gpionumber;
static u32 config;
static u32 irGpio;

static volatile struct
{
    #ifdef STRUCT_MARGINS
    u32     startMarker;
    #endif

    struct
    {
        callback    callbackFunction;
        struct
        {
            // prefix: 9ms "1" [16 x T_min], 4.5ms "0" [8 x T_min]
            // repeat: 9ms "1" [16 x T_min], 2.25ms "0" [4 x T_min]
            // bit high: 562.5us [T_min]
            // bit "1" low: 562.5us [T_min]
            // bit "0" low: 1.6875ms [3 x T_min]
            u32   Min;          // ...
            u32   Max;          // ...
        } Ticks; // Accepted ticks interval for 562.5us
    } Configuration;

    struct
    {
        u32     LastTs[2];      // ...
        u32     Duration[2];    // timestamp of last line-turned-0 moment
        u32     Line;           // expected 0 or 1 after the next irq
        u32     Data;           // current sampling state
        u32     Bitmask;        // current sampling state
        u32     LastData;       // ...
    } State;

    #ifdef STRUCT_MARGINS
    u32     EndMarker;          // ...
    #endif
} NEC;

//Used for debauching
unsigned int RemoteDeb;


// 5: Static Function Prototypes
// ----------------------------------------------------------------------------

// 6: Functions Implementation
// ----------------------------------------------------------------------------
// IR Handler
static void NECGpioHandler(u32 unusued)
{
    // Read the current timestamp
    u32 ts = (u32)DrvTimerGetSysTicks64(); // We only need 32 bits of resolution
    // Change IRQ polarity
    config ^= NEG_EDGE_INT;
    DrvIcbConfigureIrq(gpionumber, priority, config);

    // Get (expected) line state
    u32 line = NEC.State.Line ^ 1;

    NEC.State.Duration[line] = ts - NEC.State.LastTs[line ^ 1];
    NEC.State.LastTs[line] = ts;
    NEC.State.Line = line;
    u32 tmin = NEC.Configuration.Ticks.Min;
    u32 tmax = NEC.Configuration.Ticks.Max;

    // Every two interrupts, check for new bit or lead code
    if (!line)
    {
        // short high?
        if ((tmin < NEC.State.Duration[1]) && (NEC.State.Duration[1] < tmax))
        {
            // short low? ("1")
            if ((tmin < NEC.State.Duration[0]) && (NEC.State.Duration[0] < tmax))
            {
                IR_detected ('1');
                NEC.State.Data |= NEC.State.Bitmask;
                if (NEC.State.Bitmask & 0x80808080)
                {
                    NEC.State.Bitmask >>= 15;
                }
                else
                {
                    NEC.State.Bitmask <<= 1;
                }
            }

            // 3x short low? ("0")
            else if (((3 * tmin) < NEC.State.Duration[0]) && (NEC.State.Duration[0] < (3 * tmax)))
            {
                IR_detected ('0');
                if (NEC.State.Bitmask & 0x80808080)
                {
                    NEC.State.Bitmask >>= 15;
                }
                else
                {
                    NEC.State.Bitmask <<= 1;
                }
            }

            // Line noise, probably OR false positive after last bit or repeat (the later always happens)
            else if (NEC.State.Data)
            {
                IR_detected ('b'); // "bit" error
                NEC.State.Data = 0;
                NEC.State.Bitmask = 0;
                NEC.State.LastData = 0;
            }
        }

        // Really long (16x) high?
        else if (((16 * tmin) < NEC.State.Duration[1]) && (NEC.State.Duration[1] < (16 * tmax)))
        {
            // really long (8x) low?
            if (((8 * tmin) < NEC.State.Duration[0]) && (NEC.State.Duration[0] < (8 * tmax)))
            {
                // start
                startTiming = ts;
                IR_detected ('S');
                NEC.State.Data = 0;
                NEC.State.Bitmask = 0x01000000;
            }

            // not-so-long (4x) low?
            else if (((4 * tmin) < NEC.State.Duration[0]) && (NEC.State.Duration[0] < (4 * tmax)))
            {
                // repeat
                if ((ts - startTiming) <= repeatValidationMax)
                {
                    IR_detected ('R');
                    startTiming = ts;
                    NEC.State.Data = NEC.State.LastData;
                }
                else
                {
                    IR_detected ('r'); // False repeat
                    NEC.State.LastData = 0;
                }
            }
            else // wtf - line noise, probably
            {
                IR_detected ('l'); // "lead" error
                NEC.State.LastData = 0;
            }
        }
    }
    else
        if (!NEC.State.Bitmask && NEC.State.Data) // && line
        {
            // final pulse, after either data or repeat?
            if ((tmin < NEC.State.Duration[1]) && (NEC.State.Duration[1] < tmax))
            {
                // stop
                IR_detected ('P');
                NEC.State.LastData = NEC.State.Data;
                NEC.State.Data = 0;

                // check data validity
                if (0xFF != ((NEC.State.LastData ^ (NEC.State.LastData >> 8)) & 0xFF))
                {
                    IR_detected ('c'); // "consistency" error
                    NEC.State.LastData = 0;
                }
                else
                {
                    (*NEC.Configuration.callbackFunction)(NEC.State.LastData & 0xff);
                }
            }
            else
            {
                IR_detected ('t'); // "tail" error
                NEC.State.LastData = 0;
            }
        }
}

// Before initiate we must be sure that all irq are cleared and the remaining data is removed
void REMOTE_CONTROL_CODE(NECinfraRedDone) (void)
{
    // Disable interrupt source
    if (DrvIcbIrqSrc() == gpionumber)
    {
        DrvIcbDisableIrq(gpionumber);
    }

    // Prevent source from triggering interrupts
    if (DrvGpioGetMode(irGpio))
    {
        DrvGpioMode(irGpio, DrvGpioGetMode(irGpio) & (~(D_GPIO_IRQ_SRC_0 | D_GPIO_IRQ_SRC_1)));
    }

   // Clear any recent interrupt
    if (DrvIcbIrqPending(gpionumber))
    {
        DrvIcbIrqClear(gpionumber);
    }

    // Cleanup the data structure
    __memset_fast((void *)&NEC, 0, sizeof(NEC));

    #ifdef STRUCT_MARGINS
        NEC.startMarker = 'IR[[';
        NEC.EndMarker   = ']]RI';
    #endif
}

// Initialize IR Driver
int REMOTE_CONTROL_CODE(NECinfraRedInit) (int systemClockHz, int gpioNumber, int infraRedGpio, int interruptPriority, callback callbackFunction)
{
    NECinfraRedDone();

    // Check function input parameters
    if ((systemClockHz < 9600000) || (systemClockHz > 240000000))
    {
        return -1;
    }

    if ((gpioNumber < 0) || (gpioNumber > 1))
    {
        return -2;
    }

    if ((infraRedGpio < 0) || (infraRedGpio > 142))
    {
        return -3;
    }

    if ((interruptPriority < 1) || (interruptPriority > 15))
    {
        return -4;
    }

    if (!callbackFunction)
    {
        return -5;
    }

    // Save callback routine
    NEC.Configuration.callbackFunction = callbackFunction;
    irGpio =  infraRedGpio;

    // Compute ticks count with specified tolerance, for 562.5us
    float ticks = 5.625e-6f * (float)systemClockHz; // number of ticks per 5.625us
    float ticks_108 = 1.08e-3f * (float)systemClockHz; //

    // Min number of ticks per 562.5us
    NEC.Configuration.Ticks.Min = (u32)((100.0f - (float)TOLERANCE) * ticks);

    // Max number of ticks per 562.5us
    NEC.Configuration.Ticks.Max = (u32)((100.0f + (float)TOLERANCE) * ticks + 0.5f);

    // The time between a start and a repeat or between  a valid repeat and another repeat
    repeatValidationMax = (u32)((100.0f + (float)TOLERANCE) * ticks_108 + 0.5f);

    // Disable the GPIO interrupt
    gpioNumber += IRQ_GPIO_0;

    priority = interruptPriority;
    gpionumber = gpioNumber;

    DrvIcbDisableIrq(gpioNumber);

    NEC.State.Line = 1;

    // Make SURE no other GPIO generates the used GPIO interrupt
    u32 t = (gpioNumber == IRQ_GPIO_0) ? D_GPIO_IRQ_SRC_0 : D_GPIO_IRQ_SRC_1;

    DrvGpioIrqResetAll();

    DrvGpioMode(infraRedGpio, D_GPIO_MODE_7 | D_GPIO_DIR_IN | t );

    // Set interrupt handler for GPIO and configure interrupt
    DrvIcbSetIrqHandler(gpioNumber, NECGpioHandler);

    config = NEG_EDGE_INT;
    DrvIcbConfigureIrq(gpioNumber, interruptPriority, config);

    // Clear and Enable the relevant GPIO IRQ source
    DrvIcbDisableIrq(gpioNumber);
    DrvIcbEnableIrq(gpioNumber);

    return 0;
}

// The callback function
void NEConRemote (int command)
{
    IR_PRINTF ("{ cmd = 0x%02X } \n",  command);

    //debouncing
    u32 cur_ticks = (u32)DrvTimerGetSysTicks64(); // We only need 32 bits of resolution
    if ((cur_ticks - infraRedTimestamp) > RemoteDeb)
    {
        //accept key
        IR_PRINTF("ir key (0x%02x) received\n", command);
        pendingInfraRedKey = NECRemoteControlGetButton(command);
        infraRedTimestamp = cur_ticks;  // only change timestamp when accepted

        // post pending keys
        if (pendingInfraRedKey != RCU_NULL)
        {
            IR_PRINTF("post key (%d)\n", pendingInfraRedKey);
            NECRemoteControlSetCurrentKey(pendingInfraRedKey);
            pendingInfraRedKey = RCU_NULL;
        }
    }
    else
    {
        //drop key
        IR_PRINTF("ir key (0x%02x) dropped\n", command);
    }
}

// The parameters must be set in order to initiate the driver.
int REMOTE_CONTROL_CODE(NECInfraRedInit) (int infraRedGpio)
{
    NECinfraRedInit (DrvCprGetSysClockKhz() * 1000, 0, infraRedGpio, INFRA_RED_INTERRUPT_LEVEL, &NEConRemote);
    infraRedSystemClock = DrvCprGetSysClockKhz();

    // reset stuff
    infraRedTimestamp = 0;
    pendingInfraRedKey = RCU_NULL;
    RemoteDeb = 300 * DrvCprGetSysClockKhz();
    return 0;
}

// Makes sure that the timer uses the system clock properly
void REMOTE_CONTROL_CODE(NECInfraRedMain) (int infraRedGpio)
{
    // Monitor changes in sysclk that would affect the operation of timer
    if (DrvCprGetSysClockKhz() != infraRedSystemClock)
    {
        IR_PRINTF("sysclk is changed (%d->%dkHz)\n", infraRedSystemClock, DrvCprGetSysClockKhz());
        IR_PRINTF("reconfigure IR module\n");
        NECInfraRedInit(infraRedGpio);
    }
}

// Sets the cmd code.
void REMOTE_CONTROL_CODE(NECRemoteControlSetCurrentKey) (u8 key)
{
    currentRemoteKey = key;
}

