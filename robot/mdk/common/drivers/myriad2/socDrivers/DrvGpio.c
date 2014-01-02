///   
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved. 
///            For License Warranty see: common/license.txt   
///
/// @brief     GPIO driver Implementation
/// 
/// Low level driver controlling the functions of the Myriad 2
/// General Purpose IO Block
/// 

// 1: Includes
// ----------------------------------------------------------------------------
#include <mv_types.h>
#include <registersMyriad.h>
#include "DrvGpio.h"
#include "assert.h"

// 2:  Source Specific #defines and types  (typedef,enum,struct)
// ----------------------------------------------------------------------------
#define GPIO_SCAN_SHIFT_COUNT_OFFSET        (8)
#define GPIO_SCAN_BITS_PER_REGISTER         (12)
#define GPIO_SCAN_REG_MASK                  (0xFFF)

#define GPIO_SCAN_TRIGGER_SHIFT             (1 << 0)
#define GPIO_SCAN_XFER_SCANREG_TO_CFGREG    (1 << 1)
#define GPIO_SCAN_XFER_CFGREG_TO_SCANREG    (1 << 2)
#define GPIO_SCAN_IN_PROGRESS               (1 << 3)
#define GPIO_SCAN_SHIFT_ONE_REG             (GPIO_SCAN_BITS_PER_REGISTER << GPIO_SCAN_SHIFT_COUNT_OFFSET)
#define GPIO_SCAN_SHIFT_TWO_REG             ((GPIO_SCAN_BITS_PER_REGISTER*2) << GPIO_SCAN_SHIFT_COUNT_OFFSET)



// 3: Global Data (Only if absolutely necessary)
// ----------------------------------------------------------------------------

// 4: Static Local Data 
// ----------------------------------------------------------------------------

// 5: Static Function Prototypes
// ----------------------------------------------------------------------------
static inline void waitForGpioScanComplete(void);


// 6: Functions Implementation
// ----------------------------------------------------------------------------
static gpioIrqReset = 0;
void DrvGpioIrqResetAll()
{
    // Make sure no GPIO generates GPIO interrupts
    if(gpioIrqReset)
      return;
    
    SET_REG_WORD(GPIO_INT0_CFG_ADR, 0);
    SET_REG_WORD(GPIO_INT1_CFG_ADR, 0);
    SET_REG_WORD(GPIO_INT2_CFG_ADR, 0);
    SET_REG_WORD(GPIO_INT3_CFG_ADR, 0);      
  
    gpioIrqReset++;
}


// use gpio number for a gpio if it is not needed as an interrupt
void DrvGpioIrqConfig( u32 gpio0,  u32 gpio1, u32 gpio2, u32 gpio3,  // the for gpios that can trigger this interrupt 
                       u32 irSrc,                                    // gpio int source
                       u32 irqType,                                  // level 1 or 0 and edge 1 or 0 
                       u32 priority,                                 // int level sent to Leon
                       irq_handler callback)
{
    u32 tmp;

    if (irSrc > 3)
       return ;
    
    if( !gpioIrqReset )// if gpio irq reset wasn't done
        DrvGpioIrqResetAll();//by default all gpios will trigger both irqSrc
  
    tmp = 0 ;
    if (gpio0 < D_GPIO_NUMBER)    
    {
       tmp = D_GPIO_INT_EN_0 | (D_GPIO_INT_GPIO_MASK_0 & (gpio0<<0));
       DrvGpioMode( gpio0, D_GPIO_MODE_7 | D_GPIO_DIR_IN );      
    }
    if (gpio1 < D_GPIO_NUMBER)    
    {
       tmp |= D_GPIO_INT_EN_1 | (D_GPIO_INT_GPIO_MASK_1 & (gpio1<<8));
       DrvGpioMode( gpio1, D_GPIO_MODE_7 | D_GPIO_DIR_IN );               
    }
    if (gpio2 < D_GPIO_NUMBER)    
    {
       tmp |= D_GPIO_INT_EN_2 | (D_GPIO_INT_GPIO_MASK_2 & (gpio2<<16));
       DrvGpioMode( gpio2, D_GPIO_MODE_7 | D_GPIO_DIR_IN );             
    }
    if (gpio3 < D_GPIO_NUMBER)    
    {
       tmp |= D_GPIO_INT_EN_3 | (D_GPIO_INT_GPIO_MASK_3 & (gpio3<<24));
       DrvGpioMode( gpio3, D_GPIO_MODE_7 | D_GPIO_DIR_IN );               
    }
    
    SET_REG_WORD(GPIO_INT0_CFG_ADR + 4*irSrc, tmp);
    DrvIcbSetupIrq( IRQ_GPIO_0 + irSrc, priority, irqType, callback);    
}

void DrvGpioSetPin(int gpioNum,int value)
{
    // This function takes full responsibility for making sure that the GPIO is 
    // configured in Direct Mode as output
    DrvGpioMode(gpioNum, D_GPIO_DIR_OUT | D_GPIO_MODE_7); // Always ensure that GPIO is enabled

    if (value==0)
        DrvGpioSetPinLo(gpioNum);
    else
        DrvGpioSetPinHi(gpioNum);
}

int DrvGpioGetPin(int gpioNum)
{
    // This function takes full responsibility for making sure that the GPIO is 
    // configured in Direct Mode as input
    DrvGpioMode(gpioNum, D_GPIO_DIR_IN | D_GPIO_MODE_7); // Always ensure that GPIO is enabled

    return DrvGpioGet(gpioNum);
}

