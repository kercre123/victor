///
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved.
///            For License Warranty see: common/license.txt
///
/// @brief     Pings Each Module
///
///  Summary
///        Pings Each Module and reports its health
///
///

// 1: Includes
// ----------------------------------------------------------------------------
#include <Myriad2ShaveModulePingApi.h>
#include "../../../common/drivers/myriad2/socDrivers/include/registersMyriad2.h"


// 2:  Source Specific #defines and types  (typedef,enum,struct)
// ----------------------------------------------------------------------------

//#define SIMPLE_QUICK_SANITY_CHECK
#define USE_BOTH_LSU0_LSU1_FOR_PING

#define QUIET               (0)
#define VERBOSE             (1)

#define VERBOSE_FLAG        (QUIET)

#define SET_REG_WORD(a,x)   ((void)(*(volatile u32*)(((unsigned)(a))) = (u32)(x)))
#define GET_REG_WORD_VAL(a) (*(volatile u32*)(((unsigned)(a))))

// Temp defines - registers not up to date....
//
#define SHAVE_DCU_OFFSET             (0x00001000)
#define IRF_ADR_OFFSET               (SHAVE_DCU_OFFSET + 0x180)
#define VRF_ADR_OFFSET               (SHAVE_DCU_OFFSET + 0x300)
#define SHAVE_INCR                   (0x00010000)
#define SHV0_DCU_SVU_IRF_ADDR        (SHAVE_0_BASE_ADR + IRF_ADR_OFFSET)
#define SHV1_DCU_SVU_IRF_ADDR        (SHV0_DCU_SVU_IRF_ADDR + SHAVE_INCR)
#define SHV2_DCU_SVU_IRF_ADDR        (SHV1_DCU_SVU_IRF_ADDR + SHAVE_INCR)
#define SHV3_DCU_SVU_IRF_ADDR        (SHV2_DCU_SVU_IRF_ADDR + SHAVE_INCR)
#define SHV4_DCU_SVU_IRF_ADDR        (SHV3_DCU_SVU_IRF_ADDR + SHAVE_INCR)
#define SHV5_DCU_SVU_IRF_ADDR        (SHV4_DCU_SVU_IRF_ADDR + SHAVE_INCR)
#define SHV6_DCU_SVU_IRF_ADDR        (SHV5_DCU_SVU_IRF_ADDR + SHAVE_INCR)
#define SHV7_DCU_SVU_IRF_ADDR        (SHV6_DCU_SVU_IRF_ADDR + SHAVE_INCR)
#define SHV8_DCU_SVU_IRF_ADDR        (SHV7_DCU_SVU_IRF_ADDR + SHAVE_INCR)
#define SHV9_DCU_SVU_IRF_ADDR        (SHV8_DCU_SVU_IRF_ADDR + SHAVE_INCR)
#define SHV10_DCU_SVU_IRF_ADDR       (SHV9_DCU_SVU_IRF_ADDR + SHAVE_INCR)
#define SHV11_DCU_SVU_IRF_ADDR       (SHV10_DCU_SVU_IRF_ADDR + SHAVE_INCR)

#define SHV0_DCU_SVU_VRF_ADDR        (SHAVE_0_BASE_ADR + VRF_ADR_OFFSET)
#define SHV1_DCU_SVU_VRF_ADDR        (SHV0_DCU_SVU_VRF_ADDR + SHAVE_INCR)
#define SHV2_DCU_SVU_VRF_ADDR        (SHV1_DCU_SVU_VRF_ADDR + SHAVE_INCR)
#define SHV3_DCU_SVU_VRF_ADDR        (SHV2_DCU_SVU_VRF_ADDR + SHAVE_INCR)
#define SHV4_DCU_SVU_VRF_ADDR        (SHV3_DCU_SVU_VRF_ADDR + SHAVE_INCR)
#define SHV5_DCU_SVU_VRF_ADDR        (SHV4_DCU_SVU_VRF_ADDR + SHAVE_INCR)
#define SHV6_DCU_SVU_VRF_ADDR        (SHV5_DCU_SVU_VRF_ADDR + SHAVE_INCR)
#define SHV7_DCU_SVU_VRF_ADDR        (SHV6_DCU_SVU_VRF_ADDR + SHAVE_INCR)
#define SHV8_DCU_SVU_VRF_ADDR        (SHV7_DCU_SVU_VRF_ADDR + SHAVE_INCR)
#define SHV9_DCU_SVU_VRF_ADDR        (SHV8_DCU_SVU_VRF_ADDR + SHAVE_INCR)
#define SHV10_DCU_SVU_VRF_ADDR       (SHV9_DCU_SVU_VRF_ADDR + SHAVE_INCR)
#define SHV11_DCU_SVU_VRF_ADDR       (SHV10_DCU_SVU_VRF_ADDR + SHAVE_INCR)


