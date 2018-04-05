/**
 ****************************************************************************************
 *
 * @file nvds.c
 *
 * @brief Non Volatile Data Storage (NVDS) driver
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 *
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @addtogroup NVDS
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include <string.h>      // string definitions
#include <stddef.h>      // standard definitions
#include "nvds.h"        // nvds definitions
#include "arch.h"        // main
#include "co_math.h"     // math operations
#include "co_bt.h"
#include "rwip_config.h"
#include "user_config.h"

/*
 * LOCAL VARIABLES
 ****************************************************************************************
 */

const struct nvds_data_struct nvds_data_storage __attribute__((section("nvds_data_storage_area")))
#ifdef CFG_INITIALIZE_NVDS_STRUCT
=   {
    .NVDS_VALIDATION_FLAG                   = (BD_ADDRESS_VALID | DEVICE_NAME_VALID | LPCLK_DRIFT_VALID | APP_BLE_ADV_DATA_VALID \
                                              | APP_BLE_SCAN_RESP_DATA_VALID | UART_BAUDRATE_VALID | SLEEP_ENABLE_VALID | EXT_WAKEUP_ENABLE_VALID \
                                              | DIAG_BLE_HW_VALID | DIAG_SW_VALID | SECURITY_ENABLE_VALID | NEB_ID_VALID \
                                              | NVDS_BLE_CA_TIMER_DUR_VALID | NVDS_BLE_CRA_TIMER_DUR_VALID | NVDS_BLE_CA_MIN_RSSI_VALID | NVDS_BLE_CA_NB_PKT_VALID \
                                              | NVDS_BLE_CA_NB_BAD_PKT_VALID),
    .NVDS_TAG_UART_BAUDRATE                 = 115200,
    .NVDS_TAG_DIAG_SW                       = 0,
    .NVDS_TAG_DIAG_BLE_HW                   = 0,
    .NVDS_TAG_NEB_ID                        = 0,
    .NVDS_TAG_LPCLK_DRIFT                   = CFG_NVDS_TAG_LPCLK_DRIFT,
    .NVDS_TAG_SLEEP_ENABLE                  = 1,
    .NVDS_TAG_EXT_WAKEUP_ENABLE             = 0,
    .NVDS_TAG_SECURITY_ENABLE               = 1,
    .NVDS_TAG_APP_BLE_ADV_DATA              = USER_ADVERTISE_DATA,
    .NVDS_TAG_APP_BLE_SCAN_RESP_DATA        = USER_ADVERTISE_SCAN_RESPONSE_DATA,
    .NVDS_TAG_DEVICE_NAME                   = USER_DEVICE_NAME,
    .NVDS_TAG_BD_ADDRESS                    = CFG_NVDS_TAG_BD_ADDRESS,
    .ADV_DATA_TAG_LEN                       = USER_ADVERTISE_DATA_LEN,
    .SCAN_RESP_DATA_TAG_LEN                 = USER_ADVERTISE_SCAN_RESPONSE_DATA_LEN,
    .DEVICE_NAME_TAG_LEN                    = USER_DEVICE_NAME_LEN,
    /// Default Channel Assessment Timer duration (20s - Multiple of 10ms)
    .NVDS_TAG_BLE_CA_TIMER_DUR              = CFG_NVDS_TAG_BLE_CA_TIMER_DUR,
    /// Default Channel Reassessment Timer duration (Multiple of Channel Assessment Timer duration)
    .NVDS_TAG_BLE_CRA_TIMER_DUR             = CFG_NVDS_TAG_BLE_CRA_TIMER_DUR,
    /// Default Minimal RSSI Threshold - -48dBm
    .NVDS_TAG_BLE_CA_MIN_RSSI               = CFG_NVDS_TAG_BLE_CA_MIN_RSSI,
    /// Default number of packets to receive for statistics
    .NVDS_TAG_BLE_CA_NB_PKT                 = CFG_NVDS_TAG_BLE_CA_NB_PKT,
    /// Default number of bad packets needed to remove a channel
    .NVDS_TAG_BLE_CA_NB_BAD_PKT             = CFG_NVDS_TAG_BLE_CA_NB_BAD_PKT,
}
#endif
;

