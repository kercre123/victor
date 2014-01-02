///  
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved 
///            For License Warranty see: common/license.txt   
///
/// @brief     GPIO Driver API
/// 
/// This module contains the helper functions necessary to control the 
/// general purpose IO block in the Myriad Soc
/// 
/// 

#ifndef DRV_GPIO_H
#define DRV_GPIO_H 

// 1: Includes
// ----------------------------------------------------------------------------
#include "mv_types.h"
#include "DrvGpioDefines.h"
#include "DrvIcb.h"

// 2:  Exported Global Data (generally better to avoid)
// ----------------------------------------------------------------------------

// 3:  Exported Functions (non-inline)
// ----------------------------------------------------------------------------

/// Function to initialise multiple ranges of GPIOs with Pad, Pin and Level configuration
///
/// This function is typically used to enable the default GPIO configuration for a particular board
/// It allows the caller to optionally configure each of the Pin Config; Pad Config; and default level of multiple 
/// ranges of GPIOS.
/// For details of the configuration structure see drvGpioInitArrayType
/// This array is of variable length and is terminted by a ACTION_TERMINATE_ARRAY in the action field
/// If Pad configuration change is needed this is performed in a memory effiecient way
/// @param[in] initArray array of type drvGpioInitArrayType which contains a structure defining the desired GPIO config changes
/// @return    void
void DrvGpioInitialiseRange(const drvGpioInitArrayType initArray);

/// Function to initialise GPIO IRQs
///
/// This functions is used to disable IRQ generation of all GPIOs
/// It's a necessary step to using GPIO IRQs since the expected effect
/// is that only the configured GPIOs should generate interrupts.
/// Call this just once before any/all the DrvGpioIrqConfig calls
/// @return    void
void DrvGpioIrqResetAll();

/// Function to setup a GPIO IRQ
///
/// This is all that should be needed to setup a simple
/// irq callback from a toggling pin
/// @param[in] gpio gpio number corresponding to the pin
/// @param[in] irqSrc DrvGpioDefines.h: D_GPIO_IRQ_SRC_1, D_GPIO_IRQ_SRC_0
///            any/more gpios can trigger an interrupt on any/both of these lines
///            you can Logical OR them to trigger both
/// @param[in] irqType DrvIcbDefines.h: POS_EDGE_INT, NEG_EDGE_INT, POS_LEVEL_INT, NEG_LEVEL_INT
/// @param[in] priority
/// @param[in] callback pointer to a callback function, this will be treated as an ISR
/// @return    void
void DrvGpioIrqConfig( u32 gpio, u32 irqSrc, u32 irqType, u32 priority, irq_handler callback);

/// Sets the target gpio high(1) or low(0) as per value
///
/// The function takes full responsibility for ensuring the pin is configured as a GPIO output
/// @param[in]  gpioNum number of the gpio to change state 
/// @param[in]  value value to set pin to 1=high, 0=low
void DrvGpioSetPin(int gpioNum,int value);

/// Set pin to GPIO Input and read value of the pin level
///
/// The function takes full responsibility for ensuring the pin is configured as a GPIO input
/// @param[in]  gpioNum number of the gpio to read
/// @return     0 if pin is low, 1 if pin is high
int DrvGpioGetPin(int gpioNum);

/// Switches the current state of the gpio to the opposite level
///
/// If the GPIO is currently driven high, it will be driven low and vice versa
/// @param[in] gpioNum gpio number 
void DrvGpioToggleState(int gpioNum);

/* ***********************************************************************//**
   *************************************************************************
GPIO - use this to set a pin HI, when in mode 7

info:
    have to set first the gpio in mode 7 and set the gpio as output 
@param
    gpioNum - number of the GPIO pin to set HI     

************************************************************************* */
void DrvGpioSetPinHi(u32 gpioNum);

/* ***********************************************************************//**
   *************************************************************************
GPIO - use this to set a pin LO, when in mode 7

info:
    have to set first the gpio in mode 7 and set the gpio as output 
@param
    gpioNum - number of the GPIO pin to set LO

************************************************************************* */
void DrvGpioSetPinLo(u32 gpioNum);

/* ***********************************************************************//**
   *************************************************************************
GPIO - get the state of a GPIO pin conffigured as input.

info:
    have to set first the gpio in mode 7 and set the gpio as input
@param
    gpioNum - number of the GPIO pin
@return  
    value of the pin on 1 bit    

************************************************************************* */
u32 DrvGpioGet(u32 gpioNum);

/* ***********************************************************************//**
   *************************************************************************
GPIO - get the raw state of a GPIO pin configured as input 

info:
    have to set first the gpio in mode 7 and set the gpio as input
	
@param
    gpioNum - number of the GPIO pin
@return      
    value of the pin on 1 bit

************************************************************************* */
u32 DrvGpioGetRaw(u32 gpioNum);


#define DrvGpioPadSet( gpioNo,  val)                          GpioPadSet(gpioNo,  val)
#define DrvGpioPadSetRangeVal( gpioStart, gpioEnd,  val)      GpioPadSetRangeVal( gpioStart, gpioEnd,  val)