#ifdef USE_BOTH_LSU0_LSU1_FOR_PING

#   define PING_REGISTER(REG_NAME,WR_VALUE,EXPECTED) do {     \
              u32 readWord;                                   \
              shaveWrite32Lsu0(REG_NAME, WR_VALUE);           \
              readWord = shaveRead32Lsu0(REG_NAME);           \
              numErrors += (readWord == EXPECTED ? 0:1);      \
              shaveWrite32Lsu0(REG_NAME, 0x0);                \
              shaveWrite32Lsu1(REG_NAME, WR_VALUE);           \
              readWord = shaveRead32Lsu1(REG_NAME);           \
              numErrors += (readWord == EXPECTED ? 0:1);      \
              }while(0)
#else
#   define PING_REGISTER(REG_NAME,WR_VALUE,EXPECTED) do {     \
              u32 readWord;                                   \
              SET_REG_WORD(REG_NAME, WR_VALUE);               \
              readWord = GET_REG_WORD_VAL(REG_NAME);          \
              numErrors += (readWord == EXPECTED ? 0:1);      \
              }while(0)
#endif              

extern int pingRegister64(u32 regAddress,u32 writeHi, u32 writeLo,u32 expectedHi, u32 expectedLo);
              

// 3: Global Data (Only if absolutely necessary)
// ----------------------------------------------------------------------------

// 4: Static Local Data
// ----------------------------------------------------------------------------
u32 numErrors=0;

// 5: Static Function Prototypes
// ----------------------------------------------------------------------------

// 6: Functions Implementation
// ----------------------------------------------------------------------------
//

//  32 Bit functions
//
void shaveWrite32Lsu0(u32 address, u32 value)
{
    asm ( "LSU0.ST.32 %0, %1\n" : : "r"(value), "r"(address));
    return;
}

void shaveWrite32Lsu1(u32 address, u32 value)
{
    asm ( "LSU1.ST.32 %0, %1\n" : : "r"(value), "r"(address));
    return;
}

u32 shaveRead32Lsu0(u32 address)
{
    u32 retVal;
    asm ( "LSU0.LD.32 %0 , %1\n" : "=r"(retVal) : "r"(address));
    return retVal;
}

u32 shaveRead32Lsu1(u32 address)
{
    u32 retVal;
    asm ( "LSU1.LD.32 %0 , %1\n" : "=r"(retVal) : "r"(address));
    return retVal;
}

