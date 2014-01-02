#include <sipp.h>

#ifdef SIPP_TEST_APP

//##########################################################################
//Myriad_1 setup
#if defined(__sparc) && !defined(MYRIAD2)

#include "DrvCpr.h"
#include "DrvDdr.h"
#include "DrvSvu.h"
#include "DrvSvuDebug.h"
#include "isaac_registers.h"
#include "swcShaveLoader.h"

#define L2_MODE    (0x00000006)  // l2 direct mode
#define CMX_CONFIG (0x55555556)

//Timer profiling 
#define CLR_TIMER  SET_REG_WORD(TIM_FREE_CNT_ADR, 0)
#define GET_TIMER  GET_REG_WORD_VAL(TIM_FREE_CNT_ADR)
 
unsigned __l2_config __attribute__((section(".l2.mode")))   = L2_MODE;
unsigned __cmx_config __attribute__((section(".cmx.ctrl"))) = CMX_CONFIG;

tyAuxClkDividerCfg auxClkCfg180[] =
{
    {// TODO: Only the necessary clocks should be enabled by default
     .auxClockEnableMask     =(AUX_CLK_MASK_I2S0    |
                               AUX_CLK_MASK_I2S1    |
                               AUX_CLK_MASK_I2S2    |
                               AUX_CLK_MASK_USB     |
                               AUX_CLK_MASK_DDR     |
                               AUX_CLK_MASK_SDHOST_1|
                               AUX_CLK_MASK_SDHOST_2|
                               AUX_CLK_MASK_CIF1    |
                               AUX_CLK_MASK_CIF2    |
                               AUX_CLK_MASK_LCD1    |
                               AUX_CLK_MASK_LCD1X2  |
                               AUX_CLK_MASK_LCD2    |
                               AUX_CLK_MASK_LCD2X2  |
                               AUX_CLK_MASK_MEBI_P  |
                               AUX_CLK_MASK_MEBI_N  |
                               AUX_CLK_MASK_IO      |
                               AUX_CLK_MASK_32KHZ   |
                               AUX_CLK_MASK_TSI     |
                               AUX_CLK_MASK_NAL_P   |
                               AUX_CLK_MASK_NAL_S   ),
     .auxClockDividerValue   = GEN_CLK_DIVIDER(1,1),  // Run DDR at system frequency also (Bypass divider)
    },
    
    {
     .auxClockEnableMask     = AUX_CLK_MASK_SAHB,     // Clock Slow AHB Bus by default as needed for SDHOST,SDIO,USB,NAND
     .auxClockDividerValue   = GEN_CLK_DIVIDER(1,2),  // Slow AHB must run @ less than 100Mhz so we divide by 2 to give 90
    },
    
    {0,0}, // Null Terminated List
};


tySocClockConfig appClockConfig =
{
    .targetOscInputClock        =  12000,   // Default to 12Mhz input Osc
    .targetPllFreqKhz           = 180000,
    .systemClockConfig          =
        {  // TODO: Only the necessary clocks should be enabled by default 
        .sysClockEnableMask     = ALL_SYSTEM_CLOCKS,
        .sysClockDividerValue   = GEN_CLK_DIVIDER(1,1),
        },
    .pAuxClkCfg                 = auxClkCfg180,
};

//========================================================================
void initClocksAndMemory(void)
{
  //Initialise the Clock Power reset module
    DrvCprInit(NULL,NULL);
    DrvCprSetupClocks(&appClockConfig);
  //Configure all CMX RAM layouts
    SET_REG_WORD(LHB_CMX_RAMLAYOUT_CFG, CMX_CONFIG);
    DrvDdrInitialise(DrvCprGetClockFreqKhz(AUX_CLK_DDR,NULL));
    swcLeonDisableDcache();
}



//##########################################################################
#elif defined(__sparc) && defined(MYRIAD2)
#include <registersMyriad2.h>
#include <UnitTestApi.h> 
#include <VcsHooksApi.h> 
#include <swcLeonUtils.h>
#include <DrvLeonL2C.h>

