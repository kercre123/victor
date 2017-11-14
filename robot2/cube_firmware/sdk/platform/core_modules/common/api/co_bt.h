/**
 ****************************************************************************************
 *
 * @file co_bt.h
 *
 * @brief This file contains the common Bluetooth defines, enumerations and structures
 *        definitions for use by all modules in RW stack.
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 *
 ****************************************************************************************
 */

#ifndef CO_BT_H_
#define CO_BT_H_

/**
 ****************************************************************************************
 * @addtogroup COMMON Common SW Block
 * @ingroup ROOT
 * @brief The Common RW SW Block.
 *
 * The COMMON is the block with Bluetooth definitions and structures shared
 * to all the protocol stack blocks. This also contain software wide error code
 * definitions, mathematical functions, help functions, list and buffer definitions.
 *
 * @{
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @addtogroup CO_BT Common Bluetooth defines
 * @ingroup COMMON
 * @brief Common Bluetooth definitions and structures.
 *
 * @{
 ****************************************************************************************
 */


/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include <stdbool.h>       // standard boolean definitions
#include <stddef.h>        // standard definitions
#include <stdint.h>        // standard integer definitions

/*
 * DEFINES
 ****************************************************************************************
 */

///Length of fields in Bluetooth messages, in number of bytes
#define EVT_MASK_LEN        0x08
#define BD_ADDR_LEN         0x06
#define DEV_CLASS_LEN       0x03
#define ACCESS_ADDR_LEN     0x04
#define LE_PASSKEY_LEN      0x04
#define BD_NAME_SIZE        0xF8 // Was 0x20 for BLE HL
#define ADV_DATA_LEN        0x1F
#define BLE_DATA_LEN        0x1B
#define SCAN_RSP_DATA_LEN   0x1F
#define LE_CHNL_MAP_LEN     0x05
#define CHNL_MAP_LEN        0x0A
#define KEY_LEN             0x10
#define PRIV_KEY_LEN        24
#define PUB_KEY_LEN         48
#define CFM_LEN             0x10
#define ENC_DATA_LEN        0x10
#define RAND_VAL_LEN        0x10
#define RAND_NB_LEN         0x08
#define LE_FEATS_LEN        0x08
#define SUPP_CMDS_LEN       0x40
#define FEATS_LEN           0x08
#define NAME_VECT_SIZE      14
#define LMP_FEATS_LEN       0x08
#define LE_STATES_LEN       0x08
#define WHITE_LIST_LEN      0x0A
#define LE_FREQ_LEN         0x28
#define LE_DATA_FREQ_LEN    0x25
#define CRC_INIT_LEN        0x03
#define SESS_KEY_DIV_LEN    0x08
#define INIT_VECT_LEN       0x04
#define MIC_LEN             0x04

///Length of control packets
#define LL_CON_REQ_LEN      0x22
#define LL_CON_UP_REQ_LEN   0x0C
#define LL_CH_MAP_REQ_LEN   0x08
#define LL_TERM_IND_LEN     0x02
#define LL_ENC_REQ_LEN      0x17
#define LL_ENC_RSP_LEN      0x0D
#define LL_ST_ENC_REQ_LEN   0x01
#define LL_ST_ENC_RSP_LEN   0x01
#define LL_UNKN_RSP_LEN     0x02
#define LL_FEAT_REQ_LEN     0x09
#define LL_FEAT_RSP_LEN     0x09
#define LL_PA_ENC_REQ_LEN   0x01
#define LL_PA_ENC_RSP_LEN   0x01
#define LL_VERS_IND_LEN     0x06
#define LL_REJ_IND_LEN      0x02

/// Format of the Advertising packets
#define ADV_ADDR_OFFSET     0
#define ADV_ADDR_LEN        BD_ADDR_LEN
#define ADV_DATA_OFFSET    (ADV_ADDR_OFFSET + ADV_ADDR_LEN)

/// BLE supported features
#define BLE_ENC_FEATURE     1

/// BLE supported states
//byte 0
#define BLE_NON_CON_ADV_STATE               0x01
#define BLE_DISC_ADV_STATE                  0x02
#define BLE_CON_ADV_STATE                   0x04
#define BLE_DIRECT_ADV_STATE                0x08
#define BLE_PASS_SCAN_STATE                 0x10
#define BLE_ACTIV_SCAN_STATE                0x20
#define BLE_INIT_MASTER_STATE               0x40
#define BLE_CON_SLAVE_STATE                 0x80

//byte 1
#define BLE_NON_CON_ADV_PASS_SCAN_STATE     0x01
#define BLE_DISC_ADV_PASS_SCAN_STATE        0x02
#define BLE_CON_ADV_PASS_SCAN_STATE         0x04
#define BLE_DIRECT_ADV_PASS_SCAN_STATE      0x08
#define BLE_NON_CON_ADV_ACTIV_SCAN_STATE    0x10
#define BLE_DISC_ADV_ACTIV_SCAN_STATE       0x20
#define BLE_CON_ADV_ACTIV_SCAN_STATE        0x40
#define BLE_DIRECT_ADV_ACTIV_SCAN_STATE     0x80

//byte 2
#define BLE_NON_CON_ADV_INIT_STATE          0x01
#define BLE_DISC_ADV_INIT_STATE             0x02
#define BLE_NON_CON_ADV_MASTER_STATE        0x04
#define BLE_DISC_ADV_MASTER_STATE           0x08
#define BLE_NON_CON_ADV_SLAVE_STATE         0x10
#define BLE_DISC_ADV_SLAVE_STATE            0x20
#define BLE_PASS_SCAN_INIT_STATE            0x40
#define BLE_ACTIV_SCAN_INIT_STATE           0x80

//byte 3
#define BLE_PASS_SCAN_MASTER_STATE          0x01
#define BLE_ACTIV_SCAN_MASTER_STATE         0x02
#define BLE_PASS_SCAN_SLAVE_STATE           0x04
#define BLE_ACTIV_SCAN_SLAVE_STATE          0x08
#define BLE_INIT_MASTER_MASTER_STATE        0x10

/// BLE supported commands
//byte0
#define BLE_DISC_CMD                0x20
//byte2
#define BLE_RD_REM_VERS_CMD         0x80
//byte5
#define BLE_SET_EVT_MSK_CMD         0x40
#define BLE_RESET_CMD               0x80
//byte10
#define BLE_HL_BUF_SIZE_CMD         0x40
#define BLE_RD_TX_PWR_CMD           0x04

#define BLE_HL_NB_CMP_PKT_CMD       0x80
//byte14
#define BLE_RD_LOC_VERS_CMD         0x08
#define BLE_RD_LOC_SUP_FEAT_CMD     0x20
#define BLE_RD_BUF_SIZE_CMD         0x80
//byte15
#define BLE_RD_BD_ADDR_CMD          0x02
#define BLE_RD_RSSI_CMD             0x20
//byte25
#define BLE_LE_SET_EVT_MSK_CMD      0x01
#define BLE_LE_RD_BUF_SIZE_CMD      0x02
#define BLE_LE_RD_LOC_SUP_FEAT_CMD  0x04
#define BLE_LE_SET_RAND_ADDR_CMD    0x10
#define BLE_LE_SET_ADV_PARAM_CM     0x20
#define BLE_LE_RD_ADV_TX_PWR_CMD    0x40
#define BLE_LE_SET_ADV_DATA_CMD     0x80
//byte26
#define BLE_LE_SET_SC_RSP_DATA_CMD  0x01
#define BLE_LE_SET_ADV_EN_CMD       0x02
#define BLE_LE_SET_SC_PARAM_CMD     0x04
#define BLE_LE_SET_SC_EN_CMD        0x08
#define BLE_LE_CREAT_CNX_CMD        0x10
#define BLE_LE_CREAT_CNX_CNL_CMD    0x20
#define BLE_LE_RD_WL_SIZE_CMD       0x40
#define BLE_LE_CLEAR_WL_CMD         0x80
//byte27
#define BLE_LE_ADD_DEV_WL_CMD       0x01
#define BLE_LE_REM_DEV_WL_CMD       0x02
#define BLE_LE_CNX_UPDATE_CMD       0x04
#define BLE_LE_SET_HL_CH_CLASS_CMD  0x08
#define BLE_LE_RD_CH_MAP_CMD        0x10
#define BLE_LE_RD_REM_USED_FEAT_CMD 0x20
#define BLE_LE_ENCRYPT_CMD          0x40
#define BLE_LE_RAND_CMD             0x80
//byte28
#define BLE_LE_START_ENC_CMD        0x01
#define BLE_LE_LTK_REQ_RPLY_CMD     0x02
#define BLE_LE_LTK_REQ_NEG_RPLY_CMD 0x04
#define BLE_LE_RD_SUPP_STATES_CMD   0x08
#define BLE_LE_RX_TEST_CMD          0x10
#define BLE_LE_TX_TEST_CMD          0x20
#define BLE_LE_STOP_TEST_CMD        0x40

///BT supported features
#define BT_FEATURES_BYTE0  0
#define BT_FEATURES_BYTE1  0
#define BT_FEATURES_BYTE2  0
#define BT_FEATURES_BYTE3  0
#define BT_FEATURES_BYTE4  0
#define BT_FEATURES_BYTE5  0
#define BT_FEATURES_BYTE6  0
#define BT_FEATURES_BYTE7  0

#define FEATURE_PAGE_MAX   2
#define FEATURE_PAGE_0     0
#define FEATURE_PAGE_1     1

//Inquiry Length HCI:4.5.1
#define INQUIRY_LENGTH_MIN      0x01
#define INQUIRY_LENGTH_MAX      0x30

#define MAX_SLOT_CLOCK      ((1L<<27) - 1)

/// Link type             HCI:7.7.3
#define SCO_TYPE                0
#define ACL_TYPE                1
#define ESCO_TYPE               2
#define UNKNOWN_TYPE            3       // Used in LM
#define LE_TYPE                 4


/// Allow Role Switch     HCI:4.6.8
#define MASTER_ROLE             0
#define SLAVE_ROLE              1
#define UNKNOWN_ROLE            0xFF   //Used in LC to init the links role

/// Link policy HCI:4.6.9 and HCI:4.6.10
#define POLICY_SWITCH           0x0001
#define POLICY_HOLD             0x0002
#define POLICY_SNIFF            0x0004
#define POLICY_PARK             0x0008

/// Allow Role Switch     HCI:4.5.5
#define ROLE_SWITCH_NOT_ALLOWED 0
#define ROLE_SWITCH_ALLOWED     1

/// Valid Clock Offset    HCI:4.5.5
#define VALID_CLOCK_OFFSET      0x8000

/// AcceptConnection Role     HCI:4.5.8
#define ACCEPT_SWITCH_TO_MASTER 0
#define ACCEPT_REMAIN_SLAVE     1

/// Packet Type Flags     HCI:7.1.14
#define PACKET_TYPE_EDR_MSK         0x330E
#define PACKET_TYPE_GFSK_MSK        0xCCF8
#define PACKET_TYPE_NO_2_DH1_FLAG   0x0002
#define PACKET_TYPE_NO_3_DH1_FLAG   0x0004
#define PACKET_TYPE_DM1_FLAG        0x0008
#define PACKET_TYPE_DH1_FLAG        0x0010
#define PACKET_TYPE_HV1_FLAG        0x0020
#define PACKET_TYPE_HV2_FLAG        0x0040
#define PACKET_TYPE_HV3_FLAG        0x0080
#define PACKET_TYPE_NO_2_DH3_FLAG   0x0100
#define PACKET_TYPE_NO_3_DH3_FLAG   0x0200
#define PACKET_TYPE_DM3_FLAG        0x0400
#define PACKET_TYPE_DH3_FLAG        0x0800
#define PACKET_TYPE_NO_2_DH5_FLAG   0x1000
#define PACKET_TYPE_NO_3_DH5_FLAG   0x2000
#define PACKET_TYPE_DM5_FLAG        0x4000
#define PACKET_TYPE_DH5_FLAG        0x8000

/// Synchronous Packet Types     HCI
#define SYNC_PACKET_TYPE_HV1_FLAG       0x0001
#define SYNC_PACKET_TYPE_HV2_FLAG       0x0002
#define SYNC_PACKET_TYPE_HV3_FLAG       0x0004
#define SYNC_PACKET_TYPE_EV3_FLAG       0x0008
#define SYNC_PACKET_TYPE_EV4_FLAG       0x0010
#define SYNC_PACKET_TYPE_EV5_FLAG       0x0020

