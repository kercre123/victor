/**
 ****************************************************************************************
 *
 * @file nvds.h
 *
 * @brief Non Volatile Data Storage (NVDS) driver
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 ****************************************************************************************
 */
#ifndef _NVDS_H_
#define _NVDS_H_

/**
 ****************************************************************************************
 * @addtogroup NVDS
 * @ingroup COMMON
 * @brief Non Volatile Data Storage (NVDS)
 *
 *   Parameters management
 *   there are two compilation options:
 *     + NVDS_8BIT_TAGLENGTH :
 *       if set, each TAG has a maximum length of 256 bytes
 *       if not set, each TAG has a maximum length of 65536 bytes
 *     + NVDS_PACKED :
 *       if not set, all the TAG header structures and TAG data contents are stored with an
 *       alignment on 32 bit boundary
 *       if set, all the TAG header structures and TAG data contents are stored
 *       consecutively without gaps (as would be a structure with pragma packed)
 *     + NVDS_READ_WRITE :
 *       if not set, only GET action on TAGs is provided.
 *       if set, PUT/DEL/LOCK actions are provided in addition of GET action.
 *
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include <stdbool.h>           // boolean definition
#include <stdint.h>            // integer definition
#include "co_utils.h"

#if defined(__DA14583__) && !(BDADDR_FROM_DA14583_FLASH_DISABLED)
#include "user_periph_setup.h"
#include "spi_flash.h"
#endif 


/*
 * DEFINES
 ****************************************************************************************
 */

/// NVDS location in FLASH
#ifndef __DA14581__
#define NVDS_FLASH_ADDRESS          (0x00000340)
#else
#define NVDS_FLASH_ADDRESS          (0x00000350)
#endif

/// NVDS size in RAM
#define NVDS_FLASH_SIZE             (0x00000100)

/// NVDS is defined as read-write
#define NVDS_READ_WRITE          0 //1

/// NVDS is defined as packed
#define NVDS_PACKED              0//1

/// NVDS has 8-bit length tags
#define NVDS_8BIT_TAGLENGTH      1

/// Type of the tag length (8 or 16 bits)
#if (NVDS_8BIT_TAGLENGTH)
typedef uint8_t nvds_tag_len_t;
#else
typedef uint16_t nvds_tag_len_t;
#endif // NVDS_8BIT_TAGLENGTH

/*
 * ENUMERATION DEFINITIONS
 ****************************************************************************************
 */

/// Possible Returned Status
enum NVDS_STATUS
{
    /// NVDS status OK
    NVDS_OK,
    /// generic NVDS status KO
    NVDS_FAIL,
    /// NVDS TAG unrecognized
    NVDS_TAG_NOT_DEFINED,
    /// No space for NVDS
    NVDS_NO_SPACE_AVAILABLE,
    /// Length violation
    NVDS_LENGTH_OUT_OF_RANGE,
    /// NVDS parameter locked
    NVDS_PARAM_LOCKED,
    /// NVDS corrupted
    NVDS_CORRUPT
};

