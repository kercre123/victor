///   
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved. 
///            For License Warranty see: common/license.txt   
///
/// @brief     Driver to control Clocks, Power and Reset
/// 
/// 
/// 

// 1: Includes
// ----------------------------------------------------------------------------
#include "DrvCpr.h"
#include "isaac_registers.h"
#include "stdio.h"
#include "assert.h"
#include "DrvTimer.h"
#include "DrvRegUtils.h"
#include "DrvDdr.h"

// 2:  Source Specific #defines and types  (typedef,enum,struct)
// ----------------------------------------------------------------------------

//#define DRV_CPR_DEBUG

#ifdef DRV_CPR_DEBUG
#define DPRINTF1(...) printf(__VA_ARGS__)
#else
#define DPRINTF1(...)
#endif

#define DEFAULT_OSC_INPUT_CLK_KHZ           (12000)

#define PLL_BYPASS_BIT                       (1 << 0)

#define PLL_SW_TIMEOUT_US                   (5000)   // Wait 5ms for PLL to lock

#define PLL_LOCKED_MASK                     (1 << 0)
#define PLL_UPDATE_COMPLETE_MASK            (1 << 1)
#define PLL_LOCK_TIMEOUT                    (1 << 2)
#define PLL_LOCKED_OR_TIMEOUT_MASK          (PLL_LOCKED_MASK | PLL_LOCK_TIMEOUT)

#define MAX_ALLOWED_AUX_CONFIGS             (32)

typedef struct 
{
    u32                     initialised;               // Set to True once DrvCprInit called
    u32                     currentOscInClkKhz;        // Only updated on calls to DrvCprSetupClocks
    u32                     currentPllClockKhz;        // Current system clock in Khz
    u32                     currentSysClockKhz;        // Current system clock in Khz
    u32                     currentTicksPerMicoSecond; // TODO: Perhaps this should be in the timer and we notify timer?
    tyGetSetOscFreqKhzCb    pfGetSetOscFreqKhzCb;      // Call back optionally used to request a change of input Osc frequency
    tySysClkFreqChangeCb    pfSysClockFreqChange;      // Callback used to notify application of new sysClock frequency
} tyCprState;


// 3: Global Data (Only if absolutely necessary)
// ----------------------------------------------------------------------------
u32 * pGlobalSysclockKhz; // Setup to point to cprState currentSysClockKhz

// 4: Static Local Data 
// ----------------------------------------------------------------------------

static tyCprState cprState;               

/// This table is used to lookup the address of the clock divider associated with the specific enable
/// Note: Not some dividers share an enable
/// The reason for the rightshift of two is I rely on the fact that the addresses must be word aligned so
/// that I can fit the array in an char value to save some memory on the lookup table
static const u8 auxClkDividerLookupWOffset[]=
{
    (0x70   >> 2) ,           // I2S0        
    (0x74   >> 2) ,           // I2S1        
    (0x78   >> 2) ,           // I2S2        
    (0x7C   >> 2) ,           // USB         
    (0x80   >> 2) ,           // DDR         
    (0x84   >> 2) ,           // SDHOST_1    
    (0x88   >> 2) ,           // SDHOST_2    
    (0x8C   >> 2) ,           // CIF0_MCLK   
    (0x90   >> 2) ,           // CIF1_MCLK   
    (0x94   >> 2) ,           // LCD0_PCLK   
    (0x00   >> 2) ,           // LCD0_PCLK_X2     -- Note: Uses same divider as LCD0_PCLK, hence the 0x0
    (0x98   >> 2) ,           // LCD1_PCLK   
    (0x00   >> 2) ,           // LCD1_PCLK_X2     -- Note: Uses same divider as LCD1_PCLK, hence the 0x0 
    (0xA4   >> 2) ,           // MEBI_CLK_POS
    (0x00   >> 2) ,           // MEBI_CLK_NEG     -- Note: Uses same divider as MEBI_CLK_POS, hence the 0x0 
    (0xAC   >> 2) ,           // CPR_IO_CLK  
    (0xB0   >> 2) ,           // 32KHZ_CLK   
    (0xB4   >> 2) ,           // TSI_CLK     
    (0xB8   >> 2) ,           // CABAC_CLK   
    (0x124  >> 2) ,           // TVENC_CLK   
    (0x128  >> 2)             // SAHB_CLK    
};

