/**
 ****************************************************************************************
 *
 * @file dbg_task.h
 *
 * @brief DBG task header file
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 *
 ****************************************************************************************
 */

#ifndef DBG_TASK_H_
#define DBG_TASK_H_

/**
 ****************************************************************************************
 * @addtogroup DBGTASK Task
 * @ingroup DBG
 * @brief Debug task
 *
 * The DBG task is responsible for receiving and sending debug informations between the
 * HCI and the embedded.
 *
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include <stdint.h>              // standard definition
#include "ke_task.h"             // kernel task
#include "co_bt.h"               // common BT definitions

/*
 * INSTANCES
 ****************************************************************************************
 */
/// Maximum number of instances of the DBG task
#define DBG_IDX_MAX 1

/*
 * STATES
 ****************************************************************************************
 */
/// Possible states of the DBG task
enum
{
    /// IDLE state
    DBG_IDLE,
    /// Number of states.
    DBG_STATE_MAX
};

/*
 * MESSAGES
 ****************************************************************************************
 */
/// Message API of the DBG task
enum
{
    /// Debug commands description.
    DBG_MSG_ID_CMD_FIRST = KE_FIRST_MSG(TASK_DBG),
    DBG_RD_MEM_REQ = DBG_MSG_ID_CMD_FIRST,
    DBG_WR_MEM_REQ,
    DBG_DEL_PARAM_REQ,
    DBG_RD_PARAM_REQ,
    DBG_WR_PARAM_REQ,
    DBG_PLF_RESET_REQ,
    DBG_FLASH_IDENT_REQ,
    DBG_FLASH_ERASE_REQ,
    DBG_FLASH_WRITE_REQ,
    DBG_FLASH_READ_REQ,
    #if (BT_EMB_PRESENT)
    DBG_ENTER_TST_MODE_REQ,
    DBG_SEND_LMP_REQ,
    DBG_WR_SYNC_CONFIG_REQ,
    DBG_EMUL_TESTER_REQ,
    DBG_SCATT_IMPROV_REQ,
    #endif //BT_EMB_PRESENT
    DBG_RD_KE_STATS_REQ,
    DBG_RF_REG_RD_REQ,
    DBG_RF_REG_WR_REQ,
    DBG_HW_REG_RD_REQ,
    DBG_HW_REG_WR_REQ,
    #if (BLE_EMB_PRESENT || BLE_HOST_PRESENT)
    DBG_LE_SET_BD_ADDR_REQ,
    DBG_LE_SET_TYPE_PUB_REQ,
    DBG_LE_SET_TYPE_RAND_REQ,
    DBG_LE_SET_CRC_REQ,
    DBG_LE_LLCP_DISCARD_REQ,
    DBG_LE_RESET_RX_CNT_REQ,
    DBG_LE_RESET_TX_CNT_REQ,
    DBG_SET_TX_PW_REQ,
    #endif //BLE_EMB_PRESENT || BLE_HOST_PRESENT
    #if (RW_WLAN_COEX)
    DBG_WLAN_COEX_REQ,
    #if (RW_WLAN_COEX_TEST)
    DBG_WLAN_COEXTST_SCEN_REQ,
    #endif //RW_BT_WLAN_COEX_TEST
    #endif //RW_WLAN_COEX
    DBG_RD_MEM_INFO_REQ,
    DBG_MSG_ID_EVT_FIRST,

    /// Debug events description.
    DBG_RD_MEM_CMP_EVT = DBG_MSG_ID_EVT_FIRST,
    DBG_WR_MEM_CMP_EVT,
    DBG_DEL_PARAM_CMP_EVT,
    DBG_RD_PARAM_CMP_EVT,
    DBG_WR_PARAM_CMP_EVT,
    DBG_FLASH_IDENT_CMP_EVT,
    DBG_FLASH_ERASE_CMP_EVT,
    DBG_FLASH_WRITE_CMP_EVT,
    DBG_FLASH_READ_CMP_EVT,
    #if (BT_EMB_PRESENT)
    DBG_ENTER_TST_MODE_CMP_EVT,
    DBG_SEND_LMP_CMP_EVT,
    DBG_WR_SYNC_CONFIG_CMP_EVT,
    DBG_SCATT_IMPROV_CMP_EVT,
    DBG_EMUL_TESTER_CS_EVT,
    DBG_EMUL_FINISHED_EVT,
    #endif //BT_EMB_PRESENT
    DBG_RD_KE_STATS_CMP_EVT,
    DBG_TRACE_WARNING_EVT,
    DBG_RF_REG_RD_CMP_EVT,
    DBG_RF_REG_WR_CMP_EVT,
    DBG_PLF_RESET_CMP_EVT,
    DBG_HW_REG_RD_CMP_EVT,
    DBG_HW_REG_WR_CMP_EVT,
    #if (BLE_EMB_PRESENT)
    DBG_LE_SET_BD_ADDR_CMP_EVT,
    DBG_LE_SET_TYPE_PUB_CMP_EVT,
    DBG_LE_SET_TYPE_RAND_CMP_EVT,
    DBG_LE_SET_CRC_CMP_EVT,
    DBG_LE_LLCP_DISCARD_CMP_EVT,
    DBG_LE_RESET_RX_CNT_CMP_EVT,
    DBG_LE_RESET_TX_CNT_CMP_EVT,
    DBG_SET_TX_PW_CMP_EVT,
    #endif //BLE_EMB_PRESENT
    #if (RW_WLAN_COEX)
    DBG_WLAN_COEX_CMP_EVT,
    #if (RW_WLAN_COEX_TEST)
    DBG_WLAN_COEXTST_SCEN_CMP_EVT,
    #endif //RW_BT_WLAN_COEX_TEST
    #endif //RW_WLAN_COEX
    DBG_RD_MEM_INFO_CMP_EVT,
    DBG_MSG_ID_EVT_LAST
};