/// List of NVDS TAG identifiers
enum NVDS_TAG
{
    /// Definition of the tag associated to each parameters
    /// Local Bd Address
    NVDS_TAG_BD_ADDRESS                 = 0x01,
    /// Device Name
    NVDS_TAG_DEVICE_NAME                = 0x02,
    /// Minimum Encryption Key Size
    NVDS_TAG_MIN_ENCRYPTION_KEY_SIZE    = 0x03,
    /// Maximum Encryption Key Size
    NVDS_TAG_MAX_ENCRYPTION_KEY_SIZE    = 0x04,
    /// Preferred Key
    NVDS_TAG_PREFERRED_KEY              = 0x05,
    /// Unit Key
    NVDS_TAG_UNIT_KEY                   = 0x06,
    /// Radio Drift
    NVDS_TAG_LPCLK_DRIFT                = 0x07,
    /// Radio Jitter
    NVDS_TAG_LPCLK_JITTER               = 0x08,
    /// Radio Class
    NVDS_TAG_RADIO_CLASS                = 0x09,
    /// Bluejay specific Settings
    NVDS_TAG_BJ_TXCNTL1                 = 0x0A,
    /// BLE Application Advertising data
    NVDS_TAG_APP_BLE_ADV_DATA            = 0x0B,
    /// BLE Application Scan response data
    NVDS_TAG_APP_BLE_SCAN_RESP_DATA        = 0x0C,
    /// External wake-up time
    NVDS_TAG_EXT_WAKEUP_TIME            = 0x0D,
    /// Oscillator wake-up time
    NVDS_TAG_OSC_WAKEUP_TIME            = 0x0E,
    /// Radio wake-up time
    NVDS_TAG_RM_WAKEUP_TIME             = 0x0F,
    /// UART baudrate
    NVDS_TAG_UART_BAUDRATE              = 0x10,
    /// Enable sleep mode
    NVDS_TAG_SLEEP_ENABLE               = 0x11,
    /// Enable External Wakeup
    NVDS_TAG_EXT_WAKEUP_ENABLE          = 0x12,
    /// SP Private Key
    NVDS_TAG_SP_PRIVATE_KEY             = 0x13,
    /// SP Public Key
    NVDS_TAG_SP_PUBLIC_KEY              = 0x14,
    /// Errata adopted check
    NVDS_TAG_ERRATA_ADOPTED             = 0x15,
    /// CQDDR Tags
    NVDS_TAG_BASIC_THRESHOLD            = 0x16,
    NVDS_TAG_EDR_THRESHOLD              = 0x17,
    NVDS_TAG_BASIC_ALGORITHM            = 0x18,
    NVDS_TAG_EDR_ALGORITHM              = 0x19,
    NVDS_TAG_BASIC_PACKET_LUT           = 0x2A,
    NVDS_TAG_EDR_PACKET_LUT             = 0x2B,
    /// Synchronous links configuration
    NVDS_TAG_SYNC_CONFIG                = 0x2C,
    /// PCM Settings
    NVDS_TAG_PCM_SETTINGS               = 0x2D,
    /// Default size used to transmit synchronous data over HCI
    NVDS_TAG_HCI_SYNC_BUF_SIZE          = 0x2E,
    /// Diagport configuration
    NVDS_TAG_DIAG_BT_HW                 = 0x30,
    NVDS_TAG_DIAG_BLE_HW                = 0x31,
    NVDS_TAG_DIAG_SW                    = 0x32,
    NVDS_TAG_DIAG_PLF                   = 0x34,
    /// Enable security for BLE application
    NVDS_TAG_SECURITY_ENABLE            = 0x35,
    /// Neb Session ID
    NVDS_TAG_NEB_ID                     = 0x36,
    /// Mouse Sample Rate
    NVDS_TAG_MOUSE_SAMPLE_RATE          = 0x38,
    /// Peripheral Bonded
    NVDS_TAG_PERIPH_BONDED              = 0x39,
    /// Mouse NTF Cfg
    NVDS_TAG_MOUSE_NTF_CFG              = 0x3A,
    /// Mouse Timeout value
    NVDS_TAG_MOUSE_TIMEOUT              = 0x3B,
    /// Peer Device BD Address
    NVDS_TAG_PEER_BD_ADDRESS            = 0x3C,
    /// Mouse Energy Safe
    NVDS_TAG_MOUSE_ENERGY_SAFE          = 0x3D,
    /// EDIV (2bytes), RAND NB (8bytes),  LTK (16 bytes), Key Size (1 byte)
    NVDS_TAG_LTK                        = 0x3E,

    /// BLE Channel Assessment tags
    NVDS_TAG_BLE_CA_TIMER_DUR           = 0x40,
    NVDS_TAG_BLE_CRA_TIMER_DUR          = 0x41,
    NVDS_TAG_BLE_CA_MIN_RSSI            = 0x42,
    NVDS_TAG_BLE_CA_NB_PKT              = 0x43,
    NVDS_TAG_BLE_CA_NB_BAD_PKT          = 0x44,
    /// AFH algorithm tags
    NVDS_TAG_AFH_REASS_NBCH             = 0x51,
    NVDS_TAG_AFH_WINLGTH                = 0x52,
    NVDS_TAG_AFH_RSSIMIN                = 0x53,
    NVDS_TAG_AFH_PERTHRESBAD            = 0x54,
    NVDS_TAG_AFH_REASS_INT              = 0x55,
    NVDS_TAG_AFH_NMIN                   = 0x56,
    NVDS_TAG_AFH_MAXADAPT               = 0x57,
    NVDS_TAG_AFH_THSMIN                 = 0x58,


