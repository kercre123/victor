///
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved.
///            For License Warranty see: common/license.txt
///
/// @brief     
///
/// .
///
///
// 1: Includes
// ----------------------------------------------------------------------------
#include <VcsHooksApi.h>
#include <registersMyriad.h>
#include <UnitTestApi.h>
#include <DrvRegUtils.h>
#include <DrvTimer.h>
#include <swcLeonUtils.h>
#include "MssTestUtilsApi.h"
#include <stdio.h>

// 2:  Source Specific #defines and types  (typedef,enum,struct)
// ----------------------------------------------------------------------------

// 3: Global Data (Only if absolutely necessary)
// ----------------------------------------------------------------------------
                   
// 4: Static Local Data
// ----------------------------------------------------------------------------
    u32 errCntr    = 0x00000000;
    
// 5: Static Function Prototypes
// ----------------------------------------------------------------------------

// 6: Functions Implementation
// ----------------------------------------------------------------------------
u8 dphy_read_testcode(u32 dphy_sel, u8 testcode)
{
    u32 * testout_adr;
    u32 testdataout;
    
    // assert TEST_EN
    SET_REG_WORD(MIPI_DPHY_TEST_EN_ADDR, dphy_sel);
    // assert TEST_CLK
    SET_REG_WORD(MIPI_DPHY_TEST_CLK_ADDR, dphy_sel);
    // prepare testcode to TEST_DIN
    SET_REG_WORD(MIPI_DPHY_TEST_DIN_ADDR, (u32)testcode);
    // de-assert TEST_CLK to program testcode
    SET_REG_WORD(MIPI_DPHY_TEST_CLK_ADDR, 0x0);
    // de-assert TEST_EN
    SET_REG_WORD(MIPI_DPHY_TEST_EN_ADDR, 0x0);

    // read back testdataoutvalue
    if (dphy_sel&0x3)
    {
        testout_adr = (u32*)MIPI_DPHY_TEST_DOUT_DPHY01_ADDR;
        testdataout = GET_REG_WORD_VAL(testout_adr);
        if (dphy_sel&(0x1<<1))
            testdataout >>= 8;
        
    } else if (dphy_sel&(0x3<<2))
    {
        testout_adr = (u32*)MIPI_DPHY_TEST_DOUT_DPHY23_ADDR;
        testdataout = GET_REG_WORD_VAL(testout_adr);
        if (dphy_sel&(0x1<<3))
            testdataout >>= 8;
    } else if (dphy_sel&(0x3<<4))
    {
        testout_adr = (u32*)MIPI_DPHY_TEST_DOUT_DPHY45_ADDR;
        testdataout = GET_REG_WORD_VAL(testout_adr);
        if (dphy_sel&(0x1<<5))
            testdataout >>= 8;
    }
    return (u8)(testdataout);
}

u8 dphy_program_testcode(u32 dphy_sel, u8 testcode, u8 testdatain)
{
    u32 * testout_adr;
    u32 testdataout;
    
    // assert TEST_EN
    SET_REG_WORD(MIPI_DPHY_TEST_EN_ADDR, dphy_sel);
    // assert TEST_CLK
    SET_REG_WORD(MIPI_DPHY_TEST_CLK_ADDR, dphy_sel);
    // prepare testcode to TEST_DIN
    SET_REG_WORD(MIPI_DPHY_TEST_DIN_ADDR, (u32)testcode);
    // de-assert TEST_CLK to program testcode
    SET_REG_WORD(MIPI_DPHY_TEST_CLK_ADDR, 0x0);
    // de-assert TEST_EN
    SET_REG_WORD(MIPI_DPHY_TEST_EN_ADDR, 0x0);
    // prepare  value into TEST_DIN
    SET_REG_WORD(MIPI_DPHY_TEST_DIN_ADDR, (u32)(testdatain));
    // assert TEST_CLK to program value
    SET_REG_WORD(MIPI_DPHY_TEST_CLK_ADDR, dphy_sel);    
    // de-assert TEST_CLK 
    SET_REG_WORD(MIPI_DPHY_TEST_CLK_ADDR, 0x0);
    // read back testdataoutvalue
    if (dphy_sel&0x3)
    {
        testout_adr = (u32*)MIPI_DPHY_TEST_DOUT_DPHY01_ADDR;
        testdataout = GET_REG_WORD_VAL(testout_adr);
        if (dphy_sel&(0x1<<1))
            testdataout >>= 8;
        
    } else if (dphy_sel&(0x3<<2))
    {
        testout_adr = (u32*)MIPI_DPHY_TEST_DOUT_DPHY23_ADDR;
        testdataout = GET_REG_WORD_VAL(testout_adr);
        if (dphy_sel&(0x1<<3))
            testdataout >>= 8;
    } else if (dphy_sel&(0x3<<4))
    {
        testout_adr = (u32*)MIPI_DPHY_TEST_DOUT_DPHY45_ADDR;
        testdataout = GET_REG_WORD_VAL(testout_adr);
        if (dphy_sel&(0x1<<5))
            testdataout >>= 8;
    }
    return (u8)(testdataout);
}

