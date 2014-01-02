///  
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved 
///            For License Warranty see: common/license.txt              
///
/// @brief     Definitions and types needed by Clock Power Reset Module
/// 
#ifndef DRV_CPR_DEF_H
#define DRV_CPR_DEF_H 

#include "mv_types.h"

                                                                       
// 1: Defines
// ----------------------------------------------------------------------------

// Minimum output frequency of the PLL is 37.5Mhz                                                        
#define CPR_MIN_PLL_FREQ_KHZ                            (37500)                           

/// Utility Macro to generate 32 bit divider configuration from numerator, denominator                       
#define GEN_CLK_DIVIDER(numerator, denominator)  (((numerator<<16) & 0xFFFF0000) | (denominator & 0xFFFF))  
                       
// -----------------------------------------------------------------------------------------
// ---  API Error Codes
// -----------------------------------------------------------------------------------------
              
#define ERR_CPR_OK                                      ( 0)
#define ERR_CPR_NOT_INITIALISED                         (-1)
#define ERR_UNSUPPORTED_OSC_CLK                         (-2)
#define ERR_UNABLE_TO_ACHIEVE_OSC_CLK                   (-3)
#define ERR_PLL_SET_TIMEOUT                             (-4)
#define ERR_PLL_LOCK_TIMEOUT                            (-5)
#define ERR_PLL_LOCK_FAILURE                            (-6)
#define ERR_PLL_CONFIG_INVALID                          (-7)
#define ERR_BLAH3                                       (-8)
#define ERR_BLAH4                                       (-9)
#define ERR_BLAH5                                       (-10)

// -----------------------------------------------------------------------------------------
// ---  CPRGenCtrl bits
// -----------------------------------------------------------------------------------------

#define CPR_GEN_CTRL_CMX_AHB_CACHE_ENABLE         (1 << 16)
#define CPR_GEN_CTRL_LCD2_TO_CIF1_LOOPBACK_ENABLE (1 << 15)
#define CPR_GEN_CTRL_LCD1_TO_CIF2_LOOPBACK_ENABLE (1 << 14)
#define CPR_GEN_CTRL_DEBUG_CLOCK_EN_UNI_DIR_WP    (1 << 13)
#define CPR_GEN_CTRL_AXM_DEBUG_ENABLED            (1 << 11)
#define CPR_GEN_CTRL_SDIO_32KHZ_CLK_SOURCE_SEL    (1 << 3)
#define CPR_GEN_CTRL_I2S_MASTER_SEL_2             (1 << 2)
#define CPR_GEN_CTRL_I2S_MASTER_SEL_1             (1 << 1)
#define CPR_GEN_CTRL_I2S_MASTER_SEL_0             (1 << 0)

// -----------------------------------------------------------------------------------------
// ---  CPRWakeupMask Bits 
// -----------------------------------------------------------------------------------------
#define CPR_GPIO_WAKEUP_DISABLE                   (1 << 0)
#define CPR_RTC_WAKEUP_DISABLE                    (1 << 1)
#define CPR_SDH0_WAKEUP_DISABLE                   (1 << 2)

// -----------------------------------------------------------------------------------------
// ---  System Clocks
// -----------------------------------------------------------------------------------------
                       
// Special Encoding to allow user to enable modules without knowing which clock tree the module
// belongs to
// The trick is to use u64 datatype to store both clocks
// Note: We place the most significant word first in memory as is normal on a big endian system
// This also means that the clock/reset words could potentially be written as a single U64 operation

#define     DEV_LEON             ( (u64)1 <<  0ULL) 
#define     DEV_LEON_ROM         ( (u64)1 <<  1ULL)  
#define     DEV_LEON_RAM         ( (u64)1 <<  2ULL)  
#define     DEV_ICB              ( (u64)1 <<  3ULL)  
#define     DEV_DSU              ( (u64)1 <<  4ULL)  
#define     DEV_TIM              ( (u64)1 <<  5ULL)  
#define     DEV_GPIO             ( (u64)1 <<  6ULL)  
#define     DEV_IIC1             ( (u64)1 <<  7ULL)  
#define     DEV_SPI1             ( (u64)1 <<  8ULL)  
#define     DEV_SPI2             ( (u64)1 <<  9ULL)  
#define     DEV_IIC2             ( (u64)1 << 10ULL)  
#define     DEV_SPI3             ( (u64)1 << 11ULL)  
//                                                       Bit 12 not used
#define     DEV_SEBI             ( (u64)1 << 13ULL)  
#define     DEV_MEBI             ( (u64)1 << 14ULL)  
#define     DEV_UART             ( (u64)1 << 15ULL)  
#define     DEV_JTAG             ( (u64)1 << 16ULL) 
//                                                       Bits 17-19 not used
#define     DEV_TSI              ( (u64)1 << 20ULL) 
#define     DEV_GPS              ( (u64)1 << 21ULL) 
//                                                       Bit 22 not used
#define     DEV_NAL              ( (u64)1 << 23ULL) 
//                                                       Bit 24 not used
#define     DEV_APB_CTRL         ( (u64)1 << 25ULL)   
#define     DEV_KBD              ( (u64)1 << 26ULL)    
#define     DEV_I2S1             ( (u64)1 << 27ULL)    
#define     DEV_I2S2             ( (u64)1 << 28ULL)    
#define     DEV_I2S3             ( (u64)1 << 29ULL)  
//                                                       Bits 30-31 not used
                                 