#define SYNC_PACKET_TYPE_NO_EV3_2_FLAG  0x0040
#define SYNC_PACKET_TYPE_NO_EV3_3_FLAG  0x0080
#define SYNC_PACKET_TYPE_NO_EV5_2_FLAG  0x0100
#define SYNC_PACKET_TYPE_NO_EV5_3_FLAG  0x0200

#define SYNC_PACKET_TYPE_EV3_2_FLAG     0x0040
#define SYNC_PACKET_TYPE_EV3_3_FLAG     0x0080
#define SYNC_PACKET_TYPE_EV5_2_FLAG     0x0100
#define SYNC_PACKET_TYPE_EV5_3_FLAG     0x0200

/// RWBT 1.2
#define SYNC_EV3_PACKET_SIZE         30
#define SYNC_EV4_PACKET_SIZE        120
#define SYNC_EV5_PACKET_SIZE        180

/// Packet Boundary Flag   HCI:4.4.3
#define FIRST_NF_HL_FRAG        0x00    // Non-flushable packets
#define CONT_HL_FRAG            0x01
#define FIRST_HL_FRAG           0x02
#define LMP_FRAG                0x03
#define PACKET_BOUNDARY_MASK    0x03
#define CO_IMMEDIATE_FLUSH      0x10
#define CO_NO_FLUSH             0x00
#define CO_FLUSH_MASK           0x30
#define CO_NON_FLUSHABLE        0x80

/// Packet Broadcast Flag   HCI:4.4.3
#define POINT_TO_POINT          0x00
#define ACTIVE_BCST             0x04
#define PICONET_BCST            0x08
#define BROADCAST_MASK          0x0C

/// Synchronous Packet Status Flag   HCI:5.4.3
#define CORRECTLY_RX_FLAG       0x00
#define POSSIBLY_INVALID_FLAG   0x01
#define NO_RX_DATA_FLAG         0x02
#define PARTIALLY_LOST_FLAG     0x03

/// Park mode defines LMP:3.17
#define MACCESS_MSK             0x0F
#define ACCSCHEM_MSK            0xF0

/// Device Supported Features defines LMP: 3.3
#define THREE_SLOT_FEATURE      0x01
#define FIVE_SLOT_FEATURE       0x02
#define ENCRYPT_FEATURE         0x04
#define SLOTOFF_FEATURE         0x08
#define TIMEACC_FEATURE         0x10
#define SWITCH_FEATURE          0x20
#define HOLD_FEATURE            0x40
#define SNIFF_FEATURE           0x80

#define PARK_FEATURE            0x01
#define RSSI_FEATURE            0x02
#define QUALITY_FEATURE         0x04
#define SCO_FEATURE             0x08
#define HV2_FEATURE             0x10
#define HV3_FEATURE             0x20
#define MULAW_FEATURE           0x40
#define ALAW_FEATURE            0x80

#define CVSD_FEATURE            0x01
#define PAGING_FEATURE          0x02
#define POWER_FEATURE           0x04
#define TRANSPARENT_SCO_FEATURE 0x08
#define FLOW_CNTL_LAG_SHIFT     0x04
#define BCAST_ENCRYPT_FEATURE   0x80

#define EDR_2MBPS_ACL_FEATURE   0x02
#define EDR_3MBPS_ACL_FEATURE   0x04
#define ENH_INQSCAN_FEATURE     0x08
#define ENH_INQSCAN_FEATURE     0x08
#define INT_INQSCAN_FEATURE     0x10
#define INT_PAGESCAN_FEATURE    0x20
#define RSSI_INQRES_FEATURE     0x40
#define ESCO_EV3_FEATURE        0x80

#define ESCO_EV4_FEATURE        0x01
#define ESCO_EV5_FEATURE        0x02
#define AFH_CAPABLE_S_FEATURE   0x08
#define AFH_CLASS_S_FEATURE     0x10
#define BR_EDR_NOT_FEATURE      0x20
#define LE_SUPPORT_FEATURE      0x40
#define EDR_3SLOTS_ACL_FEATURE  0x80

#define EDR_5SLOTS_ACL_FEATURE  0x01
#define SNIFF_SUBRATING_FEATURE 0x02
#define PAUSE_ENCRYPT_FEATURE   0x04
#define AFH_CAPABLE_M_FEATURE   0x08
#define AFH_CLASS_M_FEATURE     0x10
#define EDR_2MBPS_ESCO_FEATURE  0x20
#define EDR_3MBPS_ESCO_FEATURE  0x40
#define EDR_3SLOTS_ESCO_FEATURE 0x80

#define EXTENDED_INQRES_FEATURE 0x01
#define LE_BREDR_DEV_FEATURE    0x02
#define SIMPLE_PAIRING_FEATURE  0x08
#define ENCAPS_PDU_FEATURE      0x10
#define ERR_DATA_REPORT_FEATURE 0x20
#define NONFLUSH_PBF_FEATURE    0x40

#define LST_CHANGE_EVT_FEATURE  0x01
#define INQRES_TXPOW_FEATURE    0x02
#define ENHANCED_PWRCNTL_FEATURE 0x04
#define EXTENDED_FEATURE        0x80

#define HOST_SIMPLE_PAIRING_FEATURE  0x01
#define HOST_LE_FEATURE         0x02
#define HOST_LE_BR_EDR_FEATURE  0x04

/// Poll interval defines LMP:5.2
#define POLL_INTERVAL_MIN       0x0002
#define POLL_INTERVAL_DFT       0x0028
#define POLL_INTERVAL_PAGE      0x0320      //Poll interval used during page/inquiry
#define POLL_INTERVAL_MAX       0xFFFF

/// Nb of Broadcast retransmissions defines
#define NB_BROADCAST_DFT        0x01

/// Different packet types
/* Packet and buffer sizes. These sizes do not include payload header (except for FHS
 * packet where there is no payload header) since payload header is written or read by
 * the RWBT in a different control structure part (TX/RXPHDR)                          */
#define FHS_PACKET_SIZE         18
#define DM1_PACKET_SIZE         17
#define DH1_PACKET_SIZE         27
#define DH1_2_PACKET_SIZE       54
#define DH1_3_PACKET_SIZE       83
#define DV_ACL_PACKET_SIZE      9
#define DM3_PACKET_SIZE         121
#define DH3_PACKET_SIZE         183
#define DH3_2_PACKET_SIZE       367
#define DH3_3_PACKET_SIZE       552
#define DM5_PACKET_SIZE         224
#define EIR_PACKET_SIZE         240
#define DH5_PACKET_SIZE         339
#define DH5_2_PACKET_SIZE       679
#define DH5_3_PACKET_SIZE       1021
#define AUX1_PACKET_SIZE        29

#define HV1_PACKET_SIZE         10
#define HV2_PACKET_SIZE         20
#define HV3_PACKET_SIZE         30
#define EV3_PACKET_SIZE         30
#define EV3_2_PACKET_SIZE       60
#define EV3_3_PACKET_SIZE       90
#define EV4_PACKET_SIZE        120
#define EV5_PACKET_SIZE        180
#define EV5_2_PACKET_SIZE      360
#define EV5_3_PACKET_SIZE      540

/// SCO Packet coding LMP:4.6.1
#define SCO_PACKET_HV1          0x00
#define SCO_PACKET_HV2          0x01
#define SCO_PACKET_HV3          0x02

/// eSCO Packet coding LMP:4.6.2
#define ESCO_PACKET_NULL        0x00
#define ESCO_PACKET_EV3         0x07
#define ESCO_PACKET_EV4         0x0C
#define ESCO_PACKET_EV5         0x0D
#define ESCO_PACKET_EV3_2       0x26
#define ESCO_PACKET_EV3_3       0x37
#define ESCO_PACKET_EV5_2       0x2C
#define ESCO_PACKET_EV5_3       0x3D

/// Max number of HV packet BaseBand:4.4.2.1
#define MAX_NB_HV1              1
#define MAX_NB_HV2              2
#define MAX_NB_HV3              3

/// Tsco (ScoInterval) BaseBand:4.4.2.1
#define TSCO_HV1                2
#define TSCO_HV2                4
#define TSCO_HV3                6

/* Inquiry train repetition length , Baseband :Table 10.4
 *      - 256 repetitions if no SCO
 *      - 512 repetitions if 1 SCO
 *      - 768 repetitions if 2 SCO  */
#define INQ_TRAIN_LENGTH_NO_SCO 256
#define INQ_TRAIN_LENGTH_1_SCO  512
#define INQ_TRAIN_LENGTH_2_SCO  768

/* Counter for train length, Npage (N*16 slots) depends on the slave page scan mode and
 * the number of active SCO:
 *    | SR mode |  no SCO  |  one SCO  |  two SCO |
 *    |   R0    |   >=1    |    >=2    |    >=3   |
 *    |   R1    |   >=128  |    >=256  |    >=384 |
 *    |   R2    |   >=256  |    >=512  |    >=768 |   */
#define PAGE_TRAIN_LENGTH_R0    1
#define PAGE_TRAIN_LENGTH_R1    128
#define PAGE_TRAIN_LENGTH_R2    256

/// Baseband timeout default value, Baseband timers: 1.1
#define PAGE_RESP_TO_DEF        8
#define INQ_RESP_TO_DEF         128
#define NEW_CONNECTION_TO_DEF   32

/// Voice mute pattern defines
#define MU_LAW_MUTE             0xFF
#define ALAW_CVSD_MUTE          0x55
#define TRANSP_MUTE             0x00

/// Air Mode  LMP:5.2
#define MU_LAW_MODE             0
#define A_LAW_MODE              1
#define CVSD_MODE               2
#define TRANS_MODE              3

/// eSCO negotiation State LMP:5.2
#define ESCO_NEGO_INIT              0
#define ESCO_NEGO_LATEST_POSSIBLE   1
#define ESCO_NEGO_SLOT_VIOLATION    2
#define ESCO_NEGO_LAT_VIOLATION     3
#define ESCO_NEGO_UNSUPPORTED       4

#define SCO_BANDWIDTH               8000
#define SYNC_BANDWIDTH_DONT_CARE    0xFFFFFFFF

#define SYNC_MIN_LATENCY            0x0004
#define SYNC_MAX_LATENCY_ESCO_S1    0x0007
#define SYNC_MAX_LATENCY_ESCO_S2    0x0007
#define SYNC_MAX_LATENCY_ESCO_S3    0x000A
#define SYNC_DONT_CARE_LATENCY      0xFFFF

#define SYNC_NO_RE_TX           0x00
#define SYNC_RE_TX_POWER        0x01
#define SYNC_RE_TX_QUALITY      0x02
#define SYNC_RE_TX_DONT_CARE    0xFF

/// Timing Control Flags  LMP:5.2
#define TIM_CHANGE_FLAG         0x01
#define INIT2_FLAG              0x02
#define ACCESS_WIN_FLAG         0x04

/// Packet Type Table defines    LMP:4.1.11
#define PACKET_TABLE_1MBPS      0x00
#define PACKET_TABLE_2_3MBPS    0x01

/// Data Rate defines    LMP:5.2
#define FEC_RATE_MSK            0x01
#define USE_FEC_RATE            0x00
#define NO_FEC_RATE             0x01
#define PREF_PACK_MSK           0x06
#define NO_PREF_PACK_SIZE       0x00
#define USE_1_SLOT_PACKET       0x02
#define USE_3_SLOT_PACKET       0x04
#define USE_5_SLOT_PACKET       0x06
#define PREF_EDR_MSK            0x18
#define USE_DM1_ONLY            0x00
#define USE_2_MBPS_RATE         0x08
#define USE_3_MBPS_RATE         0x10
#define PREF_PACK_EDR_MSK       0x60
#define USE_1_SLOT_EDR_PKT      0x20
#define USE_3_SLOT_EDR_PKT      0x40
#define USE_5_SLOT_EDR_PKT      0x60