void dphy_enable(u32 dphy_sel)
{
    u32 data;
    int i;

    vcsFastPuts("Enable selected DPHY\n");
    
    data = GET_REG_WORD_VAL(MIPI_DPHY_INIT_CTRL_ADDR);
    data |= dphy_sel;
    
//    data |= 0x3F000000;
      
    SET_REG_WORD(MIPI_DPHY_INIT_CTRL_ADDR, data);
    
    return;
}

void dphy_disable(u32 dphy_sel)
{
    u32 data;
    int i;

    vcsFastPuts("Disable selected DPHY\n");
    
    data = GET_REG_WORD_VAL(MIPI_DPHY_INIT_CTRL_ADDR);
    data &= !(dphy_sel);
    
    SET_REG_WORD(MIPI_DPHY_INIT_CTRL_ADDR, data);
    
    return;
}

void dphy_testclr_pulse(u32 dphy_sel)
{
    u32 int_data;
    u32 int_data2;
    int i;

    vcsFastPuts("Create a Pulse with Testclear\n");
    
    // Deassert all testclr bits
    int_data = 0x00000000;
    SET_REG_WORD(MIPI_DPHY_TEST_CLR_ADDR, int_data);
    // Set selected testclr bits to '1'
    int_data |= dphy_sel;
    SET_REG_WORD(MIPI_DPHY_TEST_CLR_ADDR, int_data);
    
    int_data2 = GET_REG_WORD_VAL(MIPI_DPHY_TEST_CLR_ADDR);
//    printMsgInt("DPHY set value: ", int_data);
//    printMsgInt("DPHY read value: ", int_data2);
    
    //wait some time
//    for(i=0; i<1000; i++);
    SleepTicks(10);
    
    // Deassert all testclr bits
    int_data = 0x00000000;
    SET_REG_WORD(MIPI_DPHY_TEST_CLR_ADDR, int_data);
    
    int_data2 = GET_REG_WORD_VAL(MIPI_DPHY_TEST_CLR_ADDR);
//    printMsgInt("DPHY set value: ", int_data);
//    printMsgInt("DPHY read value: ", int_data2);
    
    
    // Wait some time
//    for(i=0; i<1000; i++);
    SleepTicks(50);
    
    return;
}


void dphy_program_hsfreqrange(u32 dphy_sel, u8 hsfreqrange)
{
    vcsFastPuts("Program hsfreqrange\n");
    
    // assert TEST_EN
    SET_REG_WORD(MIPI_DPHY_TEST_EN_ADDR, dphy_sel);
    // assert TEST_CLK
    SET_REG_WORD(MIPI_DPHY_TEST_CLK_ADDR, dphy_sel);
    // prepare hsfreqrange testcode to TEST_DIN
    SET_REG_WORD(MIPI_DPHY_TEST_DIN_ADDR, 0x44);
    // de-assert TEST_CLK to program testcode
    SET_REG_WORD(MIPI_DPHY_TEST_CLK_ADDR, 0x0);
    // de-assert TEST_EN
    SET_REG_WORD(MIPI_DPHY_TEST_EN_ADDR, 0x0);
    // prepare  hsfreqrange value into TEST_DIN
    SET_REG_WORD(MIPI_DPHY_TEST_DIN_ADDR, (u32)(hsfreqrange<<1));
    // assert TEST_CLK to program hsfreqrange
    SET_REG_WORD(MIPI_DPHY_TEST_CLK_ADDR, dphy_sel);    
    // de-assert TEST_CLK 
    SET_REG_WORD(MIPI_DPHY_TEST_CLK_ADDR, 0x0);
 
    return;
}
       