#define     DEV_SVU0             ( (u64)1 << 32ULL)
#define     DEV_SVU1             ( (u64)1 << 33ULL)
#define     DEV_SVU2             ( (u64)1 << 34ULL)
#define     DEV_SVU3             ( (u64)1 << 35ULL)
#define     DEV_SVU4             ( (u64)1 << 36ULL)
#define     DEV_SVU5             ( (u64)1 << 37ULL)
#define     DEV_SVU6             ( (u64)1 << 38ULL)
#define     DEV_SVU7             ( (u64)1 << 39ULL)
#define     DEV_CMX              ( (u64)1 << 40ULL)
#define     DEV_MXI_AXI          ( (u64)1 << 41ULL)
#define     DEV_SXI_AXI          ( (u64)1 << 42ULL)
#define     DEV_AXI2AXI          ( (u64)1 << 43ULL)
#define     DEV_AXI2AHB          ( (u64)1 << 44ULL)
#define     DEV_AXI_MON          ( (u64)1 << 45ULL)
#define     DEV_L2               ( (u64)1 << 46ULL)
#define     DEV_CIF1             ( (u64)1 << 47ULL)
#define     DEV_CIF2             ( (u64)1 << 48ULL)
#define     DEV_LCD1             ( (u64)1 << 49ULL)
#define     DEV_LCD2             ( (u64)1 << 50ULL)
#define     DEV_TVE              ( (u64)1 << 51ULL)
#define     DEV_BIST             ( (u64)1 << 52ULL) 
#define     DEV_DDRC             ( (u64)1 << 53ULL)
//                                                     Bits 54,56 not used
#define     DEV_SAHB_BUS         ( (u64)1 << 56ULL)
#define     DEV_SDIO_DEV         ( (u64)1 << 57ULL)
#define     DEV_SDIO_H1          ( (u64)1 << 58ULL)
#define     DEV_SDIO_H2          ( (u64)1 << 59ULL)
#define     DEV_USB              ( (u64)1 << 60ULL)
#define     DEV_NANDC            ( (u64)1 << 61ULL)
//                                                     Bits 62,63 not used

// -----------------------------------------------------------------------------------------
// ---  This macro defines the clocks which are normally in use in most applications
// -----------------------------------------------------------------------------------------
// Notes: Shaves are currently clocked by default, but this should be removed once shave drivers
//        have been re-factored to auto-enable clocks as needed.. Note: UART is used for debug console
//        Also SAHB is enabled by default to favor "apps just working" over reliability
#define DEFAULT_CORE_BUS_CLOCKS     (  DEV_LEON         |     \
                                       DEV_LEON_ROM     |     \
                                       DEV_LEON_RAM     |     \
                                       DEV_ICB          |     \
                                       DEV_DSU          |     \
                                       DEV_TIM          |     \
                                       DEV_GPIO         |     \
                                       DEV_JTAG         |     \
                                       DEV_UART         |     \
                                       DEV_TSI          |     \
                                       DEV_APB_CTRL     |     \
                                       DEV_CMX          |     \
                                       DEV_MXI_AXI      |     \
                                       DEV_SXI_AXI      |     \
                                       DEV_AXI2AXI      |     \
                                       DEV_AXI2AHB      |     \
                                       DEV_AXI_MON      |     \
                                       DEV_L2           |     \
                                       DEV_DDRC         |     \
                                       DEV_SAHB_BUS           \
                                       )
// Note: The following define is only provided for use while debugging clock configurations
// You are strongly encouraged to only enable the necessary clocks for your application
#define ALL_SYSTEM_CLOCKS               ( (u64) 0xFFFFFFFFFFFFFFFFULL  )
// -----------------------------------------------------------------------------------------
// ---  Auxilary Clocks
// -----------------------------------------------------------------------------------------
                                                                                                       
