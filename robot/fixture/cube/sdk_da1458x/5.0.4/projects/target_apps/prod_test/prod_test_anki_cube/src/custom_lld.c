/**
 ****************************************************************************************
 *
 * @file lld.c
 *
 * @brief Definition of the functions used by the logical link driver
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 *
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @addtogroup LLD
 * @{
 ****************************************************************************************
 */


/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "co_utils.h"
#include "co_endian.h"
#include "ke_event.h"
#include "llm.h"
#include "llm_util.h"
#include "llc_cntl.h"
#include "lld.h"
#include "lld_evt.h"
#include "lld_data.h"

#if (DEEP_SLEEP)
    #if (RW_BLE_SUPPORT)
    #include "lld_sleep.h"
    #endif //RW_BLE_SUPPORT
#endif //DEEP_SLEEP

#if (RW_BLE_WLAN_COEX)
#include "lld_wlcoex.h"         // 802.11 coexistence definitions
    #if (RW_BLE_WLAN_COEX_TEST)
    #include "mwsgen.h"                // MWS generator definitions
    #endif //RW_BT_WLAN_COEX_TEST
#endif // RW_BLE_WLAN_COEX

#include "reg_blecore.h"
#include "reg_ble_em_wpb.h"
#include "reg_ble_em_wpv.h"
#include "reg_ble_em_cs.h"
#include "reg_ble_em_et.h"
#include "rwip.h"

#if (NVDS_SUPPORT)
#include "nvds.h"         // NVDS definitions
#endif // NVDS_SUPPORT



/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

/// LLD task descriptor. LLD is a dummy task that is defined just for its message API

extern uint16_t ret_winsize_var __attribute__((section("exchange_mem_case1")));

/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */

// #if (RW_DM_SUPPORT)
// /**
//  ****************************************************************************************
//  * @brief Initialization of the DM priority
//  *
//  * @param[in] step    Increment of priority
//  * @param[in] init    Default priority
//  *
//  ****************************************************************************************
//  */
// __INLINE void lld_prio_init(int idx, uint8_t step, uint8_t init)
// {
//     ble_dmpriocntl_pack(idx, step, init, 0, init);
// }
// #endif // RW_DM_SUPPORT

#if 0
void lld_init(bool reset)
{
	  typedef void (*my_function)(bool);
    my_function PtrFunc;
    PtrFunc = (my_function)(jump_table_struct[lld_init_pos]);
    PtrFunc(reset);

	
}