void dphy_program_pll(u32 dphy_sel, u8 hsfreqrange, u16 M, u8  N, u8 vcorange, u8 vcocap, u8 icpctrl, u8 lpfctrl)
{
   
    u8  data;
    u8  testdataout;

    vcsFastPuts("Program hsfreqrange\n");
    testdataout = dphy_program_testcode(dphy_sel, 0x44, (u8)(hsfreqrange<<1));

        
    vcsFastPuts("Activate PLL N and M \n");
    testdataout = dphy_program_testcode(dphy_sel, 0x19, 0x30);

    vcsFastPuts("Program PLL N\n");   
    // write (reads-back previus value)
    testdataout = dphy_program_testcode(dphy_sel, 0x17, N);
        
    vcsFastPuts("Program PLL M LSB\n");
    data  = (u8)(M) & 0x1F;
    testdataout = dphy_program_testcode(dphy_sel, 0x18, data);
        
    vcsFastPuts("Program PLL M MSB\n");
    data  = 0x80 | (u8)((M&(0xF<<5))>>5);
    testdataout = dphy_program_testcode(dphy_sel, 0x18, data);


    // read-back N 
    testdataout = dphy_read_testcode(dphy_sel, 0x17);
    unitTestCompare((u32)N, (u32)testdataout);

    // read-back M LSB 
    data  = (u8)(M) & 0x1F;
    testdataout = dphy_program_testcode(dphy_sel, 0x18, data);
    unitTestCompare((u32)(data), (u32)testdataout);

    // read-back M MSB 
    data  = 0x80 | (u8)((M&(0xF<<5))>>5);
    testdataout = dphy_program_testcode(dphy_sel, 0x18, data);
    unitTestCompare((u32)(data&0x7F), (u32)testdataout);

    vcsFastPuts("Program PLL VCO RANGE AND VCO CAP tetscode 0x10\n");
    //data  = (vcorange << 3)|(vcocap   << 1);
    // vcorange default (as set by hsfreqrange) 
    // vcocap - defined by user                 
    // bias curernt - internal
    data  = (0x0<<7)|(0x0<<3)|(vcocap<<1)|0x0;
    testdataout = dphy_program_testcode(dphy_sel, 0x10, data);
        
    //vcsFastPuts("Activate PLL ICPCTRL tetscode 0x11\n");
    //testdataout = dphy_program_testcode(dphy_sel, 0x11, (u8)icpctrl);
        
    vcsFastPuts("Program PLL LPFCTRL and activate ICPCTRLtetscode 0x12\n");
    //       CP    |  LPF   | LPR resistor 
    //data = (0x1<<7)|(0x1<<6)|(lpfctrl&0x3F);
    // default as set by hsfreqrange
    data = 0x0;
    testdataout = dphy_program_testcode(dphy_sel, 0x12, data);

    return;
}


