///   
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved. 
///            For License Warranty see: common/license.txt   
///
/// @brief     Primary Board driver for the MV0153 PCB
/// 
/// This driver contains the primary configuration settings for the MV0153 
/// PCB
/// 

// 1: Includes
// ----------------------------------------------------------------------------
#include "mv_types.h"
#include "assert.h"
#include <isaac_registers.h>
#include <DrvGpio.h>
#include "DrvPwm.h"
#include "DrvI2cMasterDefines.h"
#include "DrvLcdDefines.h"
#include "swcLedControl.h"
#include <brdMv0153.h>
#include <DrvTimer.h>
#include <assert.h>

// 2:  Source Specific #defines and types  (typedef,enum,struct)
// ----------------------------------------------------------------------------
#ifndef MIN
#   define MIN(a,b) (a>b?b:a)
#endif

#ifndef MAX
#   define MAX(a,b) (a>b?a:b)
#endif

#define NUM_I2C_DEVICES                 (2)

#define SAFE_I2C_SPEED_EXT_PLL_KHZ      (20) // at 20Khz we don't get verification errors

// 3: Global Data (Only if absolutely necessary)
// ----------------------------------------------------------------------------

// 4: Static Local Data 
// ----------------------------------------------------------------------------
// This function declaration is needed by the data initialised below
static u32 commonI2CErrorHandler(I2CM_StatusType error, u32 param1, u32 param2);

static I2CM_Device i2cDevHandle[NUM_I2C_DEVICES];    

static tyI2cConfig i2c1DefaultConfiguration = 
    {
     .device                = GPIO_BITBASH,
     .sclPin                = MV0153_I2C1_SCL_PIN,
     .sdaPin                = MV0153_I2C1_SDA_PIN,
     .speedKhz              = MV0153_I2C1_SPEED_KHZ_DEFAULT,
     .addressSize           = MV0153_I2C1_ADDR_SIZE_DEFAULT,
     .errorHandler          = &commonI2CErrorHandler,
    };

static tyI2cConfig i2c2DefaultConfiguration = 
    {
     .device                = GPIO_BITBASH,
     .sclPin                = MV0153_I2C2_SCL_PIN,
     .sdaPin                = MV0153_I2C2_SDA_PIN,
     .speedKhz              = MV0153_I2C2_SPEED_KHZ_DEFAULT,
     .addressSize           = MV0153_I2C2_ADDR_SIZE_DEFAULT,
     .errorHandler          = &commonI2CErrorHandler, 
    };

static u8 mv0153R0R1LedTable[] =
{
    200,                // Power LED not controllable, use invalid gpio number
    MV0153_R0R1_LED1_GPIO, 
    MV0153_R0R1_LED2_GPIO, 
    MV0153_R0R1_LED3_GPIO, 
    MV0153_R0R1_LED4_GPIO, 
    MV0153_R0R1_LED5_GPIO, 
    MV0153_R0R1_LED6_GPIO, 
    MV0153_R0R1_LED7_GPIO, 
    MV0153_R0R1_LED8_GPIO, 
};

static u8 mv0153R2LedTable[] =
{
    200,                // Power LED not controllable, use invalid gpio number
    MV0153_R2_LED1_GPIO, 
    MV0153_R2_LED2_GPIO, 
    MV0153_R2_LED3_GPIO, 
    MV0153_R2_LED4_GPIO, 
};

static tyLedSystemConfig mv0153R0R1LedConfig =
{
    .totalLeds      = MV0153_R0R1_NUM_LEDS,
    .pwrLedPresent  = 0,
    .ledArray       = mv0153R0R1LedTable
};


static tyLedSystemConfig mv0153R2LedConfig =
{
    .totalLeds      = MV0153_R2_NUM_LEDS,
    .pwrLedPresent  = 0,
    .ledArray       = mv0153R2LedTable
};

static tyPwmVoltageConfig mv153CoreVoltConfig =
{
    .gpioNum                    = MV0153_CORE_VOLTAGE_PWM_PIN,
    .gpioMode                   = D_GPIO_MODE_1,    // Mode 1 on this pin is PWM3
    .pwmDevice                  = 3, 
    .vminHardwareMv             = 1023,    // Hardware supports the range 1.023V => 1.26V
    .vmaxHardwareMv             = 1260, 
    .vminSoftwareClampMv        = 1023,    // Software does not limit this range any further 
    .vmaxSoftwareClampMv        = 1260, 
    .sysFreqHz                  = 180000000, 
    .pwmFreqHz                  = 4500000, 
    .postCfgDelayMs             = 100,     // 100 ms Delay for voltage to stabalise
};


