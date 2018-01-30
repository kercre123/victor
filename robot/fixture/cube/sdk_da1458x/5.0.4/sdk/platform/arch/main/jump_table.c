/**
 ****************************************************************************************
 *
 * @file jump_table.c
 *
 * @brief Jump table that holds function pointers and veriables used in ROM code.
 *
 * Copyright (C) 2012. Dialog Semiconductor Ltd, unpublished work. This computer 
 * program includes Confidential, Proprietary Information and is a Trade Secret of 
 * Dialog Semiconductor Ltd.  All use, disclosure, and/or reproduction is prohibited 
 * unless authorized in writing. All Rights Reserved.
 *
 * <bluetooth.support@diasemi.com> and contributors.
 *
 ****************************************************************************************
 */

#include "rwip_config.h"     // RW SW configuration
#include <string.h>          // for mem* functions
#include "co_version.h"
#include "rwip.h"            // RW definitions
#include "arch.h"            // Platform architecture definition
#include "em_map_ble.h"

#if (BLE_EMB_PRESENT)
#include "rwble.h"           // rwble definitions
#endif //BLE_EMB_PRESENT

#if (BLE_HOST_PRESENT)
#include "rwble_hl.h"        // BLE HL definitions
#include "gapc.h"
#include "smpc.h"
#include "gattc.h"
#include "attc.h"
#include "atts.h"
#include "l2cc.h"
#endif //BLE_HOST_PRESENT

#include "lld_sleep.h"       // definitions for sleep mode
#include "llc.h"
#include "plf.h"             // platform definition
#include "rf.h"              // RF definitions

#if (GTL_ITF)
#include "gtl.h"
#endif //GTL_ITF

#if (HCIC_ITF)
#include "hcic.h"            // HCI definition
#endif //HCIC_ITF

#include "ke.h"              // kernel definition
#include "ke_event.h"        // kernel event
#include "ke_timer.h"        // definitions for timer
#include "ke_mem.h"          // kernel memory manager
#include "dbg.h"             // debug definition


#if ((BLE_APP_PRESENT) || ((BLE_HOST_PRESENT && (!GTL_ITF))))
#include "app.h"
#endif //BLE_APP_PRESENT

#include "nvds.h"
#include "pll_vcocal_lut.h"  // struct LUT_CFG_struct declaration

/*
 * forward declarations
 ******************************/
void main_func(void);
void rf_init_func(void);
void prf_init_func(void);
void uart_init_func(void);
void uart_flow_on_func(void);
void uart_flow_off_func(void);
void uart_finish_transfers_func(void);
void uart_read_func(void);
void uart_write_func(void);
void uart_write_ext_wkup_func(void);
void UART_Handler_func(void);
void lld_sleep_compensate_func_patched(void);
void lld_sleep_init_func(void);
void hci_tx_done_func(void);
void hci_enter_sleep_func(void);
void rf_reinit_func(void);

#if !(WLAN_COEX_ENABLED)
void lld_evt_init_func(bool reset);
#else
void wlan_coex_lld_evt_init_func(bool reset);
#endif

