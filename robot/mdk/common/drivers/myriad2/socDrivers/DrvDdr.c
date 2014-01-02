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
#include <registersMyriad.h>
#include <DrvRegUtils.h>
#include <DrvTimer.h>
#include <stdio.h>
#include <assert.h>
#include <DrvDdr.h>

// 2:  Source Specific #defines and types  (typedef,enum,struct)
// ----------------------------------------------------------------------------

// Constraints for PGN28HPM18MF3000 using high band
#define DRVCPR_BS1_MINFREFK							25000
#define DRVCPR_BS1_MAXFREFK							50000
#define DRVCPR_BS1_MINFVCOK							1500000
#define DRVCPR_BS1_MAXFVCOK							3000000
#define DRVCPR_BS1_MINFOUTK							187500
#define DRVCPR_BS1_MAXFOUTK							3000000

// Constraints for PGN28HPM18MF3000 using low band
#define DRVCPR_BS0_MINFREFK							10000
#define DRVCPR_BS0_MAXFREFK							50000
#define DRVCPR_BS0_MINFVCOK							800000
#define DRVCPR_BS0_MAXFVCOK							1600000
#define DRVCPR_BS0_MINFOUTK							100000
#define DRVCPR_BS0_MAXFOUTK							1600000

// Standard limits for PGN28HPM18MF3000
#define DRVCPR_MINFEEDBACK							16 
#define DRVCPR_MAXFEEDBACK							160 
#define DRVCPR_MINIDIV								1
#define DRVCPR_MAXIDIV								33
#define DRVCPR_MINODIV								0
#define DRVCPR_MAXODIV								4

// Maximun difference allowed
#define DRVCPR_MAXDEVIATION							10000

// Standard input freq
#define DEFAULT_OSC_INPUT_CLK_KHZ           		(12000)

// Flags needed for PLL 
#define DRVCPR_PLL_BYPASS_MASK					(1 << 1)
#define DRVCPR_PLL_LOCK_MASK					(1 << 0)
#define DRVCPR_PLL_TIMEOUT_MASK					(1 << 2)
#define DRVCPR_PLL_UPDATE_COMPLETE_MASK			(1 << 1)


#define DRV_PLL_SW_TIMEOUT_US                   (5000)   // Wait 5ms for PLL to lock

#define ELPIDA_LPDDR_1Gb_533MHz_1Die
#define unitTestAssert(t) assert(t)

// 3: Global Data (Only if absolutely necessary)
// ----------------------------------------------------------------------------

static int data_v128;
static int exp_data;
static int data;
static int count;
static int fb;
static int idiv;
static int odiv;
static int bs;
static int bp;
static int pd;
static int refsrc;

// 6: Functions Implementation
// ----------------------------------------------------------------------------

/*
*	\fn  u32 calculatePllConfigValue(u32 oscIn, u32 sysClkDesired,u32 * pAchievedFreqKhz)
*	\param u32 oscIn Input freq (KHz)
*	\param u32 sysClkDesired Desired output freq (KHz)
*	\param u32 * pAchievedFreqKhz Actual freq achieved (KHz)
*	\brief  Allows to work out all the parameters for the PLL in the design except refSrc for PLL1
*	\return Returns the word that is to be written in the PLL register
*	\remarks There might an error of up to DRVCPR_MAXDEVIATION in the actual frequency 
*/
u32 calculatePllConfigValue(u32 oscIn, u32 sysClkDesired,u32 * pAchievedFreqKhz)
{
    u32 pllCtrlVal, freq;
    u32 fRef;
    u32 fVco;
    u32 diff, diffBk;

    u32 inDiv, feedbackDiv, outDiv;
    u32 inDivBk = 0, feedbackDivBk = 0, outDivBk = 0, bandBk = 0;

    diffBk = 1000000; // 1 GHz difference

    for (inDiv = DRVCPR_MINIDIV ; inDiv < DRVCPR_MAXIDIV ; ++inDiv)
    {
        fRef = oscIn / inDiv ; 		
		// Check low band constraint 
        if ((fRef >= DRVCPR_BS0_MINFREFK) && (fRef <= DRVCPR_BS0_MAXFREFK))
        {
            for (outDiv = DRVCPR_MINODIV ; outDiv < DRVCPR_MAXODIV ; ++outDiv)
            {
                fVco = sysClkDesired  * (1<<outDiv);
                if ((fVco <= DRVCPR_BS0_MAXFVCOK) && (fVco >= DRVCPR_BS0_MINFVCOK))
                {
                    feedbackDiv = (sysClkDesired * (1 << outDiv) * inDiv) / oscIn;
                    if ((feedbackDiv >= DRVCPR_MINFEEDBACK) && (feedbackDiv <= DRVCPR_MAXFEEDBACK))
                    {
                        freq = oscIn * feedbackDiv / ((1<<outDiv) * inDiv);

                        if (sysClkDesired > freq)
                            diff = sysClkDesired - freq;
                        else
                            diff = freq - sysClkDesired;
						// This will make sure that if there is the same result for bands 0 and 1, band 0 settings will be used as it offers best jitter.
                        if (diff <= diffBk)
                        {
                            inDivBk = inDiv;
                            feedbackDivBk = feedbackDiv;
                            outDivBk = outDiv;
                            bandBk = 0;
                            diffBk = diff;
                        }
                    }
                }
            }
		}		
		// Check high band constraint 
        if ((fRef >= DRVCPR_BS1_MINFREFK) && (fRef <= DRVCPR_BS1_MAXFREFK))
        {
            for (outDiv = DRVCPR_MINODIV ; outDiv < DRVCPR_MAXODIV ; ++outDiv)
            {
                fVco = sysClkDesired  * (1<<outDiv);
                if ((fVco <= DRVCPR_BS1_MAXFVCOK) && (fVco >= DRVCPR_BS1_MINFVCOK))
                {
                    feedbackDiv = (sysClkDesired * (1 << outDiv) * inDiv) / oscIn;
                    if ((feedbackDiv >= DRVCPR_MINFEEDBACK) && (feedbackDiv <= DRVCPR_MAXFEEDBACK))
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
                            bandBk = 1;
                            diffBk = diff;
                        }
                    }
                }
            }
		}		        
    }

    if (diffBk > DRVCPR_MAXDEVIATION)
    {
        *pAchievedFreqKhz = 0;
        return 0; // don't set, difference too big 
    }

    pllCtrlVal = ( (inDivBk - 1) << 6)       |
                   ( ((feedbackDivBk >> 1) - 1) << 11) | 
                   ( outDivBk << 4 )         |
                   ( bandBk << 2);

    freq = oscIn * feedbackDivBk / ((1<<outDivBk) * inDivBk);

    *pAchievedFreqKhz = freq;	
    return pllCtrlVal;
}