// 5: Static Function Prototypes
// ----------------------------------------------------------------------------
static int setupSystemClk(int pllTargetFreqKhz, int clockDivNumerator, int clockDivDenominator);
static int notifySystemClockChange(void);
static u32 calculateSystemClockKhz(u32 pllClockKhz);
static u32 calculatePllConfigValue(u32 oscIn, u32 sysClkDesired,u32 * pAchievedFreqKhz);

// 6: Functions Implementation
// ----------------------------------------------------------------------------

int DrvCprInit(tyGetSetOscFreqKhzCb pfGetSetOscFreqKhzCb,tySysClkFreqChangeCb pfSysClockFreqChange)
{
    // Setup Callbacks
    cprState.pfGetSetOscFreqKhzCb = pfGetSetOscFreqKhzCb;
    cprState.pfSysClockFreqChange = pfSysClockFreqChange;

    // Initialise some of the static data
    cprState.currentOscInClkKhz         = 0;
    cprState.currentPllClockKhz         = 0;
    cprState.currentSysClockKhz         = 12000; // Assume 12Mhz By default until told otherwise
    cprState.currentTicksPerMicoSecond  = 0;

    // Setup global pointer to our system clock
    pGlobalSysclockKhz = &cprState.currentSysClockKhz;

    // Setup Global Tick Counter using Timer Driver
    DrvTimerInit();

    SET_REG_WORD(CPR_GEN_CTRL_ADR, GET_REG_WORD_VAL(CPR_GEN_CTRL_ADR) | CPR_GEN_CTRL_CMX_AHB_CACHE_ENABLE);

    // Disable all system wakeup sources by default
    SET_REG_WORD(CPR_WAKEUP_MASK_ADR, CPR_GPIO_WAKEUP_DISABLE |
                                      CPR_RTC_WAKEUP_DISABLE  | 
                                      CPR_SDH0_WAKEUP_DISABLE   );
    
    // Set initialised flag to true
    cprState.initialised = TRUE;

    // If necessary.. Do some low power config in here. E.g. USB updates
    return ERR_CPR_OK;
}


// Essentially the replacement for drv Sysconfig
int DrvCprSetupClocks(const tySocClockConfig * pClkCfg)
{
    int retVal;

    if ( ! cprState.initialised)
    {
        retVal = ERR_CPR_NOT_INITIALISED;
        goto CPR_SETUP_ERR;
    }

    // Set the Osc in to the desired value
    // If the user specifies a callback to allow changing input osc frequency, we use it
    if (cprState.pfGetSetOscFreqKhzCb == NULL)
    {
        // User hasn't specifed a means to change the Osc clock
        // As such we assume he is on a platform with only 12Mhz Clock
        if (pClkCfg->targetOscInputClock != DEFAULT_OSC_INPUT_CLK_KHZ)
        {
            retVal = ERR_UNSUPPORTED_OSC_CLK;
            goto CPR_SETUP_ERR;
        }
        cprState.currentOscInClkKhz = DEFAULT_OSC_INPUT_CLK_KHZ;
    }
    else
    {
        // Attempt to set the Osc Clock to the desired value using user supplied callback
        if (cprState.pfGetSetOscFreqKhzCb(pClkCfg->targetOscInputClock)!= pClkCfg->targetOscInputClock)
        {
            retVal =  ERR_UNABLE_TO_ACHIEVE_OSC_CLK;
            goto CPR_SETUP_ERR;
        }
        cprState.currentOscInClkKhz = cprState.pfGetSetOscFreqKhzCb(pClkCfg->targetOscInputClock);
    }

    // The following function will bypass the PLL
    // Configure the PLL and the Sys clock divider but don't re-enable the PLL yet
    retVal = setupSystemClk(pClkCfg->targetPllFreqKhz,(pClkCfg->systemClockConfig.sysClockDividerValue >> 16)
                                                     ,(pClkCfg->systemClockConfig.sysClockDividerValue & 0xFFFF) ); 
    if (retVal )
        goto CPR_SETUP_ERR;

    // Enable the relevant system clocks
    DrvCprSysDeviceAction(SET_CLKS,pClkCfg->systemClockConfig.sysClockEnableMask);

    // Configure all of the specified Auxilary clocks
    DrvCprAuxClockArrayConfig(pClkCfg->pAuxClkCfg);

    // Finally turn back on the PLL (which also notifies the app)
    DrvCprPllBypass(FALSE);

    return ERR_CPR_OK;

CPR_SETUP_ERR:
    assert(FALSE);
    return retVal;
}