void DrvGpioToggleState(int gpioNum)
{
    int value;
    int regNo, pinNo;
    regNo = gpioNum / 32;
    pinNo = gpioNum - regNo*32;

    // This function takes full responsibility for making sure that the GPIO is 
    // configured in Direct Mode as output
    DrvGpioMode(gpioNum, D_GPIO_DIR_OUT | D_GPIO_MODE_7); // Always ensure that GPIO is enabled

    // Get the current output state of the pin
    value = GET_REG_WORD_VAL(GPIO_DATA_OUT0_ADR + 4*regNo) & (1 << pinNo);

    // Drive out the opposite state
    if (value==0)
        DrvGpioSetPinHi(gpioNum);
    else
        DrvGpioSetPinLo(gpioNum);
}

// This function is used to initialise the Pad,Pin and level config
// of an entire range of GPIOS. It aims to do this in a memory efficient
// way but not necessarily in a super fast way. 
void DrvGpioInitialiseRange(const drvGpioInitArrayType initArray)
{
    int index=0;
    int i,j;
    int padsNeedUpdate = FALSE;

    // First check if any register needs a pad config update
    do
    {
    assert((initArray[index].action & ~(ACTION_UPDATE_PIN   | 
                                        ACTION_UPDATE_PAD   |
                                        ACTION_UPDATE_LEVEL |
                                        ACTION_TERMINATE_ARRAY)) == 0); // Invalid action probably means array wasn't terminated

    if (initArray[index].action & ACTION_UPDATE_PAD)
        padsNeedUpdate = TRUE;
    index++;
    } while ((initArray[index].action  & ACTION_TERMINATE_ARRAY) == 0);


    if (padsNeedUpdate)
    {
        const int regPerCycle=2;
        const int totalGpios = D_GPIO_NUMBER;
        int numGpioOutA,numGpioOutB;
        int valGpioOutA,valGpioOutB;

        // Goal is to scan out full GPIO Chain, but while scanning it out, scan back in the new values at the same time
        // By default each new value is the same as the original value UNLESS we specifically want to change it.
        // For speed we shift out 2 register at a time (this is the max supported)
        // For this we need to clock a total of 142 + 2 registers 
        // i.e. We shift out GPIO_141;GPIO_140 first, patch if necessary and on the next cycle we shift in GPIO_141;GPIO_140
        // The scan chain looks like this:
        //              ---GPIO_0-->GPIO_1---......->GPIO->140-GPIO->141-----  (142 Scan Registers)
        //              |    |        |                  |         |        |
        // Scan_in----->|   PAD_0    PAD_1  [CFG_REGS] PAD_140   PAD_141    --> Scan_Out  [Space for 2 registers]
        // [Space for 2 reg]
        // 
        // Total loops = (142 + 2) / 2 

        // If we are still shifting, wait until that finishes
        waitForGpioScanComplete();
        // Next copy the current pad config from the CFG_REGs to the scan chain registers
        SET_REG_WORD(GPIO_PAD_CFG_CTRL, GPIO_SCAN_XFER_CFGREG_TO_SCANREG);         

        for (i=0;i< (totalGpios+regPerCycle);i+=2)
        {
            // Shift out the first two gpio_pad configs into the data register
            SET_REG_WORD(GPIO_PAD_CFG_CTRL, GPIO_SCAN_SHIFT_TWO_REG | GPIO_SCAN_TRIGGER_SHIFT); 
            waitForGpioScanComplete();

            // Data gets shifted out in reverse order (gpio_141 first)
            numGpioOutA = totalGpios - i - 1; // minus 1 as gpio_0 is the first Gpio
            numGpioOutB = totalGpios - i - 2; 

            valGpioOutA = (GET_REG_WORD_VAL(GPIO_PAD_CFG_WR) >> 0 ) & GPIO_SCAN_REG_MASK ;
            valGpioOutB = (GET_REG_WORD_VAL(GPIO_PAD_CFG_WR) >> GPIO_SCAN_BITS_PER_REGISTER ) & GPIO_SCAN_REG_MASK ;

            // Optionally patch the values here
            if (numGpioOutB >= 0) // Only if the GPIO number is valid
            {
                index = 0;
                do
                {
                if (initArray[index].action & ACTION_UPDATE_PAD)
                {
                    if ( (numGpioOutA >= initArray[index].startGpio) &&
                         (numGpioOutA <= initArray[index].endGpio) )
                    {
                        valGpioOutA = initArray[index].padConfig;
                    }

                    if ( (numGpioOutB >= initArray[index].startGpio) &&
                         (numGpioOutB <= initArray[index].endGpio) )
                    {
                        valGpioOutB = initArray[index].padConfig;
                    }
                }
                } while ((initArray[index++].action  & ACTION_TERMINATE_ARRAY) == 0);
            }

            // Write back the these register after optionally patching them
             SET_REG_WORD(GPIO_PAD_CFG_WR, (valGpioOutB << GPIO_SCAN_BITS_PER_REGISTER) | valGpioOutA ); 
        }
        // If we are still shifting, wait until that finishes
        waitForGpioScanComplete();
        // Finally write the scan chain back into the CFG registers to make it all active
        SET_REG_WORD(GPIO_PAD_CFG_CTRL, GPIO_SCAN_XFER_SCANREG_TO_CFGREG);         
    }

    index = 0;
    do
    {
    if (initArray[index].action & ACTION_UPDATE_PIN)
        DrvGpioModeRange(initArray[index].startGpio, initArray[index].endGpio,initArray[index].pinConfig);

    if (initArray[index].action & ACTION_UPDATE_LEVEL)
        for (j=initArray[index].startGpio;j<=initArray[index].endGpio;j++)
        {
            if (initArray[index].pinLevel == PIN_LEVEL_LOW)
                DrvGpioSetPinLo(j);
            else
                DrvGpioSetPinHi(j);
        }
    index++;
    } while ((initArray[index].action  & ACTION_TERMINATE_ARRAY) == 0);

}