// New function to configure PLLs 
static void configure_plls_(void)
{
    u32 afreq = 0;
    // -------------------------------------------------------------------------------
    // program up PLL 1/0
    // -------------------------------------------------------------------------------
    // program the divider to 0
    SET_REG_WORD(CPR_CLK_DIV_ADR, 0x00000000);
    // Turn off regular bypass
    //SET_REG_WORD(CPR_CLK_DIV_ADR, 0x00010004);
    SET_REG_WORD(CPR_CLK_BYPASS_ADR, 0x00000000);
    SET_REG_WORD(CPR_NUM_CYCLES_ADR, 0x0000001f);
    unitTestAssert(GET_REG_WORD_VAL(CPR_NUM_CYCLES_ADR) == 0x0000001f);
	
	
	
	//SET_REG_BITS_MASK(CPR_PLL_CTRL0_ADR, 0x02);
	
	
	
    // Boot PLL 0 to 600MHz, VCO 1200, low band
    // Start up the PLL, get word to program the register with
	data = calculatePllConfigValue(DEFAULT_OSC_INPUT_CLK_KHZ, 600000, &afreq);    
    //fb=43; idiv=0x00 ;odiv=0; bs=1; bp=0b0; pd=0b0;   // ~380Mhz
    //data = pd | (bp << 1) | (bs << 2) | (odiv << 4) | (idiv << 6) | (fb << 11);
    SET_REG_WORD(CPR_PLL_CTRL0_ADR, data);
    unitTestAssert(GET_REG_WORD_VAL(CPR_PLL_CTRL0_ADR) == data);	
	
	
	
//     // Turn second OSC on , bits 5:3
//     data_v128 = GET_REG_WORD_VAL(CPR_RETENTION6_ADR);
//     DrvRegSetBitField(&data_v128, 3, 5-3+1, 0b111);
//     SET_REG_WORD(CPR_RETENTION6_ADR, data_v128);
//     unitTestAssert(GET_REG_WORD_VAL(CPR_RETENTION6_ADR) == data_v128);

	
	//SET_REG_BITS_MASK(CPR_PLL_CTRL1_ADR, 0x02);	

    #if defined(ELPIDA_LPDDR_1Gb_533MHz_1Die)
        // Boot PLL 1 to 533MHz, VCO 533, low band, OSC 0 selected
        // Start up the PLL
		refsrc = 2;
		data = calculatePllConfigValue(DEFAULT_OSC_INPUT_CLK_KHZ, 266500, &afreq); 
		data |= (refsrc << 18);				
        //fb=43; idiv=0x00 ;odiv=2; bs=0; bp=0b0; pd=0b0; refsrc=2;   // 533MHz
        //data = pd | (bp << 1) | (bs << 2) | (odiv << 4) | (idiv << 6) | (fb << 11) | (refsrc << 18);		
        SET_REG_WORD(CPR_PLL_CTRL1_ADR, data);
        unitTestAssert(GET_REG_WORD_VAL(CPR_PLL_CTRL1_ADR) == data);		
		
    #elif defined(MICRON_LPDDR_1Gb_533MHz_1Die)
        // Boot PLL 1 to 533MHz, VCO 533, low band, OSC 0 selected
        // Start up the PLL
        //fb=43; idiv=0x00 ;odiv=2; bs=0; bp=0b0; pd=0b0; refsrc=2;   // 533MHz
        //data = pd | (bp << 1) | (bs << 2) | (odiv << 4) | (idiv << 6) | (fb << 11) | (refsrc << 18);
		refsrc = 2;
		data = calculatePllConfigValue(DEFAULT_OSC_INPUT_CLK_KHZ, 266500, &afreq); 
		data |= (refsrc << 18);
        SET_REG_WORD(CPR_PLL_CTRL1_ADR, data);
        unitTestAssert(GET_REG_WORD_VAL(CPR_PLL_CTRL1_ADR) == data);
    #else
        // Boot PLL 1 to 800MHz, VCO 800, high band, OSC 0 selected
        // Start up the PLL
        //fb=66; idiv=0x00 ;odiv=2; bs=0; bp=0b0; pd=0b0; refsrc=2;   // 800MHz		
        //data = pd | (bp << 1) | (bs << 2) | (odiv << 4) | (idiv << 6) | (fb << 11) | (refsrc << 18);
		refsrc = 2;
		data = calculatePllConfigValue(DEFAULT_OSC_INPUT_CLK_KHZ, 400000, &afreq); 
		data |= (refsrc << 18);
        SET_REG_WORD(CPR_PLL_CTRL1_ADR, data);
        unitTestAssert(GET_REG_WORD_VAL(CPR_PLL_CTRL1_ADR) == data);
    #endif 
	
	
//

    // Wait for PLL 0 to lock
    exp_data =0;count =0;
    //printf("CHeck PLL0 has locked");
    exp_data = GET_REG_WORD_VAL(CPR_PLL_STAT0_ADR);
	// Worst case possible 3000 MHz, Power up 1u < + 200u -> Consider 300u for security. Up to 900000 (4500 * 200) cycles at 3000Mhz. Very unlikely in this case
    while ((!((((exp_data >> 0) & 1) == 0x01)|(((exp_data >> 2) & 1) == 0x01))) & (count < 4500)) {
       SleepTicks(200);count++;
       exp_data = GET_REG_WORD_VAL(CPR_PLL_STAT0_ADR);
    }

	
	//CLR_REG_BITS_MASK(CPR_PLL_CTRL0_ADR,0x02);
	
    // Check PLL 1 has locked
    exp_data =0;count =0;
    //printf("Check PLL1 has locked");
    exp_data = GET_REG_WORD_VAL(CPR_PLL_STAT1_ADR);
	// Worst case possible 3000 MHz, Power up 1u < + 200u -> Consider 300u for security. Up to 900000 (4500 * 200) cycles at 3000Mhz
    while ((!((((exp_data >> 0) & 1) == 0x01)|(((exp_data >> 2) & 1) == 0x01))) & (count < 4500)) {
       SleepTicks(200);count++;
       exp_data = GET_REG_WORD_VAL(CPR_PLL_STAT1_ADR);
    }
	
	
	//CLR_REG_BITS_MASK(CPR_PLL_CTRL1_ADR,0x02);
	
    //printf("All PLL's  have locked");
}