/// Device BD address
struct bd_addr dev_bdaddr __attribute__((section("retention_mem_area0"), zero_init));

extern struct nvds_data_struct *nvds_data_ptr;

/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */

uint8_t custom_nvds_get_func(uint8_t tag, nvds_tag_len_t *lengthPtr, uint8_t *buf)
{
    // declaration of ROM function nvds_get_func()
    uint8_t nvds_get_func(uint8_t tag, nvds_tag_len_t *lengthPtr, uint8_t *buf);

    switch (tag)
    {
        case NVDS_TAG_BD_ADDRESS:
        {
            //check if dev_bdaddr is not zero
            if(memcmp(&dev_bdaddr, &co_null_bdaddr, NVDS_LEN_BD_ADDRESS))
            {
                memcpy(buf, &dev_bdaddr, NVDS_LEN_BD_ADDRESS);
                *lengthPtr = NVDS_LEN_BD_ADDRESS;
                return NVDS_OK;
            }
            break;
        }

        case NVDS_TAG_APP_BLE_SCAN_RESP_DATA:
            if (nvds_data_ptr->NVDS_VALIDATION_FLAG & APP_BLE_SCAN_RESP_DATA_VALID)
            {
                if (*lengthPtr < NVDS_LEN_APP_BLE_SCAN_RESP_DATA)
                {
                    *lengthPtr = 0;
                    return (NVDS_LENGTH_OUT_OF_RANGE);
                }
                else
                {
                    memcpy(buf, nvds_data_ptr->NVDS_TAG_APP_BLE_SCAN_RESP_DATA, nvds_data_ptr->SCAN_RESP_DATA_TAG_LEN);
                    *lengthPtr = nvds_data_ptr->SCAN_RESP_DATA_TAG_LEN;
                    return(NVDS_OK);
                }
            }
            else
                return(NVDS_FAIL);
            default: break;
    }

    return nvds_get_func(tag, lengthPtr, buf);

}

void nvds_read_bdaddr_from_otp()
{
    const uint16_t BDADDR_OFFSET = 0x7fd4; // offset of BD address in OTP header
    uint8_t *otp_bdaddr;

    if (!APP_BOOT_FROM_OTP)
    {
        int cnt = 100000;
        #define XPMC_MODE_MREAD   0x1
        otp_bdaddr = (uint8_t *)0x40000 + BDADDR_OFFSET;   //where in OTP header is BDADDR

        SetBits16(CLK_AMBA_REG, OTP_ENABLE, 1);		// enable OTP clock
        while ((GetWord16(ANA_STATUS_REG) & LDO_OTP_OK) != LDO_OTP_OK && cnt--)
            /* Just wait */;

        // set OTP in read mode
        SetWord32(OTPC_MODE_REG, XPMC_MODE_MREAD);
    }
    else
        otp_bdaddr = (uint8_t *)0x20000000 + BDADDR_OFFSET;   //where in OTP header is BDADDR


    memcpy(&dev_bdaddr, otp_bdaddr, sizeof(dev_bdaddr));
    SetBits16(CLK_AMBA_REG, OTP_ENABLE, 0);     //disable OTP clock
}


#if defined(__DA14583__) && !(BDADDR_FROM_DA14583_FLASH_DISABLED)