/// This function is used to workaround a bug in Myriad
/// whereby the SCAN_IN_PROGRESS Bit isn't set instantly.
/// As such it is possible for the application to think the 
/// scan has completed before it even started.
/// @see http://dub30/bugzilla/show_bug.cgi?id=16784
static inline void waitForGpioScanComplete(void)
{
    // Do a dummy read, and ignore it's results.
    // This will insert at least two more clock cycles
    // on the APB bus, and we can be certain that the PAD_CTRL register
    // contains something which is new enough
    GET_REG_WORD_VAL(GPIO_PAD_CFG_CTRL);
    // Then wait for bit to clear
    while (GET_REG_WORD_VAL(GPIO_PAD_CFG_CTRL) & GPIO_SCAN_IN_PROGRESS);
}

/* ***********************************************************************//**
   *************************************************************************
GPIO - use this to set a pin HI, when in mode 7
@{
----------------------------------------------------------------------------
@}
@param
    gpioNum - number of the GPIO pin to set HI
@return      
@{
info:
    have to set first the gpio in mode 7 and set the gpio as output 
@}
************************************************************************* */
void DrvGpioSetPinHi(u32 gpioNum)
{
     int regNo, pinNo;
     regNo = gpioNum / 32;
     pinNo = gpioNum - regNo*32;

     SET_REG_WORD(GPIO_DATA_OUT0_ADR + 4*regNo, GET_REG_WORD_VAL(GPIO_DATA_OUT0_ADR + 4*regNo) | (1 << pinNo));
}

/* ***********************************************************************//**
   *************************************************************************
GPIO - use this to set a pin LO, when in mode 7
@{
----------------------------------------------------------------------------
@}
@param
    gpioNum - number of the GPIO pin to set LO
@return      
@{
info:
    have to set first the gpio in mode 7 and set the gpio as output 
@}
************************************************************************* */
void DrvGpioSetPinLo(u32 gpioNum)
{
     int regNo, pinNo;
     regNo = gpioNum / 32;
     pinNo = gpioNum - regNo*32;

     SET_REG_WORD(GPIO_DATA_OUT0_ADR + 4*regNo, GET_REG_WORD_VAL(GPIO_DATA_OUT0_ADR + 4*regNo) & ~(1 << pinNo));
}

/* ***********************************************************************//**
   *************************************************************************
GPIO - set the mode register of a gpio
@{
----------------------------------------------------------------------------
@}
@param
    gpioNum - number of the gpio
@param
    val - value to be written in the mode register       
@return      
@{
info:
@}
************************************************************************* */
void DrvGpioMode(u32 gpioNum, u32 val)
{
   SET_REG_WORD(GPIO_MODE0_ADR + 4*gpioNum, val);
}


/* ***********************************************************************//**
   *************************************************************************
GPIO - get the mode register of a gpio
@{
----------------------------------------------------------------------------
@}
@param
    gpioNum - number of the gpio
@return      
@{
info:
@}
************************************************************************* */
u32 DrvGpioGetMode(u32 gpioNum)
{
   return GET_REG_WORD_VAL(GPIO_MODE0_ADR + 4*gpioNum);
}

/* ***********************************************************************//**
   *************************************************************************
GPIO - set the mode register of some gpios
@{
----------------------------------------------------------------------------
@}
@param
    gpioNum1 - number of the first gpio
@param
    gpioNum2 - number of the last gpio
@param
    val - value to be written in the mode register of each gpio in the interval
@return      
@{
info:
    - the mode of the gpio must be set first
    - gpioNum2 > gpioNum1
    .
@}
************************************************************************* */
void DrvGpioModeRange(u32 gpioNum1, u32 gpioNum2, u32 val)
{
   u32 i;
   for (i=gpioNum1 ; i<=gpioNum2 ; ++i)
        SET_REG_WORD(GPIO_MODE0_ADR + 4*i, val);
}

/* ***********************************************************************//**
   *************************************************************************
GPIO - set the mode register of a list of gpios
@{
----------------------------------------------------------------------------
@}
@param
     pList - pointer to a list of gpios
@param
     size - size of the list of gpios 
@param
     val - value to be written in each gpio mode register
     
@return      
@{
info:
    - the mode of the gpio must be set first
    - max size is 141
    .
@}
************************************************************************* */
void DrvGpioModeListVal(u32 *pList, u32 size, u32 val)
{
   u32 i;
   for (i=0 ; i<=size ; ++i)
   {
        SET_REG_WORD(GPIO_MODE0_ADR + 4*(*pList), val);
        ++pList;
   }        
}