/// Voice setting HCI:4.7.29 & 4.7.30
#define INPUT_COD_LIN           0x0000
#define INPUT_COD_MULAW         0x0100
#define INPUT_COD_ALAW          0x0200
#define INPUT_COD_MSK           0x0300
#define INPUT_COD_OFF           8
#define INPUT_DATA_1COMP        0x0000
#define INPUT_DATA_2COMP        0x0040
#define INPUT_DATA_SMAG         0x0080
#define INPUT_DATA_UNSIGNED     0x00C0
#define INPUT_DATAFORM_MSK      0x00C0
#define INPUT_DATAFORM_OFF      6
#define INPUT_SAMP_8BIT         0x0000
#define INPUT_SAMP_16BIT        0x0020
#define INPUT_SAMPSIZE_MSK      0x0020
#define INPUT_SAMPSIZE_OFF      5
#define LIN_PCM_BIT_POS_MSK     0x001C
#define LIN_PCM_BIT_POS_OFF     2
#define AIR_COD_CVSD            0x0000
#define AIR_COD_MULAW           0x0001
#define AIR_COD_ALAW            0x0002
#define AIR_COD_TRANS           0x0003
#define AIR_COD_MSK             0x0003
#define AIR_COD_OFF             0

/// ScanEnable HCI:4.7.17
#define BOTH_SCAN_DISABLE       0
#define INQUIRY_SCAN_ENABLE     1
#define PAGE_SCAN_ENABLE        2
#define BOTH_SCAN_ENABLE        3

/// PageScanInterval and PageScanWindow HCI:4.7.19
#define PAGE_SCAN_INTERVAL_MIN  0x0012
#define PAGE_SCAN_INTERVAL_MAX  0x1000
#define PAGE_SCAN_INTERVAL_DEF  0x0800
#define PAGE_SCAN_WINDOW_MIN    0x0011
#define PAGE_SCAN_WINDOW_MAX    0x1000
#define PAGE_SCAN_WINDOW_DEF    0x0012

/// InquiryScanInterval and InquiryScanWindow HCI:4.7.19
#define INQUIRY_SCAN_INTERVAL_MIN  0x0012
#define INQUIRY_SCAN_INTERVAL_MAX  0x1000
#define INQUIRY_SCAN_INTERVAL_DEF  0x1000

#define INQUIRY_SCAN_WINDOW_MIN    0x0011
#define INQUIRY_SCAN_WINDOW_MAX    0x1000
#define INQUIRY_SCAN_WINDOW_DEF    0x0012

/// General/Unlimited Inquiry Access Code (GIAC)
#define GIAC_LAP_0              51
#define GIAC_LAP_1              139
#define GIAC_LAP_2              158

/// PINType HCI:4.7.6
#define VARIABLE_PIN            0
#define FIXED_PIN               1

/// ConnectionAcceptTimeout HCI:4.7.14
#define CON_ACCEPT_TIMEOUT_MIN  0x0001
#define CON_ACCEPT_TIMEOUT_MAX  0xB540
#define CON_ACCEPT_TIMEOUT_DEF  0x1FA0

/// PageTimeout HCI:4.7.16
#define PAGE_TIMEOUT_MIN        0x0001
#define PAGE_TIMEOUT_MAX        0xFFFF
#define PAGE_TIMEOUT_DEF        0x2000

/// AuthenticationEnable HCI:4.7.24
#define AUTH_DISABLED           0x00        // Default
#define AUTH_ENABLED            0x01

/// EncryptionMode HCI:4.7.26
#define ENC_DISABLED            0x00        // Default
#define ENC_PP_ENABLED          0x01
#define ENC_PP_BC_ENABLED       0x02

/// AutomaticFlushTimeout HCI:4.7.32
#define AUTO_FLUSH_TIMEOUT_MAX  0x07FF
#define AUTO_FLUSH_TIMEOUT_OFF  0x0000
#define AUTO_FLUSH_TIMEOUT_DEF  0x0000      // Default (no automatic flush timeout)

/// Supervision Time Out  HCI:4.7.44
#define SUPERVISION_TO_OFF      0x0000
#define SUPERVISION_TO_MIN      0x0001
#define SUPERVISION_TO_DFT      0x7D00      // Default is 20 s
#define SUPERVISION_TO_MAX      0xFFFF

/// PageScanRepetitionMode HCI:4.5.5
#define R0                      0x00
#define R1                      0x01
#define R2                      0x02
#define PAGESCAN_REP_DEF        R1          // Default

/// PageScanPeriodMode HCI:4.7.49
#define P0                      0x00        // Default
#define P1                      0x01
#define P2                      0x02

/// PageScanMode HCI:4.7.51
#define MANDATORY_PAGE_SCAN_MODE 0x00       // Default

#define OPT_PAGE_SCAN_MODE_1    0x01
#define OPT_PAGE_SCAN_MODE_2    0x02
#define OPT_PAGE_SCAN_MODE_3    0x03

/// Encryption Enable HCI:4.5.17
#define ENCRYPTION_OFF          0x00
#define ENCRYPTION_ON           0x01

/// Country Code HCI:4.8.4
#define NORTH_AMERICA_EUROPE    0x00
#define FRANCE                  0x01
#define SPAIN                   0x02
#define JAPAN                   0x03

/// Country Code HCI:4.10.2
#define NO_LOOPBACK             0x00        // Default
#define LOCAL_LOOPBACK          0x01
#define REMOTE_LOOPBACK         0x02

/// LM modes HCI:5.2.20
#define LM_ACTIVE_MODE          0x00
#define LM_HOLD_MODE            0x01
#define LM_SNIFF_MODE           0x02
#define LM_PARK_MODE            0x03

/// Key Type HCI:5.2.24
#define COMB_KEY                0
#define LOCAL_UNIT_KEY          1
#define REMOTE_UNIT_KEY         2
#define DEBUG_COMB_KEY          3
#define UNAUTH_COMB_KEY         4
#define AUTH_COMB_KEY           5
#define CHANGED_COMB_KEY        6
#define UNIT_KEY                1
#define ENC_KEY_SIZE_MAX        16
#define ENC_KEY_SIZE_MIN        7

/// Key Flag HCI:5.4.18
#define SEMI_PERMANENT_KEY      0x00
#define TEMPORARY_KEY           0x01

/// QOS Service Type HCI:4.6.6
#define QOS_NO_TRAFFIC          0x00
#define QOS_BEST_EFFORT         0x01
#define QOS_GUARANTEED          0x02
#define QOS_NOTSPECIFIED        0xFF

#define QOS_WILD_CARD           0xFFFFFFFF

/// RSSI ranges
#define RSSI_TOO_HIGH         0x01
#define RSSI_GOLDEN_RG        0x00
#define RSSI_TOO_LOW          0xFF


/// Bluetooth Test Mode defines    Bluetooth Test Mode: Table 3.2
#define TEST_MODE_OFF           0x00
#define TEST_MODE_ACTIVE        0x01

#define PAUSE_MODE              0x00
#define TXTEST0_MODE            0x01
#define TXTEST1_MODE            0x02
#define TXTEST10_MODE           0x03
#define PRAND_MODE              0x04
#define ACLLOOP_MODE            0x05
#define SCOLOOP_MODE            0x06
#define ACLNOWHIT_MODE          0x07
#define SCONOWHIT_MODE          0x08
#define TXTEST1100_MODE         0x09
#define EXITTEST_MODE           0xFF

#define HOPSINGLE               0x00
#define HOPUSA                  0x01

#define FIXTXPOW                0x00
#define ADAPTTXPOW              0x01


// Event Filter HCI 4.7.3

/// Filter type
#define CLEAR_ALL_FILTER_TYPE   0x00
#define INQUIRY_FILTER_TYPE     0x01
#define CONNECTION_FILTER_TYPE  0x02

/// Inquiry & Connection Setup Filter Condition Type
#define ALL_FILTER_CONDITION_TYPE       0x00
#define CLASS_FILTER_CONDITION_TYPE     0x01
#define BD_ADDR_FILTER_CONDITION_TYPE   0x02

/// Auto Accept Flag
#define DO_NOT_AUTO_ACCEPT_CONNECTION   0x01
#define ACCEPT_CONNECTION_SLAVE         0x02
#define ACCEPT_CONNECTION_MASTER        0x03
#define REJECT_CONNECTION               0x04        // Proprietary Flag

/// Event Mask HCI 4.7.1
#define NO_EVENTS_SPECIFIED_FILTER                            0x00000000
#define INQUIRY_COMPLETE_EVENT_FILTER                         0x00000001
#define INQUIRY_RESULT_EVENT_FILTER                           0x00000002
#define CONNECTION_COMPLETE_EVENT_FILTER                      0x00000004
#define CONNECTION_REQUEST_EVENT_FILTER                       0x00000008
#define DISCONNECTION_COMPLETE_EVENT_FILTER                   0x00000010
#define AUTHENTICATION_COMPLETE_EVENT_FILTER                  0x00000020
#define REMOTE_NAME_REQUEST_COMPLETE_EVENT_FILTER             0x00000040
#define ENCRYPTION_CHANGE_EVENT_FILTER                        0x00000080
#define CHANGE_CONNECTION_LINK_KEY_COMPLETE_EVENT_FILTER      0x00000100
#define MASTER_LINK_KEY_COMPLETE_EVENT_FILTER                 0x00000200
#define READ_REMOTE_SUPPORTED_FEATURES_COMPLETE_EVENT_FILTER  0x00000400
#define READ_REMOTE_VERSION_INFORMATION_COMPLETE_EVENT_FILTER 0x00000800
#define QOS_SETUP_COMPLETE_EVENT_FILTER                       0x00001000
#define COMMAND_COMPLETE_EVENT_FILTER                         0x00002000   // Unchecked */
#define COMMAND_STATUS_EVENT_FILTER                           0x00004000   // Unchecked */
#define HARDWARE_ERROR_EVENT_FILTER                           0x00008000
#define FLUSH_OCCURRED_EVENT_FILTER                           0x00010000
#define ROLE_CHANGE_EVENT_FILTER                              0x00020000
#define NUMBER_OF_COMPLETED_PACKETS_EVENT_FILTER              0x00040000   // Unchecked */
#define MODE_CHANGE_EVENT_FILTER                              0x00080000
#define RETURN_LINK_KEYS_EVENT_FILTER                         0x00100000
#define PIN_CODE_REQUEST_EVENT_FILTER                         0x00200000
#define LINK_KEY_REQUEST_EVENT_FILTER                         0x00400000
#define LINK_KEY_NOTIFICATION_EVENT_FILTER                    0x00800000
#define LOOPBACK_COMMAND_EVENT_FILTER                         0x01000000   // Not implemented */
#define DATA_BUFFER_OVERFLOW_EVENT_FILTER                     0x02000000
#define MAX_SLOTS_CHANGE_EVENT_FILTER                         0x04000000
#define READ_CLOCK_OFFSET_COMPLETE_EVENT_FILTER               0x08000000
#define CONNECTION_PACKET_TYPE_CHANGED_EVENT_FILTER           0x10000000
#define QOS_VIOLATION_EVENT_FILTER                            0x20000000
#define PAGE_SCAN_MODE_CHANGE_EVENT_FILTER                    0x40000000   // Deprecated */
#define PAGE_SCAN_REPETITION_MODE_CHANGE_EVENT_FILTER         0x80000000

#define FLOW_SPECIFICATION_COMPLETE_EVENT_FILTER                0x00000001
#define INQUIRY_RESULT_WITH_RSSI_EVENT_FILTER                   0x00000002
#define READ_REMOTE_EXTENDED_FEATURES_COMPLETE_EVENT_FILTER     0x00000004
#define SYNCHRONOUS_CONNECTION_COMPLETE_EVENT_FILTER            0x00000800
#define SYNCHRONOUS_CONNECTION_CHANGE_EVENT_FILTER              0x00001000
#define SNIFF_SUBRATING_EVENT_FILTER                            0x00002000
#define EXTENDED_INQUIRY_RESULT_EVENT_FILTER                    0x00004000
#define ENCRYPTION_KEY_REFRESH_COMPLETE_EVENT_FILTER            0x00008000
#define IO_CAPABILITY_REQUEST_EVENT_FILTER                      0x00010000
#define IO_CAPABILITY_REQUEST_REPLY_EVENT_FILTER                0x00020000
#define USER_CONFIRMATION_REQUEST_EVENT_FILTER                  0x00040000
#define USER_PASSKEY_REQUEST_EVENT_FILTER                       0x00080000
#define REMOTE_OOB_DATA_REQUEST_EVENT_FILTER                    0x00100000
#define SIMPLE_PAIRING_COMPLETE_EVENT_FILTER                    0x00200000
#define LINK_SUPERVISION_TIMEOUT_CHANGE_EVENT_FILTER            0x00800000
#define ENHANCED_FLUSH_COMPLETE_EVENT_FILTER                    0x01000000
#define USER_PASSKEY_NOTIFICATION_EVENT_FILTER                  0x04000000
#define KEYPRESS_NOTIFICATION_EVENT_FILTER                      0x08000000
#define REM_HOST_SUPPORTED_FEATURES_NOTIFICATION_EVENT_FILTER   0x10000000

