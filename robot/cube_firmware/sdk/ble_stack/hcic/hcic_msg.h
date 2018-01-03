/**
 ****************************************************************************************
 *
 * @file hci_msg.h
 *
 * @brief Header file containing the definitions and structures corresponding to the
 * standard HCI messages: commands from Host, events from Controller.
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 *
 ****************************************************************************************
 */

#ifndef HCI_MSG_H_
#define HCI_MSG_H_

/**
 ****************************************************************************************
 * @addtogroup HCI_MSG HCI Pack/Unpack Functions
 * @ingroup HCI
 * @brief Host-Controller Interface functionalities.
 *
 * This module contains the @ref HOST "HOST" - @ref Controller "Controller" interface
 * functionalities: message IDs and handlers. \n
 *
 * @{
 ****************************************************************************************
 */


/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "rwip_config.h"     // SW configuration

#if (BLE_HCIC_ITF)

#include <stdint.h>        // standard integer definition

#include "co_bt.h"         // common bt definition
#include "co_endian.h"     // endian-ness definition
#include "ke_msg.h"        // kernel message definition



/*
 * DEFINES
 ****************************************************************************************
 */


/**************************************************************************************
 **************                       HCI COMMANDS                     ****************
 **************************************************************************************/

///Debug commands - OGF = 0x3F (spec)
#define HCI_DBG_RD_MEM_CMD_OPCODE                     0xFC01
#define HCI_DBG_WR_MEM_CMD_OPCODE                     0xFC02
#define HCI_DBG_DEL_PAR_CMD_OPCODE                    0xFC03
#define HCI_DBG_FLASH_ID_CMD_OPCODE                   0xFC05
#define HCI_DBG_FLASH_ER_CMD_OPCODE                   0xFC06
#define HCI_DBG_FLASH_WR_CMD_OPCODE                   0xFC07
#define HCI_DBG_FLASH_RD_CMD_OPCODE                   0xFC08
#define HCI_DBG_RD_PAR_CMD_OPCODE                     0xFC09
#define HCI_DBG_WR_PAR_CMD_OPCODE                     0xFC0A
#if RW_BLE_WLAN_COEX
#define HCI_DBG_WLAN_COEX_CMD_OPCODE                  0xFC0B
#endif //RW_BLE_WLAN_COEX
#if RW_BLE_WLAN_COEX_TEST
#define HCI_DBG_WLAN_COEXTST_SCEN_CMD_OPCODE          0xFC0D
#endif //RW_BLE_WLAN_COEX_TEST
#define HCI_DBG_RD_KE_STATS_CMD_OPCODE                0xFC10
#define HCI_DBG_PLF_RESET_CMD_OPCODE                  0xFC11
#define HCI_DBG_RD_MEM_INFO_CMD_OPCODE                0xFC12
#if (DSM_SUPPORT)
#define HCI_DBG_DATA_STORAGE_OP_CMD_OPCODE            0xFC16
#endif //DSM_SUPPORT
#define HCI_DBG_HW_REG_RD_CMD_OPCODE                  0xFC30
#define HCI_DBG_HW_REG_WR_CMD_OPCODE                  0xFC31
#define HCI_DBG_SET_BD_ADDR_CMD_OPCODE                0xFC32
#define HCI_DBG_SET_TYPE_PUB_CMD_OPCODE               0xFC33
#define HCI_DBG_SET_TYPE_RAND_CMD_OPCODE              0xFC34
#define HCI_DBG_SET_CRC_CMD_OPCODE                    0xFC35
#define HCI_DBG_LLCP_DISCARD_CMD_OPCODE               0xFC36
#define HCI_DBG_RESET_RX_CNT_CMD_OPCODE               0xFC37
#define HCI_DBG_RESET_TX_CNT_CMD_OPCODE               0xFC38
#define HCI_DBG_RF_REG_RD_CMD_OPCODE                  0xFC39
#define HCI_DBG_RF_REG_WR_CMD_OPCODE                  0xFC3A
#define HCI_DBG_SET_TX_PW_CMD_OPCODE                  0xFC3B
#define HCI_DBG_OPCODE_MAX                            0xFC3B