static tyResetPinCfg mv0153MbResetTable[] =
{
    {
        .deviceFlag     = MV0153_HDMI_RESET_FLAG,
        .gpioPin        = MV0153_HDMI_RESET_PIN,
        .activeLevel    = ACTIVE_LOW,
        .activeMs       = 50,
        .holdMs         = 50
    },
    {0,0,0,0,0} // Null Termination
};


static tyBoardResetConfiguration mv0153BoardResetCfg =
{
    mv0153MbResetTable,
    NULL,
    NULL,
    NULL,
    NULL
};

static const drvGpioInitArrayType brdMv0153RevDetectConfig =
{
    // -----------------------------------------------------------------------
    // PCB Revision detect, set weak pullups on the necessary pins
    // -----------------------------------------------------------------------
    {52, 52 , ACTION_UPDATE_PAD          // Only updating the PAD configuration
            ,
              PIN_LEVEL_LOW              // Don't Care, not updated
            ,
              D_GPIO_MODE_0              // Don't Care, not updated  
            , 
              D_GPIO_PAD_PULL_UP       | // Enable weak pullups so that we can detect revision
              D_GPIO_PAD_DRIVE_2mA     |  
              D_GPIO_PAD_VOLT_2V5      |
              D_GPIO_PAD_SLEW_SLOW     |
              D_GPIO_PAD_SCHMITT_OFF   |
              D_GPIO_PAD_RECEIVER_ON   |
              D_GPIO_PAD_BIAS_2V5      |
              D_GPIO_PAD_LOCALCTRL_OFF |
              D_GPIO_PAD_LOCALDATA_LO  |
              D_GPIO_PAD_LOCAL_PIN_OUT   
    },      
    {56, 57 , ACTION_UPDATE_PAD          // Only updating the PAD configuration
            ,
              PIN_LEVEL_LOW              // Don't Care, not updated
            ,
              D_GPIO_MODE_0              // Don't Care, not updated  
            , 
              D_GPIO_PAD_PULL_UP       | // Enable weak pullups so that we can detect revision 
              D_GPIO_PAD_DRIVE_2mA     |  
              D_GPIO_PAD_VOLT_2V5      |
              D_GPIO_PAD_SLEW_SLOW     |
              D_GPIO_PAD_SCHMITT_OFF   |
              D_GPIO_PAD_RECEIVER_ON   |
              D_GPIO_PAD_BIAS_2V5      |
              D_GPIO_PAD_LOCALCTRL_OFF |
              D_GPIO_PAD_LOCALDATA_LO  |
              D_GPIO_PAD_LOCAL_PIN_OUT   
    },  
    // -----------------------------------------------------------------------
    // Finally we terminate the Array
    // -----------------------------------------------------------------------
    {0,0    , ACTION_TERMINATE_ARRAY      // Do nothing but simply termintate the Array
            ,
              PIN_LEVEL_LOW               // Won't actually be updated
            ,
              D_GPIO_MODE_0               // Won't actually be updated 
            ,
              D_GPIO_PAD_DEFAULTS         // Won't actually be updated 
    },
};


// 5: Static Function Prototypes
// ----------------------------------------------------------------------------
static u32 commonI2CErrorHandler(I2CM_StatusType i2cCommsError, u32 slaveAddr, u32 regAddr);
static int processResetTable(tyResetPinCfg * rstTable, u32 devMask,tyResetAction action);

// 6: Functions Implementation
// ----------------------------------------------------------------------------



/******************************************************************************
 4: Static Function Prototypes
******************************************************************************/

/******************************************************************************
 5: Functions Implementation
******************************************************************************/

int brd153InitialiseLeds(void)
{
    tyMv0153PcbRevision brdRev;
    brdRev = brd153GetPcbRevison();
    if (brdRev == MV0153_R0R1)
        swcLedInitialise(&mv0153R0R1LedConfig);
    else if (brdRev == MV0153_R2)
        swcLedInitialise(&mv0153R2LedConfig);
    else
    {
        assert(FALSE);
        return -1;
    }
    return 0;
}