/// HostControllerToHostFlowControl (ACL) HCI 7.3.40
#define FLOW_CONTROL_OFF                0x00
#define FLOW_CONTROL_ACL                0x01
#define FLOW_CONTROL_SCO                0x02
#define FLOW_CONTROL_ACL_SCO            0x03

/// SynchroinousFlowControlEnable (SCO) HCI 7.3.39
#define SYNC_FLOW_CONTROL_OFF           0x00
#define SYNC_FLOW_CONTROL_ON            0x01

/// Tx Power HCI:4.7.37
#define CURRENT_TX_POWER                0x00
#define MAX_TX_POWER                    0x01

/// Drift and Jitter default value LMP 5.2
#define DRIFT_BLE_DFT                   500
#define DRIFT_BT_DFT                    250
#define JITTER_DFT                      10

/// Read Stored Link Key HCI:4.7.8
#define LINK_KEY_BD_ADDR                0x00
#define LINK_KEY_ALL                    0x01

/// Read/Write Hold Mode Activity HCI:4.7.35 and 4.7.36
#define HOLD_MODE_ACTIV_DEFAULT                 0x00
#define HOLD_MODE_ACTIV_SUSP_PAGE_SCAN          0x01
#define HOLD_MODE_ACTIV_SUSP_INQUIRY_SCAN       0x02
#define HOLD_MODE_ACTIV_SUSP_PERIODIC_INQ       0x04
#define HOLD_MODE_ACTIV_NOT_MASK                0xF8

/// AFH Mode
#define AFH_DISABLED                    0x00
#define AFH_ENABLED                     0x01

/// AFH Mode
#define AFH_REPORTING_DISABLED          0x00
#define AFH_REPORTING_ENABLED           0x01

/// AFH MIn/Max interval default value is 25 s, i.e. 40000 BT slots
#define AFH_REPORT_INTERVAL_MAX         0x9C40
#define AFH_REPORT_INTERVAL_MIN         (AFH_REPORT_INTERVAL_MAX/2)

/// AFH minimum switch instant offset
#define AFH_MIN_SWITCH_OFFSET           96

/// AFH default reassessment interval in nb of minutes
#define AFH_REASS_INT_DFT                2

/// InquiryScanType and PageScanType
#define STANDARD_SCAN           0x00
#define INTERLACED_SCAN         0x01

/// InquiryMode
#define STANDARD_INQUIRY        0x00
#define RSSI_INQUIRY            0x01
#define EXTENDED_INQUIRY        0x02

/// LMP Version
#define BT_LMP_V1_0             0
#define BT_LMP_V1_1             1
#define BT_LMP_V1_2             2
#define BT_LMP_V2_0             3
#define BT_LMP_V2_1             4
#define BT_LMP_V3_0             5
#define BT_LMP_V4_0             6
#define BT_LMP_V4_1             7

/// WhichClock parameter
#define LOCAL_CLOCK             0
#define PICONET_CLOCK           1

/// Clock Accuracy parameter
#define CLOCK_ACCURACY_UNKNOWN  0xFFFF

#define SP_PASSKEY_STARTED          0x00
#define SP_PASSKEY_DIGIT_ENTERED    0x01
#define SP_PASSKEY_DIGIT_ERASED     0x02
#define SP_PASSKEY_CLEARED          0x03
#define SP_PASSKEY_COMPLETED        0x04

/// Low Power Mode
#define PARK_BEACON_MIN             0x000E

/// RWBT Device can be slave of 2 master at max
#define MAX_SLAVES_FOR_DIFFERENT_MASTERS    2
// Flags for ld_util_get_nb_acl function
/// Flag for master link
#define MASTER_FLAG       0x01
/// Flag for slave link
#define SLAVE_FLAG        0x02

#define CON_HDL_UNKNOWN     0
#define CON_HDL_ACL         1
#define CON_HDL_SYNC        2

///Default BT address (if none defined in NVDS)
//#define BT_DEFAULT_BDADDR  {{0x01, 0x23, 0x45, 0x67, 0x89, 0xAB}}
#define BT_DEFAULT_BDADDR  {{0x01, 0x23, 0x45, 0x67, 0x89, RIPPLE_ID}}

/// BLE event mask
#define    LE_CON_CMP_EVT_BIT            0
#define    LE_CON_CMP_EVT_MSK            0x01
#define    LE_ADV_REP_EVT_BIT            1
#define    LE_ADV_REP_EVT_MSK            0x02
#define    LE_CON_UPD_EVT_BIT            2
#define    LE_CON_UPD_EVT_MSK            0x04
#define    LE_CON_RD_REM_FEAT_EVT_BIT    3
#define    LE_CON_RD_REM_FEAT_EVT_MSK    0x08
#define    LE_LG_TR_KEY_REQ_EVT_BIT      4
#define    LE_LG_TR_KEY_REQ_EVT_MSK      0x10
#define    LE_DFT_EVT_MSK                0x1F


/// Enhanced Synchronous Connection HCI:7.1.41 & 7.1.42
#define    CODING_FORMAT_ULAW          0x00
#define    CODING_FORMAT_ALAW          0x01
#define    CODING_FORMAT_CVSD          0x02
#define    CODING_FORMAT_TRANSP        0x03
#define    CODING_FORMAT_LINPCM        0x04
#define    CODING_FORMAT_MSBC          0x05
#define    CODING_FORMAT_VENDSPEC      0xFF

#define    PCM_FORMAT_NA               0x00
#define    PCM_FORMAT_1SCOMP           0x01
#define    PCM_FORMAT_2SCOMP           0x02
#define    PCM_FORMAT_SIGNMAG          0x03
#define    PCM_FORMAT_UNSIGNED         0x04

#define    PCM_SAMPLE_SIZE_8BITS       8
#define    PCM_SAMPLE_SIZE_16BITS      16

#define    AUDIO_DATA_PATH_HCI               0
#define    AUDIO_DATA_PATH_PCM               1
#define    AUDIO_DATA_PATH_PRBS              2

/******************************************************************************************/
/* -------------------------   HCI DEFINITIONS Part II.E     -----------------------------*/
/******************************************************************************************/

///HCI Transport Header length - change if different transport
#define HCI_TRANSPORT_HDR_LEN                       0x01

///UART header: command message type
#define HCI_CMD_MSG_TYPE                            0x01

///UART header: ACL data message type
#define HCI_ACL_MSG_TYPE                            0x02

///UART header: Synchronous data message type
#define HCI_SYNC_MSG_TYPE                           0x03

///UART header: event message type
#define HCI_EVT_MSG_TYPE                            0x04

///HCI Command Opcode byte length
#define HCI_CMD_OPCODE_LEN         (0x02)

///HCI Event code byte length
#define HCI_EVT_CODE_LEN           (0x01)

///HCI Command/Event parameter length field byte length
#define HCI_CMDEVT_PARLEN_LEN      (0x01)

///HCI Command header length
#define HCI_CMD_HDR_LEN            (HCI_CMD_OPCODE_LEN + HCI_CMDEVT_PARLEN_LEN)

///HCI Event header length
#define HCI_EVT_HDR_LEN            (HCI_EVT_CODE_LEN + HCI_CMDEVT_PARLEN_LEN)

///HCI ACL data packet header length
#define HCI_ACL_HDR_LEN            (0x04)

///HCI sync data packet header length
#define HCI_SYNC_HDR_LEN           (0x03)

///HCI Command Complete Event minimum parameter length: 1(nb_pk)+2(opcode)
#define HCI_CCEVT_HDR_PARLEN       (0x03)

///HCI Command Complete Event header length:1(code)+1(len)+1(pk)+2(opcode)
#define HCI_CCEVT_HDR_LEN          (HCI_EVT_HDR_LEN + HCI_CCEVT_HDR_PARLEN)

///HCI Basic Command Complete Event packet length
#define HCI_CCEVT_BASIC_LEN        (HCI_CCEVT_HDR_LEN + 1)

///HCI Command Status Event parameter length - constant
#define HCI_CSEVT_PARLEN           (0x04)

///HCI Command Status Event length:1(code)+1(len)+1(st)+1(pk)+2(opcode)
#define HCI_CSEVT_LEN              (HCI_EVT_HDR_LEN + HCI_CSEVT_PARLEN)

/**************************************************************************************
 **************                       HCI COMMANDS                     ****************
 **************************************************************************************/
///Commands Opcodes: OGF(6b) | OCF(10b)
/* Some Abbreviation used in names:
 *  - LK   = Link Key
 *  - PK   = PassKey
 *  - RD   = Read
 *  - WR   = Write
 *  - REM  = Remote
 *  - STGS = Settings
 *  - CON  = Connection
 *  - CHG  = Change
 *  - RPLY = Reply
 *  - DFLT = Default
 *  - PER  = Periodic
 */

#define HCI_NO_OPERATION_CMD_OPCODE                 0x0000

//Link Control Commands
#define HCI_INQ_CMD_OPCODE                          0x0401
#define HCI_INQ_CANCEL_CMD_OPCODE                   0x0402
#define HCI_PER_INQ_MODE_CMD_OPCODE                 0x0403
#define HCI_EXIT_PER_INQ_MODE_CMD_OPCODE            0x0404
#define HCI_CREATE_CON_CMD_OPCODE                   0x0405
#define HCI_DISCONNECT_CMD_OPCODE                   0x0406
#define HCI_CREATE_CON_CANCEL_CMD_OPCODE            0x0408
#define HCI_ACCEPT_CON_REQ_CMD_OPCODE               0x0409
#define HCI_REJECT_CON_REQ_CMD_OPCODE               0x040A
#define HCI_LK_REQ_RPLY_CMD_OPCODE                  0x040B
#define HCI_LK_REQ_NEG_RPLY_CMD_OPCODE              0x040C
#define HCI_PIN_REQ_RPLY_CMD_OPCODE                 0x040D
#define HCI_PIN_REQ_NEG_RPLY_CMD_OPCODE             0x040E
#define HCI_CHG_CON_PKTYPE_CMD_OPCODE               0x040F
#define HCI_AUTH_REQUESTED_CMD_OPCODE               0x0411
#define HCI_SET_CON_ENC_CMD_OPCODE                  0x0413
#define HCI_CHG_CON_LK_CMD_OPCODE                   0x0415
#define HCI_MASTER_LK_CMD_OPCODE                    0x0417
#define HCI_REM_NAME_REQ_CMD_OPCODE                 0x0419
#define HCI_REM_NAME_REQ_CANCEL_CMD_OPCODE          0x041A
#define HCI_RD_REM_SUP_FEATS_CMD_OPCODE             0x041B
#define HCI_RD_REM_EXT_FEATS_CMD_OPCODE             0x041C
#define HCI_RD_REM_VER_INFO_CMD_OPCODE              0x041D
#define HCI_RD_CLK_OFFSET_CMD_OPCODE                0x041F
#define HCI_RD_LMP_HDL_CMD_OPCODE                   0x0420
#define HCI_SETUP_SYNC_CON_CMD_OPCODE               0x0428
#define HCI_ACCEPT_SYNC_CON_CMD_OPCODE              0x0429
#define HCI_REJECT_SYNC_CON_CMD_OPCODE              0x042A
#define HCI_IO_CAP_REQ_RPLY_CMD_OPCODE              0x042B
#define HCI_USER_CFM_REQ_RPLY_CMD_OPCODE            0x042C
#define HCI_USER_CFM_REQ_NEG_RPLY_CMD_OPCODE        0x042D
#define HCI_USER_PK_REQ_RPLY_CMD_OPCODE             0x042E
#define HCI_USER_PK_REQ_NEG_RPLY_CMD_OPCODE         0x042F
#define HCI_REM_OOB_REQ_RPLY_CMD_OPCODE             0x0430
#define HCI_REM_OOB_REQ_NEG_RPLY_CMD_OPCODE         0x0433
#define HCI_IO_CAP_REQ_NEG_RPLY_CMD_OPCODE          0x0434
#define HCI_ENH_SETUP_SYNC_CON_CMD_OPCODE           0x043D
#define HCI_ENH_ACCEPT_SYNC_CON_CMD_OPCODE          0x043E
#define HCI_LK_CNTL_OPCODE_MAX                      0x043E