/* ***********************************************************************//**
   *************************************************************************
GPIO - set the mode register of a list of gpios, with a list of values 
@{
----------------------------------------------------------------------------
@}
@param
     pList - pointer to a list of gpios
@param
     size - size of the list of gpios 
@param
     *val - pointer to a list of values 
     
@return      
@{
info:
    - max size is 141 
    .
@}
************************************************************************* */
void DrvGpioModeListList(u32 *pList, u32 size, u32 *val)
{
   u32 i;
   for (i=0 ; i<=size ; ++i)
   {
        SET_REG_WORD(GPIO_MODE0_ADR + 4*(*pList), *val  & 0x3FFFF);
        ++pList;
        ++val;
   }        
}


/* ***********************************************************************//**
   *************************************************************************
GPIO - PWM settings
@{
----------------------------------------------------------------------------
@}
@param
    repeat - number of repetitions, 0 means continuous generation
@param    
    leadIn - number of cc before the signal goes HI
@param    
    hiCnt - number of cc during which the signal is high
@param    
    lowCnt - number of cc during which the signal goes low
@return      
@{
info:
   Enabling of generation is done after this function was called , by calling DrvGpioEnPwm0
@}
************************************************************************* */
void DrvGpioSetPwm0(u32 repeat, u32 leadIn, u32 hiCnt, u32 lowCnt)
{
   SET_REG_WORD(GPIO_PWM_LEADIN0_ADR, (repeat & 0xFFFF) | ((leadIn << 16) & 0x7FFF));
   SET_REG_WORD(GPIO_PWM_HIGHLOW0_ADR,((hiCnt<<16) & 0xFFFF0000) | (lowCnt & 0xFFFF));
}


/* ***********************************************************************//**
   *************************************************************************
GPIO - PWM settings
@{
----------------------------------------------------------------------------
@}
@param
    repeat - number of repetitions, 0 means continuous generation
@param    
    leadIn - number of cc before the signal goes HI
@param    
    hiCnt - number of cc during which the signal is high
@param    
    lowCnt - number of cc during which the signal goes low
@return      
@{
info:
   Enabling of generation is done after this function was called , by calling DrvGpioEnPwm1
@}
************************************************************************* */
void DrvGpioSetPwm1(u32 repeat, u32 leadIn, u32 hiCnt, u32 lowCnt)
{
   SET_REG_WORD(GPIO_PWM_LEADIN1_ADR, (repeat & 0xFFFF) | ((leadIn << 16) & 0x7FFF));
   SET_REG_WORD(GPIO_PWM_HIGHLOW1_ADR,((hiCnt<<16) & 0xFFFF0000) | (lowCnt & 0xFFFF));
}


/* ***********************************************************************//**
   *************************************************************************
GPIO - PWM settings
@{
----------------------------------------------------------------------------
@}
@param
    repeat - number of repetitions, 0 means continuous generation
@param    
    leadIn - number of cc before the signal goes HI
@param    
    hiCnt - number of cc during which the signal is high
@param    
    lowCnt - number of cc during which the signal goes low
@return      
@{
info:
   Enabling of generation is done after this function was called , by calling DrvGpioEnPwm2
@}
************************************************************************* */
void DrvGpioSetPwm2(u32 repeat, u32 leadIn, u32 hiCnt, u32 lowCnt)
{
   SET_REG_WORD(GPIO_PWM_LEADIN2_ADR, (repeat & 0xFFFF) | ((leadIn << 16) & 0x7FFF));
   SET_REG_WORD(GPIO_PWM_HIGHLOW2_ADR,((hiCnt<<16) & 0xFFFF0000) | (lowCnt & 0xFFFF));
}

/* ***********************************************************************//**
   *************************************************************************
GPIO - PWM settings
@{
----------------------------------------------------------------------------
@}
@param
    repeat - number of repetitions, 0 means continuous generation
@param    
    leadIn - number of cc before the signal goes HI
@param    
    hiCnt - number of cc during which the signal is high
@param    
    lowCnt - number of cc during which the signal goes low
@return      
@{
info:
   Enabling of generation is done after this function was called , by calling DrvGpioEnPwm3
@}
************************************************************************* */
void DrvGpioSetPwm3(u32 repeat, u32 leadIn, u32 hiCnt, u32 lowCnt)
{
   SET_REG_WORD(GPIO_PWM_LEADIN3_ADR, (repeat & 0xFFFF) | ((leadIn << 16) & 0x7FFF));
   SET_REG_WORD(GPIO_PWM_HIGHLOW3_ADR,((hiCnt<<16) & 0xFFFF0000) | (lowCnt & 0xFFFF));
}


/* ***********************************************************************//**
   *************************************************************************
GPIO - PWM settings
@{
----------------------------------------------------------------------------
@}
@param
    repeat - number of repetitions, 0 means continuous generation
@param    
    leadIn - number of cc before the signal goes HI
@param    
    hiCnt - number of cc during which the signal is high
@param    
    lowCnt - number of cc during which the signal goes low
@return      
@{
info:
   Enabling of generation is done after this function was called , by calling DrvGpioEnPwm4
@}
************************************************************************* */

void DrvGpioSetPwm4(u32 repeat, u32 leadIn, u32 hiCnt, u32 lowCnt)
{
   SET_REG_WORD(GPIO_PWM_LEADIN4_ADR, (repeat & 0xFFFF) | ((leadIn << 16) & 0x7FFF));
   SET_REG_WORD(GPIO_PWM_HIGHLOW4_ADR,((hiCnt<<16) & 0xFFFF0000) | (lowCnt & 0xFFFF));
}