void lld_init_func(bool reset)
{
    int i;
    struct bd_addr bd_addr;
    #if (NVDS_SUPPORT)
    uint8_t length = NVDS_LEN_BD_ADDRESS;
    #endif // NVDS_SUPPORT

    // Create LLD task. LLD is a dummy task that is defined just for its message API
    ke_task_create(TASK_LLD, &TASK_DESC_LLD);

    #if (RW_BLE_SUPPORT)
    // Initialize buffers
    co_buf_init();
    #endif //RW_BLE_SUPPORT

		ret_winsize_var = (uint16_t)jump_table_struct[ret_rxwinsize_pos];
	
    // Set default window size
    SetBits32(BLE_RWBTLECNTL_REG, RXWINSZDEF,LLD_EVT_DEFAULT_RX_WIN_SIZE); // ble_rxwinszdef_setf(LLD_EVT_DEFAULT_RX_WIN_SIZE);

    #if (!BLE12_HW)
    // Enable optimized connection mechanism
    SetBits32(BLE_RWBTLECNTL_REG, TXWINOFFSEL,1); // ble_txwinoffsel_setf(1);

    // Enable RX ring mechanism
    SetBits32(BLE_RWBTLECNTL_REG, RXDESCPTRSEL,1); // ble_rxdescptrsel_setf(1);
    #endif // (!BLE12_HW)

    // Reduce prefetch time to 150us
    SetWord16(BLE_TIMGENCNTL_REG, 150);  // ble_prefetch_time_setf(150);

    // All interrupts enabled except CSNT and SLP
    // ble_intcntl_set(BLE_RADIOCNTLINTMSK_BIT | BLE_RXINTMSK_BIT | BLE_EVENTINTMSK_BIT |
               // BLE_CRYPTINTMSK_BIT | BLE_ERRORINTMSK_BIT);

// 32 bit access will also clear the high 16-bits (INTCSCNTL field)
    SetWord32(BLE_INTCNTL_REG, RADIOCNTLINTMSK | RXINTMSK | EVENTINTMSK | CRYPTINTMSK | ERRORINTMSK | SLPINTMSK);

    //SetWord16(BLE_INTCNTL_REG, RADIOCNTLINTMSK | RXINTMSK | EVENTINTMSK | CRYPTINTMSK | ERRORINTMSK | SLPINTMSK);

    NVIC_EnableIRQ(BLE_SLP_IRQn);     
    NVIC_EnableIRQ(BLE_EVENT_IRQn); 
    NVIC_EnableIRQ(BLE_RF_DIAG_IRQn);
    NVIC_EnableIRQ(BLE_RX_IRQn);
    NVIC_EnableIRQ(BLE_CRYPT_IRQn);
    NVIC_EnableIRQ(BLE_FINETGTIM_IRQn);	
    NVIC_EnableIRQ(BLE_GROSSTGTIM_IRQn);	
    NVIC_EnableIRQ(BLE_WAKEUP_LP_IRQn);     	
    
    NVIC_SetPriority(BLE_SLP_IRQn,1);         
    NVIC_SetPriority(BLE_EVENT_IRQn,1);
    NVIC_SetPriority(BLE_RF_DIAG_IRQn,1);
    NVIC_SetPriority(BLE_RX_IRQn,1);
    NVIC_SetPriority(BLE_CRYPT_IRQn,1);
    NVIC_SetPriority(BLE_FINETGTIM_IRQn,1);	
    NVIC_SetPriority(BLE_GROSSTGTIM_IRQn,1);	

    NVIC_SetPriority(BLE_CSCNT_IRQn,1); 
    NVIC_EnableIRQ(BLE_CSCNT_IRQn);
	SetBits32(BLE_INTCNTL_REG, INTCSCNTL, 0xFFFF);
	SetBits32(BLE_INTCNTL_REG, CSCNTDEVMSK, 1);
	SetBits32(BLE_INTCNTL_REG, CSCNTINTMSK, 1);

    SetBits32(BLE_RWBTLECNTL_REG, SN_DSB, 0); // ble_sn_dsb_setf(0);
    SetBits32(BLE_RWBTLECNTL_REG, NESN_DSB, 0); // ble_nesn_dsb_setf(0);nesndsb

    // Initialize BD address
    #if (NVDS_SUPPORT)
    if (nvds_get(NVDS_TAG_BD_ADDRESS, &length, (uint8_t *)&bd_addr) != NVDS_OK)
    #endif // NVDS_SUPPORT
    {
        memcpy(&bd_addr, &co_default_bdaddr, sizeof(co_default_bdaddr));
    }

    // Set the BDADDR from the configuration structure
    SetWord32(BLE_BDADDRL_REG, co_read32p(&bd_addr.addr[0])); // ble_bdaddrl_set(co_read32p(&bd_addr.addr[0]));
    SetWord16(BLE_BDADDRU_REG, co_read16p(&bd_addr.addr[4])); // ble_bdaddru_set(co_read16p(&bd_addr.addr[4]));

    #if (BLE12_HW)
    ble_advertfilt_en_setf(1);
    #else // (BLE12_HW)
    SetBits32(BLE_RWBTLECNTL_REG,ADVERRFILT_EN,1); // ble_adverrfilt_en_setf(1);
    #endif // (BLE12_HW)
    SetBits32(BLE_ADVCHMAP_REG, ADVCHMAP, BLE_ADVCHMAP_RESET); // ble_advchmap_setf(BLE_ADVCHMAP_RESET);

    // Set white list addresses and count
    SetWord16(BLE_WLPUBADDPTR_REG, REG_BLE_EM_WPB_ADDR_GET(0)); // ble_wlpubaddptr_set(REG_BLE_EM_WPB_ADDR_GET(0));
    SetWord16(BLE_WLPRIVADDPTR_REG, REG_BLE_EM_WPV_ADDR_GET(0)); // ble_wlprivaddptr_set(REG_BLE_EM_WPV_ADDR_GET(0));
    SetBits32(BLE_WLNBDEV_REG, NBPUBDEV,  BLE_WHITELIST_MAX); // ble_wlnbdev_pack(BLE_WHITELIST_MAX, BLE_WHITELIST_MAX);
    SetBits32(BLE_WLNBDEV_REG, NBPRIVDEV, BLE_WHITELIST_MAX);

    // Enable dual-mode arbitration module
    #if (RW_DM_SUPPORT)
    ble_bleprioscharb_pack(1, BLE_ARB_MARGIN);
    #endif // RW_DM_SUPPORT

    // Initialize the advertising control structure
    ble_syncwl_set(LLD_ADV_HDL, 0xBED6);
    ble_syncwh_set(LLD_ADV_HDL, 0x8E89);

    // Initialize the crcinit in the CS
    ble_crcinit0_set(LLD_ADV_HDL, 0x5555);
    ble_crcinit1_setf(LLD_ADV_HDL,0x55);

    // Initialize the channel map in the CS
    ble_chmap0_set(LLD_ADV_HDL,0x0000);
    ble_chmap1_set(LLD_ADV_HDL,0x0000);
    ble_chmap2_set(LLD_ADV_HDL,0x0000);

    // Set the first RX descriptor pointer into the HW
    ble_currentrxdescptr_set(REG_BLE_EM_RX_ADDR_GET(co_buf_rx_current_get()));

    // Initialize the txrx cntl in the CS
    ble_txrxcntl_set(LLD_ADV_HDL, (LLD_RX_IRQ_THRES << BLE_RXTHR_LSB) | LLM_ADV_CHANNEL_TXPWR);

    // clear the exchange table
    for (i=0 ; i < 16; i++)
    {
        ble_extab_set(i, 0);
    }

    // Initialize the event scheduler
    lld_evt_init(reset);

    #if (DEEP_SLEEP)
    #if (RW_BLE_SUPPORT)
    // Initialize sleep manager
    lld_sleep_init();
    #endif //RW_BLE_SUPPORT
    #endif //DEEP_SLEEP

    #if (RW_BLE_WLAN_COEX)
    ble_dnabort_setf(LLD_ADV_HDL,0);
    ble_txbsy_en_setf(LLD_ADV_HDL,0);
    ble_rxbsy_en_setf(LLD_ADV_HDL,0);
    /* Enable WL coexistence by default                                                 */
    lld_wlcoex_set(BLECOEX_WLAN);
    #endif /* RW_BLE_WLAN_COEX */

    // Enable the BLE core
    ble_rwble_en_setf(1);
}
#endif

