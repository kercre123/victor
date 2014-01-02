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
#include "registersMyriad.h"

                                                                       
// 1: Defines
// ----------------------------------------------------------------------------

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


// -----------------------------------------------------------------------------------------
// ---  System Clocks
// -----------------------------------------------------------------------------------------
                       
// Special Encoding to allow user to enable modules without knowing which clock tree the module
// belongs to
// The trick is to use u64 datatype to store both clocks
                        
///////////////////////////////////////                         
// CPRUnitClockEn0 (0-31)
///////////////////////////////////////                         
#define     DEV_CSS_LOS                 ( 1ULL    <<    CSS_LOS               )
#define     DEV_CSS_LAHB_CTRL           ( 1ULL    <<    CSS_LAHB_CTRL         )
#define     DEV_CSS_APB4_CTRL           ( 1ULL    <<    CSS_APB4_CTRL         )
#define     DEV_CSS_CPR                 ( 1ULL    <<    CSS_CPR               )
#define     DEV_CSS_ROM                 ( 1ULL    <<    CSS_ROM               )
#define     DEV_CSS_LOS_L2C             ( 1ULL    <<    CSS_LOS_L2C           )
#define     DEV_CSS_MAHB_CTRL           ( 1ULL    <<    CSS_MAHB_CTRL         )
#define     DEV_CSS_LOS_ICB             ( 1ULL    <<    CSS_LOS_ICB           )
#define     DEV_CSS_LOS_DSU             ( 1ULL    <<    CSS_LOS_DSU           )
#define     DEV_CSS_LOS_TIM             ( 1ULL    <<    CSS_LOS_TIM           )
#define     DEV_CSS_GPIO                ( 1ULL    <<    CSS_GPIO              )
#define     DEV_CSS_JTAG                ( 1ULL    <<    CSS_JTAG              )
#define     DEV_CSS_SDIO                ( 1ULL    <<    CSS_SDIO              )
#define     DEV_CSS_ROIC                ( 1ULL    <<    CSS_ROIC              )         
#define     DEV_CSS_GETH                ( 1ULL    <<    CSS_GETH              )
#define     DEV_CSS_BIST                ( 1ULL    <<    CSS_BIST              )
#define     DEV_CSS_APB1_CTRL           ( 1ULL    <<    CSS_APB1_CTRL         )
#define     DEV_CSS_AHB_DMA             ( 1ULL    <<    CSS_AHB_DMA           )
#define     DEV_CSS_APB3_CTRL           ( 1ULL    <<    CSS_APB3_CTRL         )
#define     DEV_CSS_I2C0                ( 1ULL    <<    CSS_I2C0              )
#define     DEV_CSS_I2C1                ( 1ULL    <<    CSS_I2C1              )
#define     DEV_CSS_I2C2                ( 1ULL    <<    CSS_I2C2              )
#define     DEV_CSS_UART                ( 1ULL    <<    CSS_UART              )
#define     DEV_CSS_SPI0                ( 1ULL    <<    CSS_SPI0              )
#define     DEV_CSS_SPI1                ( 1ULL    <<    CSS_SPI1              )
#define     DEV_CSS_SPI2                ( 1ULL    <<    CSS_SPI2              )
#define     DEV_CSS_I2S0                ( 1ULL    <<    CSS_I2S0              )
#define     DEV_CSS_I2S1                ( 1ULL    <<    CSS_I2S1              )
#define     DEV_CSS_I2S2                ( 1ULL    <<    CSS_I2S2              )
#define     DEV_CSS_SAHB_CTRL           ( 1ULL    <<    CSS_SAHB_CTRL         )
#define     DEV_CSS_MSS_MAS             ( 1ULL    <<    CSS_MSS_MAS           )
#define     DEV_CSS_UPA_MAS             ( 1ULL    <<    CSS_UPA_MAS           )
                                                                              
