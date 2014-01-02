///   
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved. 
///            For License Warranty see: common/license.txt   
///
/// @brief     TODO_ADD_ONE_LINE_TEST_DESCRIPTION_HERE
/// 
/// 
/// 

// 1: Includes
// ----------------------------------------------------------------------------
#include <mv_types.h>
#include <isaac_registers.h>
#include <stdio.h>
#include <DrvCpr.h>
#include <assert.h>
#include <swcLeonUtils.h>
#include <DrvDdr.h>
#include <DrvTimer.h>

// 2:  Source Specific #defines and types  (typedef,enum,struct)
// ----------------------------------------------------------------------------

// If DDR is being used the following minimum set of clocks is advised                                                       
#define DDRC_AND_BUS_CLOCKS         (  DEV_MXI_AXI      |     \
                                       DEV_SXI_AXI      |     \
                                       DEV_AXI2AXI      |     \
                                       DEV_AXI2AHB      |     \
                                       DEV_AXI_MON      |     \
                                       DEV_L2           |     \
                                       DEV_DDRC               \
                                       )
// Reset State
#define DDRC_ASSERT_SOFTRESET        (0)
#define DDRC_DEASSERT_SOFTRESET      (1)

// config of the DDRC_DATA_BUS_WIDTH register
#define DDRC_DATA_BUS_WIDTH_32       (0)
#define DDRC_DATA_BUS_WIDTH_16       (1)

// States of the DDRC_OPERATING_MODE status register
#define DDRC_MODE_UNINITIALIZED      (0)
#define DDRC_MODE_NORMAL             (1)
#define DDRC_MODE_POWERDOWN          (2)
#define DDRC_MODE_SELFREFRESH        (3)

// Note: Reset values for the READ_SLAVE_RATIOS are known good
#define WRITE_SLAVE_RATIO0_VAL    (0) // Known good value, qualified by test
#define WRITE_SLAVE_RATIO1_VAL    (0) // Known good value, qualified by test 

//#define REG_DEBUG
//#define ENABLE_DDR_DEBUG_PRINTS

#ifdef ENABLE_DDR_DEBUG_PRINTS
#	define DPRINTF(var_args)    printf var_args
#else
#	define DPRINTF(var_args)
#endif

typedef enum
{
    DRAM_SIZE_16MB,
    DRAM_SIZE_64MB
} tyDramSize;
                       
// 3: Global Data (Only if absolutely necessary)
// ----------------------------------------------------------------------------

// 4: Static Local Data 
// ----------------------------------------------------------------------------

// 5: Static Function Prototypes
// ----------------------------------------------------------------------------
static inline void setupDdrIOPads (u32 ddrIOConfig);
static inline void tuneDdrPerformance(void);
static void basicDdrInit(tyDramSize dramSize);
static inline void configureDdrTimings(u32 freq);


// 6: Functions Implementation
// ----------------------------------------------------------------------------