//##########################################################################
#elif defined(SIPP_PC)
  #include <stdint.h>
  UInt8 *mbinImgSipp=0; //dummy (unused on PC)
#endif

//##########################################################################
// Shave Window Registers (also needed on PC sim to estimate SIPP_POOL size
UInt32 svuWinRegs[]=
{
   /*slice_0*/ MXI_CMX_BASE_ADR + 0x007000, MXI_CMX_BASE_ADR +0x000000, MXI_CMX_BASE_ADR +0x008000, MXI_CMX_BASE_ADR +0x008C00,
   /*slice_1*/ MXI_CMX_BASE_ADR + 0x027000, MXI_CMX_BASE_ADR +0x020000, MXI_CMX_BASE_ADR +0x028000, MXI_CMX_BASE_ADR +0x028C00,
   /*slice_2*/ MXI_CMX_BASE_ADR + 0x047000, MXI_CMX_BASE_ADR +0x040000, MXI_CMX_BASE_ADR +0x048000, MXI_CMX_BASE_ADR +0x048C00,
   /*slice_3*/ MXI_CMX_BASE_ADR + 0x067000, MXI_CMX_BASE_ADR +0x060000, MXI_CMX_BASE_ADR +0x068000, MXI_CMX_BASE_ADR +0x068C00,
   /*slice_4*/ MXI_CMX_BASE_ADR + 0x087000, MXI_CMX_BASE_ADR +0x080000, MXI_CMX_BASE_ADR +0x088000, MXI_CMX_BASE_ADR +0x088C00,
   /*slice_5*/ MXI_CMX_BASE_ADR + 0x0A7000, MXI_CMX_BASE_ADR +0x0A0000, MXI_CMX_BASE_ADR +0x0A8000, MXI_CMX_BASE_ADR +0x0A8C00,
   /*slice_6*/ MXI_CMX_BASE_ADR + 0x0C7000, MXI_CMX_BASE_ADR +0x0C0000, MXI_CMX_BASE_ADR +0x0C8000, MXI_CMX_BASE_ADR +0x0C8C00,
   /*slice_7*/ MXI_CMX_BASE_ADR + 0x0E7000, MXI_CMX_BASE_ADR +0x0E0000, MXI_CMX_BASE_ADR +0x0E8000, MXI_CMX_BASE_ADR +0x0E8C00,
   
   #if defined(MYRIAD2)
   /*slice_8*/ MXI_CMX_BASE_ADR + 0x107000, MXI_CMX_BASE_ADR +0x100000, MXI_CMX_BASE_ADR +0x108000, MXI_CMX_BASE_ADR +0x108C00,
   /*slice_9*/ MXI_CMX_BASE_ADR + 0x127000, MXI_CMX_BASE_ADR +0x120000, MXI_CMX_BASE_ADR +0x128000, MXI_CMX_BASE_ADR +0x128C00,
   /*slice10*/ MXI_CMX_BASE_ADR + 0x147000, MXI_CMX_BASE_ADR +0x140000, MXI_CMX_BASE_ADR +0x148000, MXI_CMX_BASE_ADR +0x148C00,
   /*slice11*/ MXI_CMX_BASE_ADR + 0x167000, MXI_CMX_BASE_ADR +0x160000, MXI_CMX_BASE_ADR +0x168000, MXI_CMX_BASE_ADR +0x168C00,
   #endif
};
#endif	// SIPP_TEST_APP