int brd153GetHdmiCecGpioNum(void)
{
    tyMv0153PcbRevision brdRev;
    brdRev = brd153GetPcbRevison();
    if (brdRev == MV0153_R0R1)
        return MV0153_R0R1_HDMI_CEC_PIN; 
    else if (brdRev == MV0153_R2)
        return MV0153_R2_HDMI_CEC_PIN;
    else
    {
        assert(FALSE);
        return -1;
    }
}

int brd153GetInfraRedSensorGpioNum(void)
{
    return MV0153_INFRA_RED_PIN;
}

int brd153Reset(u32 devMask,tyResetAction action)
{

    if (mv0153BoardResetCfg.mbRstTable)
          processResetTable(mv0153BoardResetCfg.mbRstTable,devMask,action);
    if (mv0153BoardResetCfg.daughterCard1RstTable)
          processResetTable(mv0153BoardResetCfg.daughterCard1RstTable,devMask,action);
    if (mv0153BoardResetCfg.daughterCard2RstTable)
          processResetTable(mv0153BoardResetCfg.daughterCard2RstTable,devMask,action);
    if (mv0153BoardResetCfg.daughterCard3RstTable)
          processResetTable(mv0153BoardResetCfg.daughterCard3RstTable,devMask,action);
    if (mv0153BoardResetCfg.daughterCard4RstTable)
          processResetTable(mv0153BoardResetCfg.daughterCard4RstTable,devMask,action);

    return 0;
}

int brd153InitialiseI2C(tyI2cConfig * i2cCfg1, tyI2cConfig * i2cCfg2,I2CM_Device ** i2c1Dev,I2CM_Device ** i2c2Dev)
{
    int ret;

    // Unless the user specifies otherwise we use the default configuration
    if (i2cCfg1 == NULL)
         i2cCfg1 = &i2c1DefaultConfiguration;

    if (i2cCfg2 == NULL)
        i2cCfg2 = &i2c2DefaultConfiguration;


    // Initialise the I2C device to use the I2C0 Hardware block
    ret = DrvI2cMInitFromConfig(&i2cDevHandle[0], i2cCfg1);
    if (ret != I2CM_STAT_OK)
    {
        return -1;
    }
    *i2c1Dev = &i2cDevHandle[0]; // Return the handle

    // Initialise the I2C device to use the I2C1 Hardware block
    ret = DrvI2cMInitFromConfig(&i2cDevHandle[1], i2cCfg2);
    if (ret != I2CM_STAT_OK)
    {
        return -2;
    }
    *i2c2Dev = &i2cDevHandle[1]; // Return the handle

    DrvI2cMSetErrorHandler(&i2cDevHandle[0], NULL); // disable the error handler
    DrvI2cMSetErrorHandler(&i2cDevHandle[1], NULL);
    // In order to dust off the cobwebs (e.g. SDA stuck low due to TSC Chip)
    // We perform a read from each I2C bus  (to the regulator) 
    // Note: One or both of these could fail due to SDA stuck low, but we don't care, this such un-stuck it!
    (void)DrvI2cMReadByte(&i2cDevHandle[0],MV0153_I2C1_REG_U15_7B,MV0153_REG_BUCK_LDO_ENABLE);
    (void)DrvI2cMReadByte(&i2cDevHandle[1],MV0153_I2C2_REG_U17_7B,MV0153_REG_BUCK_LDO_STATUS);

    // Also setup a common error handler for I2C
    if (i2cCfg1->errorHandler)
        DrvI2cMSetErrorHandler(&i2cDevHandle[0],i2cCfg1->errorHandler);
    if (i2cCfg2->errorHandler)
        DrvI2cMSetErrorHandler(&i2cDevHandle[1],i2cCfg2->errorHandler);

    // Sanity check that both I2C Busses are woking.
    // If either read fails the assert in the common error handler should trigger as the I2C bus is non-functioning
    (void)DrvI2cMReadByte(&i2cDevHandle[0],MV0153_I2C1_REG_U15_7B,MV0153_REG_BUCK_LDO_ENABLE);
    (void)DrvI2cMReadByte(&i2cDevHandle[1],MV0153_I2C2_REG_U17_7B,MV0153_REG_BUCK_LDO_STATUS);
                   
    return 0;
}