int DrvCprConfigureSystemClk(int pllTargetFreqKhz, int clockDivNumerator, int clockDivDenominator)
{
    int retVal;

    if ( ! cprState.initialised)
        return ERR_CPR_NOT_INITIALISED;

    // Ensure PLL is bypassed
    DrvCprPllBypass(TRUE);

    // Configure the PLL and the Sys clock divider but don't re-enable the PLL yet
    retVal = setupSystemClk(pllTargetFreqKhz,clockDivNumerator,clockDivDenominator);
    if (retVal )
        return retVal;

    // Re-enable PLL
    DrvCprPllBypass(FALSE);

    return cprState.currentSysClockKhz;
}

// Allows user to enable/disable/reset multiple system devices
void DrvCprSysDeviceAction(tyCprDeviceAction action,u64 devices)
{
    // It does what it says on the tin
    u32 devices0;
    u32 devices1;
    u32 base;

    if (action == RESET_DEVICES)
        base = CPR_BLK_RST0_ADR;
    else
        base = CPR_CLK_EN0_ADR;

    devices0 = GET_REG_WORD_VAL(base    );
    devices1 = GET_REG_WORD_VAL(base + 4);

    if (action == ENABLE_CLKS)
    {      // For enable we just set the relevant bits
        devices1 |= (devices >> 32       );
        devices0 |= (devices & 0xFFFFFFFF);
    } else if (action == SET_CLKS)
    {
        devices1 = ((u32)(devices >> 32));
        devices0 = ((u32)(devices & 0xFFFFFFFF));
    }
    else   // For disable and reset we clear the bits
    {
        devices1 &= ~((u32)(devices >> 32));
        devices0 &= ~((u32)(devices & 0xFFFFFFFF));
    }

    SET_REG_WORD(base       , devices0);
    SET_REG_WORD(base + 4   , devices1);

    // Delaying 10 cycles to ensure we wait long enough for the 
    // hardcoded 8 sysclk reset active period in the CPR block
    // This is common as it does no harm in the clocking case
    SleepTicks(10);

    return; 
}

int DrvCprAuxClocksEnable(u32 auxClksMask,u16 numerator, u16 denominator)
{
    u32 auxClk;
    u32 auxClockDivider;

    auxClockDivider = GEN_CLK_DIVIDER(numerator,denominator);

    for (auxClk = 0; auxClk <32; auxClk++)
    {
        if ((auxClksMask & (1<<auxClk)) == (1u<<auxClk))
        {
            DPRINTF1("\nEnabling clock %d",auxClk);
            if (auxClkDividerLookupWOffset[auxClk] != 0) // Not all enables have an associated divider
            {
                SET_REG_WORD(CPR_BASE_ADR + (auxClkDividerLookupWOffset[auxClk] << 2), auxClockDivider);
                DPRINTF1("\nWriting %08X to %08X\n", auxClockDivider, CPR_BASE_ADR + (auxClkDividerLookupWOffset[auxClk] << 2));
            }
            else
            {
                DPRINTF1("\nClock %d does not have an associated divider\n", auxClk);
            }
        }
    }
    // Finally enable the relevant clocks
    SET_REG_BITS_MASK(CPR_AUX_CLK_EN_ADR, auxClksMask);

    return 0;
}

int DrvCprAuxClockArrayConfig(const tyAuxClkDividerCfg  pAuxClkCfg[])
{
    int i;
    // Limit the loop in case someone forgets to terminate the list
    for (i = 0 ; i < MAX_ALLOWED_AUX_CONFIGS; i++)
    {
        if (pAuxClkCfg[i].auxClockEnableMask == 0) // Null Entry terminated list
            break;

        (void)DrvCprAuxClocksEnable(pAuxClkCfg[i].auxClockEnableMask,
                                    (pAuxClkCfg[i].auxClockDividerValue >> 16),
                                    (pAuxClkCfg[i].auxClockDividerValue&0xFFFF));
        DPRINTF1("\nAux Clock Config          : %d    ",i);
        DPRINTF1("\n   Aux Clock Enable Mask  : %08X  ",pAuxClkCfg[i].auxClockEnableMask);
        DPRINTF1("\n   Aux Clock Divider      : %08X  ",pAuxClkCfg[i].auxClockDividerValue);
    }

    return 0;
}