    NVDS_TAG_BT_LINK_KEY_FIRST          = 0x60,
    NVDS_TAG_BT_LINK_KEY_LAST           = 0x67,

    NVDS_TAG_BLE_LINK_KEY_FIRST         = 0x70,
    NVDS_TAG_BLE_LINK_KEY_LAST          = 0x7F,
};

// Definition of length associated to each parameters

/// Local Bd Address
static const nvds_tag_len_t      NVDS_LEN_BD_ADDRESS                 = 6;
/// Device Name
static const nvds_tag_len_t      NVDS_LEN_DEVICE_NAME                = 62;
/// Minimum Encryption Key Size
static const nvds_tag_len_t      NVDS_LEN_MIN_ENCRYPTION_KEY_SIZE    = 1;
/// Maximum Encryption Key Size
static const nvds_tag_len_t      NVDS_LEN_MAX_ENCRYPTION_KEY_SIZE    = 1;
/// Preferred Key
static const nvds_tag_len_t      NVDS_LEN_PREFERRED_KEY              = 1;
/// Unit Key
static const nvds_tag_len_t      NVDS_LEN_UNIT_KEY                   = 16;
/// Low power clock drift
static const nvds_tag_len_t      NVDS_LEN_LPCLK_DRIFT                = 2;
/// Low power clock jitter
static const nvds_tag_len_t      NVDS_LEN_LPCLK_JITTER               = 1;
/// Radio Class
static const nvds_tag_len_t      NVDS_LEN_RADIO_CLASS                = 1;
/// Bluejay specific Settings
static const nvds_tag_len_t      NVDS_LEN_BJ_TXCNTL1                 = 4;
/// BLE Application Advertising data
static const nvds_tag_len_t      NVDS_LEN_APP_BLE_ADV_DATA           = 32;
/// BLE Application Scan response data
static const nvds_tag_len_t      NVDS_LEN_APP_BLE_SCAN_RESP_DATA     = 32;
/// External wake-up time
static const nvds_tag_len_t      NVDS_LEN_EXT_WAKEUP_TIME            = 2;
/// Oscillator wake-up time
static const nvds_tag_len_t      NVDS_LEN_OSC_WAKEUP_TIME            = 2;
/// Radio wake-up time
static const nvds_tag_len_t      NVDS_LEN_RM_WAKEUP_TIME             = 2;
/// UART baudrate
static const nvds_tag_len_t      NVDS_LEN_UART_BAUDRATE              = 4;
/// Enable sleep mode
static const nvds_tag_len_t      NVDS_LEN_SLEEP_ENABLE               = 1;
/// Enable External Wakeup
static const nvds_tag_len_t      NVDS_LEN_EXT_WAKEUP_ENABLE          = 1;
/// SP Private Key
static const nvds_tag_len_t      NVDS_LEN_SP_PRIVATE_KEY             = 24;
/// SP Public Key
static const nvds_tag_len_t      NVDS_LEN_SP_PUBLIC_KEY              = 48;
/// Errata adopted check
static const nvds_tag_len_t      NVDS_LEN_ERRATA_ADOPTED             = 1;
/// CQDDR Tags
static const nvds_tag_len_t      NVDS_LEN_BASIC_THRESHOLD            = 70;
static const nvds_tag_len_t      NVDS_LEN_EDR_THRESHOLD              = 70;
static const nvds_tag_len_t      NVDS_LEN_BASIC_ALGORITHM            = 21;
static const nvds_tag_len_t      NVDS_LEN_EDR_ALGORITHM              = 21;
static const nvds_tag_len_t      NVDS_LEN_BASIC_PACKET_LUT           = 16;
static const nvds_tag_len_t      NVDS_LEN_EDR_PACKET_LUT             = 16;
/// Synchronous links configuration
static const nvds_tag_len_t      NVDS_LEN_SYNC_CONFIG                = 2;
/// PCM Settings
static const nvds_tag_len_t      NVDS_LEN_PCM_SETTINGS               = 4;
/// Default size used to transmit synchronous data over HCI
static const nvds_tag_len_t      NVDS_LEN_HCI_SYNC_BUF_SIZE          = 1;
/// Diagport configuration
static const nvds_tag_len_t      NVDS_LEN_DIAG_BT_HW                 = 4;
static const nvds_tag_len_t      NVDS_LEN_DIAG_BLE_HW                = 4;
static const nvds_tag_len_t      NVDS_LEN_DIAG_SW                    = 4;
static const nvds_tag_len_t      NVDS_LEN_DIAG_PLF                   = 4;
/// Enable security for BLE application
static const nvds_tag_len_t      NVDS_LEN_SECURITY_ENABLE            = 1;
/// Neb Session ID
static const nvds_tag_len_t      NVDS_LEN_NEB_ID                     = 2;
/// Mouse sample rate
static const nvds_tag_len_t      NVDS_LEN_MOUSE_SAMPLE_RATE          = 1;
/// Peripheral Bonded
static const nvds_tag_len_t      NVDS_LEN_PERIPH_BONDED              = 1;
/// Mouse MTF Cfg
static const nvds_tag_len_t      NVDS_LEN_MOUSE_NTF_CFG              = 2;
/// Mouse Timeout
static const nvds_tag_len_t      NVDS_LEN_MOUSE_TIMEOUT              = 2;
/// Peer device BD Address + Address Type
static const nvds_tag_len_t      NVDS_LEN_PEER_BD_ADDRESS            = 7;
/// Mouse Safe Energy
static const nvds_tag_len_t      NVDS_LEN_MOUSE_SAFE_ENERGY          = 2;
/// LTK
static const nvds_tag_len_t      NVDS_LEN_LTK                        = 28;
static const nvds_tag_len_t      NVDS_LEN_BLE_CA_TIMER_DUR           = 2;
static const nvds_tag_len_t      NVDS_LEN_BLE_CRA_TIMER_DUR          = 1;
static const nvds_tag_len_t      NVDS_LEN_BLE_CA_MIN_RSSI            = 1;
static const nvds_tag_len_t      NVDS_LEN_BLE_CA_NB_PKT              = 1;
static const nvds_tag_len_t      NVDS_LEN_BLE_CA_NB_BAD_PKT          = 1;
/// AFH algorithm tags
static const nvds_tag_len_t      NVDS_LEN_AFH_REASS_NBCH             = 1;
static const nvds_tag_len_t      NVDS_LEN_AFH_WINLGTH                = 1;
static const nvds_tag_len_t      NVDS_LEN_AFH_RSSIMIN                = 1;
static const nvds_tag_len_t      NVDS_LEN_AFH_PERTHRESBAD            = 1;
static const nvds_tag_len_t      NVDS_LEN_AFH_REASS_INT              = 1;
static const nvds_tag_len_t      NVDS_LEN_AFH_NMIN                   = 1;
static const nvds_tag_len_t      NVDS_LEN_AFH_MAXADAPT               = 1;
static const nvds_tag_len_t      NVDS_LEN_AFH_THSMIN                 = 1;
/// Link keys
static const nvds_tag_len_t      NVDS_LEN_BT_LINK_KEY                = 22;
static const nvds_tag_len_t      NVDS_LEN_BLE_LINK_KEY               = 48;