//Link Policy Commands
#define HCI_HOLD_MODE_CMD_OPCODE                    0x0801
#define HCI_SNIFF_MODE_CMD_OPCODE                   0x0803
#define HCI_EXIT_SNIFF_MODE_CMD_OPCODE              0x0804
#define HCI_PARK_STATE_CMD_OPCODE                   0x0805 //name chg
#define HCI_EXIT_PARK_STATE_CMD_OPCODE              0x0806
#define HCI_QOS_SETUP_CMD_OPCODE                    0x0807
#define HCI_ROLE_DISCOVERY_CMD_OPCODE               0x0809
#define HCI_SWITCH_ROLE_CMD_OPCODE                  0x080B
#define HCI_RD_LINK_POL_STGS_CMD_OPCODE             0x080C
#define HCI_WR_LINK_POL_STGS_CMD_OPCODE             0x080D
#define HCI_RD_DFLT_LINK_POL_STGS_CMD_OPCODE        0x080E
#define HCI_WR_DFLT_LINK_POL_STGS_CMD_OPCODE        0x080F
#define HCI_FLOW_SPECIF_CMD_OPCODE                  0x0810
#define HCI_SNIFF_SUBRATING_MODE_CMD_OPCODE         0x0811
#define HCI_LK_POL_OPCODE_MAX                       0x0811

//Controller and Baseband Commands
#define HCI_SET_EVT_MASK_CMD_OPCODE                 0x0C01
#define HCI_RESET_CMD_OPCODE                        0x0C03
#define HCI_SET_EVT_FILT_CMD_OPCODE                 0x0C05
#define HCI_FLUSH_CMD_OPCODE                        0x0C08
#define HCI_RD_PIN_TYPE_CMD_OPCODE                  0x0C09
#define HCI_WR_PIN_TYPE_CMD_OPCODE                  0x0C0A
#define HCI_CREATE_NEW_UNIT_KEY_CMD_OPCODE          0x0C0B
#define HCI_RD_STORED_LK_CMD_OPCODE                 0x0C0D
#define HCI_WR_STORED_LK_CMD_OPCODE                 0x0C11
#define HCI_DEL_STORED_LK_CMD_OPCODE                0x0C12
#define HCI_WR_LOC_NAME_CMD_OPCODE                  0x0C13
#define HCI_RD_LOC_NAME_CMD_OPCODE                  0x0C14
#define HCI_RD_CON_ACCEPT_TO_CMD_OPCODE             0x0C15
#define HCI_WR_CON_ACCEPT_TO_CMD_OPCODE             0x0C16
#define HCI_RD_PAGE_TO_CMD_OPCODE                   0x0C17
#define HCI_WR_PAGE_TO_CMD_OPCODE                   0x0C18
#define HCI_RD_SCAN_EN_CMD_OPCODE                   0x0C19
#define HCI_WR_SCAN_EN_CMD_OPCODE                   0x0C1A
#define HCI_RD_PAGE_SCAN_ACTIVITY_CMD_OPCODE        0x0C1B
#define HCI_WR_PAGE_SCAN_ACTIVITY_CMD_OPCODE        0x0C1C
#define HCI_RD_INQ_SCAN_ACTIVITY_CMD_OPCODE         0x0C1D
#define HCI_WR_INQ_SCAN_ACTIVITY_CMD_OPCODE         0x0C1E
#define HCI_RD_AUTH_EN_CMD_OPCODE                   0x0C1F
#define HCI_WR_AUTH_EN_CMD_OPCODE                   0x0C20
#define HCI_RD_CLS_DEV_CMD_OPCODE                   0x0C23
#define HCI_WR_CLS_DEV_CMD_OPCODE                   0x0C24
#define HCI_RD_VOICE_STG_CMD_OPCODE                 0x0C25
#define HCI_WR_VOICE_STG_CMD_OPCODE                 0x0C26
#define HCI_RD_AUTO_FLUSH_TO_CMD_OPCODE             0x0C27
#define HCI_WR_AUTO_FLUSH_TO_CMD_OPCODE             0x0C28
#define HCI_RD_NUM_BDCST_RETX_CMD_OPCODE            0x0C29
#define HCI_WR_NUM_BDCST_RETX_CMD_OPCODE            0x0C2A
#define HCI_RD_HOLD_MODE_ACTIVITY_CMD_OPCODE        0x0C2B
#define HCI_WR_HOLD_MODE_ACTIVITY_CMD_OPCODE        0x0C2C
#define HCI_RD_TX_PW_LVL_CMD_OPCODE                 0x0C2D
#define HCI_RD_SYNC_FLOW_CNTL_EN_CMD_OPCODE         0x0C2E
#define HCI_WR_SYNC_FLOW_CNTL_EN_CMD_OPCODE         0x0C2F
#define HCI_SET_CNTLR_TO_HOST_FLOW_CNTL_CMD_OPCODE  0x0C31
#define HCI_HOST_BUFFER_SIZE_CMD_OPCODE             0x0C33
#define HCI_HOST_NB_COMPLETED_PKTS_CMD_OPCODE       0x0C35
#define HCI_RD_LINK_SUPV_TO_CMD_OPCODE              0x0C36
#define HCI_WR_LINK_SUPV_TO_CMD_OPCODE              0x0C37
#define HCI_RD_NUM_SUP_IAC_CMD_OPCODE               0x0C38
#define HCI_RD_CURR_IAC_LAP_CMD_OPCODE              0x0C39
#define HCI_WR_CURR_IAC_LAP_CMD_OPCODE              0x0C3A
#define HCI_SET_AFH_H_CHNL_CLS_CMD_OPCODE           0x0C3F
#define HCI_RD_INQ_SCAN_TYPE_CMD_OPCODE             0x0C42
#define HCI_WR_INQ_SCAN_TYPE_CMD_OPCODE             0x0C43
#define HCI_RD_INQ_MODE_CMD_OPCODE                  0x0C44
#define HCI_WR_INQ_MODE_CMD_OPCODE                  0x0C45
#define HCI_RD_PAGE_SCAN_TYPE_CMD_OPCODE            0x0C46
#define HCI_WR_PAGE_SCAN_TYPE_CMD_OPCODE            0x0C47
#define HCI_RD_AFH_CHNL_ASSES_MODE_CMD_OPCODE       0x0C48
#define HCI_WR_AFH_CHNL_ASSES_MODE_CMD_OPCODE       0x0C49
#define HCI_RD_EXT_INQ_RSP_CMD_OPCODE               0x0C51
#define HCI_WR_EXT_INQ_RSP_CMD_OPCODE               0x0C52
#define HCI_ENC_KEY_REFR_REQ_CMD_OPCODE             0x0C53
#define HCI_RD_SIMPLE_PAIR_MODE_CMD_OPCODE          0x0C55
#define HCI_WR_SIMPLE_PAIR_MODE_CMD_OPCODE          0x0C56
#define HCI_RD_LOC_OOB_DATA_CMD_OPCODE              0x0C57
#define HCI_RD_INQ_RSP_TX_PW_LVL_CMD_OPCODE         0x0C58
#define HCI_WR_INQ_RSP_TX_PW_LVL_CMD_OPCODE         0x0C59
#define HCI_RD_DFLT_ERR_DATA_REPORT_CMD_OPCODE      0x0C5A
#define HCI_WR_DFLT_ERR_DATA_REPORT_CMD_OPCODE      0x0C5B
#define HCI_ENHANCED_FLUSH_CMD_OPCODE               0x0C5F
#define HCI_SEND_KEY_PRESS_NOTIF_CMD_OPCODE         0x0C60
#define HCI_SET_EVT_MASK_PAGE_2_CMD_OPCODE          0x0C63
#define HCI_RD_FLOW_CNTL_MODE_CMD_OPCODE            0x0C66
#define HCI_WR_FLOW_CNTL_MODE_CMD_OPCODE            0x0C67
#define HCI_RD_ENHANCED_TX_PW_LVL_CMD_OPCODE        0x0C68
#define HCI_RD_LE_HOST_SUPP_CMD_OPCODE              0x0C6C
#define HCI_WR_LE_HOST_SUPP_CMD_OPCODE              0x0C6D
#define HCI_CNTLR_BB_OPCODE_MAX                     0x0C6D

//Info Params
#define HCI_RD_LOCAL_VER_INFO_CMD_OPCODE            0x1001
#define HCI_RD_LOCAL_SUPP_CMDS_CMD_OPCODE           0x1002
#define HCI_RD_LOCAL_SUPP_FEATS_CMD_OPCODE          0x1003
#define HCI_RD_LOCAL_EXT_FEATS_CMD_OPCODE           0x1004
#define HCI_RD_BUFF_SIZE_CMD_OPCODE                 0x1005
#define HCI_RD_BD_ADDR_CMD_OPCODE                   0x1009
#define HCI_RD_LOCAL_SUPP_CODECS_CMD_OPCODE         0x100B
#define HCI_INFO_PAR_OPCODE_MAX                     0x100B

//Status Params
#define HCI_RD_FAILED_CONTACT_CNT_CMD_OPCODE        0x1401
#define HCI_RST_FAILED_CONTACT_CNT_CMD_OPCODE       0x1402
#define HCI_GET_LINK_QUALITY_CMD_OPCODE             0x1403
#define HCI_RD_RSSI_CMD_OPCODE                      0x1405
#define HCI_RD_AFH_CHNL_MAP_CMD_OPCODE              0x1406
#define HCI_RD_CLK_CMD_OPCODE                       0x1407
#define HCI_RD_ENC_KEY_SIZE_CMD_OPCODE              0x1408
#define HCI_STAT_PAR_OPCODE_MAX                     0x1408

//Testing Commands
#define HCI_RD_LPBK_MODE_CMD_OPCODE                 0x1801
#define HCI_WR_LPBK_MODE_CMD_OPCODE                 0x1802
#define HCI_EN_DUT_MODE_CMD_OPCODE                  0x1803
#define HCI_WR_SIMPLE_PAIR_DBG_MODE_CMD_OPCODE      0x1804
#define HCI_TEST_OPCODE_MAX                         0x1804

/// LE Commands Opcodes
#define HCI_LE_SET_EVT_MASK_CMD_OPCODE              0x2001
#define HCI_LE_RD_BUFF_SIZE_CMD_OPCODE              0x2002
#define HCI_LE_RD_LOCAL_SUPP_FEATS_CMD_OPCODE       0x2003
#define HCI_LE_SET_RAND_ADDR_CMD_OPCODE             0x2005
#define HCI_LE_SET_ADV_PARAM_CMD_OPCODE             0x2006
#define HCI_LE_RD_ADV_CHNL_TX_PW_CMD_OPCODE         0x2007
#define HCI_LE_SET_ADV_DATA_CMD_OPCODE              0x2008
#define HCI_LE_SET_SCAN_RSP_DATA_CMD_OPCODE         0x2009
#define HCI_LE_SET_ADV_EN_CMD_OPCODE                0x200A
#define HCI_LE_SET_SCAN_PARAM_CMD_OPCODE            0x200B
#define HCI_LE_SET_SCAN_EN_CMD_OPCODE               0x200C
#define HCI_LE_CREATE_CON_CMD_OPCODE                0x200D
#define HCI_LE_CREATE_CON_CANCEL_CMD_OPCODE         0x200E
#define HCI_LE_RD_WLST_SIZE_CMD_OPCODE              0x200F
#define HCI_LE_CLEAR_WLST_CMD_OPCODE                0x2010
#define HCI_LE_ADD_DEV_TO_WLST_CMD_OPCODE           0x2011
#define HCI_LE_RMV_DEV_FROM_WLST_CMD_OPCODE         0x2012
#define HCI_LE_CON_UPDATE_CMD_OPCODE                0x2013
#define HCI_LE_SET_HOST_CHNL_CLASSIF_CMD_OPCODE     0x2014
#define HCI_LE_RD_CHNL_MAP_CMD_OPCODE               0x2015
#define HCI_LE_RD_REM_USED_FEATS_CMD_OPCODE         0x2016
#define HCI_LE_ENC_CMD_OPCODE                       0x2017
#define HCI_LE_RAND_CMD_OPCODE                      0x2018
#define HCI_LE_START_ENC_CMD_OPCODE                 0x2019
#define HCI_LE_LTK_REQ_RPLY_CMD_OPCODE              0x201A
#define HCI_LE_LTK_REQ_NEG_RPLY_CMD_OPCODE          0x201B
#define HCI_LE_RD_SUPP_STATES_CMD_OPCODE            0x201C
#define HCI_LE_RX_TEST_CMD_OPCODE                   0x201D
#define HCI_LE_TX_TEST_CMD_OPCODE                   0x201E
#define HCI_LE_TEST_END_CMD_OPCODE                  0x201F
#define HCI_LE_OPCODE_MAX                           0x201F