// Passed an Enum of different clock types, SYSTEM, AUX clocks
int DrvCprGetClockFreqKhz(tyClockType clockType,tyClockConfig * clkCfg)
{
    u32 clockDivider;
    u32 numerator;
    u32 denominator;
    u32 clockKhz;

    clkCfg = clkCfg;   // Deliberately unused for now

    assert(clockType < LAST_CLK);

    // Updates a stuct containing nominal clock, and clock div description
    if (clockType == SYS_CLK)
    {
        clockDivider = GET_REG_WORD_VAL(CPR_CLK_DIV_ADR);  
    }
    else if (clockType == PLL_CLK)
    {
        return cprState.currentPllClockKhz;
    }
    else 
    {   // Assume it is an auxilary clock
        clockDivider = GET_REG_WORD_VAL(CPR_BASE_ADR + (auxClkDividerLookupWOffset[(int)clockType] << 2));  
    }
    numerator       = (clockDivider >> 16    );
    denominator     = (clockDivider & 0xFFFF ); 
    denominator  = (clockDivider & 0xFFFF);
/*
 // this was related to bug 17911
    if ((denominator != 0) && ((clockType == AUX_CLK_LCD1) || (clockType == AUX_CLK_LCD2) || (clockType == AUX_CLK_LCD1X2) || (clockType == AUX_CLK_LCD2X2)))
    {
        clockKhz        = cprState.currentPllClockKhz * numerator / denominator / 4;
    }
    else if (denominator != 0)
    {
        clockKhz        = cprState.currentPllClockKhz * numerator / denominator;
    }
    else
    {
        clockKhz        = cprState.currentPllClockKhz;
    }
    */

    if (denominator != 0)
        clockKhz        = cprState.currentPllClockKhz * numerator / denominator;
    else
        clockKhz        = cprState.currentPllClockKhz;

    // TODO: This function needs to optionally update the clkCfg Structure
    return clockKhz;
}

int DrvCprPllBypass(int bypass)
{
    if (bypass)
        SET_REG_BITS_MASK(CPR_CLK_BYPASS_ADR,PLL_BYPASS_BIT); // Bypass the PLL
    else
    {
        // engage PLL
        CLR_REG_BITS_MASK(CPR_CLK_BYPASS_ADR,PLL_BYPASS_BIT);
        notifySystemClockChange();
    }
    return 0;
}


////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////
// Static Function Implementations
////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////

static int notifySystemClockChange(void)
{
    // Handles the things that need to happen every time the system clock changes.
    // There are two reasons the system clock can change:
    // 1) DrvCprSetupClocks applies a new clock config
    // 2) DrvCprConfigureSystemClk updates just the system clock
    // In both cases the PLL will first be disabled resulting in a temporary switch to 12Mhz
    // and then once new clock is applied the PLL is re-enabled. At this stage this function is called
    // Note: We have a policy of not notifying the application of an actual PLL Bypass as there is 
    // an implicit assumption that this is a transient event and not one that can be validly handled by an application

    // Calculate the local parameters
    cprState.currentSysClockKhz = calculateSystemClockKhz(cprState.currentPllClockKhz);
    cprState.currentTicksPerMicoSecond = cprState.currentSysClockKhz / 1000;

    // We assume that the DDR clock will be changed in sync with the system clock
    // For most clocks, the user is notified, but given that almost everyone will be using DDR
    // we make an exception for DRAM, and handle this transparently
    DrvDdrNotifyClockChange(DrvCprGetClockFreqKhz(AUX_CLK_DDR,NULL));

    // If the user has requested an application callback for frequency we do so now
    if (cprState.pfSysClockFreqChange != 0)
        cprState.pfSysClockFreqChange(cprState.currentSysClockKhz);

    return 0;
}

static u32 calculateSystemClockKhz(u32 pllClockKhz)
{
    u32 systemClockKhz;
    u32 numerator;
    u32 denominator;
    numerator = GET_REG_WORD_VAL(CPR_CLK_DIV_ADR); 
    denominator = numerator & 0xFFFF; 
    numerator = (numerator >> 16) & 0xFFFF;

    // This function only works with "reasonable" numerator values
    // If the pll frequency is 800Mhz we can't deal with numerators over 5368 without
    // overflowing the 32 bit value. I think this assumption is reasonably safe, but adding assert JIC
    assert(numerator < 5000);

    systemClockKhz = pllClockKhz * numerator;
    systemClockKhz = systemClockKhz / denominator;

    return systemClockKhz;
}