/* ***********************************************************************//**
   *************************************************************************
GPIO - PWM settings
@{
----------------------------------------------------------------------------
@}
@param
    repeat - number of repetitions, 0 means continuous generation
@param    
    leadIn - number of cc before the signal goes HI
@param    
    hiCnt - number of cc during which the signal is high
@param    
    lowCnt - number of cc during which the signal goes low 
@return      
@{
info:
   Enabling of generation is done after this function was called , by calling DrvGpioEnPwm5
@}
************************************************************************* */

void DrvGpioSetPwm5(u32 repeat, u32 leadIn, u32 hiCnt, u32 lowCnt)
{
   SET_REG_WORD(GPIO_PWM_LEADIN5_ADR, (repeat & 0xFFFF) | ((leadIn << 16) & 0x7FFF));
   SET_REG_WORD(GPIO_PWM_HIGHLOW5_ADR,((hiCnt<<16) & 0xFFFF0000) | (lowCnt & 0xFFFF));
}

/* ***********************************************************************//**
   *************************************************************************
GPIO - PWM enable 
@{
----------------------------------------------------------------------------
@}
@return      
@{
info:
    Enabling must be done after config, since config overrides the enable bit
@}
************************************************************************* */
void DrvGpioEnPwm0(u32 en)
{
   if (en)
    SET_REG_WORD(GPIO_PWM_LEADIN0_ADR, GET_REG_WORD_VAL(GPIO_PWM_LEADIN0_ADR) | 0x80000000 );
   else
    SET_REG_WORD(GPIO_PWM_LEADIN0_ADR, GET_REG_WORD_VAL(GPIO_PWM_LEADIN0_ADR) & (!0x80000000) );   
}


/* ***********************************************************************//**
   *************************************************************************
GPIO - PWM enable 
@{
----------------------------------------------------------------------------
@}
@param
  en - enable or disable PWM
@return      
@{
info:
    Enabling must be done after config, since config overrides the enable bit
@}
************************************************************************* */
void DrvGpioEnPwm1(u32 en)
{
   if (en)
    SET_REG_WORD(GPIO_PWM_LEADIN1_ADR, GET_REG_WORD_VAL(GPIO_PWM_LEADIN1_ADR) | 0x80000000 );
   else
    SET_REG_WORD(GPIO_PWM_LEADIN1_ADR, GET_REG_WORD_VAL(GPIO_PWM_LEADIN1_ADR) & (!0x80000000) );   
}

/* ***********************************************************************//**
   *************************************************************************
GPIO - PWM enable 
@{
----------------------------------------------------------------------------
@}
@param
  en - enable or disable PWM
@return      
@{
info:
    Enabling must be done after config, since config overrides the enable bit
@}
************************************************************************* */
void DrvGpioEnPwm2(u32 en)
{
   if (en)
    SET_REG_WORD(GPIO_PWM_LEADIN2_ADR, GET_REG_WORD_VAL(GPIO_PWM_LEADIN2_ADR) | 0x80000000 );
   else
    SET_REG_WORD(GPIO_PWM_LEADIN2_ADR, GET_REG_WORD_VAL(GPIO_PWM_LEADIN2_ADR) & (!0x80000000) );   
}

/* ***********************************************************************//**
   *************************************************************************
GPIO - PWM enable 
@{
----------------------------------------------------------------------------
@}
@param
  en - enable or disable PWM
@return      
@{
info:
    Enabling must be done after config, since config overrides the enable bit
@}
************************************************************************* */
void DrvGpioEnPwm3(u32 en)
{
   if (en)
    SET_REG_WORD(GPIO_PWM_LEADIN3_ADR, GET_REG_WORD_VAL(GPIO_PWM_LEADIN3_ADR) | 0x80000000 );
   else
    SET_REG_WORD(GPIO_PWM_LEADIN3_ADR, GET_REG_WORD_VAL(GPIO_PWM_LEADIN3_ADR) & (!0x80000000) );   
}

/* ***********************************************************************//**
   *************************************************************************
GPIO - PWM enable 
@{
----------------------------------------------------------------------------
@}
@param
  en - enable or disable PWM
@return      
@{
info:
    Enabling must be done after config, since config overrides the enable bit
@}
************************************************************************* */
void DrvGpioEnPwm4(u32 en)
{
   if (en)
    SET_REG_WORD(GPIO_PWM_LEADIN4_ADR, GET_REG_WORD_VAL(GPIO_PWM_LEADIN4_ADR) | 0x80000000 );
   else
    SET_REG_WORD(GPIO_PWM_LEADIN4_ADR, GET_REG_WORD_VAL(GPIO_PWM_LEADIN4_ADR) & (!0x80000000) );   
}

/* ***********************************************************************//**
   *************************************************************************
GPIO - PWM enable 
@{
----------------------------------------------------------------------------
@}
@param
  en - enable or disable PWM
@return      
@{
info:
    Enabling must be done after config, since config overrides the enable bit
@}
************************************************************************* */
void DrvGpioEnPwm5(u32 en)
{
   if (en)
    SET_REG_WORD(GPIO_PWM_LEADIN5_ADR, GET_REG_WORD_VAL(GPIO_PWM_LEADIN5_ADR) | 0x80000000 );
   else
    SET_REG_WORD(GPIO_PWM_LEADIN5_ADR, GET_REG_WORD_VAL(GPIO_PWM_LEADIN5_ADR) & (!0x80000000) );   
}