// Bitfields corresponding for each Auxilary clock enable
#define AUX_CLK_MASK_I2S0       (1 <<   0)
#define AUX_CLK_MASK_I2S1       (1 <<   1) 
#define AUX_CLK_MASK_I2S2       (1 <<   2) 
#define AUX_CLK_MASK_USB        (1 <<   3) 
#define AUX_CLK_MASK_DDR        (1 <<   4) 
#define AUX_CLK_MASK_SDHOST_1   (1 <<   5) 
#define AUX_CLK_MASK_SDHOST_2   (1 <<   6) 
#define AUX_CLK_MASK_CIF1       (1 <<   7) 
#define AUX_CLK_MASK_CIF2       (1 <<   8) 
#define AUX_CLK_MASK_LCD1       (1 <<   9) 
#define AUX_CLK_MASK_LCD1X2     (1 <<  10)  // Double Data Rate LCD clock
#define AUX_CLK_MASK_LCD2       (1 <<  11) 
#define AUX_CLK_MASK_LCD2X2     (1 <<  12)  // Double Data Rate LCD clock 
#define AUX_CLK_MASK_MEBI_P     (1 <<  13) 
#define AUX_CLK_MASK_MEBI_N     (1 <<  14) 
#define AUX_CLK_MASK_IO         (1 <<  15) 
#define AUX_CLK_MASK_32KHZ      (1 <<  16) 
#define AUX_CLK_MASK_TSI        (1 <<  17) 
#define AUX_CLK_MASK_NAL_P      (1 <<  18)  // Nal Unit Parser (Decode Process)
#define AUX_CLK_MASK_NAL_S      (1 <<  19)  // Nal Unit Synth  (Encode Process) 
#define AUX_CLK_MASK_SAHB       (1 <<  20) 

#define AUX_CLK_MASK_ALL_CLKS   (0xFFFFFFFF) // Only provided for debug purposes

// Note: Beware the difference between the bitmasks above which are used for enabling muliple clocks and the enum below which is
// used to request info about a specific clock value. They are obviously not interchangeable!!               

// 2: Typedefs (types, enums, structs)
// ----------------------------------------------------------------------------
// Used by drvCprGetClockFreq to identify the clock being requested
typedef enum
{
    AUX_CLK_I2S0        =   0,
    AUX_CLK_I2S1        =   1,
    AUX_CLK_I2S2        =   2,
    AUX_CLK_USB         =   3,
    AUX_CLK_DDR         =   4,
    AUX_CLK_SDHOST_1    =   5,
    AUX_CLK_SDHOST_2    =   6,
    AUX_CLK_CIF1        =   7,
    AUX_CLK_CIF2        =   8,
    AUX_CLK_LCD1        =   9,
    AUX_CLK_LCD1X2      =  10,
    AUX_CLK_LCD2        =  11,
    AUX_CLK_LCD2X2      =  12,
    AUX_CLK_MEBI_P      =  13,
    AUX_CLK_MEBI_N      =  14,
    AUX_CLK_IO          =  15,
    AUX_CLK_32KHZ       =  16,
    AUX_CLK_TSI         =  17,
    AUX_CLK_NAL_P       =  18,  // Nal Unit Parser clock (Decode)
    AUX_CLK_NAL_S       =  19,  // Nal Unit Synth  clock (Encdoe)
    AUX_CLK_SAHB        =  20,
    SYS_CLK             =  21,
    PLL_CLK             =  22,
	LAST_CLK            =  23   // This is just a marker for the end of the enum!
}tyClockType;

/// This function will be called by the CPR driver to modify the current 
/// input oscillator frequency on hardware platforms that support it. 
/// It returns the achived OSC in frequency in Khz
typedef u32 (*tyGetSetOscFreqKhzCb) (u32 targetOscFreqKhz); 

/// This function will be called by the CPR driver when the current 
typedef u32 (*tySysClkFreqChangeCb) (u32 newSysClockFrequency);  


// Used by DrvCprSysDeviceAction to signal the action applied to the devices
typedef enum 
{
    ENABLE_CLKS,        // Enable the specified clocks without effecting existing clocks
    DISABLE_CLKS,       // Disable the specified clocks without effecting existing clocks 
    SET_CLKS,           // Enable the specified clocks and disable all others
    RESET_DEVICES       // Reset the specified device blocks
}tyCprDeviceAction;



// TODO: Complete this structure
typedef struct
{
    unsigned int nominalFreqKhz;
    unsigned int status; // e.g. clean or not.. TODO!!
} tyClockConfig;

typedef struct
{
    u32          auxClockEnableMask;     // Bitfield contining a number of Auxillary clocks to be enabled
    u32          auxClockDividerValue;   // Divider to be applied to the clocks specified
}tyAuxClkDividerCfg;

typedef struct
{
    u64          sysClockEnableMask;     // Bitfield contining all of the system clocks to be enabled 
    u32          sysClockDividerValue;   // Used for almost all system devices
}tySysClkDividerCfg;

typedef struct 
{
    unsigned int                    targetOscInputClock;
    unsigned int                    targetPllFreqKhz;
    tySysClkDividerCfg              systemClockConfig;
    const tyAuxClkDividerCfg       *pAuxClkCfg;            // Null Terminated array of aux clock configs
} tySocClockConfig;


// 3: Local const declarations     NB: ONLY const declarations go here
// ----------------------------------------------------------------------------

#endif // DRV_CPR_DEF_H