///HCI Commands parameter length for fixed ones
//Legacy
#define HCI_DISC_CMD_PARLEN              0x03
#define HCI_RDREMVI_CMD_PARLEN           0x02
#define HCI_SETEMSK_CMD_PARLEN           EVT_MASK_LEN
#define HCI_RST_CMD_PARLEN               0x00
#define HCI_FLSH_CMD_PARLEN              0x02
#define HCI_RDTXPW_CMD_PARLEN            0x03
#define HCI_SETFLOW_CMD_PARLEN           0x01
#define HCI_HBSZ_CMD_PARLEN              0x07
#define HCI_HNBCMPPKT_CMD_PARLEN         1+63*4
#define HCI_RDLEHS_CMD_PARLEN            0x00
#define HCI_WRLEHS_CMD_PARLEN            0x02
#define HCI_RDLOCVI_CMD_PARLEN           0x00
#define HCI_RDLOCCMD_CMD_PARLEN          0x00
#define HCI_RDLOCFEAT_CMD_PARLEN         0x00
#define HCI_RDBSZ_CMD_PARLEN             0x00
#define HCI_RDBDA_CMD_PARLEN             0x00
#define HCI_RDRSSI_CMD_PARLEN            0x02
//LE
#define HCI_LESETEMSK_CMD_PARLEN         EVT_MASK_LEN
#define HCI_LERDBSZ_CMD_PARLEN           0x00
#define HCI_LERDLOCFEAT_CMD_PARLEN       0x00
#define HCI_LESETRANDA_CMD_PARLEN        BD_ADDR_LEN
#define HCI_LESETADVP_CMD_PARLEN         0x0F
#define HCI_LERDADVPW_CMD_PARLEN         0x00
#define HCI_LESETADVDATA_CMD_PARLEN      32
#define HCI_LESETSCANRSPDATA_CMD_PARLEN  32
#define HCI_LESETADVEN_CMD_PARLEN        0x01
#define HCI_LESETSCANP_CMD_PARLEN        0x07
#define HCI_LESETSCANEN_CMD_PARLEN       0x02
#define HCI_LECRCON_CMD_PARLEN           0x19
#define HCI_LECRCONCL_CMD_PARLEN         0x00
#define HCI_LERDWLSZ_CMD_PARLEN          0x00
#define HCI_LECLWL_CMD_PARLEN            0x00
#define HCI_LEADDWL_CMD_PARLEN           0x01 + BD_ADDR_LEN
#define HCI_LEREMWL_CMD_PARLEN           0x01 + BD_ADDR_LEN
#define HCI_LECONUPD_CMD_PARLEN          0x0E
#define HCI_LESETHCHCLS_CMD_PARLEN       LE_CHNL_MAP_LEN
#define HCI_LERDCHMP_CMD_PARLEN          0x02
#define HCI_LERDREMUFEAT_CMD_PARLEN      0x02
#define HCI_LEENC_CMD_PARLEN             KEY_LEN + ENC_DATA_LEN
#define HCI_LERAND_CMD_PARLEN            0x00
#define HCI_LESTENC_CMD_PARLEN           0x02 + RAND_NB_LEN + 0x02 + KEY_LEN
#define HCI_LELTKRR_CMD_PARLEN           0x02 + KEY_LEN
#define HCI_LELTKRNR_CMD_PARLEN          0x02
#define HCI_LERDSUPST_CMD_PARLEN         0x00
#define HCI_LERXTST_CMD_PARLEN           0x01
#define HCI_LETXTST_CMD_PARLEN           0x03
#define HCI_LETSTEND_CMD_PARLEN          0x00

/**************************************************************************************
 **************                        HCI EVENTS                      ****************
 **************************************************************************************/

///Event Codes
#define HCI_INQ_CMP_EVT_CODE                          0x01
#define HCI_INQ_RES_EVT_CODE                          0x02
#define HCI_CON_CMP_EVT_CODE                          0x03
#define HCI_CON_REQ_EVT_CODE                          0x04
#define HCI_DISCONNECTION_CMPL_EVT_CODE               0x05
#define HCI_AUTH_CMP_EVT_CODE                         0x06
#define HCI_REM_NAME_REQ_CMP_EVT_CODE                 0x07
#define HCI_ENC_CHG_EVT_CODE                          0x08
#define HCI_CHG_CON_LK_CMP_EVT_CODE                   0x09
#define HCI_MASTER_LK_CMP_EVT_CODE                    0x0A
#define HCI_RD_REM_SUP_FEATS_CMP_EVT_CODE             0x0B
#define HCI_RD_REM_VER_INFO_CMPL_EVT_CODE             0x0C
#define HCI_QOS_SETUP_CMP_EVT_CODE                    0x0D
#define HCI_CMD_CMPL_EVT_CODE                         0x0E
#define HCI_CMD_STATUS_EVT_CODE                       0x0F
#define HCI_HW_ERR_EVT_CODE                           0x10
#define HCI_FLUSH_OCCURRED_EVT_CODE                   0x11
#define HCI_ROLE_CHG_EVT_CODE                         0x12
#define HCI_NB_CMPL_PKTS_EVT_CODE                     0x13
#define HCI_MODE_CHG_EVT_CODE                         0x14
#define HCI_RETURN_LINK_KEYS_EVT_CODE                 0x15
#define HCI_PIN_REQ_EVT_CODE                          0x16
#define HCI_LK_REQ_EVT_CODE                           0x17
#define HCI_LK_NOTIF_EVT_CODE                         0x18
#define HCI_DATA_BUFF_OVFLW_EVT_CODE                  0x1A
#define HCI_MAX_SLOTS_CHG_EVT_CODE                    0x1B
#define HCI_RD_CLK_OFFSET_CMP_EVT_CODE                0x1C
#define HCI_CON_PKT_TYPE_CHG_EVT_CODE                 0x1D
#define HCI_QOS_VIOL_EVT_CODE                         0x1E
#define HCI_PAGE_SCAN_REPET_MODE_CHG_EVT_CODE         0x20
#define HCI_FLOW_SPECIF_CMP_EVT_CODE                  0x21
#define HCI_INQ_RES_WITH_RSSI_EVT_CODE                0x22
#define HCI_RD_REM_EXT_FEATS_CMP_EVT_CODE             0x23
#define HCI_SYNC_CON_CMP_EVT_CODE                     0x2C
#define HCI_SYNC_CON_CHG_EVT_CODE                     0x2D
#define HCI_SNIFF_SUBRATING_EVT_CODE                  0x2E
#define HCI_EXT_INQ_RES_EVT_CODE                      0x2F
#define HCI_ENC_KEY_REFRESH_CMPL_EVT_CODE             0x30
#define HCI_IO_CAP_REQ_EVT_CODE                       0x31
#define HCI_IO_CAP_RSP_EVT_CODE                       0x32
#define HCI_USR_CFM_REQ_EVT_CODE                      0x33
#define HCI_USER_PK_REQ_EVT_CODE                      0x34
#define HCI_REM_OOB_DATA_REQ_EVT_CODE                 0x35
#define HCI_SIMPLE_PAIR_CMP_EVT_CODE                  0x36
#define HCI_LINK_SUP_TO_CHG_EVT_CODE                  0x38
#define HCI_ENHANCED_FLUSH_CMP_EVT_CODE               0x39
#define HCI_USER_PK_NOTIF_EVT_CODE                    0x3B
#define HCI_KEY_PRESS_NOTIF_EVT_CODE                  0x3C
#define HCI_REM_HOST_SUP_FEATS_NOTIF_EVT_CODE         0x3D
#define HCI_LE_META_EVT_CODE                          0x3E
#define HCI_DBG_EVT_CODE                              0xFF

/// LE Events Subcodes
#define HCI_LE_CON_CMPL_EVT_CODE                    0x01
#define HCI_LE_ADV_REPORT_EVT_CODE                  0x02
#define HCI_LE_CON_UPDATE_CMPL_EVT_CODE             0x03
#define HCI_LE_RD_REM_USED_FEATS_CMPL_EVT_CODE      0x04
#define HCI_LE_LTK_REQUEST_EVT_CODE                 0x05

///HCI Command Complete Event return parameters length in packet (status+...)
#define HCI_CCEVT_BASIC_RETPAR_LEN               0x01
#define HCI_CCEVT_CRCONCCL_RETPAR_LEN            0x01 + BD_ADDR_LEN
#define HCI_CCEVT_LKRPLY_RETPAR_LEN              0x01 + BD_ADDR_LEN
#define HCI_CCEVT_LKNRPLY_RETPAR_LEN             0x01 + BD_ADDR_LEN
#define HCI_CCEVT_PINRPLY_RETPAR_LEN             0x01 + BD_ADDR_LEN
#define HCI_CCEVT_PINNRPLY_RETPAR_LEN            0x01 + BD_ADDR_LEN
#define HCI_CCEVT_RMTNAMECCL_RETPAR_LEN          0x01 + BD_ADDR_LEN
#define HCI_CCEVT_RDLMPHDL_RETPAR_LEN            0x08
#define HCI_CCEVT_IORPLY_RETPAR_LEN              0x01 + BD_ADDR_LEN
#define HCI_CCEVT_USRCFMRPLY_RETPAR_LEN          0x01 + BD_ADDR_LEN
#define HCI_CCEVT_USRCFMNRPLY_RETPAR_LEN         0x01 + BD_ADDR_LEN
#define HCI_CCEVT_USRPKRPLY_RETPAR_LEN           0x01 + BD_ADDR_LEN
#define HCI_CCEVT_USRPKNRPLY_RETPAR_LEN          0x01 + BD_ADDR_LEN
#define HCI_CCEVT_RMTOOBRPLY_RETPAR_LEN          0x01 + BD_ADDR_LEN
#define HCI_CCEVT_RMTOOBNRPLY_RETPAR_LEN         0x01 + BD_ADDR_LEN
#define HCI_CCEVT_IONRPLY_RETPAR_LEN             0x01 + BD_ADDR_LEN
#define HCI_CCEVT_ROLEDISC_RETPAR_LEN            0x04
#define HCI_CCEVT_RDLINKSTG_RETPAR_LEN           0x05
#define HCI_CCEVT_WRLINKSTG_RETPAR_LEN           0x03
#define HCI_CCEVT_RDDFLINKSTG_RETPAR_LEN         0x03
#define HCI_CCEVT_SNIFFSUBR_RETPAR_LEN           0x03
#define HCI_CCEVT_FLUSH_RETPAR_LEN               0x03
#define HCI_CCEVT_RDPINTYPE_RETPAR_LEN           0x02
#define HCI_CCEVT_RDSTORLK_RETPAR_LEN            0x05
#define HCI_CCEVT_WRSTORLK_RETPAR_LEN            0x02
#define HCI_CCEVT_DELSTORLK_RETPAR_LEN           0x03
#define HCI_CCEVT_RDLNAME_RETPAR_LEN             249
#define HCI_CCEVT_RDCONACCTO_RETPAR_LEN          0x03
#define HCI_CCEVT_RDPAGETO_RETPAR_LEN            0x03
#define HCI_CCEVT_RDSCANEN_RETPAR_LEN            0x02
#define HCI_CCEVT_RDPAGESCANACT_RETPAR_LEN       0x05
#define HCI_CCEVT_RDINQSCANACT_RETPAR_LEN        0x05
#define HCI_CCEVT_RDAUTHEN_RETPAR_LEN            0x02
#define HCI_CCEVT_RDENC_RETPAR_LEN               0x02
#define HCI_CCEVT_RDVOICESTG_RETPAR_LEN          0x03
#define HCI_CCEVT_RDCLSDEV_RETPAR_LEN            0x04
#define HCI_CCEVT_RDVOICESTG_RETPAR_LEN          0x03
#define HCI_CCEVT_RDAFLSHTO_RETPAR_LEN           0x05
#define HCI_CCEVT_WRAFLSHTO_RETPAR_LEN           0x03
#define HCI_CCEVT_RDNBDRETX_RETPAR_LEN           0x02
#define HCI_CCEVT_RDHOLDACT_RETPAR_LEN           0x02
#define HCI_CCEVT_RDTXPW_RETPAR_LEN              0x04
#define HCI_CCEVT_RDSYNCFLWEN_RETPAR_LEN         0x02
#define HCI_CCEVT_RDLINKSUPTO_RETPAR_LEN         0x05
#define HCI_CCEVT_WRLINKSUPTO_RETPAR_LEN         0x03
#define HCI_CCEVT_RDNUMIAC_RETPAR_LEN            0x02
//#define HCI_CCEVT_RDIACLAP_RETPAR_LEN            5+3*n
#define HCI_CCEVT_RDINQSCNTYP_RETPAR_LEN         0x02
#define HCI_CCEVT_RDINQMODE_RETPAR_LEN           0x02
#define HCI_CCEVT_RDPAGESCNTYP_RETPAR_LEN        0x02
#define HCI_CCEVT_RDAFHASES_RETPAR_LEN           0x02
#define HCI_CCEVT_RDEXTINQRSP_RETPAR_LEN         242
#define HCI_CCEVT_RDSIMPPAIR_RETPAR_LEN          0x02
#define HCI_CCEVT_RDLOCOOB_RETPAR_LEN            0x21
#define HCI_CCEVT_RDINQRSPTXPW_RETPAR_LEN        0x02
#define HCI_CCEVT_RDERRDATA_RETPAR_LEN           0x02
#define HCI_CCEVT_SNDKEYNOTIF_RETPAR_LEN         1 + BD_ADDR_LEN
//new?
#define HCI_CCEVT_RDLOGLINKACCTO_RETPAR_LEN      0x03
#define HCI_CCEVT_RDFLWMOD_RETPAR_LEN            0x02
#define HCI_CCEVT_RDENHTXPW_RETPAR_LEN           0x06
#define HCI_CCEVT_RDBSTFLSHTO_RETPAR_LEN         0x05
#define HCI_CCEVT_RDFLWCNTLMODE_RETPAR_LEN       0x02
#define HCI_CCEVT_RDLEH_RETPAR_LEN               0x03
//end new