/* ***********************************************************************//**
   *************************************************************************
GPIO - get the state of a GPIO pin conffigured as input 
@{
----------------------------------------------------------------------------
@}
@param
    gpioNum - number of the GPIO pin
@return  
    value of the pin on 1 bit    
@{
info:
    have to set first the gpio in mode 7 and set the gpio as input
@}
************************************************************************* */
u32 DrvGpioGet(u32 gpioNum)
{
     int regNo, pinNo;
     u32 pin;
     regNo = gpioNum / 32;
     pinNo = gpioNum - regNo*32;
     pin = GET_REG_WORD_VAL(GPIO_DATA_IN0_ADR + 4*regNo);
     return (pin >> pinNo) & 1;
}

/* ***********************************************************************//**
   *************************************************************************
GPIO - get the raw state of a GPIO pin configured as input 
@{
----------------------------------------------------------------------------
@}
@param
    gpioNum - number of the GPIO pin
@return      
    value of the pin on 1 bit
@{
info:
    have to set first the gpio in mode 7 and set the gpio as input
@}
************************************************************************* */
u32 DrvGpioGetRaw(u32 gpioNum)
{
     int regNo, pinNo;
     u32 pin;
     regNo = gpioNum / 32;
     pinNo = gpioNum - regNo*32;
     pin = GET_REG_WORD_VAL(GPIO_DATA_IN_RAW0_ADR + 4*regNo);
     return (pin >> pinNo) & 1;
}

///////////////
// bit positions of GPIO_PAD_RD_WR fields
#define PAD_PUPD_SEL 0
#define PAD_DRIVE    2
#define PAD_VOLT_SEL 4
#define PAD_SLEW     5
#define PAD_SCHMITT  6
#define PAD_REN      7
#define OBIAS_SEL    8

#define GPIO_NUM    GPIO_NUM_PINS

#define CFG_SCAN_LEN 2                   /* 24 b = 2*scan reg len (12b) */ 

/*===========================================================
           Set all the gpio's from a table 
===========================================================*/
void GpioPadSetAllTable(u32 table[])
{
    int i;
    for (i=0;i<GPIO_NUM;++i) 
    {
         waitForGpioScanComplete();
         SET_REG_WORD(GPIO_PAD_CFG_WR, table[GPIO_NUM - i - 1] & 0xFFF); // load 
         SET_REG_WORD(GPIO_PAD_CFG_CTRL, GPIO_SCAN_SHIFT_ONE_REG | GPIO_SCAN_TRIGGER_SHIFT); // shift in register
    }
    waitForGpioScanComplete();
    SET_REG_WORD(GPIO_PAD_CFG_CTRL, GPIO_SCAN_XFER_SCANREG_TO_CFGREG);          // latch in
}


/*===========================================================
           Get all the gpio's to a table 
===========================================================*/
void GpioPadGetAllTable(u32 table[])
{
    int i;
    waitForGpioScanComplete();
    SET_REG_WORD(GPIO_PAD_CFG_CTRL, GPIO_SCAN_XFER_CFGREG_TO_SCANREG);          // latch out
    SET_REG_WORD(GPIO_PAD_CFG_CTRL, GPIO_SCAN_SHIFT_ONE_REG | GPIO_SCAN_TRIGGER_SHIFT);  // shift first register
    for (i=0;i<GPIO_NUM;++i) 
    {
        waitForGpioScanComplete();
        // The first register is now in the upper 12 bits of the CFG_RD register, read it out
        table[GPIO_NUM - i - 1] = (GET_REG_WORD_VAL(GPIO_PAD_CFG_WR) >> GPIO_SCAN_BITS_PER_REGISTER) & 0xFFF ; 
        SET_REG_WORD(GPIO_PAD_CFG_CTRL, GPIO_SCAN_SHIFT_ONE_REG | GPIO_SCAN_TRIGGER_SHIFT);              // shift again
    }                                                                                
}


/*===========================================================
           Set just one GPIO
===========================================================*/
void GpioPadSet(u32 gpioNo, u32 val)
{
    u32 i;
    unsigned int oldVal;
    waitForGpioScanComplete();
    SET_REG_WORD(GPIO_PAD_CFG_CTRL, 0x004);                                         // latch out all the values 
    for (i=0 ; i<GPIO_NUM - gpioNo + (CFG_SCAN_LEN-1) ;++i)
    {
        waitForGpioScanComplete();
        SET_REG_WORD(GPIO_PAD_CFG_CTRL, 0xC01);                                     // shift 
    }

    waitForGpioScanComplete();
    oldVal = GET_REG_WORD_VAL(GPIO_PAD_CFG_WR) & 0xfff000;             // avoid destriong the neigbour values (24b now)
    oldVal |= val & 0xfff;
    SET_REG_WORD(GPIO_PAD_CFG_WR, oldVal);                             // load 

    for (i=0 ; i < gpioNo + 1 ;++i)
    {
        waitForGpioScanComplete();
        SET_REG_WORD(GPIO_PAD_CFG_CTRL, 0xC01);                                     // shift back
    }    

    waitForGpioScanComplete();                                                      // wait for the previous shift to complete        
    SET_REG_WORD(GPIO_PAD_CFG_CTRL, 0x002);                                         // latch in all the values 
}