// void lld_reset(void)
// {
//     int i;

//     // Disable the BLE core
//     ble_rwble_en_setf(0);

//     #if (RW_BLE_SUPPORT)
//     // Reset the BLE state machines
//     ble_master_soft_rst_setf(1);
//     while(ble_master_soft_rst_getf());

//     // Reset the timing generator
//     ble_master_tgsoft_rst_setf(1);
//     while(ble_master_tgsoft_rst_getf());
//     #endif //RW_BLE_SUPPORT

//     // Clear the exchange table to prevent any further BLE processing
//     for (i=0; i < EM_BLE_EXCH_TABLE_LEN; i++)
//     {
//         ble_extab_set(i, 0);
//     }

//     // Disable all the BLE interrupts
//     ble_intcntl_set(0);

//     // And acknowledge any possible pending ones
//     ble_intack_clear(0xFFFFFFFF);
// }


// #if (BLE_CENTRAL || BLE_OBSERVER)
// struct lld_evt_tag *lld_scan_start(struct scanning_pdu_params *scan_par,
//                                    struct co_buf_tx_node *scan_req_pdu)
// {
//     // Check parameters
//     if ((scan_par->interval - scan_par->window) < LLD_EVT_PROG_LATENCY)
//         scan_par->window = scan_par->interval - LLD_EVT_PROG_LATENCY;

//     // Create an event to handle the scan
//     struct lld_evt_tag *evt = lld_evt_scan_create(LLD_ADV_HDL,
//                                                   scan_par->window,
//                                                   scan_par->interval/2,
//                                                   scan_par->interval,
//                                                   0);

//     // Update the control structure according to the parameters
//     ble_cntl_set(LLD_ADV_HDL, scan_req_pdu?LLD_ACTIVE_SCANNING:LLD_PASSIVE_SCANNING);
//     ble_hopcntl_set(LLD_ADV_HDL, BLE_FH_EN_BIT | 39);
//     ble_crcinit1_set(LLD_ADV_HDL, (scan_par->filterpolicy <<BLE_FILTER_POLICY_LSB) | 0x55);
//     ble_rxwincntl_set(LLD_ADV_HDL, BLE_RXWIDE_BIT | evt->duration);
//     ble_maxevtime_set(LLD_ADV_HDL, evt->duration);
//     #if (RW_DM_SUPPORT)
//     // Set the priority properties
//     lld_prio_init(LLD_ADV_HDL, BLE_SCAN_PRIO_INC, BLE_SCAN_PRIO_DEF);
//     #endif // RW_DM_SUPPORT