//dBG
#define HCI_DBGRDMEM_CMD_PARLEN          6
#define HCI_DBGWRMEM_CMD_PARLEN          135
#define HCI_DBGDELPAR_CMD_PARLEN         2
#define HCI_DBGFLASHID_CMD_PARLEN        0
#define HCI_DBGFLASHER_CMD_PARLEN        9
#define HCI_DBGFLASHWR_CMD_PARLEN        134
#define HCI_DBGFLASHRD_CMD_PARLEN        6
#define HCI_DBGRDPAR_CMD_PARLEN          2
#define HCI_DBGWRPAR_CMD_PARLEN          131
#define HCI_DBGPLFRESET_CMD_PARLEN       1
#define HCI_DBGRDKESTATS_CMD_PARLEN      0
#define HCI_DBGRDMEMINFO_CMD_PARLEN      0
#if (DSM_SUPPORT)
#define HCI_DBGDATASTOROP_CMD_PARLEN     2+128
#endif //(DSM_SUPPORT)
#define HCI_DBGHWREGRD_CMD_PARLEN        2
#define HCI_DBGHWREGWR_CMD_PARLEN        8
#define HCI_DBGSETBD_CMD_PARLEN          6
#define HCI_DBGSETPUB_CMD_PARLEN         0
#define HCI_DBGSETRAND_CMD_PARLEN        0
#define HCI_DBGSETCRC_CMD_PARLEN         2+CRC_INIT_LEN
#define HCI_DBGLLCPDISC_CMD_PARLEN       3
#define HCI_DBGRSTRXCNT_CMD_PARLEN       2
#define HCI_DBGRSTTXCNT_CMD_PARLEN       2
#define HCI_DBGRFREGRD_CMD_PARLEN        2
#define HCI_DBGRFREGWR_CMD_PARLEN        8
#define HCI_DBGSETTXPW_CMD_PARLEN        3


/**************************************************************************************
 **************                        HCI EVENTS                      ****************
 **************************************************************************************/

///HCI Debug command complete event
#define HCI_CCEVT_DBGFLASHID_RETPAR_LEN          2
#define HCI_CCEVT_DBGRDKESTATS_RETPAR_LEN        8
#define HCI_CCEVT_DBGRDMEMINFO_RETPAR_LEN        (5 + (2*(KE_MEM_BLOCK_MAX)))
#define HCI_CCEVT_DBGHWREGRD_RETPAR_LEN          7
#define HCI_CCEVT_DBGHWREGWR_RETPAR_LEN          3
#define HCI_CCEVT_DBGRFREGRD_RETPAR_LEN          7
#define HCI_CCEVT_DBGRFREGWR_RETPAR_LEN          3

#if (DSM_SUPPORT)
#define HCI_DBG_DATA_STORAGE_IND_EVT_SUBCODE     0x02
#endif //DSM_SUPPORT

/*
 * FUNCTION DECLARATIONS - Lower Layers Command unpacking
 ****************************************************************************************
 */
/**
****************************************************************************************
* @brief HCI command unpack function for commands for which the received parameter
*        buffer maps exactly to the structure used in the lower layers handler.
*
* @param bufptr Pointer to the buffer containing the received packet. Space is part of
*               allocated kernel message prior to reception. This ke_msg will be updated
*               with extracted data.
*****************************************************************************************
*/
void hci_cmd_aligned_unpk(uint8_t * bufptr);


#if (BLE_PERIPHERAL || BLE_CENTRAL)
/**
****************************************************************************************
* @brief HCI Command unpacking function for commands addressed to LLC using connection
*        handle only as parameter.
*
* @param bufptr Pointer to the buffer containing the received packet. Space is part of
*               allocated kernel message prior to reception. This ke_msg will be updated
*               with extracted data.
*****************************************************************************************
*/
void hci_llc_basic_cmd_unpk( uint8_t * bufptr);

#endif //(BLE_PERIPHERAL || BLE_CENTRAL)

/**
****************************************************************************************
* @brief GENERAL COMMENT FOR hci_..._cmd_unpk : HCI function for LL Command unpacking
*        after transport.
*
* @param bufptr Pointer to the buffer containing the received packet. Space is part of
*               allocated kernel message prior to reception. This ke_msg will be updated
*               with extracted data.
*****************************************************************************************
*/
//HCI Legacy commands unpack handler prototypes
void hci_host_buffer_size_cmd_unpk( uint8_t * bufptr);
void hci_host_nb_completed_pkts_cmd_unpk( uint8_t * bufptr);

//HCI LE commands unpack handler prototypes
#if (BLE_PERIPHERAL || BLE_BROADCASTER)
void hci_le_set_adv_param_cmd_unpk( uint8_t * bufptr);
#endif // BLE_PERIPHERAL || BLE_BROADCASTER