/// Buffer structure
struct buffer_tag
{
    /// length of buffer
    uint8_t length;
    /// data of 128 bytes length
    uint8_t data[128];
};

/// HCI debug command complete event structure
struct dbg_common_cmd_cmp_evt
{
    /// status
    uint8_t  status;
};

///HCI Debug command status event parameters
struct dbg_cmd_stat_evt
{
    ///Status
    uint8_t status;
};

///HCI Debug read memory variable command parameters - vendor specific
struct dbg_rd_mem_cmd
{
    ///Start address to read
    uint32_t start_addr;
    ///Access size
    uint8_t type;
    ///Length to read
    uint8_t length;
};

///HCI Debug write memory variable command parameters - vendor specific
struct dbg_wr_mem_cmd
{
    ///Start address to read
    uint32_t start_addr;
    ///Access size
    uint8_t type;
    ///buffer structure to return
    struct buffer_tag buf;
};

///HCI Debug delete parameter command parameters - vendor specific
struct dbg_del_param_cmd
{
    ///Parameter tag
    uint16_t param_tag;
};

///HCI Debug erase flash command parameters - vendor specific
struct dbg_flash_erase_cmd
{
    ///Flash type
    uint8_t flashtype;
    ///Start offset address
    uint32_t startoffset;
    ///Size to erase
    uint32_t size;
};

///HCI Debug write flash command parameters - vendor specific
struct dbg_flash_write_cmd
{
    ///Flash type
    uint8_t flashtype;
    ///Start offset address
    uint32_t startoffset;
    ///buffer structure
    struct buffer_tag buf;
};

///HCI Debug read flash command parameters - vendor specific
struct dbg_flash_read_cmd
{
    ///Flash type
    uint8_t flashtype;
    ///Start offset address
    uint32_t startoffset;
    ///Size to read
    uint8_t size;
};

///HCI Debug read parameter command parameters - vendor specific
struct dbg_rd_param_cmd
{
    ///Parameter tag
    uint16_t param_tag;
};

///HCI Debug read parameters command parameters - vendor specific
struct dbg_wr_param_cmd
{
    ///Parameter tag
    uint16_t param_tag;
    ///Structure buffer
    struct buffer_tag buf;
};

#if (BT_EMB_PRESENT)
///HCI Debug send lmp packet command parameters - vendor specific
struct dbg_send_lmp_cmd
{
    ///Link Identifier tag
    uint8_t linkid;
    ///Structure buffer
    struct buffer_tag buf;
};

///HCI Debug send lmp packet command parameters - vendor specific
struct dbg_wr_sync_config_cmd
{
    ///Sync configuration
    uint16_t config;
};

///HCI Debug emulate tester command parameters - vendor specific
struct dbg_emul_tester_cmd
{
    /// link identifier
    uint8_t link_id;
    /// test scenario
    uint8_t test_scenario;
    /// hopping mode
    uint8_t hopping_mode;
    /// transmit frequency
    uint8_t tx_freq;
    /// reception frequency
    uint8_t rx_freq;
    /// power control
    uint8_t power_control;
    /// polling period
    uint8_t poll_period;
    /// packet type
    uint8_t packet_type;
    /// size of the pay load
    uint16_t payload_length;
};

///HCI Debug scatternet improvement command
struct dbg_scatt_improv_cmd
{
    ///Enable or disable scatternet improvement mechanism
    uint8_t action;
    ///Correction step
    uint8_t step;
    ///Bit offset wanted
    uint8_t bitoff;
};
#endif //BT_EMB_PRESENT

///HCI Debug Read Kernel Statistics complete event parameters - vendor specific
struct dbg_rd_ke_stats_cmp_evt
{
    ///Status
    uint8_t status;
    ///Max message sent
    uint8_t max_msg_sent;
    ///Max message saved
    uint8_t max_msg_saved;
    ///Max timer used
    uint8_t max_timer_used;
    ///Max heap used
    uint16_t max_heap_used;
    ///Max stack used
    uint16_t max_stack_used;
};


/// HCI Debug Read information about memory usage. - vendor specific
struct dbg_rd_mem_info_cmp_evt
{
    ///Status
    uint8_t status;
    /// memory size currently used into each heaps.
    uint16_t mem_used[KE_MEM_BLOCK_MAX];
    /// peak of memory usage measured
    uint32_t max_mem_used;
};