//     #if (RW_BLE_WLAN_COEX)
//     if(scan_req_pdu)
//     {
//         ble_pti_setf(LLD_ADV_HDL,BLEMPRIO_ACTSC);
//         if(lld_wlcoex_enable)
//         {
//             ble_dnabort_setf(LLD_ADV_HDL,1);
//             ble_txbsy_en_setf(LLD_ADV_HDL,1);
//             ble_rxbsy_en_setf(LLD_ADV_HDL,0);
//         }
//         #if (RW_BLE_WLAN_COEX_TEST)
//         if(lld_wlcoex_scenario & BLE_WLCOEX_TST_CONADV_ACTSC)
//         {
//             mwsgen_start();
//         }
//         #endif // RW_BLE_WLAN_COEX_TEST
//     }
//     else // passive scanning
//     {
//         ble_pti_setf(LLD_ADV_HDL,BLEMPRIO_PASSC);
//         if(lld_wlcoex_enable)
//         {
//             ble_dnabort_setf(LLD_ADV_HDL,0);
//             ble_txbsy_en_setf(LLD_ADV_HDL,0);
//             ble_rxbsy_en_setf(LLD_ADV_HDL,0);
//         }
//         #if (RW_BLE_WLAN_COEX_TEST)
//         if(lld_wlcoex_scenario & BLE_WLCOEX_TST_NCONADV_PASSC)
//         {
//             mwsgen_start();
//         }
//         #endif // RW_BLE_WLAN_COEX_TEST
//     }
//     #endif //RW_BLE_WLAN_COEX

//     // If active scanning is requested, prepare the scan request
//     if (scan_req_pdu)
//     {
//         // Push the scan request
//         lld_data_tx_push(evt, scan_req_pdu);

//         // Loop the scan request data
//         lld_data_tx_loop(evt);
//     }

//     return (evt);
// }

// void lld_scan_stop(struct lld_evt_tag *evt)
// {
//     // Delete the event associated with this handle
//     lld_evt_delete(evt, BLE_SCAN_ABORT_BIT);
// }

// #endif // BLE_CENTRAL || BLE_OBSERVER

// #endif //if 0


// #if (BLE_CENTRAL)
// struct lld_evt_tag *lld_con_start(struct llm_le_create_con_cmd const *con_par,
//                                   struct co_buf_tx_node *con_req_pdu,
//                                   uint16_t conhdl)
// {
//     uint16_t scan_win;
//     uint16_t ce_len;
//     uint16_t bw_req;
//     uint16_t int_min = con_par->con_intv_min*2;

//     // Scan window shall not be more than 6.25ms to match the transmit window size
//     // constraints (i.e. max transmit window should be the lesser between 10ms and
//     // interval - 1.25ms). Some margin is taken to have time to program the first
//     // connection event once connect request has been transmitted
//     if (con_par->scan_window > (int_min - (2 + LLD_EVT_PROG_LATENCY)))
//         scan_win = int_min - (2 + LLD_EVT_PROG_LATENCY);
//     else
//         scan_win = con_par->scan_window;

//     // Compute bandwidth requirement for this connection
//     if ((CO_ALIGN2_HI(con_par->ce_len_min)) > (CO_ALIGN2_HI((int_min - (2 + LLD_EVT_PROG_LATENCY)))))
//         ce_len = CO_ALIGN2_HI(int_min - (2 + LLD_EVT_PROG_LATENCY));
//     else
//         ce_len = CO_ALIGN2_HI(con_par->ce_len_min);

//     // Ensure CE len is not null
//     ce_len = (ce_len == 0)?2:ce_len;

//     // Keep the maximum value of the two
//     bw_req = co_max(scan_win, ce_len);
//     // Create an event to handle the connection
//     struct lld_evt_tag *evt = lld_evt_scan_create(LLD_ADV_HDL,
//                                                   bw_req,
//                                                   con_par->con_intv_min*2,
//                                                   con_par->con_intv_max*2,
//                                                   con_par->con_latency);
//     // Adjust the event duration parameters depending on what was allocated by LLDEVT
//     scan_win = co_min(scan_win, evt->duration);
//     ce_len = co_min(ce_len, evt->duration);
//     // Update the initiating control structure according to the parameters
//     ble_cntl_set(LLD_ADV_HDL, LLD_INITIATING);
//     ble_hopcntl_set(LLD_ADV_HDL, BLE_FH_EN_BIT | 39);
//     // Copy the advertiser address into the control structure
//     for (uint8_t i=0; i < (sizeof(struct bd_addr)/2); i++)
//     {
//         ble_adv_bd_addr_set(LLD_ADV_HDL, i, co_read16p(&con_par->peer_addr.addr[i * 2]));
//     }
//     // Copy the advertiser address type
//     ble_adv_bd_addr_type_setf(LLD_ADV_HDL, con_par->peer_addr_type);
//     // Copy the filtering policy
//     ble_crcinit1_set(LLD_ADV_HDL, (con_par->init_filt_policy<<BLE_FILTER_POLICY_LSB) | 0x55);
//     ble_rxwincntl_set(LLD_ADV_HDL, BLE_RXWIDE_BIT | scan_win);
//     ble_winoffset_set(LLD_ADV_HDL, evt->interval / 2 - 2);
//     ble_maxevtime_set(LLD_ADV_HDL, scan_win);
//     #if (RW_DM_SUPPORT)
//     // Set the priority properties
//     lld_prio_init(LLD_ADV_HDL, BLE_INIT_PRIO_INC, BLE_INIT_PRIO_DEF);
//     #endif // RW_DM_SUPPORT