void dphy_init(u32 dphy_sel, u8 hsfreqrange)
{
    DrvTimerInit();
    
    u32 data;
    u32 dphy_sel_m;
    int i;

    vcsFastPuts("Initializing selected D-PHYs\n");
    
    data = GET_REG_WORD_VAL(MIPI_DPHY_INIT_CTRL_ADDR);
    
    dphy_sel_m = dphy_sel;
    
    // Set RSTZ and SHUTDOWNZ to '0'    
    data &= ~dphy_sel_m;
    
    dphy_sel_m = (dphy_sel_m << 8);
    data &= ~dphy_sel_m;
    
    dphy_sel_m = (dphy_sel_m << 8);
    data &= ~dphy_sel_m;
    
    dphy_sel_m = (dphy_sel_m << 8);
    data &= ~dphy_sel_m;
    
    SET_REG_WORD(MIPI_DPHY_INIT_CTRL_ADDR, data);
    
    // Generate pulse on TESTCLR
    dphy_testclr_pulse(dphy_sel);
    
    // Program hsfreqrange
    dphy_program_hsfreqrange(dphy_sel, hsfreqrange);
    
    // Deactivate RSTZ and SHUTDOWNZ
    data = GET_REG_WORD_VAL(MIPI_DPHY_INIT_CTRL_ADDR);
    
    // Deactivate SHUTDOWNZ    
    data |= (dphy_sel<<16)|dphy_sel;                 // SHUTDOWNZ + EN
    SET_REG_WORD(MIPI_DPHY_INIT_CTRL_ADDR, data);    // MIPI_DPHY_CTRL_OFFSET

    // Deactivate RSTZ 
    data |= (dphy_sel<<16)|(dphy_sel<<8)|dphy_sel;   // SHUTDOWNZ + RSTZ + EN
    SET_REG_WORD(MIPI_DPHY_INIT_CTRL_ADDR, data);    // MIPI_DPHY_CTRL_OFFSET

    return;
}



void dphy_init_pll(u32 dphy_sel, u8 hsfreqrange, u16 M, u8  N, u8 vcorange, u8 vcocap, u8 icpctrl, u8 lpfctrl)
{
    DrvTimerInit();
    
    u32 data;
    u32 dphy_sel_m;
    int i;

    vcsFastPuts("Initializing selected D-PHYs\n");
    
    data = GET_REG_WORD_VAL(MIPI_DPHY_INIT_CTRL_ADDR);
    
    dphy_sel_m = dphy_sel;
    
    // Set RSTZ and SHUTDOWNZ to '0'    
    data &= ~dphy_sel_m;
    
    dphy_sel_m = (dphy_sel_m << 8);
    data &= ~dphy_sel_m;
    
    dphy_sel_m = (dphy_sel_m << 8);
    data &= ~dphy_sel_m;
    
    dphy_sel_m = (dphy_sel_m << 8);
    data &= ~dphy_sel_m;
    
    SET_REG_WORD(MIPI_DPHY_INIT_CTRL_ADDR, data);
    
    // Generate pulse on TESTCLR
    dphy_testclr_pulse(dphy_sel);
    
    // Program hsfreqrange
    dphy_program_pll(dphy_sel, hsfreqrange, M, N, vcorange, vcocap, icpctrl, lpfctrl);
    
    // Deactivate RSTZ and SHUTDOWNZ
    data = GET_REG_WORD_VAL(MIPI_DPHY_INIT_CTRL_ADDR);
    
    // Deactivate SHUTDOWNZ    
    data |= (dphy_sel<<16)|dphy_sel;                 // SHUTDOWNZ + EN
    SET_REG_WORD(MIPI_DPHY_INIT_CTRL_ADDR, data);    // MIPI_DPHY_CTRL_OFFSET

    // Deactivate RSTZ 
    data |= (dphy_sel<<16)|(dphy_sel<<8)|dphy_sel;   // SHUTDOWNZ + RSTZ + EN
    SET_REG_WORD(MIPI_DPHY_INIT_CTRL_ADDR, data);    // MIPI_DPHY_CTRL_OFFSET

    return;
}