/*===========================================================
          Set just one GPIO, using faster shifts 
===========================================================*/
#pragma weak GpioPadSetFast // this means use ROM Version when available
void GpioPadSetFast(u32 gpioNo, u32 val)
{
    u32 noShft,i,oldVal;
    waitForGpioScanComplete();                                                  // wait for the previous shift to complete
    SET_REG_WORD(GPIO_PAD_CFG_CTRL, 0x004);                                     // latch out all the values 
//--------------------------------------------------------------------------------------------------------------------------------
    noShft = GPIO_NUM - gpioNo + (CFG_SCAN_LEN-1);
    while (noShft >= 20)
    {    
         waitForGpioScanComplete();                                             // wait for the previous shift to complete    
         SET_REG_WORD(GPIO_PAD_CFG_CTRL, 0xF001);                               // shift with 20 registers (20*12=240)
         noShft -= 20;
    }
    while (noShft >= 5)
    {    
         waitForGpioScanComplete();                                             // wait for the previous shift to complete
         SET_REG_WORD(GPIO_PAD_CFG_CTRL, 0x3C01);                               // shift with 5 registers (5*12=60)
         noShft -= 5;
    }
    for (i=0 ; i < noShft ;++i)
    {
         waitForGpioScanComplete();                                              // wait for the previous shift to complete    
         SET_REG_WORD(GPIO_PAD_CFG_CTRL, 0xC01);                                 // shift 
    }
//--------------------------------------------------------------------------------------------------------------------------------    
    waitForGpioScanComplete();                                                   // wait for the previous shift to complete    
    oldVal = GET_REG_WORD_VAL(GPIO_PAD_CFG_WR) & 0xfff000;          // avoid destriong the neigbour values
    oldVal |= val & 0xfff;
    SET_REG_WORD(GPIO_PAD_CFG_WR, oldVal);                          // load 
//--------------------------------------------------------------------------------------------------------------------------------    
    noShft = gpioNo +1;
    while (noShft >= 20)
    {    
         waitForGpioScanComplete();                                             // wait for the previous shift to complete    
         SET_REG_WORD(GPIO_PAD_CFG_CTRL, 0xF001);                               // shift with 20 registers (20*12=240)
         noShft -= 20;
    }
    while (noShft >= 5)
    {    
         waitForGpioScanComplete();                                             // wait for the previous shift to complete    
         SET_REG_WORD(GPIO_PAD_CFG_CTRL, 0x3C01);                               // shift with 5 registers (5*12=60)
         noShft -= 5;
    }
    for (i=0 ; i < noShft ;++i)
    {
        waitForGpioScanComplete();                                              // wait for the previous shift to complete    
        SET_REG_WORD(GPIO_PAD_CFG_CTRL, 0xC01);                                 // shift 
    }
//--------------------------------------------------------------------------------------------------------------------------------        
    waitForGpioScanComplete();                                                  // wait for the previous shift to complete    
    SET_REG_WORD(GPIO_PAD_CFG_CTRL, 0x002);                                     // latch in all the values 
}

/*======================================================================================================================
          Set just one GPIO, using faster shifts and masks
          - clear the bits set in maskClear
          - set bits set in maskSet
======================================================================================================================*/
#pragma weak GpioPadSetFastMask // this means use ROM Version when available
void GpioPadSetFastMask(u32 gpioNo, u32 maskClear,u32 maskSet)
{
    u32 noShft,i;
    waitForGpioScanComplete();                                                  // wait for the previous shift to complete        
    SET_REG_WORD(GPIO_PAD_CFG_CTRL, 0x004);                                     // latch out all the values 
//--------------------------------------------------------------------------------------------------------------------------------    
    noShft = GPIO_NUM - gpioNo + (CFG_SCAN_LEN-1);
    while (noShft >= 20)
    {    
         waitForGpioScanComplete();                                             // wait for the previous shift to complete    
         SET_REG_WORD(GPIO_PAD_CFG_CTRL, 0xF001);                               // shift with 20 registers (20*12=240)
         noShft -= 20;
    }
    while (noShft >= 5)
    {    
         waitForGpioScanComplete();                                             // wait for the previous shift to complete    
         SET_REG_WORD(GPIO_PAD_CFG_CTRL, 0x3C01);                               // shift with 5 registers (5*12=60)
         noShft -= 5;
    }
    for (i=0 ; i < noShft ;++i)
    {
         waitForGpioScanComplete();                                              // wait for the previous shift to complete    
         SET_REG_WORD(GPIO_PAD_CFG_CTRL, 0xC01);                                 // shift 
    }
//--------------------------------------------------------------------------------------------------------------------------------    
    maskClear &= 0xfff;
    maskSet &= 0xfff;    
    waitForGpioScanComplete();                                                   // wait for the previous shift to complete    
    SET_REG_WORD(GPIO_PAD_CFG_WR, (GET_REG_WORD_VAL(GPIO_PAD_CFG_WR) & (~maskClear)) | maskSet );  // load 
//--------------------------------------------------------------------------------------------------------------------------------    
    noShft = gpioNo + 1;
    while (noShft >= 20)
    {    
         waitForGpioScanComplete();                                              // wait for the previous shift to complete    
         SET_REG_WORD(GPIO_PAD_CFG_CTRL, 0xF001);                                // shift with 20 registers
         noShft -= 20;
    }
    while (noShft >= 5)
    {    
         waitForGpioScanComplete();                                              // wait for the previous shift to complete    
         SET_REG_WORD(GPIO_PAD_CFG_CTRL, 0x3C01);                                // shift with 5 registers
         noShft -= 5;
    }
    for (i=0 ; i < noShft ;++i)
    {
        waitForGpioScanComplete();                                               // wait for the previous shift to complete    
        SET_REG_WORD(GPIO_PAD_CFG_CTRL, 0xC01);                                  // shift 
    }
//--------------------------------------------------------------------------------------------------------------------------------
    waitForGpioScanComplete();                                                   // wait for the previous shift to complete    
    SET_REG_WORD(GPIO_PAD_CFG_CTRL, 0x002);                                      // latch in all the values 
}