struct nvds_data_struct
{
    uint32_t    NVDS_VALIDATION_FLAG;
    uint32_t    NVDS_TAG_UART_BAUDRATE;
    uint32_t    NVDS_TAG_DIAG_SW;
    uint32_t    NVDS_TAG_DIAG_BLE_HW;
    uint16_t    NVDS_TAG_NEB_ID;
    uint16_t    NVDS_TAG_LPCLK_DRIFT;
    uint8_t     NVDS_TAG_SLEEP_ENABLE;
    uint8_t     NVDS_TAG_EXT_WAKEUP_ENABLE;
    uint8_t     NVDS_TAG_SECURITY_ENABLE;
    uint8_t     ADV_DATA_TAG_LEN;
    uint8_t     SCAN_RESP_DATA_TAG_LEN;	
    uint8_t     DEVICE_NAME_TAG_LEN;	
    uint8_t     NVDS_TAG_APP_BLE_ADV_DATA[32];
    uint8_t     NVDS_TAG_APP_BLE_SCAN_RESP_DATA[32];
    uint8_t     NVDS_TAG_DEVICE_NAME[62];
    uint8_t     NVDS_TAG_BD_ADDRESS[6];
    uint16_t    NVDS_TAG_BLE_CA_TIMER_DUR;
    uint8_t     NVDS_TAG_BLE_CRA_TIMER_DUR;
    uint8_t     NVDS_TAG_BLE_CA_MIN_RSSI;
    uint8_t     NVDS_TAG_BLE_CA_NB_PKT;
    uint8_t     NVDS_TAG_BLE_CA_NB_BAD_PKT;
};