///////////////////////////////////////                                       
// CPRUnitClockEn1 (32-64)                                                    
///////////////////////////////////////                                       
#define     DEV_CSS_DSS_APB             ( 1ULL    <<    CSS_DSS_APB           )
#define     DEV_CSS_DSS_APB_PHY         ( 1ULL    <<    CSS_DSS_APB_PHY       )  // meant to be the same as above  (reset only bit)
#define     DEV_CSS_DSS_APB_CTRL        ( 1ULL    <<    CSS_DSS_APB_CTRL      )
#define     DEV_CSS_DSS_BUS             ( 1ULL    <<    CSS_DSS_BUS           )
#define     DEV_CSS_DSS_BUS_MAHB        ( 1ULL    <<    CSS_DSS_BUS_MAHB      )  // meant to be the same as above  (reset only bit) 
#define     DEV_CSS_DSS_BUS_DXI         ( 1ULL    <<    CSS_DSS_BUS_DXI       )
#define     DEV_CSS_DSS_BUS_AAXI        ( 1ULL    <<    CSS_DSS_BUS_AAXI      )
#define     DEV_CSS_DSS_BUS_MXI         ( 1ULL    <<    CSS_DSS_BUS_MXI       )
#define     DEV_CSS_LAHB2SHB            ( 1ULL    <<    CSS_LAHB2SHB          )
#define     DEV_CSS_SAHB2MAHB           ( 1ULL    <<    CSS_SAHB2MAHB         )
#define     DEV_CSS_USB                 ( 1ULL    <<    CSS_USB               )

////////////////////////////////////////////////////////                         
// MSS_CLK_CTRL_ADR (Media Subsystem Clocks (0-31)
////////////////////////////////////////////////////////                         
#define     DEV_MSS_APB_SLV             ( (u32)1   <<   MSS_APB_SLV           )
#define     DEV_MSS_APB2_CTRL           ( (u32)1   <<   MSS_APB2_CTRL         )
#define     DEV_MSS_RTAHB_CTRL          ( (u32)1   <<   MSS_RTAHB_CTRL        )
#define     DEV_MSS_RTBRIDGE            ( (u32)1   <<   MSS_RTBRIDGE          )
#define     DEV_MSS_LRT                 ( (u32)1   <<   MSS_LRT               )
#define     DEV_MSS_LRT_DSU             ( (u32)1   <<   MSS_LRT_DSU           )
#define     DEV_MSS_LRT_L2C             ( (u32)1   <<   MSS_LRT_L2C           )
#define     DEV_MSS_LRT_ICB             ( (u32)1   <<   MSS_LRT_ICB           )
#define     DEV_MSS_MXI_CTRL            ( (u32)1   <<   MSS_MXI_CTRL          )
#define     DEV_MSS_MXI_DEFSLV          ( (u32)1   <<   MSS_MXI_DEFSLV        )
#define     DEV_MSS_AXI_MON             ( (u32)1   <<   MSS_AXI_MON           )
#define     DEV_MSS_NAL                 ( (u32)1   <<   MSS_NAL               )
#define     DEV_MSS_SEBI                ( (u32)1   <<   MSS_SEBI              )
#define     DEV_MSS_MIPI                ( (u32)1   <<   MSS_MIPI              )
#define     DEV_MSS_CIF0                ( (u32)1   <<   MSS_CIF0              )
#define     DEV_MSS_CIF1                ( (u32)1   <<   MSS_CIF1              )
#define     DEV_MSS_LCD                 ( (u32)1   <<   MSS_LCD               )
#define     DEV_MSS_AMC                 ( (u32)1   <<   MSS_AMC               )
#define     DEV_MSS_TIM                 ( (u32)1   <<   MSS_TIM               )
#define     DEV_MSS_SIPP_ABPSLV         ( (u32)1   <<   MSS_SIPP_ABPSLV       )
#define     DEV_MSS_SIPP                ( (u32)1   <<   MSS_SIPP              )
                                                                              
////////////////////////////////////////////////////////                               
// CMX_CLK_CTRL (UPA (micro processor array) Subsystem Clocks (0-31)
////////////////////////////////////////////////////////                         
#define     DEV_UPA_SH0                 ( (u32)1   <<   UPA_SH0               )
#define     DEV_UPA_SH1                 ( (u32)1   <<   UPA_SH1               )
#define     DEV_UPA_SH2                 ( (u32)1   <<   UPA_SH2               )
#define     DEV_UPA_SH3                 ( (u32)1   <<   UPA_SH3               )
#define     DEV_UPA_SH4                 ( (u32)1   <<   UPA_SH4               )
#define     DEV_UPA_SH5                 ( (u32)1   <<   UPA_SH5               )
#define     DEV_UPA_SH6                 ( (u32)1   <<   UPA_SH6               )
#define     DEV_UPA_SH7                 ( (u32)1   <<   UPA_SH7               )
#define     DEV_UPA_SH8                 ( (u32)1   <<   UPA_SH8               )
#define     DEV_UPA_SH9                 ( (u32)1   <<   UPA_SH9               )
#define     DEV_UPA_SH10                ( (u32)1   <<   UPA_SH10              )
#define     DEV_UPA_SH11                ( (u32)1   <<   UPA_SH11              )
#define     DEV_UPA_SHAVE_L2            ( (u32)1   <<   UPA_SHAVE_L2          )
#define     DEV_UPA_CDMA                ( (u32)1   <<   UPA_CDMA              )
#define     DEV_UPA_BIC                 ( (u32)1   <<   UPA_BIC               )
#define     DEV_UPA_CTRL                ( (u32)1   <<   UPA_CTRL              )
                          