static void configure_plls()
{
	

    // -------------------------------------------------------------------------------
    // program up PLL 1/0
    // -------------------------------------------------------------------------------
    // program the divider to 0
    SET_REG_WORD(CPR_CLK_DIV_ADR, 0x00000000);
    // Turn off regular bypass
    //SET_REG_WORD(CPR_CLK_DIV_ADR, 0x00010004);
    SET_REG_WORD(CPR_CLK_BYPASS_ADR, 0x00000000);
    SET_REG_WORD(CPR_NUM_CYCLES_ADR, 0x0000001f);
    unitTestAssert(GET_REG_WORD_VAL(CPR_NUM_CYCLES_ADR) == 0x0000001f);
    // Boot PLL 0 to 600MHz, VCO 1200, low band
    // Start up the PLL
    fb=49; idiv=0x00 ;odiv=1; bs=0; bp=0b0; pd=0b0;   // 600MHz
    //fb=43; idiv=0x00 ;odiv=0; bs=1; bp=0b0; pd=0b0;   // ~380Mhz
    data = pd | (bp << 1) | (bs << 2) | (odiv << 4) | (idiv << 6) | (fb << 11);
    SET_REG_WORD(CPR_PLL_CTRL0_ADR, data);
    unitTestAssert(GET_REG_WORD_VAL(CPR_PLL_CTRL0_ADR) == data);
//     // Turn second OSC on , bits 5:3
//     data_v128 = GET_REG_WORD_VAL(CPR_RETENTION6_ADR);
//     DrvRegSetBitField(&data_v128, 3, 5-3+1, 0b111);
//     SET_REG_WORD(CPR_RETENTION6_ADR, data_v128);
//     unitTestAssert(GET_REG_WORD_VAL(CPR_RETENTION6_ADR) == data_v128);

    #if defined(ELPIDA_LPDDR_1Gb_533MHz_1Die)
        // Boot PLL 1 to 533MHz, VCO 533, low band, OSC 0 selected
        // Start up the PLL
        fb=43; idiv=0x00 ;odiv=2; bs=0; bp=0b0; pd=0b0; refsrc=2;   // 533MHz
        data = pd | (bp << 1) | (bs << 2) | (odiv << 4) | (idiv << 6) | (fb << 11) | (refsrc << 18);
        SET_REG_WORD(CPR_PLL_CTRL1_ADR, data);
        unitTestAssert(GET_REG_WORD_VAL(CPR_PLL_CTRL1_ADR) == data);
    #elif defined(MICRON_LPDDR_1Gb_533MHz_1Die)
        // Boot PLL 1 to 533MHz, VCO 533, low band, OSC 0 selected
        // Start up the PLL
        fb=43; idiv=0x00 ;odiv=2; bs=0; bp=0b0; pd=0b0; refsrc=2;   // 533MHz
        data = pd | (bp << 1) | (bs << 2) | (odiv << 4) | (idiv << 6) | (fb << 11) | (refsrc << 18);
        SET_REG_WORD(CPR_PLL_CTRL1_ADR, data);
        unitTestAssert(GET_REG_WORD_VAL(CPR_PLL_CTRL1_ADR) == data);
    #else
        // Boot PLL 1 to 800MHz, VCO 800, high band, OSC 0 selected
        // Start up the PLL
        fb=66; idiv=0x00 ;odiv=2; bs=0; bp=0b0; pd=0b0; refsrc=2;   // 800MHz
        data = pd | (bp << 1) | (bs << 2) | (odiv << 4) | (idiv << 6) | (fb << 11) | (refsrc << 18);
        SET_REG_WORD(CPR_PLL_CTRL1_ADR, data);
        unitTestAssert(GET_REG_WORD_VAL(CPR_PLL_CTRL1_ADR) == data);
    #endif 

//

    // Wait for PLL 0 to lock
    exp_data =0;count =0;
    //printf("CHeck PLL0 has locked");
    exp_data = GET_REG_WORD_VAL(CPR_PLL_STAT0_ADR);
    while ((!((((exp_data >> 0) & 1) == 0b1)|(((exp_data >> 2) & 1) == 0b1))) & (count < 100)) {
       SleepTicks(200);count++;
       exp_data = GET_REG_WORD_VAL(CPR_PLL_STAT0_ADR);
    }

    // Check PLL 1 has locked
    exp_data =0;count =0;
    //printf("Check PLL1 has locked");
    exp_data = GET_REG_WORD_VAL(CPR_PLL_STAT1_ADR);
    while ((!((((exp_data >> 0) & 1) == 0b1)|(((exp_data >> 2) & 1) == 0b1))) & (count < 100)) {
       SleepTicks(200);count++;
       exp_data = GET_REG_WORD_VAL(CPR_PLL_STAT1_ADR);
    }
    //printf("All PLL's  have locked");

}