// This is the single function you need to call to enable the DDR for a given frequency
void DrvDdrInitialise(u32 ddrFreqKhz)
{
    u32 ddrSavedWord;
    u32 ddrReadWord;

  // The first step in configuring the DRAM controller is to check if 
  // the dram has already been configured.
  // In most use cases by the time this function is called in user code 
  // it will already have been necessary for DRAM to be initialised. 
  // e.g. For any application which stores data or code in DRAM
  // by the time code execution starts the code/data must be loaded in DDR
  // by either a debugger or the Boot ROM Image.
  // As such it is normally the responsibility of the debugger or ROM Image
  // to perform the basic DRAM initialisation
  // Therefore the purpose of this function is twofold.
  // 1) For the exceptional cases where DRAM has not been pre-initialised
  //    -- e.g. Application with no DDR section; or VCS simulation in which no debugger is involved.
  //    -- In these cases this function detects that DDRC is in its reset state and performs basic init
  // 2) We cannot expect the debugger or boot image to know the target operation frequency of the app
  //    -- As such it this functions task to tuning the DDR parameters to be optimal for the 
  //       the specific system frequency of the application.

  // Detect if the DDRC is in power down mode

  if (GET_REG_WORD_VAL(DDRC_REG_DDRC_POWERDOWN_EN_ADR) != 0) // Register resets to 1
  {
      // DRAM controller is not powered up, it is necessary to perform basic ddr init
      DPRINTF(("\nDDR was not initialised.. initialising now\n"));
      // First assume the DRAM is 64MB and then test to see if this is true
      basicDdrInit(DRAM_SIZE_64MB);
      // Backup contents of first memory location
      ddrSavedWord = GET_REG_WORD_VAL(0x48000000);
      // Set magic value into DDR
      SET_REG_WORD(0x48000000,0xBA5EBA11);
      // Next we rely on a property that if a 16MB DDR is incorrectly 
      // configured for 64MB it will start to alias at address bit 10 (1K)
      ddrReadWord =  GET_REG_WORD_VAL(0x48000000); 
      if (ddrReadWord != 0xBA5EBA11)
      {
          DPRINTF(("\ndrvDdr: Error accessing DRAM\n"));
          assert(FALSE);
      }
      else
      {
          ddrReadWord =  GET_REG_WORD_VAL(0x48000400); 
          if (ddrReadWord == 0xBA5EBA11)
          {
              // This must actually be a 16MB DDR, reconfigure as such
              // But first restore the word we saved
              SET_REG_WORD(0x48000000,ddrSavedWord);
              basicDdrInit(DRAM_SIZE_16MB);
              DPRINTF(("\n16MB DDR Detected and configured\n"));
              // If the user is expecting this to be a 64MB chip 
              // We should flag an error as otherwise the system will
              // work in the debugger but not when booted from ROM
#if DRAM_SIZE_MB == 64 
              assert(FALSE);
#endif
          }
          else
          {
              // Correctly configured as 64MB DDR, just restore the word
              SET_REG_WORD(0x48000000,ddrSavedWord);
              DPRINTF(("\n64MB DDR Detected and configured\n"));
          }
      }
  }

  DrvDdrNotifyClockChange(ddrFreqKhz);

  tuneDdrPerformance();

  //Temporary workaround for DDRC timings issue described in Bug 17687 http://dub30/bugzilla/show_bug.cgi?id=17687
  SleepMicro(40);

  return;
}

void DrvDdrNotifyClockChange(u32 newDdrClockKhz)
{
    configureDdrTimings(newDdrClockKhz);
    return;
}