/*===========================================================
           Get just one GPIO
===========================================================*/
#pragma weak GpioPadGet // this means use ROM Version when available
u32 GpioPadGet(u32 gpioNo)
{  
   u32 PAD;
   u32 i;
   waitForGpioScanComplete();                                                    // wait for the previous shift to complete
   SET_REG_WORD(GPIO_PAD_CFG_CTRL, 0x004);                                       // latch out all the values 
   for (i=0 ; i<GPIO_NUM - gpioNo + (CFG_SCAN_LEN-1) ;++i)
   {
       waitForGpioScanComplete();                                                // wait for the previous shift to complete    
       SET_REG_WORD(GPIO_PAD_CFG_CTRL, 0xC01);                                   // shift to the right register 
   }
   waitForGpioScanComplete();                                                    // wait for the previous shift to complete       
   PAD = GET_REG_WORD_VAL(GPIO_PAD_CFG_WR) & 0xfff;                  // get the register
   return PAD;
}

/*===========================================================
          Set an array of GPIOs to a value 
===========================================================*/
#pragma weak GpioPadSetArrayVal // this means use ROM Version when available
void GpioPadSetArrayVal(u32 gpioArray[],u32 size, u32 val)
{
    u32 i;
    for (i=0 ; i<size ; ++i)
       GpioPadSetFast(gpioArray[i],val);
}

/*===========================================================
          Set an array of GPIOs to an array of values
===========================================================*/
#pragma weak GpioPadSetArrayArray // this means use ROM Version when available
void GpioPadSetArrayArray(u32 gpioArray[],u32 size, u32 valArray[])
{
    u32 i;
    for (i=0 ; i<size ; ++i)
       GpioPadSetFast(gpioArray[i],valArray[i]);
}


/*===========================================================
          Set a range of GPIOs to a value
===========================================================*/
#pragma weak GpioPadSetRangeVal // this means use ROM Version when available
void GpioPadSetRangeVal(u32 gpioStart,u32 gpioEnd, u32 val)
{
    u32 i;
    u32 noShft;
    unsigned int oldVal;
    if (gpioStart>gpioEnd)
    return ; // exit if the range is invalid 
    
    waitForGpioScanComplete();                                                  // wait for the previous shift to complete    
    SET_REG_WORD(GPIO_PAD_CFG_CTRL, 0x004);                                     // latch out all the values     
     
//--------------------------------------------------------------------------------------------------------------------------------    
    noShft = GPIO_NUM - gpioEnd + (CFG_SCAN_LEN-1);                           // shift to the last register 
    
    while (noShft >= 20)
    {    
         waitForGpioScanComplete();                                             // wait for the previous shift to complete    
         SET_REG_WORD(GPIO_PAD_CFG_CTRL, 0xF001);                               // shift with 20 registers (20*12=240)
         noShft -= 20;
    }
    while (noShft >= 5)
    {    
         waitForGpioScanComplete();                                             // wait for the previous shift to complete    
         SET_REG_WORD(GPIO_PAD_CFG_CTRL, 0x3C01);                               // shift with 5 registers (5*12=60)
         noShft -= 5;
    }
    for (i=0 ; i < noShft ;++i)
    {
         waitForGpioScanComplete();                                              // wait for the previous shift to complete    
         SET_REG_WORD(GPIO_PAD_CFG_CTRL, 0xC01);                                 // shift 
    }
//--------------------------------------------------------------------------------------------------------------------------------    
    for (i=0 ; i<(gpioEnd-gpioStart+1) ; ++i)
    {
         waitForGpioScanComplete();                                              // wait for the previous shift to complete    
         oldVal = GET_REG_WORD_VAL(GPIO_PAD_CFG_WR) & 0xfff000;     // avoid destriong the neigbour values
         oldVal |= val & 0xfff;
         SET_REG_WORD(GPIO_PAD_CFG_WR, oldVal);                     // load    
         SET_REG_WORD(GPIO_PAD_CFG_CTRL, 0x0C01);                                // shift 
    }
//--------------------------------------------------------------------------------------------------------------------------------    
    noShft = gpioStart;
    
    while (noShft >= 20)
    {    
         waitForGpioScanComplete();                                             // wait for the previous shift to complete    
         SET_REG_WORD(GPIO_PAD_CFG_CTRL, 0xF001);                               // shift with 20 registers (20*12=240)
         noShft -= 20;
    }
    while (noShft >= 5)
    {    
         waitForGpioScanComplete();                                             // wait for the previous shift to complete    
         SET_REG_WORD(GPIO_PAD_CFG_CTRL, 0x3C01);                               // shift with 5 registers (5*12=60)
         noShft -= 5;
    }
    for (i=0 ; i < noShft ;++i)
    {
        waitForGpioScanComplete();                                              // wait for the previous shift to complete    
        SET_REG_WORD(GPIO_PAD_CFG_CTRL, 0xC01);                                 // shift 
    }
//--------------------------------------------------------------------------------------------------------------------------------
    waitForGpioScanComplete();                                                  // wait for the previous shift to complete    
    SET_REG_WORD(GPIO_PAD_CFG_CTRL, 0x002);                                     // latch in all the values 
}