bool nvds_read_bdaddr_from_da14583_flash(void)
{
    struct product_header ph;
    bool status = false;

    // Power up peripherals' power domain
    SetBits16(PMU_CTRL_REG, PERIPH_SLEEP, 0);
    while (!(GetWord16(SYS_STAT_REG) & PER_IS_UP))
        ;

    // Enable the pads
    SetBits16(SYS_CTRL_REG, PAD_LATCH_EN, 1);

    // Initialize DA14583 internal flash
    da14583_spi_flash_init();

    // Zero out product header variable
    memset(&ph, 0, sizeof(struct product_header));

    // Read product header from SPI flash
    spi_flash_read_data((uint8_t*)&ph, (unsigned long)PRODUCT_HEADER_OFFSET, (unsigned long)sizeof(struct product_header));

    // If product header is valid
    if( (ph.signature[0] == PRODUCT_HEADER_SIGNATURE1) || (ph.signature[1] == PRODUCT_HEADER_SIGNATURE2))
    {
        // if BD address field of product header is not equal to all zeros or all 0xFFs
        if ( (0 != memcmp(&ph.bd_addr, &all_ones_bdaddr, 6))
            && (0 != memcmp(&ph.bd_addr, &co_null_bdaddr, 6)) )
        {
            // Set the product header BD address field as the device's BD address
            memcpy(&dev_bdaddr, &ph.bd_addr, sizeof(dev_bdaddr));
            status  = true;
        }
    }

    // Release DA14583 internal flash
    da14583_spi_flash_release();

    return status;
}

static void da14583_spi_flash_init(void)
{
    SPI_Pad_t cs_pad_param;

    // configure internal flash GPIOS
    GPIO_ConfigurePin( DA14583_SPI_FLASH_EN_GPIO_PORT,    DA14583_SPI_FLASH_EN_GPIO_PIN,    OUTPUT, PID_SPI_EN, true );
    GPIO_ConfigurePin( DA14583_SPI_FLASH_CLK_GPIO_PORT,   DA14583_SPI_FLASH_CLK_GPIO_PIN,   OUTPUT, PID_SPI_CLK, false );
    GPIO_ConfigurePin( DA14583_SPI_FLASH_DO_GPIO_PORT,    DA14583_SPI_FLASH_DO_GPIO_PIN,    OUTPUT, PID_SPI_DO, false );
    GPIO_ConfigurePin( DA14583_SPI_FLASH_DI_GPIO_PORT,    DA14583_SPI_FLASH_DI_GPIO_PIN,    INPUT,  PID_SPI_DI, false );

    cs_pad_param.port = DA14583_SPI_FLASH_EN_GPIO_PORT;
    cs_pad_param.pin = DA14583_SPI_FLASH_EN_GPIO_PIN;

    // Initialize SPI driver
    spi_init(&cs_pad_param, SPI_MODE_8BIT, SPI_ROLE_MASTER, SPI_CLK_IDLE_POL_LOW, SPI_PHA_MODE_0, SPI_MINT_DISABLE, SPI_XTAL_DIV_8);
    // Initialize SPI Flash driver
    spi_flash_init(DA14583_SPI_FLASH_SIZE, DA14583_SPI_FLASH_PAGE_SIZE);

    // Power up flash
    spi_flash_release_from_power_down();
}

static void da14583_spi_flash_release(void)
{
    // Power down flash
    spi_flash_power_down();
    // Release SPI controller
    spi_release();

    // Release internal flash GPIOs
    GPIO_ConfigurePin( DA14583_SPI_FLASH_EN_GPIO_PORT,  DA14583_SPI_FLASH_EN_GPIO_PIN,  OUTPUT, PID_GPIO, true );
    GPIO_ConfigurePin( DA14583_SPI_FLASH_CLK_GPIO_PORT, DA14583_SPI_FLASH_CLK_GPIO_PIN, INPUT_PULLDOWN, PID_GPIO, false );
    GPIO_ConfigurePin( DA14583_SPI_FLASH_DO_GPIO_PORT,  DA14583_SPI_FLASH_DO_GPIO_PIN,  INPUT_PULLDOWN, PID_GPIO, false );
    GPIO_ConfigurePin( DA14583_SPI_FLASH_DI_GPIO_PORT,  DA14583_SPI_FLASH_DI_GPIO_PIN,  INPUT_PULLDOWN,  PID_GPIO, false );
}
#endif // defined(__DA14583__) && !(BDADDR_FROM_DA14583_FLASH_DISABLED)

/// @} NVDS