#if (BLE_CENTRAL || BLE_OBSERVER)
void hci_le_set_scan_param_cmd_unpk( uint8_t * bufptr);
#endif // BLE_CENTRAL || BLE_OBSERVER

#if (BLE_CENTRAL)
void hci_le_create_con_cmd_unpk( uint8_t * bufptr);
#endif // BLE_CENTRAL

#if (BLE_CENTRAL || BLE_PERIPHERAL)
void hci_le_con_update_cmd_unpk( uint8_t * bufptr);
void hci_le_start_enc_cmd_unpk( uint8_t * bufptr);
#endif // BLE_CENTRAL || BLE_PERIPHERAL

//debug commands unpacking functions
#if (BLE_DEBUG)
void hci_dbg_rd_mem_cmd_unpk( uint8_t * bufptr);
void hci_dbg_wr_mem_cmd_unpk( uint8_t * bufptr);
void hci_dbg_del_param_cmd_unpk( uint8_t * bufptr);
void hci_dbg_flash_erase_cmd_unpk( uint8_t * bufptr);
void hci_dbg_flash_write_cmd_unpk( uint8_t * bufptr);
void hci_dbg_flash_read_cmd_unpk( uint8_t * bufptr);
void hci_dbg_rd_param_cmd_unpk( uint8_t * bufptr);
void hci_dbg_wr_param_cmd_unpk( uint8_t * bufptr);
void hci_dbg_hw_reg_rd_cmd_unpk( uint8_t * bufptr);
void hci_dbg_hw_reg_wr_cmd_unpk( uint8_t * bufptr);
void hci_dbg_set_crc_cmd_unpk( uint8_t * bufptr);
void hci_dbg_llcp_discard_cmd_unpk( uint8_t * bufptr);
void hci_dbg_reset_cnt_cmd_unpk( uint8_t * bufptr);
void hci_dbg_rf_reg_rd_cmd_unpk( uint8_t * bufptr);
void hci_dbg_rf_reg_wr_cmd_unpk( uint8_t * bufptr);
void hci_dbg_set_tx_pw_cmd_unpk( uint8_t * bufptr);
#endif // BLE_DEBUG

/*
 * FUNCTION DECLARATIONS - Lower Layers Event packing
 ****************************************************************************************
 */
/**
****************************************************************************************
* @brief HCI function for packing basic command complete events, for commands with no
*        return parameters or aligned return paramer struct
*
* @param ke_msg Pointer to the ke_msg containing the parameters necessary to create the
*               HCI type packet. The ke_msg space is used for packing, and the message is
*               queued before TX to avoid loss in case TX needs to be postponed.
*****************************************************************************************
*/
int hci_ccevt_aligned_pk(struct ke_msg * msg);


/**
****************************************************************************************
* @brief GENERAL COMMENT FOR hci_..._evt_pk : HCI function for Lower Layers event packing
*        for transport to HL.
*
* @param ke_msg Pointer to the ke_msg containing the parameters necessary to create the
*               HCI type packet. The ke_msg space is used for packing, and the message is
*               queued before TX to avoid loss in case TX needs to be postponed.
*****************************************************************************************
*/
//legacy
int hci_flush_ccevt_pk(struct ke_msg * msg);
int hci_rd_tx_pw_lvl_ccevt_pk(struct ke_msg * msg);
int hci_rd_local_ver_info_ccevt_pk(struct ke_msg * msg);
int hci_rd_buff_size_ccevt_pk(struct ke_msg * msg);
#if (BLE_CENTRAL || BLE_PERIPHERAL)
int hci_rd_rssi_ccevt_pk(struct ke_msg * msg);
#endif // BLE_CENTRAL || BLE_PERIPHERAL
//LE
int hci_le_rd_buff_size_ccevt_pk(struct ke_msg * msg);
#if (BLE_CENTRAL || BLE_PERIPHERAL)
int hci_le_rd_chnl_map_ccevt_pk(struct ke_msg * msg);
int hci_le_ltk_req_rply_ccevt_pk(struct ke_msg * msg);
int hci_le_ltk_req_neg_rply_ccevt_pk(struct ke_msg * msg);
#endif // #if (BLE_CENTRAL || BLE_PERIPHERAL)
int hci_le_test_end_ccevt_pk(struct ke_msg * msg);