//     // Update connection request fields
//     struct co_buf_tx_desc *txdesc = co_buf_tx_desc_get(con_req_pdu->idx);
//     struct llm_pdu_con_req_tx *data = (struct llm_pdu_con_req_tx *)txdesc->data;
//     data->interval = co_htobs(evt->interval) / 2;
//     data->winsize = 2;

//     // Push the connection request
//     lld_data_tx_push(evt, con_req_pdu);

//     // Update the connection control structure according to the parameters
//     ble_cntl_set(conhdl, LLD_MASTER_CONNECTED);
//     ble_hopcntl_set(conhdl, BLE_FH_EN_BIT | ((data->hop_sca & 0x1F) << BLE_HOP_INT_LSB));
//     ble_syncwl_set(conhdl, (data->aa.addr[1] << 8) | data->aa.addr[0]);
//     ble_syncwh_set(conhdl, (data->aa.addr[3] << 8) | data->aa.addr[2]);
//     ble_crcinit0_set(conhdl, (data->crcinit.crc[1] << 8) | data->crcinit.crc[0]);
//     ble_crcinit1_set(conhdl, data->crcinit.crc[2]);
//     ble_maxevtime_set(conhdl, 1);
//     ble_txrxcntl_set(conhdl, (LLD_RX_IRQ_THRES << BLE_RXTHR_LSB) | rwip_rf.txpwr_max);
//     ble_fcntoffset_set(conhdl, 0);
//     ble_chmap0_set(conhdl,co_read16(&data->chm.map[0]));
//     ble_chmap1_set(conhdl,co_read16(&data->chm.map[2]));
//     ble_chmap2_set(conhdl,
//            (uint16_t)(llm_util_check_map_validity(&data->chm.map[0],LE_CHNL_MAP_LEN) << 8)
//            | data->chm.map[4]);
//     #if (RW_DM_SUPPORT)
//     // Set the priority properties
//     lld_prio_init(conhdl, BLE_MCONNECT_PRIO_INC, BLE_MCONNECT_PRIO_DEF);
//     #endif // RW_DM_SUPPORT

//     #if (RW_BLE_WLAN_COEX)
//     ble_pti_setf(conhdl,BLEMPRIO_CONREQ);
//     if(lld_wlcoex_enable)
//     {
//         ble_dnabort_setf(LLD_ADV_HDL,1);
//         ble_rxbsy_en_setf(LLD_ADV_HDL,1);
//         ble_txbsy_en_setf(LLD_ADV_HDL,1);
//         ble_dnabort_setf(conhdl,1);
//         ble_rxbsy_en_setf(conhdl,1);
//         ble_txbsy_en_setf(conhdl,1);
//     }
//     #if (RW_BLE_WLAN_COEX_TEST)
//     if(lld_wlcoex_scenario & BLE_WLCOEX_TST_INITCONREQ)
//     {
//         mwsgen_start();
//     }
//     #endif // RW_BLE_WLAN_COEX_TEST
//     #endif // RW_BLE_WLAN_COEX

//     return (evt);
// }

// // already in rom
// #if 0

// void lld_move_to_master(struct lld_evt_tag *evt, uint16_t conhdl)
// {
//     // Move to master connection state
//     lld_evt_move_to_master(evt, conhdl);
//     #if (RW_BLE_WLAN_COEX)
//     if (lld_wlcoex_enable)
//     {
//         ble_dnabort_setf(conhdl,1);
//         ble_rxbsy_en_setf(conhdl,1);
//         ble_txbsy_en_setf(conhdl,1);
//         /* Update WL coexistence mailbox with this new information                  */
//         lld_wlcoex_connection_complete();
//     }
//     #endif // RW_BT_WLAN_COEX
// }

// void lld_con_update_req(struct lld_evt_tag *old_evt,
//                         struct llc_le_con_update_cmd const *param,
//                         struct llcp_con_up_req *param_pdu)
// {
//     struct lld_evt_update_tag upd_par;
//     struct lld_evt_tag *evt;