static void basicDdrInit(tyDramSize dramSize)
{
    u32 config;
    u32 index;
    u32 ddrcOperatingMode;

    // Ensure that we have at least the minimum set of clocks enabled to 
    // talk to the DDR controller
    DrvCprSysDeviceAction(ENABLE_CLKS,DDRC_AND_BUS_CLOCKS);

    // Place DDRC in soft reset, as most registers should not be modified in "normal" operating mode
    SET_REG_WORD(DDRC_SOFT_RSTN_ADR , DDRC_ASSERT_SOFTRESET); 

    config =  DRAM_PADS_SR_SLOW 
             |DRAM_IO_PULL_NORM  
             |DRAM_IO_DRV_2MA   
             |DRAM_IO_VOLT_18   
             |DRAM_IO_SW_25     
             |DRAM_IO_DQ_REN       
             |DRAM_IO_DQS_REN      
             |DRAM_IO_FIFO_WE_IN_REN  
             |DRAM_IO_FIFO_WE_IN_OEN_DIS  ;	

    // For reference: The following bits are not set in config=>
    //  |DRAM_IO_DQ0_SMT_ON  |DRAM_IO_DQ1_SMT_ON  |DRAM_IO_DQ2_SMT_ON  |DRAM_IO_DQ3_SMT_ON
    //	|DRAM_IO_DQS0_SMT_ON |DRAM_IO_DQS1_SMT_ON |DRAM_IO_DQS2_SMT_ON |DRAM_IO_DQS3_SMT_ON  
    //	|DRAM_IO_OUT_PADS_REN  |DRAM_IO_OUT_PADS_OEN_DIS | DRAM_IO_OUT_PADS_SMT_ON   
    //	|DRAM_IO_FIFO_WE_IN_SMT_ON 

    setupDdrIOPads(config);

    if (dramSize == DRAM_SIZE_16MB)
    {
        SET_REG_WORD(DDRC_REG_DDRC_ADDRMAP_BANK_B0_ADR  , 0x08);
        SET_REG_WORD(DDRC_REG_DDRC_ADDRMAP_BANK_B1_ADR  , 0x08);
        SET_REG_WORD(DDRC_REG_DDRC_ADDRMAP_COL_B8_ADR   , 0x0F);
        SET_REG_WORD(DDRC_REG_DDRC_ADDRMAP_ROW_B0_ADR   , 0x00);
        SET_REG_WORD(DDRC_REG_DDRC_ADDRMAP_ROW_B1_ADR   , 0x00);
        SET_REG_WORD(DDRC_REG_DDRC_ADDRMAP_ROW_B2_11_ADR, 0x00);
        SET_REG_WORD(DDRC_REG_DDRC_ADDRMAP_ROW_B12_ADR  , 0x00);
    } 
    else // In all other cases we assume 64MB as these are the only two types we support
    {
        SET_REG_WORD(DDRC_REG_DDRC_ADDRMAP_BANK_B0_ADR  , 0x0A);
        SET_REG_WORD(DDRC_REG_DDRC_ADDRMAP_BANK_B1_ADR  , 0x0A);
        SET_REG_WORD(DDRC_REG_DDRC_ADDRMAP_COL_B8_ADR   , 0x00);
        SET_REG_WORD(DDRC_REG_DDRC_ADDRMAP_ROW_B0_ADR   , 0x01);
        SET_REG_WORD(DDRC_REG_DDRC_ADDRMAP_ROW_B1_ADR   , 0x01);
        SET_REG_WORD(DDRC_REG_DDRC_ADDRMAP_ROW_B2_11_ADR, 0x01);
        SET_REG_WORD(DDRC_REG_DDRC_ADDRMAP_ROW_B12_ADR  , 0x01);
    }

    SET_REG_WORD(DDRC_REG_PHY_WR_SLAVE_RATIO0_ADR, WRITE_SLAVE_RATIO0_VAL );
    SET_REG_WORD(DDRC_REG_PHY_WR_SLAVE_RATIO1_ADR, WRITE_SLAVE_RATIO1_VAL);

    for (index=0; index<10; index++)
        {NOP;}

    // Take DDRC out of reset and power down mode.
    SET_REG_WORD(DDRC_REG_DDRC_DATA_BUS_WIDTH_ADR, 0x00000000);  // set DDR bus width to 32 bits
    SET_REG_WORD(DDRC_SOFT_RSTN_ADR              , DDRC_DEASSERT_SOFTRESET);  
    SET_REG_WORD(DDRC_REG_DDRC_POWERDOWN_EN_ADR  , 0x00000000);

    // Poll DRAM state until we get out of Uninit state
    // TODO: Consider timeout here
    do
    {
        ddrcOperatingMode = GET_REG_WORD_VAL(DDRC_DDRC_REG_OPERATING_MODE_ADR);
    } while ( ddrcOperatingMode == DDRC_MODE_UNINITIALIZED );

    return;
}


static inline void setupDdrIOPads (u32 ddrIOConfig)
{
	u32 cprRetentionReg6 = GET_REG_WORD_VAL(CPR_RETENTION6_ADR);

	cprRetentionReg6 &= ~DRAM_IO_MASK;

	cprRetentionReg6 |= (ddrIOConfig << DRAM_IO_OFFSET);

	SET_REG_WORD(CPR_RETENTION6_ADR,cprRetentionReg6);

	return;
}

/* Set DDR timing parameters based on user defined DDR frequency in kHz - as much as possible use multiples of 1000 kHz
    the PLL can only do multiples of 6 or 12 MHz and with the CPR divider I think 1MHz is a good enough resolution 
	See the following bugzilla tickets for an explaination of these parameters:
    * http://dub30/bugzilla/show_bug.cgi?id=12409
    * http://dub30/bugzilla/show_bug.cgi?id=12426
	
	*/