/*
// This Macro is only intended for testing purposes
// Best practice is to only enable the clcoks you need
#define ALL_CSS_DSS_CLOCKS     ( DEV_CSS_LOS             |
                                 DEV_CSS_LAHB_CTRL       |
                                 DEV_CSS_APB4_CTRL       |
                                 DEV_CSS_CPR             |
                                 DEV_CSS_ROM             |
                                 DEV_CSS_LOS_L2C         |
                                 DEV_CSS_MAHB_CTRL       |
                                 DEV_CSS_LOS_ICB         |
                                 DEV_CSS_LOS_DSU         |
                                 DEV_CSS_LOS_TIM         |
                                 DEV_CSS_GPIO            |
                                 DEV_CSS_JTAG            |
                                 DEV_CSS_SDIO            |
                                 DEV_CSS_ROIC            |
                                 DEV_CSS_GETH            |
                                 DEV_CSS_BIST            |
                                 DEV_CSS_APB1_CTRL       |
                                 DEV_CSS_AHB_DMA         |
                                 DEV_CSS_APB3_CTRL       |
                                 DEV_CSS_I2C0            |
                                 DEV_CSS_I2C1            |
                                 DEV_CSS_I2C2            |
                                 DEV_CSS_UART            |
                                 DEV_CSS_SPI0            |
                                 DEV_CSS_SPI1            |
                                 DEV_CSS_SPI2            |
                                 DEV_CSS_I2S0            |
                                 DEV_CSS_I2S1            |
                                 DEV_CSS_I2S2            |
                                 DEV_CSS_SAHB_CTRL       |
                                 DEV_CSS_MSS_MAS         |
                                 DEV_CSS_UPA_MAS         |
                                 DEV_CSS_DSS_APB         |
                                 DEV_CSS_DSS_APB_PHY     |
                                 DEV_CSS_DSS_APB_CTRL    |
                                 DEV_CSS_DSS_BUS         |
                                 DEV_CSS_DSS_BUS_MAHB    |
                                 DEV_CSS_DSS_BUS_DXI     |
                                 DEV_CSS_DSS_BUS_AAXI    |
                                 DEV_CSS_DSS_BUS_MXI     |
                                 DEV_CSS_LAHB2SHB        |
                                 DEV_CSS_SAHB2MAHB       |
                                 DEV_CSS_USB             | 
                                 )
*/                                 
/*
#define DEFAULT_LOS_CLOCKS         (DEV_LEON         |     \
                                    DEV_LOS_CORE     |     \
                                    DEV_LHB_CTRL     |     \
                                    DEV_APB4_CTRL    |     \
                                    DEV_ROM          |     \
                                    DEV_LOS_L2CACHE  |     \
                                    DEV_MAHB_CTRL    |     \
                                    DEV_LOS_ICB      |     \
                                    DEV_LOS_DSU      |     \
                                    DEV_TIMER        |     \
                                    DEV_GPIO         |     \
                                    DEV_SAHB_CTRL    |     \
                                    DEV_CMX          |     \
                                    DEV_DDR_PHY      |     \
                                    DEV_DDR_CTRL     |     \
                                    DEV_DXI_CTRL     )

// This Macro is only intended for testing purposes
// Best practice is to only enable the clcoks you need
#define ALL_MSS_CLOCKS     (  DEV_LEON            |     \
                              DEV_MSS_CIF         |     \
                              DEV_MSS_LCD         |     \
                              DEV_MSS_SEBI        |     \
                              DEV_MSS_NAL         |     \
                              DEV_MSS_AMC         |     \
                              DEV_MSS_MIPI0       |     \
                              DEV_MSS_MIPI1       |     \
                              DEV_MSS_MIPI2       |     \
                              DEV_MSS_MIPI3       |     \
                              DEV_MSS_MIPI4       |     \
                              DEV_MSS_MIPI5       |     \
                              DEV_MSS_AXI2AHB     |     \
                              DEV_MSS_MXI_CTRL    |     \
                              DEV_MSS_AXI_BUSMON  |     \
                              DEV_MSS_LRT_CPU     |     \
                              DEV_MSS_LRT_ICB     |     \
                              DEV_MSS_LRT_DSU     |     \
                              DEV_MSS_LRT_L2CACHE |     \
                              DEV_MSS_RTHB_CTRL   |     \
                              DEV_MSS_APB2_CTRL   |     \
                              DEV_MSS_SIPP_MASTER |     \
                              DEV_MSS_SIIP_MEDIA  |     \
                              DEV_MSS_SIPP_APB    )
*/
// Note: The following define is only provided for use while debugging clock configurations
// You are strongly encouraged to only enable the necessary clocks for your application
//#define ALL_SYSTEM_CLOCKS               ( (u64) 0xFFFFFFFFFFFFFFFFULL  )