//     // Create an event to handle the connection update
//     evt = lld_evt_update_create(old_evt,
//                                 param->con_intv_min*2,
//                                 param->con_intv_max*2,
//                                 param->con_latency,
//                                 &upd_par);

//     // Update the fields of the connection update request
//     param_pdu->timeout = param->superv_to;
//     param_pdu->interv = evt->interval / 2;
//     param_pdu->latency = param->con_latency;
//     param_pdu->win_size = upd_par.win_size;
//     param_pdu->win_off = upd_par.win_offset;
//     param_pdu->instant = upd_par.instant;
// }

// uint16_t lld_ch_map_req(struct lld_evt_tag *evt)
// {
//     return (lld_evt_ch_map_update_req(evt));
// }

// #endif //if 0

// #endif // BLE_CENTRAL

// #if 0
// #if (BLE_PERIPHERAL || BLE_BROADCASTER)
// struct lld_evt_tag *lld_adv_start(struct advertising_pdu_params *adv_par,
//                                   struct co_buf_tx_node *adv_pdu,
//                                   struct co_buf_tx_node *scan_rsp_pdu,
//                                   uint8_t adv_pwr)
// {
//     uint8_t restart_pol;
//     uint32_t adv_int;

//     // Check the advertising type to put the correct restart policy (directed advertising
//     // is programmed only one)
//     if (adv_par->type == LLM_ADV_CONN_DIR)
//     {
//         restart_pol = LLD_NO_RESTART;
//         adv_int = 1250;
//     }
//     else
//     {
//         restart_pol = LLD_ADV_RESTART;
//         adv_int = 1500;
//     }

//     // Create an event to handle the advertising
//     struct lld_evt_tag *evt = lld_evt_adv_create(LLD_ADV_HDL,
//                                                  adv_par->intervalmin,
//                                                  adv_par->intervalmax,
//                                                  restart_pol);

//     // Update the control structure according to the parameters
//     ble_cntl_set(LLD_ADV_HDL, LLD_ADVERTISER);
//     ble_hopcntl_set(LLD_ADV_HDL, BLE_FH_EN_BIT | 39);
//     ble_crcinit1_set(LLD_ADV_HDL, (adv_par->filterpolicy <<BLE_FILTER_POLICY_LSB) | 0x55);
//     ble_rxwincntl_set(LLD_ADV_HDL, BLE_RXWIDE_BIT | evt->duration);
//     ble_maxevtime_set(LLD_ADV_HDL, 2400);
//     ble_txrxcntl_set(LLD_ADV_HDL, (LLD_RX_IRQ_THRES << BLE_RXTHR_LSB) | adv_pwr );
//     ble_fcntoffset_set(LLD_ADV_HDL,0);
//     ble_timgencntl_set(150);
//     // Set the advertising channel map
//     ble_advchmap_set(adv_par->channelmap);

//     // Set advertising timing register
//     ble_advtim_set(adv_int);

//     #if (RW_DM_SUPPORT)
//     // Set the priority properties
//     lld_prio_init(LLD_ADV_HDL, BLE_ADV_PRIO_INC, BLE_ADV_PRIO_DEF);
//     #endif // RW_DM_SUPPORT

//     #if (RW_BLE_WLAN_COEX)
//     if(adv_par->type == LLM_ADV_NONCONN_UNDIR)
//     {
//         ble_pti_setf(LLD_ADV_HDL,BLEMPRIO_NCONADV);
//         #if (RW_BLE_WLAN_COEX_TEST)
//         if(lld_wlcoex_enable)
//         {
//             ble_dnabort_setf(LLD_ADV_HDL,0);
//             ble_rxbsy_en_setf(LLD_ADV_HDL,0);
//             ble_txbsy_en_setf(LLD_ADV_HDL,0);
//         }
//         if(lld_wlcoex_scenario & BLE_WLCOEX_TST_NCONADV_PASSC)
//         {
//             mwsgen_start();
//         }
//         #endif // RW_BLE_WLAN_COEX_TEST
//     }
//     else // connectable advertising
//     {
//         ble_pti_setf(LLD_ADV_HDL,BLEMPRIO_CONADV);
//         if(lld_wlcoex_enable)
//         {
//             ble_dnabort_setf(LLD_ADV_HDL,1);
//             ble_rxbsy_en_setf(LLD_ADV_HDL,1);
//             ble_txbsy_en_setf(LLD_ADV_HDL,0);
//         }
//         #if (RW_BLE_WLAN_COEX_TEST)
//         if(lld_wlcoex_scenario & BLE_WLCOEX_TST_CONADV_ACTSC)
//         {
//             mwsgen_start();
//         }
//         #endif // RW_BLE_WLAN_COEX_TEST
//     }
//     #endif //RW_BLE_WLAN_COEX
//     // Update the TX Power in the event
//     evt->tx_pwr = adv_pwr;