void ke_timer_init_func(void);
void ke_task_init_func(void);
void gtl_eif_init_func(void);
void llm_encryption_done_func(void);
uint8_t nvds_get_func(uint8_t, nvds_tag_len_t*, uint8_t*);
const struct rwip_eif_api* rwip_eif_get_func(uint8_t type);
void prf_cleanup_func(uint8_t conidx, uint16_t conhdl, uint8_t reason);
void enable_BLE_core_irq_func(void);
void lld_init_func(bool reset);
void gtl_hci_rx_header_func(void);
void gtl_hci_rx_payload_func(void);
uint8_t * gtl_hci_tx_evt_func(uint8_t * length);
void GPADC_init_func(void);
uint16_t meas_precharge_freq_func(uint8_t  calcap);
int8_t  check_pll_lock_func(uint16_t freq, uint8_t  mode_flag);
uint8_t  update_calcap_min_channel_func(uint8_t  calcap_idx, int8_t  num_runs, int8_t  firstrun_flag);
uint8_t  update_calcap_max_channel_func(uint8_t  calcap_idx, int8_t  num_runs, int8_t  firstrun_flag);
void write_one_SW_LUT_entry_func(uint8_t  index, uint8_t  calcap) ;
void write_HW_LUT_func(uint8_t * calcap_tbl) ;
void clear_HW_LUT_func(void);
uint8_t update_LUT_func(void);
void save_configure_restore_func(uint16_t * save_regs_ptr, int8_t mode);
uint8_t update_calcap_ranges_func(uint8_t rt_flg);
void find_initial_calcap_ranges_func(void);
uint8_t pll_vcocal_LUT_InitUpdate_func(uint8_t mode);
void set_rf_cal_cap_func(uint16_t cn);	
void enable_rf_diag_irq_func(void);
bool modulationGainCalibration_func(void);
bool DCoffsetCalibration_func(void);
bool IffCalibration_func(void);
bool rf_calibration_func(void);
uint16_t get_rc16m_count_func(void);
void lld_test_stop_func(struct lld_evt_tag *evt);
void set_gauss_modgain_func(uint16_t cn);
struct lld_evt_tag * lld_test_mode_tx_func(struct co_buf_tx_node *txdesc, uint8_t tx_freq);
struct lld_evt_tag * lld_test_mode_rx_func(uint8_t rx_freq);
#if PRODUCTION_TEST
void prodtest_platform_reset_func(uint32_t error);
#endif

#if GTL_SPI_SUPPORTED
extern const struct rwip_eif_api* rwip_eif_get_func_spi(uint8_t type);
#endif

// Coarse Calibration configuration
const struct LUT_CFG_struct LUT_CFG =
{
        /*.HW_LUT_MODE               =*/ 1,                     // 1: HW LUT, 0: SW LUT
        /*.RX_HSI_ENABLED            =*/ 1,
        /*.PA_PULLING_OFFSET         =*/ 0,
        /*.NR_CCUPD_1ST              =*/ 10,
        /*.NR_CCUPD_REST             =*/ 4,
        /*.NR_CCUPD_OL               =*/ 40,
        /*.BLE_BAND_MARGIN           =*/ 10,                    // in MHz
        /*.EST_HALF_OVERLAP          =*/ 4,                     // in MHz
        /*.REQUIRED_CHAN_OVERLAP     =*/ 2,                     // At least 2 channels should be synthesizable by two adjacent calcaps (both of them)
        /*.PLL_LOCK_VTUNE_NUMAVGPOW  =*/ 3,                     // So 2^3=8 samples averaging
        /*.PLL_LOCK_VTUNE_LIMIT_LO   =*/ (1024*200/1200),       // Min acceptable Vtune = 0200mV
        /*.PLL_LOCK_VTUNE_LIMIT_HI   =*/ (1024*1000/1200),      // Max acceptable Vtune = 1000mV
        /*.PLL_LOCK_VTUNE_P2PVAR     =*/ (1024*50/1200),        // Vtune has to be stable within 50mV
        /*.PLL_LOCK_TIMING           =*/ 416,                   // 416*62.5nsec=26usec
        /*.VCO_CALCNT_STARTVAL       =*/ 0xFFFF,                // Just in case it is modified by the Metal Fixes
        /*.VCO_CALCNT_TIMEOUT        =*/ 300,                   // Just in case the while loops lock in meas_precharge_freq()
};

uint32_t rwip_heap_non_ret[RWIP_CALC_HEAP_LEN(RWIP_HEAP_NON_RET_SIZE_JT)]  __attribute__((section("heap_mem_area_not_ret"), zero_init));