//info params
#define HCI_CCEVT_RDLOCVER_RETPAR_LEN            0x09
#define HCI_CCEVT_RDLOCCMD_RETPAR_LEN            0x01 + SUPP_CMDS_LEN
#define HCI_CCEVT_RDLOCFEAT_RETPAR_LEN           0x01 + FEATS_LEN
#define HCI_CCEVT_RDLOCEXTFEAT_RETPAR_LEN        0x0B
#define HCI_CCEVT_RDBUFSZ_RETPAR_LEN             0x08
#define HCI_CCEVT_RDBDADDR_RETPAR_LEN            0x01 + BD_ADDR_LEN
#define HCI_CCEVT_RDLOCCODECS_RETPAR_LEN         0x01 //+ SUPP_CODECS_LEN (variable length)

//status params
#define HCI_CCEVT_RDFAILCNT_RETPAR_LEN           0x05
#define HCI_CCEVT_RSTFAILCNT_RETPAR_LEN          0x03
#define HCI_CCEVT_GETLINKQ_RETPAR_LEN            0x04
#define HCI_CCEVT_RDRSSI_RETPAR_LEN              0x04
#define HCI_CCEVT_RDAFHMAP_RETPAR_LEN            0x0E
#define HCI_CCEVT_RDCLK_RETPAR_LEN               0x09
#define HCI_CCEVT_RDENCKSZ_RETPAR_LEN            0x04
//testing
#define HCI_CCEVT_RDLPBKMOD_RETPAR_LEN           0x02

///HCI LE command complete events length
#define HCI_CCEVT_LERDBFSZ_RETPAR_LEN    0x04
#define HCI_CCEVT_LERDLOCFEAT_RETPAR_LEN 0x01 + LE_FEATS_LEN
#define HCI_CCEVT_LERDADVTXPW_RETPAR_LEN 0x02
#define HCI_CCEVT_LERDWLSZ_RETPAR_LEN    0x02
#define HCI_CCEVT_LERDCHMAP_RETPAR_LEN   0x03 + LE_CHNL_MAP_LEN
#define HCI_CCEVT_LERDREMFEAT_RETPAR_LEN 0x04 + LE_FEATS_LEN
#define HCI_CCEVT_LEENC_RETPAR_LEN       0x01 + ENC_DATA_LEN
#define HCI_CCEVT_LERAND_RETPAR_LEN      0x01 + RAND_NB_LEN
#define HCI_CCEVT_LELTKRR_RETPAR_LEN     0x03
#define HCI_CCEVT_LELTKRNR_RETPAR_LEN    0x03
#define HCI_CCEVT_LERDSUPST_RETPAR_LEN   0x01 + LE_STATES_LEN
#define HCI_CCEVT_LETSTEND_RETPAR_LEN    0x03
///HCI Special LE events length
#define HCI_LECONCMP_EVTPAR_LEN                  0x13
#define HCI_LECONUPDP_EVTPAR_LEN                 0x0A
#define HCI_LERDREMFEATS_EVTPAR_LEN              0x0C
#define HCI_LELTKREQ_EVTPAR_LEN                  0x0D
#define HCI_DISCCMP_EVTPAR_LEN                   0x04
#define HCI_RDREMVI_EVTPAR_LEN                   0x08
#define HCI_FLSHOCC_EVTPAR_LEN                   0x02
#define HCI_ENCCHG_EVTPAR_LEN                    0x04
#define HCI_ENCREFSH_EVTPAR_LEN                  0x03
#define HCI_HWERR_EVTPAR_LEN                     0x01
#define HCI_DBOVFLW_EVTPAR_LEN                   0x01

//special events param len - their hdr is only 2B(code+len)
//#define HCI_INQRES_EVTPAR_LEN            variable
#define HCI_INQCMP_EVTPAR_LEN            0x01
#define HCI_CONCMP_EVTPAR_LEN            0x0B
#define HCI_CONREQ_EVTPAR_LEN            0x0A
#define HCI_DISCCMP_EVTPAR_LEN           0x04
#define HCI_AUTHCMP_EVTPAR_LEN           0x03
#define HCI_RDRMTNAME_EVTPAR_LEN         0xFF
#define HCI_ENCCHG_EVTPAR_LEN            0x04
#define HCI_CHGCONLK_EVTPAR_LEN          0x03
#define HCI_MLKCMP_EVTPAR_LEN            0x04
#define HCI_RDREMFEATS_EVTPAR_LEN        0x03 + FEATS_LEN
#define HCI_RDREMVI_EVTPAR_LEN           0x08
#define HCI_SETQOS_EVTPAR_LEN            0x15
#define HCI_HWERR_EVTPAR_LEN             0x01
#define HCI_FLSHOCC_EVTPAR_LEN           0x02
#define HCI_ROLECHG_EVTPAR_LEN           0x08
#define HCI_MODECHG_EVTPAR_LEN           0x06
#define HCI_RETLK_EVTPAR_LEN             0x17
#define HCI_PINREQ_EVTPAR_LEN            BD_ADDR_LEN
#define HCI_LKREQ_EVTPAR_LEN             BD_ADDR_LEN
#define HCI_LKNTF_EVTPAR_LEN             BD_ADDR_LEN+KEY_LEN+1
#define HCI_DBOVFLW_EVTPAR_LEN           0x01
#define HCI_MAXSLOTS_EVTPAR_LEN          0x03
#define HCI_RDCLKOFF_EVTPAR_LEN          0x05
#define HCI_CONPKTYPCHG_EVTPAR_LEN       0x05
#define HCI_QOSVIOL_EVTPAR_LEN           0x02
#define HCI_FLWSPECCMP_EVTPAR_LEN        22
#define HCI_RDRMTEXTF_EVTPAR_LEN         0x05 + FEATS_LEN
#define HCI_SYNCCONCMP_EVTPAR_LEN        0x11
#define HCI_SYNCCONCHG_EVTPAR_LEN        0x09
#define HCI_SNIFFSUBR_EVTPAR_LEN         0x0B
#define HCI_ENCREFSH_EVTPAR_LEN          0x03
#define HCI_IOCAPREQ_EVTPAR_LEN          BD_ADDR_LEN
#define HCI_IOCAPRSP_EVTPAR_LEN          BD_ADDR_LEN + 0x03
#define HCI_USRCFMREQ_EVTPAR_LEN         BD_ADDR_LEN + 0x04
#define HCI_USRPKREQ_EVTPAR_LEN          BD_ADDR_LEN
#define HCI_RMTOOBREQ_EVTPAR_LEN         BD_ADDR_LEN
#define HCI_SPCMP_EVTPAR_LEN             0x01 + BD_ADDR_LEN
#define HCI_LKSUPTOCHG_EVTPAR_LEN        0x04
#define HCI_ENHFLSHCMP_EVTPAR_LEN        0x02
#define HCI_USRPKNTF_EVTPAR_LEN          BD_ADDR_LEN + 0x04
#define HCI_KEYPRESSNTF_EVTPAR_LEN       BD_ADDR_LEN + 0x01
#define HCI_RMTHFEATNTF_EVTPAR_LEN       0x0E
/*
 * ENUMERATIONS
 ****************************************************************************************
 */

///Transmit Power level types
enum
{
    ///Current Power Level
    TX_PW_LVL_CURRENT             = 0x00,
    ///Maximum power level
    TX_PW_LVL_MAX,
    ///Enumeration end value for power level type value check
    TX_PW_LVL_END,
};

///Controller to Host flow control
enum
{
    /// C-> H flow control off
    FLOW_CTRL_OFF                 = 0x00,
    ///C->H ACL flow control on only
    FLOW_CTRL_ON_ACL_OFF_SYNC,
    ///C->H Sync flow control on only
    FLOW_CTRL_OFF_ACL_ON_SYNC,
    ///C->H ACL and Sync  flow control on
    FLOW_CTRL_ON_ACL_ON_SYNC,
    ///Enumeration end value for flow control value check
    FLOW_CTRL_END
};

///LE Supported Host enable
enum
{
    ///Disable LE supported Host
    LE_SUPP_HOST_DIS              = 0x00,
    ///Enable LE Supported Host
    LE_SUPP_HOST_EN,
    ///Enumeration end value for LE supported Host enable check
    LE_SUPP_HOST_END
};

///Simultaneous LE Host enable
enum
{
    ///Disable LE simultaneous Host disable
    SIMULT_LE_HOST_DIS            = 0x00,
    ///Enable LE simultaneous Host disable
    SIMULT_LE_HOST_EN,
    ///Enumeration end value for LE simultaneous Host enable check
    SIMULT_LE_HOST_END
};

///Advertising HCI Type
enum
{
    ///Connectable Undirected advertising
    ADV_CONN_UNDIR                = 0x00,
    ///Connectable directed advertising
    ADV_CONN_DIR,
    ///Discoverable undirected advertising
    ADV_DISC_UNDIR,
    ///Non-connectable undirected advertising
    ADV_NONCONN_UNDIR,
    ///Enumeration end value for advertising type value check
    ADV_END
};

///Scanning HCI Type
enum
{
    ///Scan request
    SCAN_REQ,
    ///Scan response
    SCAN_RSP,
    ///Enumeration end value for scanning type value check
    SCAN_LEN
};

///BD address type
enum
{
    ///Public BD address
    ADDR_PUBLIC                   = 0x00,
    ///Random BD Address
    ADDR_RAND,
    ///Enumeration end value for BD address type value check
    ADDR_END
};