//     // Chain the advertising data into the control structure
//     lld_data_tx_push(evt, adv_pdu);

//     // Chain the scan response data into the control structure
//     if (scan_rsp_pdu != NULL)
//         lld_data_tx_push(evt, scan_rsp_pdu);

//     // Loop the advertising data
//     lld_data_tx_loop(evt);

//     return (evt);
// }

// void lld_adv_stop(struct lld_evt_tag *evt)
// {
//     // Delete the event associated with this handle
//     lld_evt_delete(evt, BLE_ADVERT_ABORT_BIT);
// }
// #endif // BLE_PERIPHERAL || BLE_BROADCASTER

// #endif //if 0

// #if (BLE_PERIPHERAL || BLE_CENTRAL)
// void lld_ch_map_ind(struct lld_evt_tag *evt, uint16_t instant)
// {
//     lld_evt_schedule_next(evt);
//     lld_evt_ch_map_update_ind(evt, instant);
// }

// void lld_con_update_ind(struct lld_evt_tag *old_evt,
//                         struct llcp_con_up_req const *param_pdu)
// {
//     lld_evt_schedule_next(old_evt);
//     // Create an event to handle the connection update
//     lld_evt_slave_update(param_pdu, old_evt);
// }

// struct lld_evt_tag *lld_move_to_slave(struct llc_create_con_req_ind const *con_par,
//                                       struct llm_pdu_con_req_rx *con_req_pdu,
//                                       struct lld_evt_tag *evt_adv,
//                                       uint16_t conhdl)
// {
//     // Update the connection control structure according to the parameters
//     ble_cntl_set(conhdl, LLD_SLAVE_CONNECTED);
//     ble_hopcntl_set(conhdl, BLE_FH_EN_BIT | ((con_req_pdu->hop_sca & 0x1F) << BLE_HOP_INT_LSB));
//     ble_syncwl_set(conhdl, (con_req_pdu->aa.addr[1] << 8) | con_req_pdu->aa.addr[0]);
//     ble_syncwh_set(conhdl, (con_req_pdu->aa.addr[3] << 8) | con_req_pdu->aa.addr[2]);
//     ble_crcinit0_set(conhdl, (con_req_pdu->crcinit.crc[1] << 8) | con_req_pdu->crcinit.crc[0]);
//     ble_crcinit1_set(conhdl, con_req_pdu->crcinit.crc[2]);
//     ble_txrxcntl_set(conhdl, (LLD_RX_IRQ_THRES << BLE_RXTHR_LSB) | rwip_rf.txpwr_max);
//     ble_maxevtime_set(conhdl, co_htobs(co_btohs(con_req_pdu->interval * 2)
//                                  - LLD_EVT_PROG_LATENCY + 1));
//     ble_chmap0_set(conhdl,co_read16(&con_req_pdu->chm.map[0]));
//     ble_chmap1_set(conhdl,co_read16(&con_req_pdu->chm.map[2]));
//     ble_chmap2_set(conhdl,
//            (uint16_t)(llm_util_check_map_validity(&con_req_pdu->chm.map[0],LE_CHNL_MAP_LEN) << 8)
//            | con_req_pdu->chm.map[4]);
//     #if (RW_DM_SUPPORT)
//     // Set the priority properties
//     lld_prio_init(conhdl, BLE_SCONNECT_PRIO_INC, BLE_SCONNECT_PRIO_DEF);
//     #endif // RW_DM_SUPPORT

//     #if (RW_BLE_WLAN_COEX)
//     ble_pti_setf(conhdl,BLEMPRIO_CONREQ);
//     if(lld_wlcoex_enable)
//     {
//         ble_dnabort_setf(conhdl,1);
//         ble_rxbsy_en_setf(conhdl,1);
//         ble_txbsy_en_setf(conhdl,1);
//     }

//     #if (RW_BLE_WLAN_COEX_TEST)
//     if(lld_wlcoex_scenario & BLE_WLCOEX_TST_INITSC)
//     {
//         mwsgen_start();
//     }
//     #endif // RW_BLE_WLAN_COEX_TEST
//     #endif // RW_BLE_WLAN_COEX
//     // Move to the slave connected state
//     struct lld_evt_tag *evt = lld_evt_move_to_slave_p(con_par, con_req_pdu, evt_adv, conhdl);

//     #if (RW_BLE_WLAN_COEX)
//     if (lld_wlcoex_enable)
//     {
//         /* Update WL coexistence mailbox with this new information                  */
//         lld_wlcoex_connection_complete();
//     }
//     #endif // RW_BT_WLAN_COEX
//     return (evt);
// }
// #endif // BLE_PERIPHERAL || BLE_CENTRAL