#define BD_ADDRESS_VALID                0x00000001
#define DEVICE_NAME_VALID               0x00000002
#define LPCLK_DRIFT_VALID               0x00000004
#define APP_BLE_ADV_DATA_VALID          0x00000008
#define APP_BLE_SCAN_RESP_DATA_VALID    0x00000010
#define UART_BAUDRATE_VALID             0x00000020
#define SLEEP_ENABLE_VALID              0x00000040
#define EXT_WAKEUP_ENABLE_VALID         0x00000080
#define DIAG_BLE_HW_VALID               0x00000100
#define DIAG_SW_VALID                   0x00000200
#define SECURITY_ENABLE_VALID           0x00000400
#define NEB_ID_VALID                    0x00000800
#define NVDS_BLE_CA_TIMER_DUR_VALID     0x00001000
#define NVDS_BLE_CRA_TIMER_DUR_VALID    0x00002000
#define NVDS_BLE_CA_MIN_RSSI_VALID      0x00004000
#define NVDS_BLE_CA_NB_PKT_VALID        0x00008000
#define NVDS_BLE_CA_NB_BAD_PKT_VALID    0x00010000

/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Initialize NVDS.
 * @return NVDS_OK
 ****************************************************************************************
 */
uint8_t nvds_init(uint8_t *base, uint32_t len);

/**
 ****************************************************************************************
 * @brief Look for a specific tag and return, if found and matching (in length), the
 *        DATA part of the TAG. 
 *
 * If the length does not match, the TAG header structure is still filled, in order for
 * the caller to be able to check the actual length of the TAG.
 *
 * @param[in]     tag     TAG to look for whose DATA is to be retrieved
 * @param[in|out] length  Expected length of the TAG
 * @param[out]    buf     A pointer to the buffer allocated by the caller to be filled with
 *                        the DATA part of the TAG
 *
 * @return  NVDS_OK                  The read operation was performed
 *          NVDS_LENGTH_OUT_OF_RANGE The length passed in parameter is different than the TAG's
 ****************************************************************************************
 */
uint8_t nvds_get(uint8_t tag, nvds_tag_len_t * lengthPtr, uint8_t *buf);

uint8_t custom_nvds_get_func(uint8_t tag, nvds_tag_len_t * lengthPtr, uint8_t *buf);

#if (NVDS_READ_WRITE == 1)

/**
 ****************************************************************************************
 * @brief Look for a specific tag and delete it (Status set to invalid)
 *
 * Implementation notes
 * 1. The write function call return status is not handled
 *
 * @param[in]  tag    TAG to mark as deleted
 *
 * @return NVDS_OK                TAG found and deleted
 *         NVDS_PARAM_LOCKED    TAG found but can not be deleted because it is locked
 *         (others)        return values from function call @ref nvds_browse_tag
 ****************************************************************************************
 */
uint8_t nvds_del(uint8_t tag);

/**
 ****************************************************************************************
 * @brief Look for a specific tag and lock it (Status lock bit set to LOCK).
 *
 * The write function call return status is not handled
 *
 * @param[in]  tag    TAG to mark as locked
 *
 * @return NVDS_OK    TAG found and locked
 *         (others)        return values from function call @ref nvds_browse_tag
 ****************************************************************************************
 */