static inline void configureDdrTimings(u32 freq)
{
    u32 cl, bl, bt, modeReg;

	/* Mode settings: parameters are defined in the driver defines file - drv_ddr_def.h */
	/* ================================================================================ */
	/* Set the mode register value to set CAS latency and Burst length */

    DPRINTF(("Configure DDR for Freq: %d Khz\n",freq));
    SET_REG_WORD(DDRC_REG_DDRC_MR_ADR, ( (OP_MODE << 7) |
                                         ((CAS_LATENCY & 0x7) << 4) |
                                         ((BURST_TYPE & 0x1) << 3) |
                                         (((BURST_LEN == 8) ? BL8_ENC : BL4_ENC) & 0x7) ) );

    DPRINTF(("Mode register configuration:\n"));
    DPRINTF(("Operating mode: 0x%X \n", OP_MODE));
    modeReg = GET_REG_WORD_VAL(DDRC_REG_DDRC_MR_ADR);
    bl = modeReg & 0x7;
    if (bl == 0x1)
    {
        DPRINTF(("Burst length (BL) = 2 -- unsupported\n"));
    }
    else if (bl == 0x2)
    {
        DPRINTF(("Burst length (BL) = 4\n"));
    }
    else if (bl == 0x3)
    {
        DPRINTF(("Burst length (BL) = 8\n"));
    }
    else if (bl == 0x4)
    {
        DPRINTF(("Burst length (BL) = 16 -- unsupported\n"));
    }
    else
    {
        DPRINTF(("Burst length (BL) = Reserved Value\n"));
    }

    bt = (modeReg & 0x8) >> 3;
    if (bt)
    {
        DPRINTF(("Burst type = Interleaved\n"));
    }
    else
    {
        DPRINTF(("Burst type = Sequential\n"));
    }

    cl = (modeReg & 0x70) >> 4;
    if (cl == 0x2)
    {
        DPRINTF(("CAS latency (CL) = 2\n"));
    }
    else if (cl == 0x3)
    {
        DPRINTF(("CAS latency (CL) = 3\n"));
    }
    else
    {
        DPRINTF(("CAS latency (CL) = Reserved Value\n"));
    }

    /* Set the Burst Length in the controller */
    SET_REG_WORD(DDRC_REG_DDRC_BURST8_RDWR_ADR, ((BURST_LEN == 8) ? 1 : 0) );
/* =================================================================================================================================================== */

    /* minimum time from refresh to refresh - min. 80 ns */
    SET_REG_WORD( DDRC_REG_DDRC_T_RFC_MIN_ADR, (((0x50 * freq)/1000000) + 1) );

    /* average time between refreshes - max. 15.6 us */
    SET_REG_WORD( DDRC_REG_DDRC_T_RFC_NOM_X32_ADR, (((0x9C * freq)/320000) - 1) );

    /* Valid only in burst of 8 mode. At most 4 banks must be activated in a rolling window of tFAW cycles. - valid only for bursts of 8 mode */
    /* min. 15 ns */
    SET_REG_WORD( DDRC_REG_DDRC_T_FAW_ADR, (((0xF * freq)/1000000) + 1) ); //not defined in model or datasheet

    /* Max time between activate and precharge to same bank. Max time that a page can be kept open - max. 70 us */
    SET_REG_WORD( DDRC_REG_DDRC_T_RAS_MAX_ADR, ((0x7 * freq)/102400 - 1) );

    /* Min time between activate and precharge to same bank. Min time that a page can be kept open - min. 40 ns */
    SET_REG_WORD( DDRC_REG_DDRC_T_RAS_MIN_ADR, (((0x28 * freq)/1000000) + 1) );

    /* min time between activates to same bank  - min. 55 ns */
    SET_REG_WORD( DDRC_REG_DDRC_T_RC_ADR, (((0x39 * freq)/1000000) + 1) );

    /* cycles to wait after reset before driving CKE high to start the RAM init sequence - min. 200 us */
    SET_REG_WORD( DDRC_REG_DDRC_PRE_CKE_X1024_ADR, (((0x2 * freq)/10240) + 1) );

    /* cycles to wait after reset before driving CKE high to start the RAM init sequence - min. 400 ns */
    SET_REG_WORD( DDRC_REG_DDRC_POST_CKE_X1024_ADR, (((0x4 * freq)/10240000) + 1) );

    /* min time from activate to read or write command to same bank - min. 15 ns */
    SET_REG_WORD( DDRC_REG_DDRC_T_RCD_ADR, (((0xF * freq)/1000000) + 1) );

    /* min time from precharge to activate of same bank - min. 15 ns */
    SET_REG_WORD( DDRC_REG_DDRC_T_RP_ADR, (((0xF * freq)/1000000) + 1) );

    /* Min time between activates from bank a to bank b - min. 10 ns */
    SET_REG_WORD( DDRC_REG_DDRC_T_RRD_ADR, (((0xA * freq)/1000000) + 1) );

    /* Min time between two reads or two writes (from bank a to bank b) - spec 2 cycles */
    /* Value programmed is num clocks minus 1) */
    SET_REG_WORD( DDRC_REG_DDRC_T_CCD_ADR, 0x1);

    /* min time after power down exit to any operation - min. 1 ns */
    SET_REG_WORD( DDRC_REG_DDRC_T_XP_ADR, ((freq/1000000) + 1) );

    /* minimum number of cycles of CKE HIGH/LOW during power down and self refresh - min. 1 ns */
    SET_REG_WORD( DDRC_REG_DDRC_T_CKE_ADR, ((freq/1000000) + 1) );

    /* min. cycles between Load Mode commands - min. 2 clk periods */
    SET_REG_WORD( DDRC_REG_DDRC_T_MRD_ADR, 0x2 );

    /* min time between write and precharge to same bank - min. 15 ns + WL + BL/2 clk cycles  -- according to DDRC spec -- WL=RL=CAS=3, BL = 4 according to reset value of
             mode register (which will not be set by this function */
    SET_REG_WORD( DDRC_REG_DDRC_WR2PRE_ADR, (((0xF * freq)/1000000) + WRITE_LATENCY + BURST_LEN/2 + 1) );

    /* min time from write command to read command - WL + tW2R + BL/2 = 3 + min. 2 clk periods + 4/2*/
    SET_REG_WORD( DDRC_REG_DDRC_WR2RD_ADR, 0x2 + WRITE_LATENCY + BURST_LEN/2 );

    /* min time to wait after coming out of self refresh before doing anything - min. 80 ns */
    SET_REG_WORD( DDRC_REG_DDRC_POST_SELFREF_GAP_X32_ADR, (((0x50 * freq)/32000000) + 1) );

    /* min time from read command to write command - RL + 1 + BL/2 - WL = 3 + 1 + 4/2 - 3 = 3*/
    SET_REG_WORD( DDRC_REG_DDRC_RD2WR_ADR, 0x1 + READ_LATENCY + BURST_LEN/2 - WRITE_LATENCY );

    /* min time from read command toprecharge of the same bank = tRTP for BL=4, tRTP+2 for BL=8  - tRTP min. 7.5 ns */
    SET_REG_WORD( DDRC_REG_DDRC_RD2PRE_ADR, (((0x4B * freq)/10000000) + 1 + (BURST_LEN - 4)/2) );

#ifdef REG_DEBUG

        DPRINTF(("DDRC_REG_DDRC_BURST8_RDWR_ADR: ........ 0x%08X\n", GET_REG_WORD_VAL(DDRC_REG_DDRC_BURST8_RDWR_ADR)));
		DPRINTF(("DDRC_REG_DDRC_T_RFC_MIN_ADR: .......... 0x%08X\n", GET_REG_WORD_VAL(DDRC_REG_DDRC_T_RFC_MIN_ADRDDRC_REG_DDRC_T_RFC_MIN_ADR)));
		DPRINTF(("DDRC_REG_DDRC_T_RFC_NOM_X32_ADR: ...... 0x%08X\n", GET_REG_WORD_VAL(DDRC_REG_DDRC_T_RFC_NOM_X32_ADR)));
		DPRINTF(("DDRC_REG_DDRC_T_FAW_ADR: .............. 0x%08X\n", GET_REG_WORD_VAL(DDRC_REG_DDRC_T_FAW_ADR)));
		DPRINTF(("DDRC_REG_DDRC_T_RAS_MAX_ADR: .......... 0x%08X\n", GET_REG_WORD_VAL(DDRC_REG_DDRC_T_RAS_MAX_ADR)));
		DPRINTF(("DDRC_REG_DDRC_T_RAS_MIN_ADR: .......... 0x%08X\n", GET_REG_WORD_VAL(DDRC_REG_DDRC_T_RAS_MIN_ADR)));
		DPRINTF(("DDRC_REG_DDRC_T_RC_ADR: ............... 0x%08X\n", GET_REG_WORD_VAL(DDRC_REG_DDRC_T_RC_ADR)));
		DPRINTF(("DDRC_REG_DDRC_PRE_CKE_X1024_ADR: ...... 0x%08X\n", GET_REG_WORD_VAL(DDRC_REG_DDRC_PRE_CKE_X1024_ADR)));
		DPRINTF(("DDRC_REG_DDRC_POST_CKE_X1024_ADR: ..... 0x%08X\n", GET_REG_WORD_VAL(DDRC_REG_DDRC_POST_CKE_X1024_ADR)));
		DPRINTF(("DDRC_REG_DDRC_T_RCD_ADR: .............. 0x%08X\n", GET_REG_WORD_VAL(DDRC_REG_DDRC_T_RCD_ADR)));
		DPRINTF(("DDRC_REG_DDRC_T_RP_ADR: ............... 0x%08X\n", GET_REG_WORD_VAL(DDRC_REG_DDRC_T_RP_ADR)));
		DPRINTF(("DDRC_REG_DDRC_T_RRD_ADR: .............. 0x%08X\n", GET_REG_WORD_VAL(DDRC_REG_DDRC_T_RRD_ADR)));
		DPRINTF(("DDRC_REG_DDRC_T_CCD_ADR: .............. 0x%08X\n", GET_REG_WORD_VAL(DDRC_REG_DDRC_T_CCD_ADR)));
		DPRINTF(("DDRC_REG_DDRC_T_XP_ADR: ............... 0x%08X\n", GET_REG_WORD_VAL(DDRC_REG_DDRC_T_XP_ADR)));
		DPRINTF(("DDRC_REG_DDRC_T_CKE_ADR: .............. 0x%08X\n", GET_REG_WORD_VAL(DDRC_REG_DDRC_T_CKE_ADR)));
		DPRINTF(("DDRC_REG_DDRC_T_MRD_ADR: .............. 0x%08X\n", GET_REG_WORD_VAL(DDRC_REG_DDRC_T_MRD_ADR)));
		DPRINTF(("DDRC_REG_DDRC_WR2PRE_ADR: ............. 0x%08X\n", GET_REG_WORD_VAL(DDRC_REG_DDRC_WR2PRE_ADR)));
		DPRINTF(("DDRC_REG_DDRC_WR2RD_ADR: .............. 0x%08X\n", GET_REG_WORD_VAL(DDRC_REG_DDRC_WR2RD_ADR)));
		DPRINTF(("DDRC_REG_DDRC_POST_SELFREF_GAP_X32_ADR: 0x%08X\n", GET_REG_WORD_VAL(DDRC_REG_DDRC_POST_SELFREF_GAP_X32_ADR)));
		DPRINTF(("DDRC_REG_DDRC_RD2WR_ADR: .............. 0x%08X\n", GET_REG_WORD_VAL(DDRC_REG_DDRC_RD2WR_ADR)));
		DPRINTF(("DDRC_REG_DDRC_RD2PRE_ADR: ............. 0x%08X\n", GET_REG_WORD_VAL(DDRC_REG_DDRC_RD2PRE_ADR)));
#endif
}