void dphy_wait_ready(u32 dphy_sel)
{
    u32 data;
    int i;
    int j;

    u32 phy_sel[6];
    u32 addr[6];
    u32 exp_data[6];

    vcsFastPuts("Wait ready on selected D-PHYs\n");
    
    phy_sel[0] = dphy_sel & (1 << 0);
    phy_sel[0] = (phy_sel[0] >> 0); 
     
    phy_sel[1] = dphy_sel & (1 << 1);
    phy_sel[1] = (phy_sel[1] >> 1);
     
    phy_sel[2] = dphy_sel & (1 << 2);
    phy_sel[2] = (phy_sel[2] >> 2);
    
    phy_sel[3] = dphy_sel & (1 << 3);
    phy_sel[3] = (phy_sel[3] >> 3);
     
    phy_sel[4] = dphy_sel & (1 << 4);
    phy_sel[4] = (phy_sel[4] >> 4);
    
    phy_sel[5] = dphy_sel & (1 << 5);
    phy_sel[5] = (phy_sel[5] >> 5);

    
    addr[0] = MIPI_DPHY_STAT_DPHY01_ADDR; 
    addr[1] = MIPI_DPHY_STAT_DPHY01_ADDR;
    addr[2] = MIPI_DPHY_STAT_DPHY23_ADDR; 
    addr[3] = MIPI_DPHY_STAT_DPHY23_ADDR;
    addr[4] = MIPI_DPHY_STAT_DPHY45_ADDR;
    addr[5] = MIPI_DPHY_STAT_DPHY45_ADDR;
   
    exp_data[0] = 0x00000010;   
    exp_data[1] = 0x00001000;
    exp_data[2] = 0x00000010;   
    exp_data[3] = 0x00001000;
    exp_data[4] = 0x00000010;   
    exp_data[5] = 0x00001000;
    
    for(i=0; i<6; i++)
    {
        if(phy_sel[i] == 0b1)
	{
	    do
	    {
	        for(j=0; j<10; j++)
		{
		    data = GET_REG_WORD_VAL(addr[i]);
		   // printMsgInt("addr: ", data);
		}
	    }
	    while((data & exp_data[i]) != exp_data[i]);
	}
    }
   
    return;
}

void configure_dphy(u32 dphy_sel, u8 hsfreqrange)
{
    vcsFastPuts("-- CONFIGURE DPHY(S) --\n");
    
    dphy_enable(dphy_sel);
    dphy_init(dphy_sel, hsfreqrange);
    dphy_wait_ready(dphy_sel);
    
    return;
}

void check_mipi_rx_hs_line_sync_err_cnt(u32 base_addr, u32 expected_err_cntr)
{
    u32 data = 0x00000000;
    
    data = GET_REG_WORD_VAL(base_addr + MIPI_RX_HS_LINE_SYNC_ERR_CNT_OFFSET);
    if(data != expected_err_cntr){
        vcsFastPuts("MIPI RX HS ERROR COUNTER ERROR: MIPI_RX_HS_LINE_SYNC_ERR_CNT not equal with expected value");
	errCntr++;
    }
	
    return;
}

void check_mipi_rx_hs_frm_sync_err_cnt(u32 base_addr, u32 expected_err_cntr)
{
    u32 data = 0x00000000;
    
    data = GET_REG_WORD_VAL(base_addr + MIPI_RX_HS_FRM_SYNC_ERR_CNT_OFFSET);
    if(data != expected_err_cntr){
        vcsFastPuts("MIPI RX HS ERROR COUNTER ERROR: MIPI_RX_HS_FRM_SYNC_ERR_CNT not equal with expected value");
	errCntr++;
    }
    
    return;
}

void check_mipi_rx_hs_frm_data_err_cnt(u32 base_addr, u32 expected_err_cntr)
{
    u32 data = 0x00000000;
    
    data = GET_REG_WORD_VAL(base_addr + MIPI_RX_HS_FRM_DATA_ERR_CNT_OFFSET);
    if(data != expected_err_cntr){
        vcsFastPuts("MIPI RX HS ERROR COUNTER ERROR: MIPI_RX_HS_FRM_DATA_ERR_CNT not equal with expected value");
	errCntr++;
    }
    
    return;
}

void check_mipi_rx_hs_crc_err_cnt(u32 base_addr, u32 expected_err_cntr)
{
    u32 data = 0x00000000;
    
    data = GET_REG_WORD_VAL(base_addr + MIPI_RX_HS_CRC_ERR_CNT_OFFSET);
    if(data != expected_err_cntr){
        vcsFastPuts("MIPI RX HS ERROR COUNTER ERROR: MIPI_RX_HS_CRC_ERR_CNT not equal with expected value");
	errCntr++;
    }
    
    return;
}

void check_mipi_rx_hs_fifo_full_err_cnt(u32 base_addr, u32 expected_err_cntr)
{
    u32 data = 0x00000000;
    
    data = GET_REG_WORD_VAL(base_addr + MIPI_RX_HS_FIFO_FULL_ERR_CNT_OFFSET);
    if(data != expected_err_cntr){
        vcsFastPuts("MIPI RX HS ERROR COUNTER ERROR: MIPI_RX_HS_FIFO_FULL_ERR_CNT not equal with expected value");
	errCntr++;
    }
    
    return;
}