int brd153SetCoreVoltage(int coreVoltMv) 
{
    // TODO: Optionally update the system frequency here
    return DrvPwmSetupVoltageControl(&mv153CoreVoltConfig,coreVoltMv);
}

// ---------------------------------------------------------------------------------
// External PLL
// ---------------------------------------------------------------------------------
int brd153ExternalPllConfigure(u32 configIndex)
{
    I2CM_Device *   pllI2cHandle;
    tyI2cConfig     orgI2cConfig;
    u32             lcdFormatCfg;
    int             retVal; 

    // Get handle for the I2C 
    pllI2cHandle = &i2cDevHandle[1];

    // Backup I2C Configuration
    DrvI2cMGetCurrentConfig(pllI2cHandle,&orgI2cConfig);

    // Re-Configure for Bit-bash as for nasty hardware reasons this module only works with Bit-Bash I2C
    // Note: We re-use the original I2C handle
    (void)DrvI2cMInit(pllI2cHandle,
                        GPIO_BITBASH,           // This switches the block to bit-bash mode
                        orgI2cConfig.sclPin,
                        orgI2cConfig.sdaPin,
                        SAFE_I2C_SPEED_EXT_PLL_KHZ, // Determined by empirical testing
                        orgI2cConfig.addressSize);

    // Disable the PLL output
    DrvGpioSetPinLo(MV0153_CLK_SEL_PIN);

    // Configure the PLL
    if ((retVal=icPllCDCE913Configure(pllI2cHandle,configIndex)))
        return retVal;


    // Restore the I2C Configuration by re-init the block in with its original configuration
    (void)DrvI2cMInitFromConfig(pllI2cHandle, &orgI2cConfig);

    // Make sure that the LCD PCLK pin as an input
    DrvGpioMode(MV0153_LCD1_PCLK_PIN, DrvGpioGetMode(MV0153_LCD1_PCLK_PIN) | D_GPIO_DIR_IN | D_GPIO_MODE_7 );  // PCLK = GPIO input for now

    DrvCprSysDeviceAction(ENABLE_CLKS,DEV_MXI_AXI |  
                                      DEV_SXI_AXI |  
                                      DEV_AXI2AXI |  
                                      DEV_AXI2AHB |  
                                      DEV_AXI_MON |  
                                      DEV_LCD1       
                                      ); // Clock LCD          

	lcdFormatCfg = GET_REG_WORD_VAL(MXI_DISP1_BASE_ADR + LCD_OUT_FORMAT_CFG_OFFSET);
	SET_REG_WORD(MXI_DISP1_BASE_ADR + LCD_OUT_FORMAT_CFG_OFFSET, lcdFormatCfg | D_LCD_OUTF_PCLK_EXT);

	// Finally configure pin for LCD mode 
    // Note: It is crucial that the pin is configured as an input as the LCD controller doesn't know this by default
    // we must tell it through the pin configuration
	DrvGpioMode(MV0153_LCD1_PCLK_PIN,  (DrvGpioGetMode(MV0153_LCD1_PCLK_PIN) & (~D_GPIO_MODE_7) )| D_GPIO_DIR_IN  | MV0153_LCD1_PCLK_MODE_LCD);

    // Finally enable the PLL output
    DrvGpioMode(MV0153_CLK_SEL_PIN, D_GPIO_DIR_OUT | D_GPIO_MODE_7 );  // configured as output in GPIO mode
    DrvGpioSetPinHi(MV0153_CLK_SEL_PIN);

	return 0; // Success
}