///HCI Debug read parameters complete event parameters - vendor specific
struct dbg_rd_param_cmp_evt
{
    ///Status
    uint8_t status;
    ///Structure buffer
    struct buffer_tag buf;
};

///HCI Debug read flash complete event parameters - vendor specific
struct dbg_flash_read_cmp_evt
{
    ///Status
    uint8_t status;

    ///Structure buffer
    struct buffer_tag buf;
};

///HCI Debug remap complete event parameters - vendor specific
struct dbg_flash_identify_cmp_evt
{
    ///Status
    uint8_t status;
    ///Flash identity
    uint8_t flash_id;
};

///HCI Debug read memory complete event parameters - vendor specific
struct dbg_rd_mem_cmp_evt
{
    ///Status
    uint8_t status;
    ///buffer structure to return
    struct buffer_tag buf;
};

#if (BT_EMB_PRESENT)
///HCI Debug emulation finished event parameters - vendor specific
struct dbg_emul_finished_evt
{
    ///Status
    uint8_t status;
};
#endif //BT_EMB_PRESENT

/// Debug printing trace
struct dbg_trace_warning_evt
{
    ///buffer structure containing message
    struct buffer_tag buf;
};

///HCI Debug RF Register read command
struct dbg_rf_reg_rd_cmd
{
    /// register address
    uint16_t addr;
};

///HCI Debug RF Register read command complete event
struct dbg_rf_reg_rd_cmp_evt
{
    /// status
    uint8_t status;
    /// register address
    uint16_t addr;
    /// register value
    uint32_t value;
};

///HCI Debug RF Register write command
struct dbg_rf_reg_wr_cmd
{
    /// register address
    uint16_t addr;
    /// register value
    uint32_t value;
};

///HCI Debug RF Register write command complete event
struct dbg_rf_reg_wr_cmp_evt
{
    /// status
    uint8_t status;
    /// address
    uint16_t addr;
};

///HCI Debug platform reset command parameters - vendor specific
struct dbg_plf_reset_cmd
{
    /// reason
    uint8_t reason;
};

#if (RW_WLAN_COEX)
///HCI Debug wlan coexistence command parameters - vendor specific
struct dbg_wlan_coex_cmd
{
    /// State
    uint8_t state;
};
#if (RW_WLAN_COEX_TEST)
///HCI Debug wlan coexistence test scenario command parameters - vendor specific
struct dbg_wlan_coextst_scen_cmd
{
    /// Scenario
    uint32_t scenario;
};
#endif //RW_WLAN_COEX_TEST
#endif //RW_WLAN_COEX

///HCI Debug HW Register Read command parameters - vendor specific
struct dbg_hw_reg_rd_cmd
{
    /// register address
    uint16_t reg_addr;
};


///HCI Debug HW Register write command parameters - vendor specific
struct dbg_hw_reg_wr_cmd
{
    /// register address
    uint16_t reg_addr;
    /// extra parameter
    uint16_t reserved;
    /// register value
    uint32_t reg_value;
};

///HCI Debug HW Register Read Complete event parameters - vendor specific
struct dbg_hw_reg_rd_cmd_cmp_evt
{
    /// status
    uint8_t  status;
    /// register address
    uint16_t reg_addr;
    /// register value
    uint32_t reg_value;
};

///HCI Debug HW Register Write Complete event parameters - vendor specific
struct dbg_hw_reg_wr_cmd_cmp_evt
{
    /// status
    uint8_t  status;
    /// register address
    uint16_t reg_addr;
};


#if (BLE_EMB_PRESENT || BLE_HOST_PRESENT)
///HCI Debug bd address write command parameters - vendor specific
struct dbg_set_bd_addr_cmd
{
    ///bd address to set
    struct bd_addr addr;
};

///HCI Debug crc write command parameters - vendor specific
struct dbg_set_crc_cmd
{
    /// Handle pointing to the connection for which CRC has to be modified
    uint16_t conhdl;
    /// CRC to set
    struct crc_init crc;
};

///HCI Debug LLC discard command parameters - vendor specific
struct dbg_le_llcp_discard_req
{
    /// Handle pointing to the connection for which LLCP commands have to be discarded
    uint16_t conhdl;
    /// Flag indicating if the discarding has to be enabled or disabled
    uint8_t enable;
};

///HCI Debug reset counter command parameters - vendor specific
struct dbg_reset_cnt_req
{
    /// Handle pointing to the connection for which the counter have to be reseted
    uint16_t conhdl;
};

///HCI Debug Set TX Power Level Command parameters
struct dbg_set_tx_pw_cmd
{
    /// Connection handle
    uint16_t conhdl;
    /// Power level
    uint8_t  pw_lvl;
};
#endif //BLE_EMB_PRESENT || BLE_HOST_PRESENT

/*
 * TASK DESCRIPTOR DECLARATIONS
 ****************************************************************************************
 */
extern const struct ke_state_handler dbg_default_handler;
extern ke_state_t dbg_state[DBG_IDX_MAX];

/// @} DBGTASK

#endif // DBG_TASK_H_