uint8_t nvds_lock(uint8_t tag);

/**
 ****************************************************************************************
 * @brief This function adds a specific TAG to the NVDS.
 *
 * Steps:
 * 1) parse all the TAGs to:
 * 1.1) calculate the total size of all the valid TAGs
 * 1.2) erase the existing TAGs that have the same ID
 * 1.3) check if we can use the same TAG area in case of an EEPROM
 * 1.4) check that the TAG is not locked
 * 2) if we have to add the new TAG at the end fo the NVDS (cant use same area):
 * 2.1) allocate the appropriate amount of memory
 * 2.2) purge the NVDS
 * 2.3) free the memory allocated
 * 2.4) check that there is now enough room for the new TAG or return
 *      NO_SPACE_AVAILABLE
 * 3) add the new TAG
 *
 * @param[in]  tag     TAG to look for whose DATA is to be retrieved
 * @param[in]  length  Expected length of the TAG
 * @param[in]  buf     Pointer to the buffer containing the DATA part of the TAG to add to
 *                     the NVDS
 *
 * @return NVDS_OK                  New TAG correctly written to the NVDS
 *         NVDS_PARAM_LOCKED        New TAG is trying to overwrite a TAG that is locked
 *         NO_SPACE_AVAILABLE       New TAG can not fit in the available space in the NVDS
 ****************************************************************************************
 */
uint8_t nvds_put(uint8_t tag, nvds_tag_len_t length, uint8_t *buf);

#endif //(NVDS_READ_WRITE == 1)

void nvds_read_bdaddr_from_otp(void);

bool nvds_read_bdaddr_from_da14583_flash(void);

static __inline void  nvds_read_bdaddr(void)
{
#if defined(__DA14583__) && !(BDADDR_FROM_DA14583_FLASH_DISABLED)
    // First check and read BD address from DA14583's internal SPI flash
    // If no BD address was found in flash, then try to read BD address from OTP
    if( !nvds_read_bdaddr_from_da14583_flash() )
#endif
    {
        // Check and read BD address from OTP
        nvds_read_bdaddr_from_otp();
    }
}

#if defined(__DA14583__) && !(BDADDR_FROM_DA14583_FLASH_DISABLED)

struct product_header
{
    uint8_t     signature[2];
    uint8_t     version[2];
    uint32_t    offset1;
    uint32_t    offset2;
    uint8_t     bd_addr[6];
    uint8_t     reserved[2];
    uint32_t    config_offset;
};

#define PRODUCT_HEADER_OFFSET       0x1F000
#define PRODUCT_HEADER_SIGNATURE1   0x70
#define PRODUCT_HEADER_SIGNATURE2   0x52
#define PRODUCT_HEADER_SIGNATURE16LE   (PRODUCT_HEADER_SIGNATURE1 | ( PRODUCT_HEADER_SIGNATURE2 << 8))

/// All 1s BD address
static const struct bd_addr all_ones_bdaddr = {{0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}};

// DA14583 internal SPI flash pins
#define DA14583_SPI_FLASH_EN_GPIO_PORT    GPIO_PORT_2
#define DA14583_SPI_FLASH_EN_GPIO_PIN     GPIO_PIN_3

#define DA14583_SPI_FLASH_CLK_GPIO_PORT   GPIO_PORT_2
#define DA14583_SPI_FLASH_CLK_GPIO_PIN    GPIO_PIN_0

#define DA14583_SPI_FLASH_DO_GPIO_PORT    GPIO_PORT_2
#define DA14583_SPI_FLASH_DO_GPIO_PIN     GPIO_PIN_9

#define DA14583_SPI_FLASH_DI_GPIO_PORT    GPIO_PORT_2
#define DA14583_SPI_FLASH_DI_GPIO_PIN     GPIO_PIN_4

#define DA14583_SPI_FLASH_SIZE            0x20000
#define DA14583_SPI_FLASH_PAGE_SIZE       0x100

static void da14583_spi_flash_init(void);
static void da14583_spi_flash_release(void);
#endif

#endif // _NVDS_H_