/* This function configures the DDR Controller performnace parameters
   to optimise DDR bandwidth and thus avoid the potential for bus lock situations
*/
inline void tuneDdrPerformance(void)
{
/* 
    The DDR Controller has a simple 3-state state machine (See Transaction Service
	Control section of the spec) to avoid starvation of the read queue.

	The states are NORMAL, CRITICAL, HARD NON-CRITICAL

	If a transaction store has been starved for a certain length of time, it moves
	to the CRITICAL state in which it becomes prioritized for service. Once
	serviced it moves to the HARD NON-CRITICAL which specifies a wait time to
	prevent the transaction store becoming cricital again too soon.

	The state machines are controlled by the following registers:
	DDRC_REG_DDRC_HPR_MAX_STARVE_X32_ADR       (transition from NORMAL to CRITICAL)
	DDRC_REG_DDRC_HPR_XACT_RUN_LENGTH_ADR      (length of time in CRITICAL)
	DDRC_REG_DDRC_HPR_MIN_NON_CRITICAL_X32_ADR (hold off before returning to NORMAL)

	All the above registers have reset value of 0xF. This means that in the normal
	state the queue would have to be starved for between 480 and 512 DDR clock
	cycles before moving to the CRITICAL state.

	A value of 0 for MAX_STARVE_X32 disables the state machine. So if you want to
	minimise read latencies you should set
	DDRC_REG_DDRC_HPR_MAX_STARVE_X32_ADR       = 1
	DDRC_REG_DDRC_HPR_MIN_NON_CRITICAL_X32_ADR = 0
    
	The DDR Controller Spec specifies that
	"The controller .... prioritizes requests to minimize the latency of reads
	(especially high priority reads) and maximize page hits."
*/

	SET_REG_WORD( DDRC_REG_DDRC_LPR_NUM_ENTRIES_ADR,          0 );  // Num entries in Low pritority queue
	SET_REG_WORD( DDRC_REG_DDRC_HPR_MAX_STARVE_X32_ADR,       1 );  // transition from NORMAL to CRITICAL
	SET_REG_WORD( DDRC_REG_DDRC_HPR_MIN_NON_CRITICAL_X32_ADR, 1 );  // hold off before returning to NORMAL
    SET_REG_WORD( DDRC_REG_DDRC_HPR_XACT_RUN_LENGTH_ADR,     15 );  // length of time in CRITICAL

	return;
}

