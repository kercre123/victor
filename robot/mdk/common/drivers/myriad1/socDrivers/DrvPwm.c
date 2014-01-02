///   
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved. 
///            For License Warranty see: common/license.txt   
///
/// @brief     Driver to utilise PWM on Myriad Soc
/// 
/// 
/// 

// 1: Includes
// ----------------------------------------------------------------------------
#include "mv_types.h"
#include "DrvPwm.h"
#include "DrvGpio.h"
#include "isaac_registers.h"
#include "assert.h"

// 2:  Source Specific #defines and types  (typedef,enum,struct)
// ----------------------------------------------------------------------------
#ifndef MIN
#   define MIN(a,b) (a>b?b:a)
#endif

#ifndef MAX
#   define MAX(a,b) (a>b?a:b)
#endif
// 3: Global Data (Only if absolutely necessary)
// ----------------------------------------------------------------------------

// 4: Static Local Data 
// ----------------------------------------------------------------------------

// 5: Static Function Prototypes
// ----------------------------------------------------------------------------
static u32 getPwmValueForRatio(int minVal,int maxVal,int targetVal,int pwmPeriod);

// 6: Functions Implementation
// ----------------------------------------------------------------------------

// Configures PWM to drive out a given voltage
int DrvPwmSetupVoltageControl(tyPwmVoltageConfig * cfg,u32  targetVoltageMv)
{
    u32 pwmValue;
    u32 pwmPeriod;

    // We cannot set a voltage that the hardware does not support, so we clamp
    targetVoltageMv = MIN(cfg->vmaxHardwareMv ,targetVoltageMv); 
    targetVoltageMv = MAX(cfg->vminHardwareMv ,targetVoltageMv);

    // Software may further limit the range to which we can configure a voltage
    targetVoltageMv = MIN(targetVoltageMv , cfg->vmaxSoftwareClampMv);
    targetVoltageMv = MAX(targetVoltageMv , cfg->vminSoftwareClampMv);
                                      
    // Calculate the necessary pwm value to give the desired voltage
    pwmPeriod = cfg->sysFreqHz/cfg->pwmFreqHz;
    pwmValue = getPwmValueForRatio(cfg->vminHardwareMv,cfg->vmaxHardwareMv,targetVoltageMv,pwmPeriod);

    // Finally setup the PWM to generate the desired voltage

    DrvGpioMode(cfg->gpioNum, cfg->gpioMode);
    // Configure the appropriate PWM and enable by default
    DrvPwmConfigure(cfg->pwmDevice,/*repeat*/0, /*leadIn*/0, /*hiCnt*/ (pwmValue >> 16), /*lowCnt*/(pwmValue&0xFFFF) + 1,1);

    return 0; // For now this function cannot fail
}

// Single function to configure and enable any of the 5 PWM outputs
void DrvPwmConfigure(u32 pwmNumber,u32 repeat, u32 leadIn, u32 highCount, u32 lowCount,u32 defaultEnable)
{
    u32 pwmLeadInAddr  = GPIO_PWM_LEADIN0_ADR + (4 * pwmNumber);
    u32 pwmHighLowAddr = GPIO_PWM_HIGHLOW0_ADR + (4 * pwmNumber);
    u32 leadinValue;

    assert(pwmNumber <= 5);

    leadinValue = (repeat & 0xFFFF) | ((leadIn << 16) & 0x7FFF);

    if (defaultEnable)
        leadinValue |= 0x80000000; 

    SET_REG_WORD(pwmLeadInAddr ,leadinValue);
    SET_REG_WORD(pwmHighLowAddr,((highCount<<16) & 0xFFFF0000) | (lowCount & 0xFFFF));

    return;
}

// Single function to simply enable or disable any of the 5 PWM outputs (assumes they are pre-configured)
void DrvPwmEnable(u32 pwmNumber,u32 enable)
{
    u32 pwmLeadInAddr  = GPIO_PWM_LEADIN0_ADR + (4 * pwmNumber);

    assert(pwmNumber <= 5);

    if (enable)
     SET_REG_WORD(pwmLeadInAddr, GET_REG_WORD_VAL(pwmLeadInAddr) | 0x80000000 );
    else
     SET_REG_WORD(pwmLeadInAddr, GET_REG_WORD_VAL(pwmLeadInAddr) & (!0x80000000) );   

    return;
}


/// Simple function which translates a value within a range into
/// a PWM High,Low Pair with an equivalent Ratio
///
static u32 getPwmValueForRatio(int minVal,int maxVal,int targetVal,int pwmPeriod)
{
    int deltaFromMin;
    int lowTime;
    int highTime;

    deltaFromMin =  targetVal - minVal;
    lowTime      = (deltaFromMin * pwmPeriod) / (maxVal - minVal);
    highTime     = pwmPeriod - lowTime;

    if( lowTime < 0 || highTime < 0 )
      return 0;

    return ( highTime << 16 )| lowTime;
}