int Myriad2ShaveModulePing(void)
{
    numErrors = 0; 

#ifdef SIMPLE_QUICK_SANITY_CHECK
    // Used for running quick sanity check
    pingRegister64(SHV1_DCU_SVU_VRF_ADDR , 0x01234567,0x01234567);
    PING_REGISTER(CMX_MTX_LOS_INT_EN_ADR,0x11223344,0x11223344);
#else
    pingRegister64(SHV0_DCU_SVU_IRF_ADDR  , 0xcafebabe,0xf001ba11, 0xcafebabe,0xf001ba11);
    pingRegister64(SHV1_DCU_SVU_VRF_ADDR  , 0xcafebabe,0xf001ba11, 0xcafebabe,0xf001ba11);
    pingRegister64(SHV2_DCU_SVU_VRF_ADDR  , 0xcafebabe,0xf001ba11, 0xcafebabe,0xf001ba11);
    pingRegister64(SHV3_DCU_SVU_VRF_ADDR  , 0xcafebabe,0xf001ba11, 0xcafebabe,0xf001ba11);
    pingRegister64(SHV4_DCU_SVU_VRF_ADDR  , 0xcafebabe,0xf001ba11, 0xcafebabe,0xf001ba11);
    pingRegister64(SHV5_DCU_SVU_VRF_ADDR  , 0xcafebabe,0xf001ba11, 0xcafebabe,0xf001ba11);
    pingRegister64(SHV6_DCU_SVU_VRF_ADDR  , 0xcafebabe,0xf001ba11, 0xcafebabe,0xf001ba11);
    pingRegister64(SHV7_DCU_SVU_VRF_ADDR  , 0xcafebabe,0xf001ba11, 0xcafebabe,0xf001ba11);
    pingRegister64(SHV8_DCU_SVU_VRF_ADDR  , 0xcafebabe,0xf001ba11, 0xcafebabe,0xf001ba11);
    pingRegister64(SHV9_DCU_SVU_VRF_ADDR  , 0xcafebabe,0xf001ba11, 0xcafebabe,0xf001ba11);
    pingRegister64(SHV10_DCU_SVU_VRF_ADDR , 0xcafebabe,0xf001ba11, 0xcafebabe,0xf001ba11);
    pingRegister64(SHV11_DCU_SVU_VRF_ADDR , 0xcafebabe,0xf001ba11, 0xcafebabe,0xf001ba11);
    PING_REGISTER(CMX_MTX_LOS_INT_EN_ADR,0x11223344,0x11223344);
    PING_REGISTER(ROIC_DELAYCFG_ADDR, 0x00000401,0x00000401);
    PING_REGISTER(AHB_DMA_BASE_ADR, 0xab1def01,0xab1def01);
    PING_REGISTER(BIC_CACHE2_WDATA_ADR, 0xab1d3f01,0xab1d3f01);
    pingRegister64(CDMA_DST_SRC_ADR, 0x71234560,0x87654320,0x71234560,0x87654320);  // 64 bit register
    PING_REGISTER(CIF2_DMA0_START_ADR_ADR       , 0xabcd4c67,0xabcd4c67);
    PING_REGISTER(L2C_LEON_OS_MTRR0       , 0xabcd45d7,0xabcd45d7);       
    PING_REGISTER(L2C_LEON_RT_MTRR0       , 0xabcd75d7,0xabcd75d7);       
    PING_REGISTER(MIPI0_HS_CTRL_ADDR, 0xffffffff,0xffff0777);
    PING_REGISTER(MIPI1_HS_CTRL_ADDR, 0xfffffffa,0xffff0772);
    PING_REGISTER(MIPI2_HS_CTRL_ADDR, 0xfffffffb,0xffff0773);
    PING_REGISTER(MIPI3_HS_CTRL_ADDR, 0xfffffffc,0xffff0774);
    PING_REGISTER(MIPI4_HS_CTRL_ADDR, 0xfffffffd,0xffff0775);
    PING_REGISTER(MIPI5_HS_CTRL_ADDR, 0xfffffffe,0xffff0776);
    PING_REGISTER(SPI1_CTRLR0_ADR , 0x00001234,0x00001204); // bits 4,5 expected to be read-only
    PING_REGISTER(SPI2_CTRLR0_ADR , 0x00001235,0x00001205); // bits 4,5 expected to be read-only 
    PING_REGISTER(SPI3_CTRLR0_ADR , 0x00001236,0x00001206); // bits 4,5 expected to be read-only 
    PING_REGISTER(SDIOH1_DBADDR_S1, 0xe123b5a7,0xe123b5a7);
    PING_REGISTER(USB_PHY_CFG0_ADR, 0xe1234567,0xe1234567);
    PING_REGISTER(L2C_DBG_TADDR_ADR, 0xe1234567,0x00000567);
    PING_REGISTER(SIPP_IBUF0_BASE_ADR           , 0x0123a567,0x0123a560);  // 64 bit alligned...
    PING_REGISTER(SIPP_IBUF18_BASE_ADR          , 0x0183a567,0x0183a560);  // 64 bit alligned...
    PING_REGISTER(SIPP_IBUF0_BASE_SHADOW_ADR    , 0x0123a567,0x0123a560);  // 64 bit alligned...
    PING_REGISTER(SIPP_IBUF18_BASE_SHADOW_ADR   , 0x0183a567,0x0183a560);  // 64 bit alligned...
    PING_REGISTER(SIPP_OBUF0_BASE_ADR           , 0x0123a567,0x0123a560);  // 64 bit alligned...
    PING_REGISTER(SIPP_OBUF15_BASE_ADR          , 0x0183a567,0x0183a560);  // 64 bit alligned...
    PING_REGISTER(SIPP_OBUF0_BASE_SHADOW_ADR    , 0x0123a567,0x0123a560);  // 64 bit alligned...
    PING_REGISTER(SIPP_OBUF15_BASE_SHADOW_ADR   , 0x0183a567,0x0183a560);  // 64 bit alligned...
    PING_REGISTER(SIPP_DBYR_FRM_DIM_ADR         , 0x5ca1ab1e,0x5ca1ab1e);  // 64 bit alligned...
    PING_REGISTER(SIPP_DBYR_FRM_DIM_SHADOW_ADR  , 0x5ca1ab1e,0x5ca1ab1e);  // 64 bit alligned...
    PING_REGISTER(SIPP_LUMA_FRM_DIM_ADR         , 0x5ca1ab1e,0x5ca1ab1e);  // 64 bit alligned...
    PING_REGISTER(SIPP_LUMA_FRM_DIM_SHADOW_ADR  , 0x5ca1ab1e,0x5ca1ab1e);  // 64 bit alligned...
    PING_REGISTER(SIPP_CONV_FRM_DIM_ADR         , 0x5ca1ab1e,0x5ca1ab1e);  // 64 bit alligned...
    PING_REGISTER(SIPP_CONV_FRM_DIM_SHADOW_ADR  , 0x5ca1ab1e,0x5ca1ab1e);  // 64 bit alligned...
    PING_REGISTER(GETH_BASE_ADR                 , 0x08000000,0x08000000);  // Reset Value
    PING_REGISTER(DDRC_MRCTRL0_ADR              , 0x00003020,0x00003020);  // Reset Value
    PING_REGISTER(LCD1_LAYER0_COL_START_ADR     , 0x01b4356b,0x0000356b);  // 14 bits....
    PING_REGISTER(NAL_ENC_DMA_TX_STARTADR_ADR   , 0x0123456c,0x0123456c);
    PING_REGISTER(CPR_AUX0_CLK_CTRL_ADR         , 0x01234567,0x01234567);
    PING_REGISTER(ICB0_SVE7_INT_CFG_ADR         , 0xabcdefba,0xabcdefba);
    PING_REGISTER(TIM0_0_RELOAD_VAL_ADR         , 0x1234abcd,0x1234abcd); 
    PING_REGISTER(GPIO_PWM_HIGHLOW0_ADR         , 0x567890ef,0x567890ef);
    PING_REGISTER(IIC1_SS_HCNT                  , 0x1234abcd,0x0000abcd);     // lower 16 bits only
    PING_REGISTER(IIC2_SS_HCNT                  , 0x567890ef,0x000090ef);     // lower 16 bits only
    PING_REGISTER(IIC3_SS_HCNT                  , 0x1234dcba,0x0000dcba);     // lower 16 bits only
    PING_REGISTER(UART_SCR_ADR                  , 0x567890ef,0x000000ef);     // 8 bit only
    PING_REGISTER(CIF1_DMA0_START_ADR_ADR       , 0xabcd4567,0xabcd4567);
    PING_REGISTER(I2S1_IMR0                     , 0xffffffff,0x00000033);     // 8 bit only
    PING_REGISTER(I2S2_IMR0                     , 0xffffff11,0x00000011);     // 8 bit only
    PING_REGISTER(I2S3_IMR0                     , 0xffffff22,0x00000022);     // 8 bit only
    PING_REGISTER(DSU_LEON_OS_CTRL_ADR          , 0, 0);
    PING_REGISTER(DSU_LEON_OS_CTRL_ADR          , 0x23, 0x23);
    PING_REGISTER(DSU_LEON_RT_CTRL_ADR          , 0, 0x800); // (0x800 is the powerdown bit)
    PING_REGISTER(DSU_LEON_RT_CTRL_ADR          , 0x23, 0x823);
    //
#endif
    return numErrors;
}
