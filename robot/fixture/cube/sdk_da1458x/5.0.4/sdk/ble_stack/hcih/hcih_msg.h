/**
 ****************************************************************************************
 *
 * @file hcih_msg.h
 *
 * @brief Header file containing the definitions and structures corresponding to the
 * standard HCI messages: commands from Host, events from Controller.
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 *
 ****************************************************************************************
 */

#ifndef HCIH_MSG_H_
#define HCIH_MSG_H_

/**
 ****************************************************************************************
 * @addtogroup HCIH_MSG HCI Pack/Unpack Functions
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

#if (BLEHL_HCIH_ITF)

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
#if (BLEHL_WLAN_COEX)
#define HCI_DBG_WLAN_COEX_CMD_OPCODE                  0xFC0B
#endif //BLEHL_WLAN_COEX
#if (BLEHL_WLAN_COEX_TEST)
#define HCI_DBG_WLAN_COEXTST_SCEN_CMD_OPCODE          0xFC0D
#endif //BLEHL_WLAN_COEX_TEST
#define HCI_DBG_RD_KE_STATS_CMD_OPCODE                0xFC10
#define HCI_DBG_PLF_RESET_CMD_OPCODE                  0xFC11
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

///HCI Commands parameter length for fixed ones
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
#define HCI_CCEVT_DBGHWREGRD_RETPAR_LEN          7
#define HCI_CCEVT_DBGHWREGWR_RETPAR_LEN          3
#define HCI_CCEVT_DBGRFREGRD_RETPAR_LEN          7
#define HCI_CCEVT_DBGRFREGWR_RETPAR_LEN          3


/*
 * FUNCTION DECLARATIONS - Higher Layers Command packing
 ****************************************************************************************
 */
/**
****************************************************************************************
* @brief HCI function for packing basic command packets, for commands with no parameters.
*
* @param ke_msg Pointer to the ke_msg containing the parameters necessary to create the
*               HCI type packet. The ke_msg space is used for packing, and the message is
*               queued before TX to avoid loss in case TX needs to be postponed.
*****************************************************************************************
*/
int hcih_cmd_aligned_pk(struct ke_msg * msg);


int hcih_llc_basic_cmd_pk(struct ke_msg * msg);


/**
****************************************************************************************
* @brief GENERAL COMMENT FOR hcih_..._cmd_pk : HCI function for Higher Layers
*        Command packing in view of transport.
*
* @param ke_msg Pointer to the kernel message containing the parameters to send. The
*               parameters will be packed into this space, in order ot avoid allocating
*               more space. The message will be queued fot transport, sent if possible
*               then freed.
*****************************************************************************************
*/
//legacy
int hcih_host_buffer_size_cmd_pk(struct ke_msg * msg);
int hcih_host_nb_completed_pkts_cmd_pk(struct ke_msg * msg);
int hcih_rd_buff_size_cmd_pk(struct ke_msg * msg);

//le
int hcih_le_set_adv_param_cmd_pk(struct ke_msg * msg);
int hcih_le_set_scan_param_cmd_pk(struct ke_msg * msg);
int hcih_le_create_con_cmd_pk(struct ke_msg * msg);
int hcih_le_con_update_cmd_pk(struct ke_msg * msg);
int hcih_le_start_enc_cmd_pk(struct ke_msg * msg);

//dbg
int hcih_dbg_hw_reg_rd_cmd_pk(struct ke_msg * msg);
int hcih_dbg_hw_reg_wr_cmd_pk(struct ke_msg * msg);
int hcih_dbg_set_crc_cmd_pk(struct ke_msg * msg);


/*
 * FUNCTION DECLARATIONS - Higher Layers Event unpacking
 ****************************************************************************************
 */
void hcih_init_ccevt_unpk(uint8_t * bufptr);

//ccevt legacy
void hcih_ccevt_aligned_unpk(uint8_t * bufptr);
#if (BLE_CENTRAL || BLE_PERIPHERAL)
void hcih_flush_ccevt_unpk(uint8_t * bufptr);
void hcih_rd_tx_pw_lvl_ccevt_unpk(uint8_t * bufptr);
void hcih_rd_rssi_ccevt_unpk(uint8_t * bufptr);
#endif // #if (BLE_CENTRAL || BLE_PERIPHERAL)

void hcih_rd_local_ver_info_ccevt_unpk(uint8_t * bufptr);
void hcih_rd_buff_size_ccevt_unpk(uint8_t * bufptr);

//ccevt le
void hcih_le_rd_buff_size_ccevt_unpk(uint8_t * bufptr);
#if (BLE_CENTRAL || BLE_PERIPHERAL)
void hcih_le_rd_chnl_map_ccevt_unpk(uint8_t * bufptr);
void hcih_le_ltk_req_rply_ccevt_unpk(uint8_t * bufptr);
void hcih_le_ltk_req_neg_rply_ccevt_unpk(uint8_t * bufptr);
#endif // #if (BLE_CENTRAL || BLE_PERIPHERAL)
void hcih_le_test_end_ccevt_unpk(uint8_t * bufptr);

//command status events
void hcih_csevt_unpk(ke_msg_id_t msg_id, ke_task_id_t dest_id,
                    ke_task_id_t src_id, uint8_t status);

//special legacy events
#if (BLE_CENTRAL || BLE_PERIPHERAL)
void hcih_disconnection_cmp_evt_unpk(uint8_t * bufptr);
void hcih_rd_rem_ver_info_cmp_evt_unpk(uint8_t * bufptr);
void hcih_enc_chg_evt_unpk(uint8_t * bufptr);
void hcih_enc_key_refresh_cmp_evt_unpk(uint8_t * bufptr);
void hcih_nb_cmpl_pkts_evt_unpk(uint8_t * bufptr);
void hcih_flush_occurred_evt_unpk(uint8_t * bufptr);
#endif /* (BLE_CENTRAL || BLE_PERIPHERAL) */
void hcih_hw_err_evt_unpk(uint8_t * bufptr);
void hcih_data_buff_ovflw_evt_unpk(uint8_t * bufptr);

//special le events
#if (BLE_CENTRAL || BLE_PERIPHERAL)
void hcih_le_con_cmpl_evt_unpk(uint8_t * bufptr);
void hcih_le_con_update_cmp_evt_unpk(uint8_t * bufptr);
void hcih_le_rd_rem_used_feats_cmp_evt_unpk(uint8_t * bufptr);
void hcih_le_ltk_request_evt_unpk(uint8_t * bufptr);
#endif /* (BLE_CENTRAL || BLE_PERIPHERAL) */
void hcih_le_adv_report_evt_unpk(uint8_t * bufptr);

//debug events
void hcih_dbg_hw_reg_rd_cmp_evt_unpk(uint8_t * bufptr);
void hcih_dbg_hw_reg_wr_cmp_evt_unpk(uint8_t * bufptr);

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
int hcih_data_pk(struct ke_msg * msg);
#endif // #if (BLE_CENTRAL || BLE_PERIPHERAL)
#endif //BLEHL_HCIH_ITF

/// @} HCIH_MSG

#endif // HCIH_MSG_H_