// -----------------------------------------------------------------------------------------
// ---  Auxilary Clocks
// -----------------------------------------------------------------------------------------
#define     AUX_CLK_MASK_I2S0             ( (u32)1   <<  CSS_AUX_I2S0         ) 
#define     AUX_CLK_MASK_I2S1             ( (u32)1   <<  CSS_AUX_I2S1         ) 
#define     AUX_CLK_MASK_I2S2             ( (u32)1   <<  CSS_AUX_I2S2         ) 
#define     AUX_CLK_MASK_SDIO             ( (u32)1   <<  CSS_AUX_SDIO         ) 
#define     AUX_CLK_MASK_GPIO             ( (u32)1   <<  CSS_AUX_GPIO         ) 
#define     AUX_CLK_MASK_32KHZ            ( (u32)1   <<  CSS_AUX_32KHZ        ) 
#define     AUX_CLK_MASK_IRDA             ( (u32)1   <<  CSS_AUX_IRDA         ) 
#define     AUX_CLK_MASK_CIF0             ( (u32)1   <<  CSS_AUX_CIF0         ) 
#define     AUX_CLK_MASK_CIF1             ( (u32)1   <<  CSS_AUX_CIF1         ) 
#define     AUX_CLK_MASK_LCD              ( (u32)1   <<  CSS_AUX_LCD          ) 
#define     AUX_CLK_MASK_NAL_PAR          ( (u32)1   <<  CSS_AUX_NAL_PAR      ) 
#define     AUX_CLK_MASK_NAL_SYN          ( (u32)1   <<  CSS_AUX_NAL_SYN      ) 
#define     AUX_CLK_MASK_MIPI_TX0         ( (u32)1   <<  CSS_AUX_MIPI_TX0     ) 
#define     AUX_CLK_MASK_MIPI_TX1         ( (u32)1   <<  CSS_AUX_MIPI_TX1     ) 
#define     AUX_CLK_MASK_USB_PHY_EXTREFCLK          ( (u32)1   <<  CSS_AUX_USB_PHY_EXTREFCLK      ) 
#define     AUX_CLK_MASK_USB_CTRL_SUSPEND_CLK      ( (u32)1   <<  CSS_AUX_USB_CTRL_SUSPEND_CLK  ) 
#define     AUX_CLK_MASK_USB_CTRL_REF_CLK         ( (u32)1   <<  CSS_AUX_USB_CTRL_REF_CLK     ) 
#define     AUX_CLK_MASK_DDR_REF          ( (u32)1   <<  CSS_AUX_DDR_REF      ) 
#define     AUX_CLK_MASK_ROIC             ( (u32)1   <<  CSS_AUX_ROIC         ) 
#define     AUX_CLK_MASK_MEDIA            ( (u32)1   <<  CSS_AUX_MEDIA        ) 
#define     AUX_CLK_MASK_DDR_CORE         ( (u32)1   <<  CSS_AUX_DDR_CORE     ) 
#define     AUX_CLK_MASK_DDR_CORE_CTRL    ( (u32)1   <<  CSS_AUX_DDR_CORE_CTRL) 
#define     AUX_CLK_MASK_DDR_CORE_PHY     ( (u32)1   <<  CSS_AUX_DDR_CORE_PHY ) 
#define     AUX_CLK_MASK_MIPI_ECFG        ( (u32)1   <<  CSS_AUX_MIPI_ECFG    ) 
#define     AUX_CLK_MASK_MIPI_CFG         ( (u32)1   <<  CSS_AUX_MIPI_CFG     ) 
#define     AUX_CLK_MASK_USB_PHY_REF_ALT_CLK         ( (u32)1   <<  CSS_AUX_USB_PHY_REF_ALT_CLK     ) 
 