static int setupSystemClk(int pllTargetFreqKhz, int clockDivNumerator, int clockDivDenominator)
{
    u32 pllConfigWord;

    // Configure the PLL for the desired frequency
    pllConfigWord = calculatePllConfigValue(cprState.currentOscInClkKhz,pllTargetFreqKhz,&cprState.currentPllClockKhz);
    if (cprState.currentPllClockKhz == 0)
    {
        assert(0);
        return ERR_PLL_CONFIG_INVALID;
    }

    // We must bypass the PLL before reconfiguring it
    DrvCprPllBypass(TRUE);

    // Set the new PLL configuration
    SET_REG_WORD(CPR_PLL_CTRL_ADR, pllConfigWord); 

    // Wait for PLL to acknowledge its new settings
    if (DrvRegWaitForBitsSetAll(CPR_PLL_STAT_ADR, PLL_UPDATE_COMPLETE_MASK,PLL_SW_TIMEOUT_US))
    {
        assert(FALSE); 
        return ERR_PLL_SET_TIMEOUT;
    }

    // Wait for the PLL to either lock or signal its own timeout
    if (DrvRegWaitForBitsSetAny(CPR_PLL_STAT_ADR,PLL_LOCKED_OR_TIMEOUT_MASK,PLL_SW_TIMEOUT_US))
    {
        assert(FALSE); 
        return ERR_PLL_LOCK_TIMEOUT;
    }

    // Check if the PLL hardware failed to lock
    if (!(GET_REG_WORD_VAL(CPR_PLL_STAT_ADR) & PLL_LOCKED_MASK))
       return ERR_PLL_LOCK_FAILURE;

    // Setup the clock divider
    SET_REG_WORD(CPR_CLK_DIV_ADR,(clockDivNumerator << 16) | (clockDivDenominator & 0xFFFF));

    // Crucially we don't renable the PLL at this stage.
    // It is up the to caller to handle that

    return ERR_CPR_OK;
}

static u32 calculatePllConfigValue(u32 oscIn, u32 sysClkDesired,u32 * pAchievedFreqKhz)
{
    u32 pllCtrlVal, freq;
    u32 fRef;
    u32 fVco;
    u32 diff, diffBk;

    u32 inDiv, feedbackDiv, outDiv, band;
    u32 inDivBk = 0, feedbackDivBk = 0, outDivBk = 0, bandBk = 0;

    diffBk = 1000000; // 1 GHz difference

    for (inDiv = 1 ; inDiv < 33 ; ++inDiv)
    {
        fRef = oscIn / inDiv ; 
        if ((fRef >= 10000) && (fRef <= 50000))
        {
            band = 0;   
            for (outDiv = 0 ; outDiv < 4 ; ++outDiv)
            {
                fVco = sysClkDesired  * (1<<outDiv);
                if ((fVco <= 600000) && (fVco >= 300000))
                {
                    feedbackDiv = (sysClkDesired * (1 << outDiv) * inDiv) / oscIn;
                    if ((feedbackDiv >= 6) && (feedbackDiv <= 100))
                    {
                        freq = oscIn * feedbackDiv / ((1<<outDiv) * inDiv);

                        if (sysClkDesired > freq)
                            diff = sysClkDesired - freq;
                        else
                            diff = freq - sysClkDesired;

                        if (diff < diffBk)
                        {
                            inDivBk = inDiv;
                            feedbackDivBk = feedbackDiv;
                            outDivBk = outDiv;
                            bandBk = band;
                            diffBk = diff;
                        }
                    }
                }
            }

            // check band 1 for possible smaller frq
            band = 1;   
            for (outDiv = 0 ; outDiv < 4 ; ++outDiv)
            {
                fVco = sysClkDesired  * (1<<outDiv);
                if ((fVco <= 1000000) && (fVco >= 500000))
                {
                    feedbackDiv = (sysClkDesired * (1 << outDiv) * inDiv) / oscIn;
                    if ((feedbackDiv >= 6) && (feedbackDiv <= 100))
                    {
                        freq = oscIn * feedbackDiv / ((1<<outDiv) * inDiv);

                        if (sysClkDesired > freq)
                            diff = sysClkDesired - freq;
                        else
                            diff = freq - sysClkDesired;

                        if (diff < diffBk)
                        {
                            inDivBk = inDiv;
                            feedbackDivBk = feedbackDiv;
                            outDivBk = outDiv;
                            bandBk = band;
                            diffBk = diff;
                        }
                    }
                }
            }                       
        }
    }

    if (diffBk > 10000)
    {
        *pAchievedFreqKhz = 0;
        return 0; // don't set, diference too big 
    }

    pllCtrlVal = ( (inDivBk - 1) << 6)       |
                   ( (feedbackDivBk - 1)<< 11) | 
                   ( outDivBk << 4 )         |
                   ( bandBk << 2);

    freq = oscIn * feedbackDivBk / ((1<<outDivBk) * inDivBk);

    *pAchievedFreqKhz = freq;
    return pllCtrlVal;
}