//##########################################################################
// Myriad initialization function
// Undocumented - for internal test cases only!
void sippPlatformInit()
{
#ifdef SIPP_TEST_APP

    #if defined(__sparc) && !defined(MYRIAD2) //Sabre setup
       initClocksAndMemory();
       dcuInit(); 
       NOP;NOP;NOP;NOP;NOP;NOP;NOP;NOP;NOP;NOP;
       NOP;NOP;NOP;NOP;NOP;NOP;NOP;NOP;NOP;NOP;
       
       
    #elif defined(__sparc) && defined(MYRIAD2) //Myriad2
     //This is needed for the simulator at the moment!
       SET_REG_WORD(CPR_CLK_EN0_ADR,       0xFFFFFFFF);
       SET_REG_WORD(CPR_CLK_EN1_ADR,       0xFFFFFFFF);
       
     //(RR)Enable media subsystem gated clocks and resets
       SET_REG_WORD(MSS_CLK_CTRL_ADR,      0x001fffff);
       SET_REG_WORD(MSS_RSTN_CTRL_ADR,     0x001fffff);
       SET_REG_WORD(MSS_SIPP_CLK_CTRL_ADR, 0x001fffff);
       
     //(BB)Enable PMB subsystem gated clocks and resets
       SET_REG_WORD(CMX_RSTN_CTRL,         0x00000000); //engage reset
       SET_REG_WORD(CMX_CLK_CTRL,          0x0000ffff); //turn on clocks
       SET_REG_WORD(CMX_RSTN_CTRL,         0x0000ffff); //dis-engage reset
     //(BB)Set bit per Slice, turns on balanced arbitration between AMC ports 
       SET_REG_WORD(CMX_SLICE_ARB_CTRL,    0x0000ffff); 

     //Reset SIPP units:
       SET_REG_WORD(SIPP_SOFTRST_ADR,     0xFFFFFFFF);
       NOP;NOP;NOP;NOP;NOP;NOP;NOP;NOP;NOP;NOP;
       NOP;NOP;NOP;NOP;NOP;NOP;NOP;NOP;NOP;NOP;
     //Disable all SIPP units:
       SET_REG_WORD(SIPP_CONTROL_ADR,     0x00000000);
       
     //Critical to avoid huge (thousands of c.c.) Leon STALL:  
      {
       #if !defined(CMX_SLICE_ISI_PRIORITY0_CTRL)
       //These defines are not yet in registersMyriad2.h, so copy-paste from fragrak_constants.inc
        #define CMX_SLICE_ISI_PRIORITY0_CTRL  (CMX_CTRL_BASE_ADR + 0x000e0)
        #define CMX_SLICE_ISI_PRIORITY1_CTRL  (CMX_CTRL_BASE_ADR + 0x000e4)
        #define CMX_SLICE_ISI_PRIORITY2_CTRL  (CMX_CTRL_BASE_ADR + 0x000e8)
        #define CMX_SLICE_ISI_PRIORITY3_CTRL  (CMX_CTRL_BASE_ADR + 0x000ec)
        #define CMX_SLICE_ISI_PRIORITY4_CTRL  (CMX_CTRL_BASE_ADR + 0x000f0)
        #define CMX_SLICE_ISI_PRIORITY5_CTRL  (CMX_CTRL_BASE_ADR + 0x000f4)
        #define CMX_SLICE_ISI_PRIORITY6_CTRL  (CMX_CTRL_BASE_ADR + 0x000f8)
        #define CMX_SLICE_ISI_PRIORITY7_CTRL  (CMX_CTRL_BASE_ADR + 0x000fc)
      #endif
      
      //each 16bit field shows priority from other 15 slices (1 bit is don't care there...)
        SET_REG_WORD(CMX_SLICE_ISI_PRIORITY0_CTRL, 0x80008000); //hi=slice_1, lo=slice_0
        SET_REG_WORD(CMX_SLICE_ISI_PRIORITY1_CTRL, 0x80008000); //hi=slice_3, lo=slice_2
        SET_REG_WORD(CMX_SLICE_ISI_PRIORITY2_CTRL, 0x80008000);
        SET_REG_WORD(CMX_SLICE_ISI_PRIORITY3_CTRL, 0x80008000);
        SET_REG_WORD(CMX_SLICE_ISI_PRIORITY4_CTRL, 0x80008000);
        SET_REG_WORD(CMX_SLICE_ISI_PRIORITY5_CTRL, 0x80008000);
        SET_REG_WORD(CMX_SLICE_ISI_PRIORITY6_CTRL, 0x80008000);
        SET_REG_WORD(CMX_SLICE_ISI_PRIORITY7_CTRL, 0x80008000);
      }
       
       unitTestInit();
    #endif
    
#endif	// SIPP_TEST_APP
}