/* ***********************************************************************//**
   *************************************************************************
	Set all the gpio's from a table 
   
************************************************************************* */
void GpioPadSetAllTable(u32 table[]);

/* ***********************************************************************//**
   *************************************************************************
	Get all the gpio's to a table 
   
************************************************************************* */
void GpioPadGetAllTable(u32 table[]);

/* ***********************************************************************//**
   *************************************************************************
	Set just one GPIO
   
************************************************************************* */
void GpioPadSet(u32 gpioNo, u32 val);

/* ***********************************************************************//**
   *************************************************************************
	Set just one GPIO, using faster shifts 
   
************************************************************************* */
void GpioPadSetFast(u32 gpioNo, u32 val);

///Set just one GPIO, using faster shifts and masks
/// - clear the bits set in maskClear
/// - set bits set in maskSet
void GpioPadSetFastMask(u32 gpioNo, u32 maskClear,u32 maskSet);

/* ***********************************************************************//**
   *************************************************************************
	Get just one GPIO
   
************************************************************************* */
u32  GpioPadGet(u32 gpioNo);

/* ***********************************************************************//**
   *************************************************************************
	Set an array of GPIOs to a value
   
************************************************************************* */ 
void GpioPadSetArrayVal(u32 gpioArray[],u32 size, u32 val);

/* ***********************************************************************//**
   *************************************************************************
	Set an array of GPIOs to an array of values
   
************************************************************************* */ 
void GpioPadSetArrayArray(u32 gpioArray[],u32 size, u32 valArray[]);

/* ***********************************************************************//**
   *************************************************************************
	Set a range of GPIOs to a value
   
************************************************************************* */ 
void GpioPadSetRangeVal(u32 gpioStart,u32 gpioEnd, u32 val);

/* ***********************************************************************//**
   *************************************************************************
GPIO - set the mode register of a gpio

@param
    gpioNum - number of the gpio
@param
    val - value to be written in the mode register       

************************************************************************* */
void DrvGpioMode(u32 gpioNum, u32 val);

/* ***********************************************************************//**
   *************************************************************************
GPIO - get the mode register of a gpio

@param
    gpioNum - number of the gpio
@return      

************************************************************************* */
u32 DrvGpioGetMode(u32 gpioNum);

/* ***********************************************************************//**
   *************************************************************************
GPIO - set the mode register of some gpios

info:
    - the mode of the gpio must be set first
    - gpioNum2 > gpioNum1
    .
@param
    gpioNum1 - number of the first gpio
@param
    gpioNum2 - number of the last gpio
@param
    val - value to be written in the mode register of each gpio in the interval

************************************************************************* */
void DrvGpioModeRange(u32 gpioNum1, u32 gpioNum2, u32 val);

/* ***********************************************************************//**
   *************************************************************************
GPIO - set the mode register of a list of gpios
info:
    - the mode of the gpio must be set first
    - max size is 141
    .
@param
     pList - pointer to a list of gpios
@param
     size - size of the list of gpios 
@param
     val - value to be written in each gpio mode register

************************************************************************* */
void DrvGpioModeListVal(u32 *pList, u32 size, u32 val);

/* ***********************************************************************//**
   *************************************************************************
GPIO - set the mode register of a list of gpios, with a list of values 
info:
    - max size is 141 
    .
@param
     pList - pointer to a list of gpios
@param
     size - size of the list of gpios 
@param
     *val - pointer to a list of values 

************************************************************************* */
void DrvGpioModeListList(u32 *pList, u32 size, u32 *val);

//!@{
/* ***********************************************************************//**
   *************************************************************************
GPIO - PWM settings
info:
   Enabling of generation is done after this function was called , by calling DrvGpioEnPwm0
@param
    repeat - number of repetitions, 0 means continuous generation
@param    
    leadIn - number of cc before the signal goes HI
@param    
    hiCnt - number of cc during which the signal is high
@param    
    lowCnt - number of cc during which the signal goes low

************************************************************************* */
void DrvGpioSetPwm0(u32 repeat, u32 leadIn, u32 hiCnt, u32 lowCnt);
void DrvGpioSetPwm1(u32 repeat, u32 leadIn, u32 hiCnt, u32 lowCnt);
void DrvGpioSetPwm2(u32 repeat, u32 leadIn, u32 hiCnt, u32 lowCnt);
void DrvGpioSetPwm3(u32 repeat, u32 leadIn, u32 hiCnt, u32 lowCnt);
void DrvGpioSetPwm4(u32 repeat, u32 leadIn, u32 hiCnt, u32 lowCnt);
void DrvGpioSetPwm5(u32 repeat, u32 leadIn, u32 hiCnt, u32 lowCnt);
//!@}

//!@{
/* ***********************************************************************//**
   *************************************************************************
GPIO - PWM enable 
info:
    Enabling must be done after config, since config overrides the enable bit
@param
  en - enable or disable PWM
************************************************************************* */
void DrvGpioEnPwm0(u32 en);

void DrvGpioEnPwm1(u32 en);
void DrvGpioEnPwm2(u32 en);
void DrvGpioEnPwm3(u32 en);
void DrvGpioEnPwm4(u32 en);
void DrvGpioEnPwm5(u32 en);
//!@}
#endif // DRV_GPIO_H  