#define AUX_CLK_MASK_ALL_CLKS   (0xFFFFFFFF) // Only provided for debug purposes

// Note: Beware the difference between the bitmasks above which are used for enabling muliple clocks and the enum below which is
// used to request info about a specific clock value. They are obviously not interchangeable!!               

// 2: Typedefs (types, enums, structs)
// ----------------------------------------------------------------------------
// Used by drvCprGetClockFreq to identify the clock being requested
typedef enum
{
    AUX_CLK_I2S0          =  CSS_AUX_I2S0         ,
    AUX_CLK_I2S1          =  CSS_AUX_I2S1         ,
    AUX_CLK_I2S2          =  CSS_AUX_I2S2         ,
    AUX_CLK_SDIO          =  CSS_AUX_SDIO         ,
    AUX_CLK_GPIO          =  CSS_AUX_GPIO         ,
    AUX_CLK_32KHZ         =  CSS_AUX_32KHZ        ,
    AUX_CLK_IRDA          =  CSS_AUX_IRDA         ,
    AUX_CLK_CIF0          =  CSS_AUX_CIF0         ,
    AUX_CLK_CIF1          =  CSS_AUX_CIF1         ,
    AUX_CLK_LCD           =  CSS_AUX_LCD          ,
    AUX_CLK_NAL_PAR       =  CSS_AUX_NAL_PAR      ,
    AUX_CLK_NAL_SYN       =  CSS_AUX_NAL_SYN      ,
    AUX_CLK_MIPI_TX0      =  CSS_AUX_MIPI_TX0     ,
    AUX_CLK_MIPI_TX1      =  CSS_AUX_MIPI_TX1     ,
    AUX_CLK_USB_PHY_EXTREFCLK       =  CSS_AUX_USB_PHY_EXTREFCLK      ,
    AUX_CLK_USB_CTRL_SUSPEND_CLK   =  CSS_AUX_USB_CTRL_SUSPEND_CLK  ,
    AUX_CLK_USB_CTRL_REF_CLK      =  CSS_AUX_USB_CTRL_REF_CLK     ,
    AUX_CLK_DDR_REF       =  CSS_AUX_DDR_REF      ,
    AUX_CLK_ROIC          =  CSS_AUX_ROIC         ,
    AUX_CLK_MEDIA         =  CSS_AUX_MEDIA        ,
    AUX_CLK_DDR_CORE      =  CSS_AUX_DDR_CORE     ,
    AUX_CLK_DDR_CORE_CTRL =  CSS_AUX_DDR_CORE_CTRL,
    AUX_CLK_DDR_CORE_PHY  =  CSS_AUX_DDR_CORE_PHY ,
    AUX_CLK_MIPI_ECFG     =  CSS_AUX_MIPI_ECFG    ,
    AUX_CLK_MIPI_CFG      =  CSS_AUX_MIPI_CFG     ,
    AUX_CLK_USB_PHY_REF_ALT_CLK	  =  CSS_AUX_USB_PHY_REF_ALT_CLK,
    SYS_CLK               =  CSS_AUX_USB_PHY_REF_ALT_CLK + 1 ,   // Master Clock post divider
    PLL_CLK               =  CSS_AUX_USB_PHY_REF_ALT_CLK + 2 ,   // TODO: Need to add PLL0, PLL1 clock
	LAST_CLK              =  CSS_AUX_USB_PHY_REF_ALT_CLK + 3 ,   // This is just a marker for the end of the enum!
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


typedef struct 
{
    unsigned int                    refClk0InputKhz;
    unsigned int                    refClk1InputKhz;
    unsigned int                    targetPll0FreqKhz;
    unsigned int                    targetPll1FreqKhz;
    unsigned int                    inputSrcPll1;
    unsigned int                    masterClkDividerValue;
    u64                             cssDssClockEnableMask;
    u32                             mssClockEnableMask;
    u32                             upaClockEnableMask;
    const tyAuxClkDividerCfg       *pAuxClkCfg;            // Null Terminated array of aux clock configs
} tySocClockConfigM2;


// 3: Local const declarations     NB: ONLY const declarations go here
// ----------------------------------------------------------------------------

#endif // DRV_CPR_DEF_H