#ifndef __DA14581__
uint32_t rwip_heap_env_ret[RWIP_CALC_HEAP_LEN(RWIP_HEAP_ENV_SIZE_JT)]  	__attribute__((section("heap_env_area"), zero_init));
uint32_t rwip_heap_msg_ret[RWIP_CALC_HEAP_LEN(RWIP_HEAP_MSG_SIZE_JT)]  	__attribute__((section("heap_msg_area"), zero_init));
uint32_t rwip_heap_db_ret[RWIP_CALC_HEAP_LEN(RWIP_HEAP_DB_SIZE_JT)]  	__attribute__((section("heap_db_area"), zero_init));
#else
uint32_t rwip_heap_msg_ret[RWIP_CALC_HEAP_LEN(RWIP_HEAP_MSG_SIZE_JT)]  	__attribute__((section("heap_msg_area"),zero_init));
#if (BLE_CONNECTION_MAX_USER <= 4) 
uint32_t rwip_heap_env_ret[RWIP_CALC_HEAP_LEN(RWIP_HEAP_ENV_SIZE_JT)]  	__attribute__((section("heap_env_area"),zero_init));
uint32_t rwip_heap_db_ret[RWIP_CALC_HEAP_LEN(RWIP_HEAP_DB_SIZE_JT)]  	__attribute__((section("heap_db_area"),zero_init));
#else
uint32_t rwip_heap_env_ret[RWIP_CALC_HEAP_LEN(RWIP_HEAP_ENV_SIZE_JT)]  	__attribute__((section("heap_mem_area_sysram"),zero_init));
uint32_t rwip_heap_db_ret[RWIP_CALC_HEAP_LEN(RWIP_HEAP_DB_SIZE_JT)]  	__attribute__((section("heap_mem_area_sysram"),zero_init));
#endif
#endif

void dummyf() {}