// External Voltage Regulator
int brd153CfgDaughterCardVoltages(tyDcVoltageCfg * voltageConfig)
{
    int retVal;

    // First Ensure all voltages are disabled
    // This is important as the regulators have a default voltage of:
    // 1.2 ; 3.3 ; 3.3 ; 1.8 and until they get reprogrammed setting the power
    // pin high will drive these volages up the the daughtercard from both regulators.
    DrvGpioMode(MV0153_CAMERA_POWER_EN_PIN, D_GPIO_DIR_OUT | D_GPIO_MODE_7 );
    DrvGpioSetPinHi(MV0153_CAMERA_POWER_EN_PIN);

    if ((retVal = icLT3906Configure(&i2cDevHandle[0],MV0153_I2C1_REG_U15_7B, &voltageConfig->voltCfgU15)))
        return retVal;

    if ((retVal = icLT3906Configure(&i2cDevHandle[1],MV0153_I2C1_REG_U15_7B, &voltageConfig->voltCfgU17)))
        return retVal;

    // Now enable the voltages
    DrvGpioSetPinLo(MV0153_CAMERA_POWER_EN_PIN);

    // Finally check that the voltage status is OK
    if ((retVal = icLT3906CheckVoltageOK(&i2cDevHandle[0],MV0153_I2C1_REG_U15_7B, &voltageConfig->voltCfgU15)))
        return retVal;

    if ((retVal = icLT3906CheckVoltageOK(&i2cDevHandle[1],MV0153_I2C1_REG_U15_7B, &voltageConfig->voltCfgU17)))
        return retVal;

    return 0; // Assume all well if we manage to get to here
}

tyMv0153PcbRevision brd153GetPcbRevison(void)
{
    static tyMv0153PcbRevision detectedPcbRevision = MV0153_REV_NOT_INIT; // Default to unknown

    u32 revisionCode=0;
    // Only read the revision the first time this function is called. Afterwards we just return the value
    if (detectedPcbRevision == MV0153_REV_NOT_INIT)
    {
        // Setup the necessary GPIOS to enable weak pullups.
        // we do this so that this function can be called before performing the primary GPIO config
        DrvGpioInitialiseRange(brdMv0153RevDetectConfig);

        revisionCode  = (DrvGpioGetPin(MV0153_REV_DETECT_BIT0) & 0x1) << 0;
        revisionCode |= (DrvGpioGetPin(MV0153_REV_DETECT_BIT1) & 0x1) << 1;
        revisionCode |= (DrvGpioGetPin(MV0153_REV_DETECT_BIT2) & 0x1) << 2;

        if (revisionCode == MV0153_REV_CODE_R0R1)
            return MV0153_R0R1;

        if (revisionCode == MV0153_REV_CODE_R2)
            return MV0153_R2;

        if (revisionCode == MV0153_REV_CODE_R0R1)
            detectedPcbRevision = MV0153_R0R1;
        if (revisionCode == MV0153_REV_CODE_R2)
            detectedPcbRevision = MV0153_R2;
        // Otherwise we have a faulty PCB
        detectedPcbRevision = MV0153_REV_NOT_INIT;
        assert(FALSE);
    }

    return detectedPcbRevision;
}
// -----------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------
// Static Function Implementations
// -----------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------

static u32 commonI2CErrorHandler(I2CM_StatusType i2cCommsError, u32 slaveAddr, u32 regAddr)
{
    slaveAddr=slaveAddr;
    regAddr=regAddr;

    if(i2cCommsError != I2CM_STAT_OK)
    {
    	printf("\nI2C Error (%d) Slave (%02X) Reg (%02X)",i2cCommsError,slaveAddr,regAddr);
    	assert(i2cCommsError == 0);
    }
    return i2cCommsError; // Because we haven't really handled the error, pass it back to the caller
}


static int processResetTable(tyResetPinCfg * rstTable, u32 devMask,tyResetAction action)
{
    int index=0;
    u32 activeMs = 0;
    int holdMs   = 0;

    while (rstTable[index].deviceFlag)
    {
        if (rstTable[index].deviceFlag & devMask) // If this device needs resetting
        {
            DrvGpioSetPin(rstTable[index].gpioPin,rstTable[index].activeLevel);
            activeMs = MAX(activeMs,rstTable[index].activeMs); // When doing more than one reset we use the worst case delays
            holdMs   = MAX(activeMs,rstTable[index].holdMs);
        }
        index++;
    }

    if (action != RST_AND_HOLD)
    {
        SleepMs(activeMs);
        index = 0; // Run through once more and remove the resets
        while (rstTable[index].deviceFlag)
        {
            if (rstTable[index].deviceFlag & devMask) // If this device is targetted remove its reset
            {
                DrvGpioSetPin(rstTable[index].gpioPin,! (rstTable[index].activeLevel));
            }
            index++;
        }
        SleepMs(holdMs);
    }

    return 0;
}