static void enable_clocks()
{


    // Enable Aux clock for DRAM from PLL 1, no divide
    SET_REG_WORD(CPR_AUX20_CLK_CTRL_ADR, 0x10000000);
    SET_REG_WORD(CPR_AUX_CLK_RST_ADR  , 0x00000000);
     data_v128 = 0x00000000; DrvRegSetBitField(&data_v128, CSS_AUX_DDR_CORE, 1, 0b1);
    SET_REG_WORD(CPR_AUX_CLK_EN_ADR   , data_v128);
    //SET_REG_WORD(CPR_AUX_CLK_RST_ADR, 0x00000041);

    // Enable all other clocks
    SET_REG_WORD(CPR_CLK_EN0_ADR, 0xffffffff);
    // Enable DDR core clock
    SET_REG_WORD(CPR_CLK_EN0_ADR, 0xffffffff);
    SET_REG_WORD(CPR_CLK_EN1_ADR, 0xffffffff);
    SET_REG_WORD(CPR_BLK_RST0_ADR, 0xffffffff);
    SET_REG_WORD(CPR_BLK_RST1_ADR, 0xffffffff);

}

static void configure_ddr_ctrl()
{

    u32 data_v128;
    //printf("-----------------------------------------------------------------------------");
    //printf("--  DDR Controller init, power up ");
    //printf("-----------------------------------------------------------------------------");
    // Reset everything in controller(bit 18 and 13)
	data_v128 = GET_REG_WORD_VAL(CPR_BLK_RST1_ADR);
    DrvRegSetBitField(&data_v128, CSS_DSS_APB_PHY-32, 1, 0b0);
    DrvRegSetBitField(&data_v128, CSS_DSS_APB_CTRL-32, 1, 0b0);

    SET_REG_WORD(CPR_BLK_RST1_ADR, data_v128);
    SleepTicks(10);
    SET_REG_WORD(CPR_BLK_RST1_ADR, 0xffffffff);

    //

    // Leave ddr core clock reset (Aux clock 20)
	data_v128 = GET_REG_WORD_VAL(CPR_AUX_CLK_RST_ADR);
    DrvRegSetBitField(&data_v128, CSS_AUX_DDR_CORE, 1, 0b0);
    DrvRegSetBitField(&data_v128, CSS_AUX_DDR_CORE_PHY, 1, 0b0);
    SET_REG_WORD(CPR_AUX_CLK_RST_ADR, data_v128);
    SleepTicks(100);
    SET_REG_WORD(DDRC_DFIMISC_ADR, 0x00000000);
    //  1 rank, burst of 8 , LP-ddr3
	SET_REG_WORD(DDRC_DBG1_ADR, 0x00000001);
	SET_REG_WORD(DDRC_PWRCTL_ADR, 0x00000001);
	unitTestAssert(GET_REG_WORD_VAL(DDRC_STAT_ADR) == 0x00000000);

    #if defined( MICRON_LPDDR3_4Gb_800MHz_1Die)
        // MICRON_LPDDR3_4Gb_800MHz_1Die
	    SET_REG_WORD(DDRC_MSTR_ADR, 0x01041008);
    #else 
        // ELPIDA_LPDDR_1Gb_533MHz_1Die
	    SET_REG_WORD(DDRC_MSTR_ADR, 0x01041004);        
    #endif
        
	SET_REG_WORD(DDRC_MRCTRL0_ADR, 0x00009030);
	SET_REG_WORD(DDRC_MRCTRL1_ADR, 0x0000d845);
	SET_REG_WORD(DDRC_DERATEEN_ADR, 0x00000002);
	SET_REG_WORD(DDRC_DERATEINT_ADR, 0x00000000);
	SET_REG_WORD(DDRC_PWRCTL_ADR, 0x00000002);
	SET_REG_WORD(DDRC_PWRTMG_ADR, 0x001d4b03);
	SET_REG_WORD(DDRC_HWLPCTL_ADR, 0x000c0003);

    #if defined( MICRON_LPDDR3_4Gb_800MHz_1Die)
        // MICRON_LPDDR3_4Gb_800MHz_1Die
	    SET_REG_WORD(DDRC_RFSHCTL0_ADR, 0x00113064);
	    SET_REG_WORD(DDRC_RFSHCTL1_ADR, 0x0050001c);
	    SET_REG_WORD(DDRC_RFSHCTL3_ADR, 0x00000000);
	    SET_REG_WORD(DDRC_RFSHTMG_ADR, 0x00060019);
	    SET_REG_WORD(DDRC_CRCPARCTL0_ADR, 0x00000000);
	    SET_REG_WORD(DDRC_INIT0_ADR, 0x00020003);
	    SET_REG_WORD(DDRC_INIT1_ADR, 0x00020303);
	    SET_REG_WORD(DDRC_INIT2_ADR, 0x00000b06);
	    SET_REG_WORD(DDRC_INIT3_ADR, 0x0043001a);
	    SET_REG_WORD(DDRC_INIT4_ADR, 0x00020000);
	    SET_REG_WORD(DDRC_INIT5_ADR, 0x000e0009);
	    SET_REG_WORD(DDRC_DIMMCTL_ADR, 0x00000000);
	    SET_REG_WORD(DDRC_RANKCTL_ADR, 0x00000337);
	    SET_REG_WORD(DDRC_DRAMTMG0_ADR, 0x0b141a11);
	    SET_REG_WORD(DDRC_DRAMTMG1_ADR, 0x0003031a);
	    SET_REG_WORD(DDRC_DRAMTMG2_ADR, 0x030c0809);
	    SET_REG_WORD(DDRC_DRAMTMG3_ADR, 0x00500006);
	    SET_REG_WORD(DDRC_DRAMTMG4_ADR, 0x08020408);
	    SET_REG_WORD(DDRC_DRAMTMG5_ADR, 0x01010606);
	    SET_REG_WORD(DDRC_DRAMTMG6_ADR, 0x01010004);
	    SET_REG_WORD(DDRC_DRAMTMG7_ADR, 0x00000101);
	    SET_REG_WORD(DDRC_DRAMTMG8_ADR, 0x00000019);
	    SET_REG_WORD(DDRC_ZQCTL0_ADR, 0x40900024);
	    SET_REG_WORD(DDRC_ZQCTL1_ADR, 0x01400070);
	    SET_REG_WORD(DDRC_ZQCTL2_ADR, 0x00000000);
	    SET_REG_WORD(DDRC_DFITMG0_ADR, 0x02050102);
	    SET_REG_WORD(DDRC_DFITMG1_ADR, 0x00030202);
	    SET_REG_WORD(DDRC_DFILPCFG0_ADR, 0x07917030);
	    SET_REG_WORD(DDRC_DFIUPD0_ADR, 0x00400008);
	    SET_REG_WORD(DDRC_DFIUPD1_ADR, 0x002500a4);
	    SET_REG_WORD(DDRC_DFIUPD2_ADR, 0x015c0257);
	    SET_REG_WORD(DDRC_DFIUPD3_ADR, 0x03100101);
	    SET_REG_WORD(DDRC_DFIMISC_ADR, 0x00000001);
	    SET_REG_WORD(DDRC_ADDRMAP0_ADR, 0x0000001f);
	    SET_REG_WORD(DDRC_ADDRMAP1_ADR, 0x00021501);
	    SET_REG_WORD(DDRC_ADDRMAP2_ADR, 0x00050100);
	    SET_REG_WORD(DDRC_ADDRMAP3_ADR, 0x0f030504);
	    SET_REG_WORD(DDRC_ADDRMAP4_ADR, 0x00000f0f);
	    SET_REG_WORD(DDRC_ADDRMAP5_ADR, 0x06050101);
	    SET_REG_WORD(DDRC_ADDRMAP6_ADR, 0x0f020707);
    #else 
        // ELPIDA_LPDDR_1Gb_533MHz_1Die / Micron_LPDDR_1Gb_533MHz_1Die
	    SET_REG_WORD(DDRC_RFSHCTL0_ADR, 0x00113024);
	    SET_REG_WORD(DDRC_RFSHCTL1_ADR, 0x0030003c);
	    SET_REG_WORD(DDRC_RFSHCTL3_ADR, 0x00000000);
	    SET_REG_WORD(DDRC_RFSHTMG_ADR, 0x00040011);
	    SET_REG_WORD(DDRC_CRCPARCTL0_ADR, 0x00000000);
	    SET_REG_WORD(DDRC_INIT0_ADR, 0x00020003);
	    SET_REG_WORD(DDRC_INIT1_ADR, 0x00020303);
	    SET_REG_WORD(DDRC_INIT2_ADR, 0x00000206);
	    SET_REG_WORD(DDRC_INIT3_ADR, 0x00cb0006);
	    SET_REG_WORD(DDRC_INIT4_ADR, 0x00020000);
	    SET_REG_WORD(DDRC_INIT5_ADR, 0x000a0007);
	    SET_REG_WORD(DDRC_DIMMCTL_ADR, 0x00000000);
	    SET_REG_WORD(DDRC_RANKCTL_ADR, 0x00000337);
	    SET_REG_WORD(DDRC_DRAMTMG0_ADR, 0x080e110b);
	    SET_REG_WORD(DDRC_DRAMTMG1_ADR, 0x00020311);
	    SET_REG_WORD(DDRC_DRAMTMG2_ADR, 0x02080607);
	    SET_REG_WORD(DDRC_DRAMTMG3_ADR, 0x00300006);
	    SET_REG_WORD(DDRC_DRAMTMG4_ADR, 0x05010306);
	    SET_REG_WORD(DDRC_DRAMTMG5_ADR, 0x01010404);
	    SET_REG_WORD(DDRC_DRAMTMG6_ADR, 0x01010003);
	    SET_REG_WORD(DDRC_DRAMTMG7_ADR, 0x00000101);
	    SET_REG_WORD(DDRC_DRAMTMG8_ADR, 0x00000020);
	    SET_REG_WORD(DDRC_ZQCTL0_ADR, 0x40600018);
	    SET_REG_WORD(DDRC_ZQCTL1_ADR, 0x00e00070);
	    SET_REG_WORD(DDRC_ZQCTL2_ADR, 0x00000000);
	    SET_REG_WORD(DDRC_DFITMG0_ADR, 0x02030101);
	    SET_REG_WORD(DDRC_DFITMG1_ADR, 0x00030202);
	    SET_REG_WORD(DDRC_DFILPCFG0_ADR, 0x07917030);
	    SET_REG_WORD(DDRC_DFIUPD0_ADR, 0x00400008);
	    SET_REG_WORD(DDRC_DFIUPD1_ADR, 0x002500a4);
	    SET_REG_WORD(DDRC_DFIUPD2_ADR, 0x00300052);
	    SET_REG_WORD(DDRC_DFIUPD3_ADR, 0x006c0023);
	    SET_REG_WORD(DDRC_DFIMISC_ADR, 0x00000001);
	    
	    // 32-bit, half bus-width mode
	    // 8 banks
	    // columns: C0 to C8
	    // rows: R0 to R12
	    // we'll be using BL=8, therefore C0, C1, and C2 don't specify addresses.
	    
	    // address mapping: {R12..R0,B2..B0,C8..C3}
	    //  for HIF ((addresses >> 1) & ((1 << (22 - 1 + 1)) - 1)), or for AXI/AHB ((addresses >> 5) & ((1 << (26 - 5 + 1)) - 1))
	    // example: the HIF bit of C4 should be 2, and it's configured by addrmap_col_b2, which is bits ((ADDRMAP2 >> 0) & ((1 << (3 - 0 + 1)) - 1)) + 2
	        
//	    SET_REG_WORD(DDRC_ADDRMAP0_ADR, 0x00001f1f);
// 	    SET_REG_WORD(DDRC_ADDRMAP1_ADR, 0x00050505);
// 	    SET_REG_WORD(DDRC_ADDRMAP2_ADR, 0x00000000);
// 	    SET_REG_WORD(DDRC_ADDRMAP3_ADR, 0x0f0f0f00);
// 	    SET_REG_WORD(DDRC_ADDRMAP4_ADR, 0x00000f0f);
// 	    SET_REG_WORD(DDRC_ADDRMAP5_ADR, 0x04040404);
// 	    SET_REG_WORD(DDRC_ADDRMAP6_ADR, 0x0f0f0f04);

        // Updated Address Map to simplify using the DRAM memory Models
	    // address mapping: {B2..B0,R12..R0,C8..C3}
        // Now the bank bits are the most significant bits
        // to enable us to treat the memories
        SET_REG_WORD(DDRC_ADDRMAP0_ADR, 0x00001f1f);
        SET_REG_WORD(DDRC_ADDRMAP1_ADR, 0x00121212);
        SET_REG_WORD(DDRC_ADDRMAP2_ADR, 0x00000000);
        SET_REG_WORD(DDRC_ADDRMAP3_ADR, 0x0f0f0f00);
        SET_REG_WORD(DDRC_ADDRMAP4_ADR, 0x00000f0f);
        SET_REG_WORD(DDRC_ADDRMAP5_ADR, 0x01010101);
        SET_REG_WORD(DDRC_ADDRMAP6_ADR, 0x0f0f0f01);

    #endif
    
	SET_REG_WORD(DDRC_ODTCFG_ADR, 0x04060418);
	SET_REG_WORD(DDRC_ODTMAP_ADR, 0x00000000);
	SET_REG_WORD(DDRC_SCHED_ADR, 0x10c51c03);
	SET_REG_WORD(DDRC_SCHED1_ADR, 0x00000012);
	SET_REG_WORD(DDRC_PERFHPR1_ADR, 0xa400a347);
	SET_REG_WORD(DDRC_PERFLPR1_ADR, 0x290081a4);
	SET_REG_WORD(DDRC_PERFWR1_ADR, 0xaa008da1);
	SET_REG_WORD(DDRC_PERFVPR1_ADR, 0x000002f7);
	SET_REG_WORD(DDRC_DBG0_ADR, 0x00000015);
	SET_REG_WORD(DDRC_DBG1_ADR, 0x00000000);
	SET_REG_WORD(DDRC_DBGCMD_ADR, 0x00000000);
	SET_REG_WORD(DDRC_PCCFG_ADR, 0x00000010);
	SET_REG_WORD(DDRC_PCFGR_0_ADR, 0x0001324a);
	SET_REG_WORD(DDRC_PCFGW_0_ADR, 0x000032ea);
	SET_REG_WORD(DDRC_PCTRL_0_ADR, 0x00000001);
	SET_REG_WORD(DDRC_PCFGR_1_ADR, 0x00011195);
	SET_REG_WORD(DDRC_PCFGW_1_ADR, 0x00003205);
	SET_REG_WORD(DDRC_PCFGQOS0_1_ADR, 0x00210000);
	SET_REG_WORD(DDRC_PCFGQOS1_1_ADR, 0x004d0006);
	SET_REG_WORD(DDRC_PCFGR_2_ADR, 0x0001620d);
	SET_REG_WORD(DDRC_PCFGW_2_ADR, 0x0000104a);
	SET_REG_WORD(DDRC_PCTRL_2_ADR, 0x00000001);
	SET_REG_WORD(DDRC_PCFGQOS0_2_ADR, 0x00120001);
	SET_REG_WORD(DDRC_PCFGQOS1_2_ADR, 0x07ca0237);
	SET_REG_WORD(DDRC_PCFGR_3_ADR, 0x0000098e);
	SET_REG_WORD(DDRC_PCFGW_3_ADR, 0x0000301d);
	SET_REG_WORD(DDRC_PCFGIDMASKCH0_3_ADR, 0x000000cb);
	SET_REG_WORD(DDRC_PCFGIDVALUECH0_3_ADR, 0x000000c2);
	SET_REG_WORD(DDRC_PCFGIDMASKCH1_3_ADR, 0x0000000c);
	SET_REG_WORD(DDRC_PCFGIDVALUECH1_3_ADR, 0x00000001);
	SET_REG_WORD(DDRC_PCFGIDMASKCH2_3_ADR, 0x00000054);
	SET_REG_WORD(DDRC_PCFGIDVALUECH2_3_ADR, 0x0000005e);
	SET_REG_WORD(DDRC_PCFGIDMASKCH3_3_ADR, 0x00000079);
	SET_REG_WORD(DDRC_PCFGIDVALUECH3_3_ADR, 0x000000b1);
	SET_REG_WORD(DDRC_PCTRL_3_ADR, 0x00000001);
	SET_REG_WORD(DDRC_PCFGQOS0_3_ADR, 0x00110003);
	SET_REG_WORD(DDRC_PCFGQOS1_3_ADR, 0x0095005b);
	SET_REG_WORD(DDRC_PCFGR_3_ADR, 0x0000018e);
	SET_REG_WORD(DDRC_PCTRL_0_ADR, 0x00000001);
	SET_REG_WORD(DDRC_PCTRL_1_ADR, 0x00000001);
	SET_REG_WORD(DDRC_PCTRL_2_ADR, 0x00000001);
	SET_REG_WORD(DDRC_PCTRL_3_ADR, 0x00000001);
	unitTestAssert(GET_REG_WORD_VAL(DDRC_PCFGC_0_ADR) == 0x00000000);
	SET_REG_WORD(DDRC_DBG1_ADR, 0x00000000);
	SET_REG_WORD(DDRC_PWRCTL_ADR, 0x00000000);
	SET_REG_WORD(DDRC_DFIMISC_ADR, 0x00000000);

/*
        // Changing Address mapping (HIF -> DRAM) for
        //  - as per 4Gb spec- to the 0.5G byte DDR is:
        //     8 banks selected by ((addr >> 26) & ((1 << (28 - 26 + 1)) - 1)),
        // 16384 rows  selected by ((addr >> 12) & ((1 << (25 - 12 + 1)) - 1)).
        // 1024 (32-bit) columns selected by ((addr >>  2) & ((1 << (11 -  2 + 1)) - 1)),

        // Bits 1:0 can select a byte within a 32-bit word.
        // Bits 3:0 cannot be set as only 4x32 bits are fetched every time
	SET_REG_WORD(DDRC_ADDRMAP0_ADR, 0x0000001f);
	SET_REG_WORD(DDRC_ADDRMAP1_ADR, 0x00141414);
	SET_REG_WORD(DDRC_ADDRMAP2_ADR, 0x00000000);
	SET_REG_WORD(DDRC_ADDRMAP3_ADR, 0x0f000000);
	SET_REG_WORD(DDRC_ADDRMAP4_ADR, 0x00000f0f);
	SET_REG_WORD(DDRC_ADDRMAP5_ADR, 0x02020202);
	SET_REG_WORD(DDRC_ADDRMAP6_ADR, 0x0f0f0202);
*/

    //printf("-- DDR controller init complete");
    // Ok controller good, now turn on clock
	data_v128 = GET_REG_WORD_VAL(CPR_AUX_CLK_RST_ADR);
     DrvRegSetBitField(&data_v128, CSS_AUX_DDR_CORE, 1, 0b1);
     DrvRegSetBitField(&data_v128, CSS_AUX_DDR_CORE_PHY, 1, 0b1);
    SET_REG_WORD(CPR_AUX_CLK_RST_ADR, data_v128);

}