///Advertising channels enables
enum adv_channel_map
{
    ///Byte value for advertising channel map for channel 37 enable
    ADV_CHNL_37_EN                = 0x01,
    ///Byte value for advertising channel map for channel 38 enable
    ADV_CHNL_38_EN,
    ///Byte value for advertising channel map for channel 39 enable
    ADV_CHNL_39_EN                = 0x04,
    ///Byte value for advertising channel map for channel 37, 38 and 39 enable
    ADV_ALL_CHNLS_EN              = 0x07,
    ///Enumeration end value for advertising channels enable value check
    ADV_CHNL_END
};

///Advertising filter policy
enum adv_filter_policy
{
    ///Allow both scan and connection requests from anyone
    ADV_ALLOW_SCAN_ANY_CON_ANY    = 0x00,
    ///Allow both scan req from White List devices only and connection req from anyone
    ADV_ALLOW_SCAN_WLST_CON_ANY,
    ///Allow both scan req from anyone and connection req from White List devices only
    ADV_ALLOW_SCAN_ANY_CON_WLST,
    ///Allow scan and connection requests from White List devices only
    ADV_ALLOW_SCAN_WLST_CON_WLST,
    ///Enumeration end value for advertising filter policy value check
    ADV_ALLOW_SCAN_END
};

///Advertising enables
enum
{
    ///Disable advertising
    ADV_DIS                       = 0x00,
    ///Enable advertising
    ADV_EN,
    ///Enumeration end value for advertising enable value check
    ADV_EN_END
};

///LE Scan type
enum
{
    ///Passive scan
    SCAN_PASSIVE                  = 0x00,
    ///Active scan
    SCAN_ACTIVE,
    ///Enumeration end value for scan type value check
    SCAN_END
};

///Scan filter policy
enum scan_filter_policy
{
    ///Allow advertising packets from anyone
    SCAN_ALLOW_ADV_ALL            = 0x00,
    ///Allow advertising packets from White List devices only
    SCAN_ALLOW_ADV_WLST,
    ///Enumeration end value for scan filter policy value check
    SCAN_ALLOW_ADV_END
};

///Le Scan enables
enum
{
    ///Disable scan
    SCAN_DIS                      = 0x00,
    ///Enable scan
    SCAN_EN,
    ///Enumeration end value for scan enable value check
    SCAN_EN_END
};

///Filter duplicates
enum scan_dup_filter_policy
{
    ///Disable filtering of duplicate packets
    SCAN_FILT_DUPLIC_DIS          = 0x00,
    ///Enable filtering of duplicate packets
    SCAN_FILT_DUPLIC_EN,
    ///Enumeration end value for scan duplicate filtering value check
    SCAN_FILT_DUPLIC_END
};

///Initiator Filter policy
enum
{
    ///Initiator will ignore White List
    INIT_FILT_IGNORE_WLST         = 0x00,
    ///Initiator will use White List
    INIT_FILT_USE_WLST,
    ///Enumeration end value for initiator filter policy value check
    INIT_FILT_END
};

///Transmitter test Packet Payload Type
enum
{
    ///Pseudo-random 9 TX test payload type
    PAYL_PSEUDO_RAND_9            = 0x00,
    ///11110000 TX test payload type
    PAYL_11110000,
    ///10101010 TX test payload type
    PAYL_10101010,
    ///Pseudo-random 15 TX test payload type
    PAYL_PSEUDO_RAND_15,
    ///All 1s TX test payload type
    PAYL_ALL_1,
    ///All 0s TX test payload type
    PAYL_ALL_0,
    ///00001111 TX test payload type
    PAYL_00001111,
    ///01010101 TX test payload type
    PAYL_01010101,
    ///Enumeration end value for TX test payload type value check
    PAYL_END
};

/// Constant defining the role
enum
{
    ///Master role
    ROLE_MASTER,
    ///Slave role
    ROLE_SLAVE,
    ///Enumeration end value for role value check
    ROLE_END
};

/// Constant clock accuracy
enum
{
    ///Clock accuracy at 500PPM
    SCA_500PPM,
    ///Clock accuracy at 250PPM
    SCA_250PPM,
    ///Clock accuracy at 150PPM
    SCA_150PPM,
    ///Clock accuracy at 100PPM
    SCA_100PPM,
    ///Clock accuracy at 75PPM
    SCA_75PPM,
    ///Clock accuracy at 50PPM
    SCA_50PPM,
    ///Clock accuracy at 30PPM
    SCA_30PPM,
    ///Clock accuracy at 20PPM
    SCA_20PPM
};

/// Controller to host flow control
enum
{
    ///Flow control OFF
    FC_OFF,
    ///ACL data flow control ON, Sync data OFF
    FC_DATA_ON_SYNC_OFF,
    ///ACL data flow control OFF, Sync data ON
    FC_DATA_OFF_SYNC_ON,
    /// ACL and Sync data flow control ON
    FC_ON
};

/// Control packet opcode
enum
{
    /// Connection update request
    LL_CONNECTION_UPDATE_REQ,
    /// Channel map request
    LL_CHANNEL_MAP_REQ,
    /// Termination indication
    LL_TERMINATE_IND,
    /// Encryption request
    LL_ENC_REQ,
    /// Encryption response
    LL_ENC_RSP,
    /// Start encryption request
    LL_START_ENC_REQ,
    /// Start encryption response
    LL_START_ENC_RSP,
    /// Unknown response
    LL_UNKNOWN_RSP,
    /// Feature request
    LL_FEATURE_REQ,
    /// Feature response
    LL_FEATURE_RSP,
    /// Pause encryption request
    LL_PAUSE_ENC_REQ,
    /// Pause encryption response
    LL_PAUSE_ENC_RSP,
    /// Version indication
    LL_VERSION_IND,
    /// Reject indication
    LL_REJECT_IND,
    /// Opcode length
    LL_OPCODE_MAX,
    /// Idle
    LL_IDLE
};

/// LLID packet
enum
{
    /// Reserved for future use
    LLID_RFU,
    /// Continue
    LLID_CONTINUE,
    /// Start
    LLID_START,
    /// Control
    LLID_CNTL,
    /// End
    LLID_END,
};


/*
 * STRUCTURE DEFINITONS
 ****************************************************************************************
 */

///BD name structure
struct bd_name
{
    ///length for name
    uint8_t namelen;
    ///array of bytes for name
    uint8_t name[BD_NAME_SIZE];
};

///Structure device name
struct device_name
{
    ///array of bytes for name
    uint8_t name[BD_NAME_SIZE];
};

///Structure name vector
struct name_vect
{
    uint8_t vect[NAME_VECT_SIZE];
};

/// lap structure
struct lap
{
    /// LAP
    uint8_t A[3];
};

/// class structure
struct devclass
{
    /// class
    uint8_t A[DEV_CLASS_LEN];
};

///Extended inquiry response structure
struct eir
{
    /// eir data
    uint8_t data[240];
};

///Duration structure
struct duration
{
    /// Time duration
    uint32_t time;
};

///Event mask structure
struct evt_mask
{
    ///8-byte array for mask value
    uint8_t mask[EVT_MASK_LEN];
};

///Host number of completed packets structure, for 1 connection handle
struct host_cmpl_pkts
{
    ///Connection handle
    uint16_t  con_hdl;
    ///Number of completed packets
    uint16_t  nb_cmpl_pkts;
};

///BD Address structure
struct bd_addr
{
    ///6-byte array address value
    uint8_t  addr[BD_ADDR_LEN];
};

///Access Address structure
struct access_addr
{
    ///4-byte array access address
    uint8_t  addr[ACCESS_ADDR_LEN];
};

///Advertising data structure
struct adv_data
{
    ///Maximum length data bytes array
    uint8_t        data[ADV_DATA_LEN];
};

///Scan response data structure
struct scan_rsp_data
{
    ///Maximum length data bytes array
    uint8_t        data[SCAN_RSP_DATA_LEN];
};

///Channel map structure
struct chnl_map
{
    ///10-bytes channel map array
    uint8_t map[CHNL_MAP_LEN];
};

///Channel map structure
struct le_chnl_map
{
    ///5-byte channel map array
    uint8_t map[LE_CHNL_MAP_LEN];
};

///Long Term Key structure
struct ltk
{
    ///16-byte array for LTK value
    uint8_t ltk[KEY_LEN];
};

/// Bluetooth address with link key
struct bd_addr_plus_key
{
    ///BdAddr
    struct bd_addr  bd_addr;
    ///Read all flag
    struct ltk link_key;
};

///Random number structure
struct rand_nb
{
    ///8-byte array for random number
    uint8_t     nb[RAND_NB_LEN];
};

///Advertising report structure
struct adv_report
{
    ///Event type:
    /// - ADV_CONN_UNDIR: Connectable Undirected advertising
    /// - ADV_CONN_DIR: Connectable directed advertising
    /// - ADV_DISC_UNDIR: Discoverable undirected advertising
    /// - ADV_NONCONN_UNDIR: Non-connectable undirected advertising
    uint8_t        evt_type;
    ///Advertising address type: public/random
    uint8_t        adv_addr_type;
    ///Advertising address value
    struct bd_addr adv_addr;
    ///Data length in advertising packet
    uint8_t        data_len;
    ///Data of advertising packet
    uint8_t        data[ADV_DATA_LEN];
    ///RSSI value for advertising packet
    uint8_t        rssi;
};

///Supported LE Features structure
struct le_features
{
    ///8-byte array for LE features
    uint8_t feats[LE_FEATS_LEN];
};

///Simple pairing hash structure
struct hash
{
    ///16-byte array for LTK value
    uint8_t C[KEY_LEN];
};

///Simple pairing randomizer structure
struct randomizer
{
    ///16-byte array for LTK value
    uint8_t R[KEY_LEN];
};

///Pin code structure
struct pin_code
{
    ///16-byte array for PIN value
    uint8_t pin[16];
};

///Sres structure
struct sres_nb
{
    ///8-byte array for random number
    uint8_t  nb[4];
};

///aco structure
struct aco
{
    ///8-byte array for random number
    uint8_t a[12];
};

///struct byte 16 to stay align with the sdl version
struct byte16{
    uint8_t A[16];
};

///Controller number of completed packets structure
struct nb_cmpl_pk
{
    ///Connection handle
    uint16_t con_hdl;
    ///Controller number of data packets that have been completed since last time
    uint16_t nb_hc_cmpl_pkts;
};

///Supported Features  structure
struct features
{
    ///8-byte array for features
    uint8_t feats[FEATS_LEN];
};

///Supported commands structure
struct supp_cmds
{
    ///64-byte array for supported commands
    uint8_t cmds[SUPP_CMDS_LEN];
};

///Supported LMP features structure
struct lmp_features
{
    ///8-byte array for LMp features
    uint8_t feats[FEATS_LEN];
};

///Simple pairing IO capabilities
struct io_capability
{
    ///IO capability
    uint8_t  io_cap;
    ///Out Of Band Data present
    uint8_t  oob_data;
    ///Authentication Requirement
    uint8_t  aut_req;
};

///Public key
struct pub_key
{
    uint8_t p_key[PUB_KEY_LEN/2];
};

///Simple pairing public keys
struct sp_pub_key
{
    ///Public key X
    struct pub_key X;
    ///Public key Y
    struct pub_key Y;
};

///Supported LE states structure
struct le_states
{
    ///8-byte array for LE states
    uint8_t  supp_states[LE_STATES_LEN];
};

///White List element structure
struct white_list
{
    ///BD address of device entry
    struct bd_addr wl_bdaddr;
    ///BD address type of device entry
    uint8_t wl_bdaddr_type;
};

///CRC initial value structure
struct crc_init
{
    ///3-byte array CRC initial value
    uint8_t crc[CRC_INIT_LEN];
};

///Session key diversifier master or slave structure
struct sess_k_div_x
{
    ///8-byte array for diversifier value
    uint8_t skdiv[SESS_KEY_DIV_LEN];
};

///Session key diversifier structure
struct sess_k_div
{
    ///16-byte array for session key diversifier.
    uint8_t skd[2*SESS_KEY_DIV_LEN];
};

///Initiator vector
struct init_vect
{
    ///4-byte array for vector
    uint8_t iv[INIT_VECT_LEN];
};


/// @} CO_BT
#endif // CO_BT_H_