/**
****************************************************************************************
* @brief HCI function for packing basic command status events - they all have the same
*        components.
*
* @param ke_msg Pointer to the ke_msg containing the parameters necessary to create the
*               HCI type packet. The ke_msg space is used for packing, and the message is
*               queued before TX to avoid loss in case TX needs to be postponed.
*****************************************************************************************
*/
int hci_csevt_pk(struct ke_msg * msg);
/**
****************************************************************************************
* @brief HCI function for packing unknown connection handle complete event.
*
* @param ke_msg Pointer to the ke_msg containing the parameters necessary to create the
*               HCI type packet. The ke_msg space is used for packing, and the message is
*               queued before TX to avoid loss in case TX needs to be postponed.
*****************************************************************************************
*/
int hci_unknwn_conhdl_ccevt_pk(struct ke_msg * msg);


//Special Events - legacy
#if (BLE_CENTRAL || BLE_PERIPHERAL)
int hci_disconnection_cmp_evt_pk(struct ke_msg * msg);
int hci_rd_rem_ver_info_cmp_evt_pk(struct ke_msg * msg);
int hci_flush_occurred_evt_pk(struct ke_msg * msg);
int hci_enc_change_evt_pk(struct ke_msg * msg);
int hci_enc_key_refresh_evt_pk(struct ke_msg * msg);
#endif // BLE_CENTRAL || BLE_PERIPHERAL

int hci_hw_error_evt_pk(struct ke_msg * msg);
int hci_data_buf_ovflw_evt_pk(struct ke_msg * msg);
int hci_nb_cmp_pkts_evt_pk(struct ke_msg * msg);

//Special events - LE
/**
****************************************************************************************
* @brief HCI functions for packing special events:
*        - LE Connection Complete Event
*        - LE Connection Update Complete Event
*        - LE Read Remote Used Features Complete Event
*        - LE Long Term Key Request Event
*        - LE Advertising Report Event
*
* @param ke_msg Pointer to the ke_msg containing the parameters necessary to create the
*               HCI type packet. The ke_msg space is used for packing, and the message is
*               queued before TX to avoid loss in case TX needs to be postponed.
*****************************************************************************************
*/
#if (BLE_CENTRAL || BLE_PERIPHERAL)
int hci_le_con_cmp_evt_pk(struct ke_msg * msg);
int hci_le_con_update_cmp_evt_pk(struct ke_msg * msg);
int hci_le_rd_rem_used_feats_cmp_evt_pk(struct ke_msg * msg);
int hci_le_ltk_req_evt_pk(struct ke_msg * msg);
#endif // BLE_CENTRAL || BLE_PERIPHERAL

int hci_le_adv_report_evt_pk(struct ke_msg * msg);

//Debug events
#if (BLE_DEBUG)
int hci_dbg_rd_mem_ccevt_pk(struct ke_msg * msg);
int hci_dbg_rd_par_ccevt_pk(struct ke_msg * msg);
int hci_dbg_hw_reg_rd_ccevt_pk(struct ke_msg * msg);
int hci_dbg_rd_mem_info_ccevt_pk(struct ke_msg * msg);
#if (DSM_SUPPORT)
int hci_dbg_data_storage_ind_evt_pk(struct ke_msg * msg);
#endif //DSM_SUPPORT
int hci_dbg_hw_reg_wr_ccevt_pk(struct ke_msg * msg);
int hci_dbg_flash_rd_evt_pk(struct ke_msg * msg);
int hci_dbg_rf_reg_rd_ccevt_pk(struct ke_msg * msg);
int hci_dbg_rf_reg_wr_ccevt_pk(struct ke_msg * msg);
#endif // BLE_DEBUG

#if (BLE_CENTRAL || BLE_PERIPHERAL)
/**
 ****************************************************************************************
 * @brief Function handling the packing of data in view of transport through HCI.
 *
 * This function is used in either of higher or lower layers, where it translates the
 * components of a TX descriptor into the packet contents and respectively of a
 * RX descriptor.
 *
 * @param[in,out] msg Pointer to the kernel message which contains the parameters to send,
 *                    and whose space is used to pack the HCI ACL packet components.
 *
 *****************************************************************************************
 */
int hci_data_pk(struct ke_msg * msg);
#endif // BLE_CENTRAL || BLE_PERIPHERAL

#endif //BLE_HCIC_ITF

/// @} HCI_MSG

#endif // HCI_MSG_H_