// #if 0
// void lld_con_stop(struct lld_evt_tag *evt)
// {
//     // Delete the event associated with this handle
//     lld_evt_delete(evt, 0);

//     #if (RW_BLE_WLAN_COEX)
//     if((lld_wlcoex_enable) &&(co_list_is_empty(&lld_evt_env.evt_prog)))
//     {
//         lld_wlcoex_remove_connection();
//     }
//     #endif //RW_BLE_WLAN_COEX
// }

void lld_test_stop(struct lld_evt_tag *evt)
{
		typedef void (*my_function)(struct lld_evt_tag *evt);
    my_function PtrFunc;
    PtrFunc = (my_function)(jump_table_struct[lld_test_stop_func_pos]);
    PtrFunc(evt);
	
}
void lld_test_stop_func(struct lld_evt_tag *evt)
{
    // check that event still exists
    if(evt != NULL)
	{
		// Delete the event associated with this handle
		lld_evt_delete(evt, BLE_RFTEST_ABORT_BIT);

	}
		// Re-enable force AGC mechanism
		rwip_rf.force_agc_enable(true);
}
// void lld_crypt_isr(void)
// {
//     // Clear the interrupt
//     ble_intack_clear(BLE_CRYPTINTACK_BIT);

//     // Set kernel event for deferred handling
//     ke_event_set(KE_EVENT_BLE_CRYPT);
// }

// struct lld_evt_tag * lld_test_mode_tx(struct co_buf_tx_node *txdesc, uint8_t tx_freq)
// {
//     // Create an event to handle the tx test mode
//     struct lld_evt_tag *evt = lld_evt_adv_create(LLD_ADV_HDL, 0, 1, LLD_NO_RESTART);

//     // Update the control structure according to the parameters
//     ble_cntl_set(LLD_ADV_HDL, LLD_TXTEST_MODE);
//     // Initialize the advertising control structure
//     ble_syncwl_set(LLD_ADV_HDL, 0x4129);
//     ble_syncwh_set(LLD_ADV_HDL, 0x7176);
//     ble_txpwr_setf(LLD_ADV_HDL, 0xF);
//     #if (RW_DM_SUPPORT)
//     // Set the priority properties
//     lld_prio_init(LLD_ADV_HDL, BLE_SCAN_PRIO_INC, BLE_SCAN_PRIO_DEF);
//     #endif // RW_DM_SUPPORT

//     switch (tx_freq)
//     {
//      case 0:
//          tx_freq = 37;
//          break;
//      case 12:
//          tx_freq = 38;
//          break;
//      case 39:
//          break;
//      default:
//          if (tx_freq < 12)
//              tx_freq -=1;
//          else
//              tx_freq -=2;
//          break;
//     }
//     ble_ch_idx_setf(LLD_ADV_HDL,tx_freq);

//     // Chain the tx test mode data into the control structure
//     lld_data_tx_push(evt, txdesc);

//     // Loop the tx test mode data
//     lld_data_tx_loop(evt);

//     return (evt);
// }

// struct lld_evt_tag * lld_test_mode_rx(uint8_t rx_freq)
// {
//     // Create an event to handle the advertising
//     struct lld_evt_tag *evt = lld_evt_adv_create(LLD_ADV_HDL, 0, 1, LLD_NO_RESTART);
//     // Update the control structure according to the parameters
//     ble_cntl_set(LLD_ADV_HDL, LLD_RXTEST_MODE);
//     ble_rxwincntl_set(LLD_ADV_HDL, BLE_RXWIDE_BIT | 75);
//     // Initialize the advertising control structure
//     ble_syncwl_set(LLD_ADV_HDL, 0x4129);
//     ble_syncwh_set(LLD_ADV_HDL, 0x7176);
//     #if (RW_DM_SUPPORT)
//     // Set the priority properties
//     lld_prio_init(LLD_ADV_HDL, BLE_SCAN_PRIO_INC, BLE_SCAN_PRIO_DEF);
//     #endif // RW_DM_SUPPORT
//     switch (rx_freq)
//     {
//      case 0:
//          rx_freq = 37;
//          break;
//      case 12:
//          rx_freq = 38;
//          break;
//      case 39:
//          break;
//      default:
//          if (rx_freq < 12)
//              rx_freq -=1;
//          else
//              rx_freq -=2;
//          break;
//     }
//     ble_ch_idx_setf(LLD_ADV_HDL,rx_freq);

//     // Disable force AGC mechanism
//     rwip_rf.force_agc_enable(false);

/// @} LLD
