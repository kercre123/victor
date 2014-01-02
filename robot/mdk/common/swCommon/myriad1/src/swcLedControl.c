///   
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved. 
///            For License Warranty see: common/license.txt   
///
/// @brief     Common General Purpose module for the control of LEDS
/// 
/// @todc      Add Cylon Eye Function
/// @todc      Add support for active low LEDs
/// 

// 1: Includes
// ----------------------------------------------------------------------------
#include <isaac_registers.h>
#include <DrvGpio.h>
#include <swcLedControl.h>
#include <DrvTimer.h>

// 2:  Source Specific #defines and types  (typedef,enum,struct)
// ----------------------------------------------------------------------------

// 3: Global Data (Only if absolutely necessary)
// ----------------------------------------------------------------------------

// 4: Static Local Data 
// ----------------------------------------------------------------------------
static tyLedSystemConfig * ledCfg = 0;

// 5: Static Function Prototypes
// ----------------------------------------------------------------------------

// 6: Functions Implementation
// ----------------------------------------------------------------------------

void swcLedInitialise(tyLedSystemConfig * boardLedConfig)
{
    ledCfg = boardLedConfig;
    return;
}

int swcLedSet(int ledNum, int enable) 
{
    if (ledCfg==NULL)
        return SWC_LED_NO_CFG;

    if (ledNum > ledCfg->totalLeds)
        return SWC_LED_INVALID;

    DrvGpioSetPin(ledCfg->ledArray[ledNum],enable);

    return SWC_LED_SUCCESS;
}

int swcLedToggle(int ledNum) 
{
    if (ledCfg==NULL)
        return SWC_LED_NO_CFG;

    if (ledNum > ledCfg->totalLeds)
        return SWC_LED_INVALID;

    DrvGpioToggleState(ledCfg->ledArray[ledNum]);

    return SWC_LED_SUCCESS;
}


int swcLedSetRange(int ledStart,int ledEnd, int enable) 
{
    int retval = SWC_LED_SUCCESS;
    int i;
    if (ledCfg==NULL)
        return SWC_LED_NO_CFG;

    if ((ledStart < 0) || ledEnd > (ledCfg->totalLeds))
        return SWC_LED_INVALID;

    for (i=ledStart;i<=ledEnd;i++)
        retval = swcLedSet(i,enable);

    return retval;
}

int swcLedSetBinValue(int lsbLed,int msbLed, u32 value) 
{
    int retval = SWC_LED_SUCCESS;
    int i;
    u32 bLsbToLowestLed;
    u32 bitNumber;
    int ledStart;
    int ledEnd;

    if (ledCfg==NULL)
        return SWC_LED_NO_CFG;

    if (lsbLed < msbLed)
    {
        bLsbToLowestLed = TRUE;
        ledStart = lsbLed;
        ledEnd   = msbLed;
    }
    else
    {
        bLsbToLowestLed = FALSE;
        ledStart = msbLed;
        ledEnd   = lsbLed;
    }

    if ((ledEnd - ledStart -1) > 32)   // Can't do this if we have more than 32 LEDS!!
        return SWC_LED_INVALID;

    if ((ledStart < 0) || ledEnd > (ledCfg->totalLeds))
        return SWC_LED_INVALID;

    for (i=ledStart;i<=ledEnd;i++)
        {
        if (bLsbToLowestLed)
            bitNumber = i-ledStart; // Lowest bit references lowest LED number
        else
            bitNumber = ledEnd-i;   // Lowest bit references highest LED number

        swcLedSet(i,(value& (1 << bitNumber))); // Any non-zero value for enable will turn the led on
        }
    return retval;
}

int swcLedCylonEye(u32 ledStart,u32 ledEnd,u32 eyeWidth, int loops,u32 delayMs)
{
    int i;
//    u32 bits=0x3;
    u32 value;
    if (ledCfg==NULL)
        return SWC_LED_NO_CFG;

    value = 0xFFFFFFFF >> (32 - eyeWidth);
    for (i=0;i<loops;i++)
    {
        do
        {
           swcLedSetBinValue(ledStart,ledEnd,value);
           SleepMs(delayMs);
           value = value << 1;
        } while ((value & (1 << (ledEnd - ledStart +1))) == 0);
        do
        {
           swcLedSetBinValue(ledStart,ledEnd,value);
           SleepMs(delayMs);
           value = value >> 1;
        } while ((value & 1) == 0);
    }

    return 0;
}

int swcLedGetNumLeds(void)
{
    if (ledCfg==NULL)
        return SWC_LED_NO_CFG;

    return ledCfg->totalLeds;
}



