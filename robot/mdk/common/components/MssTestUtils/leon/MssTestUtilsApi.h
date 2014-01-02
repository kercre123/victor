///
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved
///            For License Warranty see: common/license.txt
///
/// @brief     Set of functions to allow test case interaction with the VCS Test Environment 
///
///  
///

#ifndef _MSS_TEST_UTILS_API_H_
#define _MSS_TEST_UTILS_API_H_

// 1: Includes
// ----------------------------------------------------------------------------

#include "MssTestUtilsDefines.h"
#include "DrvRegUtilsDefines.h"

// 2:  Exported Global Data (generally better to avoid)
// ----------------------------------------------------------------------------


// 3:  Exported Functions (non-inline)
// ----------------------------------------------------------------------------
    u8 dphy_read_testcode(u32 dphy_sel, u8 testcode);
    
    u8   dphy_program_testcode(u32 dphy_sel, u8 testcode, u8 testdatain);
    
    void dphy_enable(u32 dphy_sel);
    
    void dphy_disable(u32 dphy_sel);
    
    void dphy_testclr_pulse(u32 dphy_sel);
    
    void dphy_program_hsfreqrange(u32 dphy_sel, u8 hsfreqrange);

    void dphy_program_pll(u32 dphy_sel, u8 hsfreqrange, u16 M, u8  N, u8 vcorange, u8 vcocap, u8 icpctrl, u8 lpfctrl);
    
    void dphy_init(u32 dphy_sel, u8 hsfreqrange);

    void dphy_init_pll(u32 dphy_sel, u8 hsfreqrange, u16 M, u8  N, u8 vcorange, u8 vcocap, u8 icpctrl, u8 lpfctrl);
    
    void dphy_wait_ready(u32 dphy_sel);
    
    void configure_dphy(u32 dphy_sel, u8 hsfreqrange);

    void check_mipi_rx_hs_line_sync_err_cnt(u32 base_addr, u32 expected_err_cntr);
    
    void check_mipi_rx_hs_frm_sync_err_cnt(u32 base_addr, u32 expected_err_cntr);
    
    void check_mipi_rx_hs_frm_data_err_cnt(u32 base_addr, u32 expected_err_cntr);
    
    void check_mipi_rx_hs_crc_err_cnt(u32 base_addr, u32 expected_err_cntr);
    
    void check_mipi_rx_hs_fifo_full_err_cnt(u32 base_addr, u32 expected_err_cntr);
    
    void check_mipi_rx_hs_wc_err_cnt(u32 base_addr, u32 expected_err_cntr);
    
    void check_mipi_rx_hs_ecc_wrn_cnt(u32 base_addr, u32 expected_err_cntr);
    
    void check_mipi_rx_hs_ecc_err_cnt(u32 base_addr, u32 expected_err_cntr);
    
    void check_mipi_rx_hs_eid_err_cnt(u32 base_addr, u32 expected_err_cntr);
    
    void check_mipi_tx_hs_fifo_empty_err_cnt(u32 base_addr, u32 expected_err_cntr);

    void mipi_check_no_error(u32 base_addr, u32 expected_err_cntr);



#endif // _MSS_TEST_UTILS_API_H_