static void configure_ddr_phy()
{

    u32 data_v128;
    //printf("-----------------------------------------------------------------------------");
    //printf("-- PHY initialiation");
    //printf("-----------------------------------------------------------------------------");
    //printf(" DDR PHY init, power up");
    // power up sequence
    // Configure the PLL
    data_v128 = GET_REG_WORD_VAL(DDR_PHY_PLLCR  );
    // Set bits
    //DrvRegSetBitField(&data_v128, 11, 1, 0b1);            // Gear shift, speed up lock
    //DrvRegSetBitField(&data_v128, 18, 1, 0b1);            // Gear shift, speed up re-lock
    DrvRegSetBitField(&data_v128, 19, 20-19+1, 0b01);           // Range 225 - 490Mhz
    SET_REG_WORD(DDR_PHY_PLLCR  , data_v128);

    // Choose which bytes to disable
// 	SET_REG_WORD(DDR_PHY_DX4GCR0, 0x7c000284);
// 	SET_REG_WORD(DDR_PHY_DX5GCR0, 0x7c000284);
// 	SET_REG_WORD(DDR_PHY_DX6GCR0, 0x7c000284);
// 	SET_REG_WORD(DDR_PHY_DX7GCR0, 0x7c000284);
    data_v128 = GET_REG_WORD_VAL(DDR_PHY_DX4GCR0  ); DrvRegSetBitField(&data_v128, 0, 1, 0b0);
    SET_REG_WORD(DDR_PHY_DX4GCR0, data_v128);
    data_v128 = GET_REG_WORD_VAL(DDR_PHY_DX5GCR0  ); DrvRegSetBitField(&data_v128, 0, 1, 0b0);
    SET_REG_WORD(DDR_PHY_DX5GCR0, data_v128);
    data_v128 = GET_REG_WORD_VAL(DDR_PHY_DX6GCR0  ); DrvRegSetBitField(&data_v128, 0, 1, 0b0);
    SET_REG_WORD(DDR_PHY_DX6GCR0, data_v128);
    data_v128 = GET_REG_WORD_VAL(DDR_PHY_DX7GCR0  ); DrvRegSetBitField(&data_v128, 0, 1, 0b0);
    SET_REG_WORD(DDR_PHY_DX7GCR0, data_v128);


	SET_REG_WORD(DDR_PHY_DSGCR, 0x002064bf);
	SET_REG_WORD(DDR_PHY_DXCCR, 0x00181884);
	SET_REG_WORD(DDR_PHY_PTR0, 0x42c01810);
	SET_REG_WORD(DDR_PHY_PTR1, 0x008005f0);

    #if defined( MICRON_LPDDR3_4Gb_800MHz_1Die)
        // MICRON_LPDDR3_4Gb_800MHz_1Die
	    SET_REG_WORD(DDR_PHY_DCR, 0x002064b9);
	    SET_REG_WORD(DDR_PHY_MR0, 0x00000043);
	    SET_REG_WORD(DDR_PHY_MR1, 0x00000043);
	    SET_REG_WORD(DDR_PHY_MR2, 0x0000001a);
	    SET_REG_WORD(DDR_PHY_MR3, 0x00000002);
        SET_REG_WORD(DDR_PHY_DTPR0, 0xce227766);
        SET_REG_WORD(DDR_PHY_DTPR1, 0x22818500);
        SET_REG_WORD(DDR_PHY_DTPR2, 0x10061806);
        SET_REG_WORD(DDR_PHY_DTPR3, 0x0000002a);
        SET_REG_WORD(DDR_PHY_PGCR2, 0x00fc1860);
    #else 
        // ELPIDA_LPDDR_1Gb_533MHz_1Die / Micron_LPDDR_1Gb_533MHz_1Die
	    SET_REG_WORD(DDR_PHY_DCR, 0x002064b8);
	    SET_REG_WORD(DDR_PHY_MR0, 0x000000cb);
	    SET_REG_WORD(DDR_PHY_MR1, 0x000000cb);
	    SET_REG_WORD(DDR_PHY_MR2, 0x00000006);
	    SET_REG_WORD(DDR_PHY_MR3, 0x00000002);
	    SET_REG_WORD(DDR_PHY_DTPR0, 0x8997aa46);
	    SET_REG_WORD(DDR_PHY_DTPR1, 0x22810360);
	    SET_REG_WORD(DDR_PHY_DTPR2, 0x10041004);
	    SET_REG_WORD(DDR_PHY_DTPR3, 0x00000019);
	    SET_REG_WORD(DDR_PHY_PGCR2, 0x00fc103d);
    #endif
    
    
    // Set IO mode for faster IO's
    data_v128 = GET_REG_WORD_VAL(DDR_PHY_PGCR1  ); DrvRegSetBitField(&data_v128, 7, 8-7+1, 0b01);DrvRegSetBitField(&data_v128, 25, 1, 0b1);
	SET_REG_WORD(DDR_PHY_PGCR1, data_v128);



	SET_REG_WORD(DDR_PHY_PTR3, 0x12c000c8);
	SET_REG_WORD(DDR_PHY_PTR4, 0x0c802260);

    //printf("------------------------- Start init Z calib -----------------------------------------------");
    // Init sequence , Extra added init
	//unitTestAssert(GET_REG_WORD_VAL(DDR_PHY_PGSR0) == 0x8000000f);
    //SET_REG_WORD(DDR_PHY_PIR, 0x00000073);
    //SET_REG_WORD(DDR_PHY_PIR, 0x00000171);
    data_v128 = 0x0;
    while (((data_v128 >> 0) & 1) != 0b1 ) {
        SleepTicks(5000);
       data_v128 = GET_REG_WORD_VAL(DDR_PHY_PGSR0);
    }
    //SleepTicks(32);
    //printf("-------------- DDR DRAM init stage 1 complete --------------------------------------");

	SET_REG_WORD(DDR_PHY_PIR, 0x00000181);
    data_v128 = 0x0;
    while (((data_v128 >> 0) & 1) != 0b1 ) {
        SleepTicks(5000);
       data_v128 = GET_REG_WORD_VAL(DDR_PHY_PGSR0);
    }
    //SleepTicks(32);
    //printf("-------------- DDR PHY init stage 2 complete ----------------------------------------");

	SET_REG_WORD(DDR_PHY_DX0LCDLR2, 0x00006e6e);
	SET_REG_WORD(DDR_PHY_DX1LCDLR2, 0x00006e6e);
	SET_REG_WORD(DDR_PHY_DX2LCDLR2, 0x00006e6e);
	SET_REG_WORD(DDR_PHY_DX3LCDLR2, 0x00006e6e);
	SET_REG_WORD(DDR_PHY_DX4LCDLR2, 0x00006e6e);
	SET_REG_WORD(DDR_PHY_DX5LCDLR2, 0x00006e6e);
	SET_REG_WORD(DDR_PHY_DX6LCDLR2, 0x00006e6e);
	SET_REG_WORD(DDR_PHY_DX7LCDLR2, 0x00006e6e);


    //printf("----------------- Step 4 PHY initialization -----------------------------------------");
    SleepTicks(100);
	SET_REG_WORD(DDRC_DFIMISC_ADR, 0x00000001);

    data_v128 = 0x0;
    while (((data_v128 >> 0) & 1) != 0b1 ) {
        SleepTicks(5000);
       data_v128 = GET_REG_WORD_VAL(DDRC_STAT_ADR);
    }
    //SleepTicks(32);
    SET_REG_WORD(DDRC_PWRCTL_ADR, 0x00000000);
	SET_REG_WORD(DDRC_PWRCTL_ADR, 0x00000002);
	//SET_REG_WORD(DDRC_RFSHCTL3_ADR, 0x00000001);
    // Re-enable the ports
 	SET_REG_WORD(DDRC_PCTRL_0_ADR, 0x00000001);
 	SET_REG_WORD(DDRC_PCTRL_1_ADR, 0x00000001);
 	SET_REG_WORD(DDRC_PCTRL_2_ADR, 0x00000001);
 	SET_REG_WORD(DDRC_PCTRL_3_ADR, 0x00000001);

    //printf("-----------------------------------------------------------------------------");
    //printf("DDR Controller complete");
    //printf("-----------------------------------------------------------------------------");
}


// This is the single function you need to call to enable the DDR for a given frequency
void DrvDdrInitialise(u32 ddrFreqKhz)
{
	configure_plls_();
	enable_clocks();
    // -------------------------------------------------------------------------------
    // DDR contoller init
    // -------------------------------------------------------------------------------
    configure_ddr_ctrl();
    // -------------------------------------------------------------------------------
    // PHY initialiation
    // -------------------------------------------------------------------------------
    configure_ddr_phy();
}