#ifndef __DA14581__
const uint32_t* const jump_table_base[88] __attribute__((section("jump_table_mem_area"))) =
#else
const uint32_t* const jump_table_base[92] __attribute__((section("jump_table_mem_area"))) =
#endif
        {
#if (BLE_APP_PRESENT)
                (const uint32_t*) TASK_APP,                  
#else           
                (const uint32_t*) TASK_GTL,
#endif
                (const uint32_t*) main_func,                            //  1. main_func
                (const uint32_t*) rf_init_func,                         //  2. rf_init_func
#if (BLE_SERVER_PRF || BLE_CLIENT_PRF)
                (const uint32_t*) prf_init_func,                        //  3. prf_init_func
#else
                (const uint32_t*) dummyf,
#endif            
                0,                                                      //  4. calibrate_rc32KHz_func
                0,                                                      //  5. calibrate_RF_func
                (const uint32_t*) uart_init_func,                       //  6. uart_init_func
                (const uint32_t*) uart_flow_on_func,                    //  7. uart_flow_on_func
                (const uint32_t*) uart_flow_off_func,                   //  8. uart_flow_off_func              
                (const uint32_t*) uart_finish_transfers_func,           //  9. uart_finish_transfers_func
                (const uint32_t*) uart_read_func,                       // 10. uart_read_func
#if (WKUP_EXT_PROCESSOR)
                (const uint32_t*) uart_write_ext_wkup_func,             // 11. uart_write_ext_wkup_func
                    
#else
                (const uint32_t*) uart_write_func,                      // 11. uart_write_func
#endif                
                (const uint32_t*) UART_Handler_func,                    // 12. UART_Handler_func
                (const uint32_t*) lld_sleep_compensate_func_patched,    // 13. correct BLE counters after wakeUp
                (const uint32_t*) lld_sleep_init_func,                  // 14. init sleep
                (const uint32_t*) lld_sleep_us_2_lpcycles_sel_func,     // 15. calc us to lp
                (const uint32_t*) lld_sleep_lpcycles_2_us_sel_func,     // 16. calc lp to us
#if (BLE_HOST_PRESENT)								
                0,                                                      // 17. hci tx done
                0,                                                      // 18. hci allows sleep
#else
                (const uint32_t*) hci_tx_done_func,                     // 17. hci tx done
                (const uint32_t*) hci_enter_sleep_func,                 // 18. hci allows sleep
#endif								
                (const uint32_t*) TASK_GTL, //TASK_APP_SEC              // 19. app task          this entry is for the security app 
                (const uint32_t*) &rwip_heap_non_ret[0],                // 20. rwip_heap_non_ret_pos
                (const uint32_t*) RWIP_HEAP_NON_RET_SIZE_JT,            // 21. rwip_heap_non_ret_size
                (const uint32_t*) &rwip_heap_env_ret[0],                // 22. rwip_heap_env_pos
                (const uint32_t*) RWIP_HEAP_ENV_SIZE_JT,                // 23. rwip_heap_env_size
                (const uint32_t*) &rwip_heap_db_ret[0],                 // 24. rwip_heap_db_pos
                (const uint32_t*) RWIP_HEAP_DB_SIZE_JT,                 // 25. rwip_heap_db_size
                (const uint32_t*) &rwip_heap_msg_ret[0],                // 26. rwip_heap_msg_pos
                (const uint32_t*) RWIP_HEAP_MSG_SIZE_JT,                // 27. rwip_heap_msg_size	
				(const uint32_t*) EM_BLE_ET_OFFSET,                     // 28. offset_em_et		         
				(const uint32_t*) EM_BLE_FT_OFFSET,                     // 29. offset_em_ft	         
				(const uint32_t*) EM_BLE_ENC_PLAIN_OFFSET,              // 30. offset_em_enc_plain	
				(const uint32_t*) EM_BLE_ENC_CIPHER_OFFSET,             // 31. offset_em_enc_cipher	                
#pragma diag_suppress 170
				(const uint32_t*) EM_BLE_CS_OFFSET,                     // 32. offset_em_cs	                 
				(const uint32_t*) EM_BLE_WPB_OFFSET,                    // 33. offset_em_wpb
                (const uint32_t*) EM_BLE_WPV_OFFSET,                    // 34. offset_em_wpv                
				(const uint32_t*) EM_BLE_CNXADD_OFFSET,                 // 35. offset_em_cnxadd	
                (const uint32_t*) EM_BLE_TXE_OFFSET,                    // 36. offset_em_txe
#pragma diag_warning 170
				(const uint32_t*) EM_BLE_TX_OFFSET,                     // 37. offset_em_tx
				(const uint32_t*) EM_BLE_RX_OFFSET,                     // 38. offset_em_rx
				(const uint32_t*) BLE_CONNECTION_MAX_USER,              // 39. nb_links_user
				(const uint32_t*) 5000,                                 // 40. sleep_wake_up_delay
				(const uint32_t*) rf_reinit_func,                       // 41. rf_reinit_func
#if (WLAN_COEX_ENABLED)
                (const uint32_t*) wlan_coex_lld_evt_init_func,          // 42. lld_evt_init
#else
				(const uint32_t*) lld_evt_init_func,                    // 42. lld_evt_init
#endif
				(const uint32_t*) gtl_eif_init_func,                    // 43. gtl_eif_init
				(const uint32_t*) ke_task_init_func,                    // 44. ke_task_init
				(const uint32_t*) ke_timer_init_func,                   // 45. ke_timer_init
				(const uint32_t*) llm_encryption_done_func,             // 46. llm_encryption_done
				(const uint32_t*) custom_nvds_get_func,                 // 47. nvds_get
				(const uint32_t*) 3,                                    // 48. lld_evt_prog_latency (MUST BE > 2!!!)
#if (GTL_SPI_SUPPORTED)
                (const uint32_t*) rwip_eif_get_func_spi,                // 49. rwip_eif_get
#else				
				(const uint32_t*) rwip_eif_get_func,                    // 49. rwip_eif_get
#endif				
#if (BLE_SERVER_PRF || BLE_CLIENT_PRF)   
				(const uint32_t*) prf_cleanup_func,                     // 50.	prf_cleanup
#else
                (const uint32_t*) dummyf,
#endif
#if (PRODUCTION_TEST)
				(const uint32_t*) 1,                                    // 51. BLE rx irq threshold
#else
				(const uint32_t*) (BLE_RX_BUFFER_CNT/2),                // 51. BLE rx irq threshold
#endif
				(const uint32_t*) lld_init_func,                        // 52. lld_init
				(const uint32_t*) enable_BLE_core_irq_func,             // 53. enable_BLE_core_irq
#if (PRODUCTION_TEST)
				(const uint32_t*) prodtest_platform_reset_func,         // 54. platform reset
#else
				(const uint32_t*) wrap_platform_reset,                  // 54. platform reset
#endif
				(const uint32_t*) gtl_hci_rx_header_func,               // 55. gtl_hci_rx_header
				(const uint32_t*) gtl_hci_rx_payload_func,              // 56. gtl_hci_rx_payload
				(const uint32_t*) gtl_hci_tx_evt_func,                  // 57. gtl_hci_tx_evt
#if (LUT_PATCH_ENABLED)
                (const uint32_t*) GPADC_init_func,                      // 58. GPADC_init
                (const uint32_t*) meas_precharge_freq_func,             // 59. meas_precharge_freq
                (const uint32_t*) &LUT_CFG,                             // 60. LUT_CFG
                (const uint32_t*) check_pll_lock_func,                  // 61. check_pll_lock
                (const uint32_t*) update_calcap_min_channel_func,       // 62. update_calcap_min_channel
                (const uint32_t*) update_calcap_max_channel_func,       // 63. update_calcap_max_channel
                (const uint32_t*) write_one_SW_LUT_entry_func,          // 64. write_one_SW_LUT_entry
                (const uint32_t*) write_HW_LUT_func,                    // 65. write_HW_LUT
                (const uint32_t*) clear_HW_LUT_func,                    // 66. clear_HW_LUT
                (const uint32_t*) update_LUT_func,                      // 67. update_LUT
                (const uint32_t*) save_configure_restore_func,          // 68. save_configure_restore
                (const uint32_t*) update_calcap_ranges_func,            // 69. update_calcap_ranges
                (const uint32_t*) find_initial_calcap_ranges_func,      // 70. find_initial_calcap_ranges
                (const uint32_t*) pll_vcocal_LUT_InitUpdate_func,       // 71. pll_vcocal_LUT_InitUpdate
                (const uint32_t*) set_rf_cal_cap_func,                  // 72. set_rf_cal_cap
#else
                0,                                                      // 58.	
                0,                                                      // 59.
                0,                                                      // 60.
                0,                                                      // 61.	
                0,                                                      // 62.
                0,                                                      // 63.
                0,                                                      // 64.
                0,                                                      // 65.
                0,                                                      // 66.
                0,                                                      // 67.
                0,                                                      // 68.
                0,                                                      // 69.
                0,                                                      // 70.
                0,                                                      // 71.
                0,                                                      // 72.
#endif
                (const uint32_t*) enable_rf_diag_irq_func,              // 73. enable_rf_diag_irq
#if (RADIO_580)
                (const uint32_t*) modulationGainCalibration_func,       // 74. modulationGainCalibration
                (const uint32_t*) DCoffsetCalibration_func,             // 75. DCoffsetCalibration
                (const uint32_t*) IffCalibration_func,                  // 76. IffCalibration
                (const uint32_t*) rf_calibration_func,                  // 77. rf_calibration
                (const uint32_t*) get_rc16m_count_func,                 // 78. get_rc16m_count
                (const uint32_t*) set_gauss_modgain_func,               // 79. set_gauss_modgain
#else
                0,                                                      // 74.
                0,                                                      // 75.
                0,                                                      // 76.
                0,                                                      // 77.
                0,                                                      // 78.
                0,                                                      // 79.
#endif
                (const uint32_t*) lld_test_stop_func,                   // 80. lld_test_stop
                (const uint32_t*) lld_test_mode_tx_func,                // 81. lld_test_mode_tx
                (const uint32_t*) lld_test_mode_rx_func,                // 82. lld_test_mode_rx
                (const uint32_t*) 16,                                   // 83. rxwinsize
                (const uint32_t*) MAX_SLEEP_DURATION_PERIODIC_WAKEUP,   // 84.
                (const uint32_t*) MAX_SLEEP_DURATION_EXTERNAL_WAKEUP,   // 85.
#ifdef __DA14581__
				#if BLE_HOST_PRESENT
				(const uint32_t*) TASK_GTL,		                        // 86.
				(const uint32_t*) TASK_GAPM,	                        // 87.
				#else // BLE_HOST_PRESENT
				(const uint32_t*) TASK_HCI,		                        // 86.
				(const uint32_t*) TASK_HCI,		                        // 87.
				#endif
				//(const uint32_t*) KE_TASK_SIZE+KE_USER_TASK_SIZE, 	// 88.
                (const uint32_t*) (23) 	                                // 88. Maximum number of available tasks
                 
#endif
        };


        