void check_mipi_rx_hs_wc_err_cnt(u32 base_addr, u32 expected_err_cntr)
{
    u32 data = 0x00000000;
    
    data = GET_REG_WORD_VAL(base_addr + MIPI_RX_HS_WC_ERR_CNT_OFFSET);
    if(data != expected_err_cntr){
        vcsFastPuts("MIPI RX HS ERROR COUNTER ERROR: MIPI_RX_HS_WC_ERR_CNT not equal with expected value");
	errCntr++;
    }
    
    return;
}


void check_mipi_rx_hs_ecc_wrn_cnt(u32 base_addr, u32 expected_err_cntr)
{
    u32 data = 0x00000000;
    
    data = GET_REG_WORD_VAL(base_addr + MIPI_RX_HS_ECC_WRN_CNT_OFFSET);
    if(data != expected_err_cntr){
        vcsFastPuts("MIPI RX HS ERROR COUNTER ERROR: MIPI_RX_HS_ECC_WRN_CNT not equal with expected value");
	errCntr++;
    }
    
    return;
}

void check_mipi_rx_hs_ecc_err_cnt(u32 base_addr, u32 expected_err_cntr)
{
    u32 data = 0x00000000;
    
    data = GET_REG_WORD_VAL(base_addr + MIPI_RX_HS_ECC_ERR_CNT_OFFSET);
    if(data != expected_err_cntr){
        vcsFastPuts("MIPI RX HS ERROR COUNTER ERROR: MIPI_RX_HS_ECC_ERR_CNT not equal with expected value");
	errCntr++;
    }
    
    return;
}

void check_mipi_rx_hs_eid_err_cnt(u32 base_addr, u32 expected_err_cntr)
{
    u32 data = 0x00000000;
    
    data = GET_REG_WORD_VAL(base_addr + MIPI_RX_HS_EID_ERR_CNT_OFFSET);
    if(data != expected_err_cntr){
        vcsFastPuts("MIPI RX HS ERROR COUNTER ERROR: MIPI_RX_HS_EID_ERR_CNT not equal with expected value");
	errCntr++;
    }
    
    return;
}

void check_mipi_tx_hs_fifo_empty_err_cnt(u32 base_addr, u32 expected_err_cntr)
{
    u32 data = 0x00000000;
    
    data = GET_REG_WORD_VAL(base_addr + MIPI_TX_HS_FIFO_EMPTY_ERR_CNT_OFFSET);
    if(data != expected_err_cntr){
        vcsFastPuts("MIPI RX HS ERROR COUNTER ERROR: MIPI_TX_HS_FIFO_EMPTY_ERR_CNT not equal with expected value");
	errCntr++;
    }
    
    return;
}

void mipi_check_no_error(u32 base_addr, u32 expected_err_cntr)
{
    check_mipi_rx_hs_line_sync_err_cnt(base_addr, expected_err_cntr);
    check_mipi_rx_hs_frm_sync_err_cnt(base_addr, expected_err_cntr);
    check_mipi_rx_hs_frm_data_err_cnt(base_addr, expected_err_cntr);
     check_mipi_rx_hs_crc_err_cnt(base_addr, expected_err_cntr);
    check_mipi_rx_hs_fifo_full_err_cnt(base_addr, expected_err_cntr);
    check_mipi_rx_hs_wc_err_cnt(base_addr, expected_err_cntr);
   
    check_mipi_rx_hs_ecc_wrn_cnt(base_addr, expected_err_cntr);
   
    check_mipi_rx_hs_ecc_err_cnt(base_addr, expected_err_cntr);
    check_mipi_rx_hs_eid_err_cnt(base_addr, expected_err_cntr);
    check_mipi_tx_hs_fifo_empty_err_cnt(base_addr, expected_err_cntr);
    
    unitTestCompare(errCntr ,0x00000000);
    if(errCntr != 0x00000000)
        vcsFastPuts("Error counter is not zero!");
    
    return;
}
