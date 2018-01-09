/**
 *****************************************************************************************
 *
 * @file app_bond_db.c
 *
 * @brief Bond database code file.
 *
 * Copyright (C) 2012. Dialog Semiconductor Ltd, unpublished work. This computer
 * program includes Confidential, Proprietary Information and is a Trade Secret of
 * Dialog Semiconductor Ltd. All use, disclosure, and/or reproduction is prohibited
 * unless authorized in writing. All Rights Reserved.
 *
 * <bluetooth.support@diasemi.com> and contributors.
 *
 *****************************************************************************************
 */

/**
 ****************************************************************************************
 * @addtogroup APP_BOND_DB
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "rwip_config.h"             // SW configuration

#if (BLE_APP_PRESENT)

#include "app_bond_db.h"

#if defined (USER_CFG_APP_BOND_DB_USE_SPI_FLASH)
#include "spi_flash.h"
#elif defined (USER_CFG_APP_BOND_DB_USE_I2C_EEPROM)
#include "i2c_eeprom.h"
#endif

/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */

struct bond_db
{
    uint16_t start_hdr;
    struct bond_db_data data[APP_BOND_DB_MAX_BONDED_PEERS];
    uint8_t next_slot;
    uint16_t end_hdr;
};

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

static struct bond_db bdb __attribute__((section("retention_mem_area0"), zero_init)); //@RETENTION MEMORY

/*
 * FUCTION DEFINITIONS
 ****************************************************************************************
 */

#if defined (USER_CFG_APP_BOND_DB_USE_SPI_FLASH)

static void bond_db_spi_flash_init(void)
{
    static int8_t dev_id;

    dev_id = spi_flash_enable(SPI_EN_GPIO_PORT, SPI_EN_GPIO_PIN);
    if (dev_id == SPI_FLASH_AUTO_DETECT_NOT_DETECTED)
    {
        // The device was not identified. The default parameters are used.
        // Alternatively, an error can be asserted here.
        spi_flash_init(SPI_FLASH_DEFAULT_SIZE, SPI_FLASH_DEFAULT_PAGE);
    }
}

static void bond_db_load_flash(void)
{
    bond_db_spi_flash_init();

    spi_flash_read_data((uint8_t *)&bdb, APP_BOND_DB_DATA_OFFSET, sizeof(struct bond_db));

    spi_release();
}

static int8_t bond_db_erase_flash_sectors(void)
{
    uint32_t sector_nb;
    uint32_t offset;
    int8_t ret;
    int i;

    // Calculate the starting sector offset
    offset = (APP_BOND_DB_DATA_OFFSET / SPI_SECTOR_SIZE) * SPI_SECTOR_SIZE;

    // Calculate the numbers of sectors to erase
    sector_nb = (sizeof(bdb) / SPI_SECTOR_SIZE);
    if (sizeof(bdb) % SPI_SECTOR_SIZE)
        sector_nb++;

    // Erase flash sectors
    for (i = 0; i < sector_nb; i++)
    {
        ret = spi_flash_block_erase(offset, SECTOR_ERASE);
        offset += SPI_SECTOR_SIZE;
        if (ret != ERR_OK)
            break;
    }
    return ret;
}

static void bond_db_store_flash(void)
{
    int8_t ret;

    bond_db_spi_flash_init();

    ret = bond_db_erase_flash_sectors();
    if (ret == ERR_OK)
    {
        spi_flash_write_data((uint8_t *)&bdb, APP_BOND_DB_DATA_OFFSET, sizeof(struct bond_db));
    }

    spi_release();
}

static void bond_db_clear_flash(void)
{
    int8_t ret;

    bond_db_spi_flash_init();

    ret = bond_db_erase_flash_sectors();
    ASSERT_WARNING(ret == ERR_OK);

    spi_release();
}

#elif defined (USER_CFG_APP_BOND_DB_USE_I2C_EEPROM)

static void bond_db_load_eeprom(void)
{
    uint32_t bytes_read;

    i2c_eeprom_init(I2C_SLAVE_ADDRESS, I2C_SPEED_MODE, I2C_ADDRESS_MODE, I2C_ADDRESS_SIZE);

    i2c_eeprom_read_data((uint8_t *)&bdb, APP_BOND_DB_DATA_OFFSET, sizeof(struct bond_db), &bytes_read);
    ASSERT_ERROR(bytes_read == sizeof(struct bond_db));

    i2c_eeprom_release();
}

static void bond_db_store_eeprom(void)
{
    uint32_t bytes_written;

    i2c_eeprom_init(I2C_SLAVE_ADDRESS, I2C_SPEED_MODE, I2C_ADDRESS_MODE, I2C_ADDRESS_SIZE);

    i2c_eeprom_write_data((uint8_t *)&bdb, APP_BOND_DB_DATA_OFFSET, sizeof(struct bond_db), &bytes_written);
    ASSERT_ERROR(bytes_written == sizeof(struct bond_db));

    i2c_eeprom_release();
}

#endif

static inline void bond_db_load_ext(void)
{
    #if defined (USER_CFG_APP_BOND_DB_USE_SPI_FLASH)
    bond_db_load_flash();
    #elif defined (USER_CFG_APP_BOND_DB_USE_I2C_EEPROM)
    bond_db_load_eeprom();
    #endif
}

static inline void bond_db_store_ext(void)
{
    #if defined (USER_CFG_APP_BOND_DB_USE_SPI_FLASH)
    bond_db_store_flash();
    #elif defined (USER_CFG_APP_BOND_DB_USE_I2C_EEPROM)
    bond_db_store_eeprom();
    #endif
}

static inline void bond_db_clear_ext(void)
{
    #if defined (USER_CFG_APP_BOND_DB_USE_SPI_FLASH)
    bond_db_clear_flash();
    #endif
}

static void bond_db_store_at_idx(struct bond_db_data *data, int idx)
{
    // Update the cache
    memcpy(&bdb.data[idx], data, sizeof(struct bond_db_data));

    bond_db_store_ext();
}

void bond_db_init(void)
{
    // Load bond data from the external memory resource
    bond_db_load_ext();

    // Simple check for garbage in memory (this also catches the 0xFF of cleared memory)
    if ((bdb.next_slot > APP_BOND_DB_MAX_BONDED_PEERS) ||
        (bdb.start_hdr != BOND_DB_HEADER_START) || (bdb.end_hdr != BOND_DB_HEADER_END))
    {
        bond_db_clear();
    }
}

void bond_db_store(struct bond_db_data *data)
{
    int idx_emtpy_slot_found = -1;
    int idx_found = -1;
    int idx = 0;

    // search if it already exists in db
    for(idx = 0; idx < APP_BOND_DB_MAX_BONDED_PEERS; ++idx)
    {
        // skip invalid
        if (bdb.data[idx].valid != 0xAA)
        {
            if (-1 == idx_emtpy_slot_found)
            {
                idx_emtpy_slot_found = idx;
            }

            continue;
        }

        if (bdb.data[idx].flags && BOND_DB_ENTRY_IRK_PRESENT)
        {
            // match IRK
            if ( 0 == memcmp(&data->irk, &bdb.data[idx].irk, sizeof(struct gapc_irk)) )
            {
                idx_found = idx;
                break;
            }
        }
        else
        {
            if ( 0 == memcmp(&data->bdaddr, &bdb.data[idx].bdaddr, sizeof(struct gap_bdaddr)) )
            {
                idx_found = idx;
                break;
            }
        }
    }

    if (-1 != idx_emtpy_slot_found)
    {
        idx_found = idx_emtpy_slot_found;
    }
    else
    {
        // no empty slot found - delete the oldest bond data slot
        idx_found = bdb.next_slot++;
        bdb.next_slot = bdb.next_slot % APP_BOND_DB_MAX_BONDED_PEERS;
    }

    // store bond db entry
    bond_db_store_at_idx(data, idx_found);
}

const struct bond_db_data* bond_db_lookup_by_ediv(const struct rand_nb *rand_nb,
                                                  uint16_t ediv)
{
    int idx = 0;
    struct bond_db_data *found_data = 0;

    // search if it already exists in db
    for(idx = 0; idx < APP_BOND_DB_MAX_BONDED_PEERS; ++idx)
    {
        // match rand_nd and ediv
        if ( 0 == memcmp(rand_nb, &bdb.data[idx].ltk.randnb, RAND_NB_LEN)
            && ediv == bdb.data[idx].ltk.ediv )
        {
            found_data = &bdb.data[idx];
            break;
        }
    }

    return found_data;
}

void bond_db_clear(void)
{
    memset((void *)&bdb, 0, sizeof(struct bond_db) ); // zero bond data
    bdb.start_hdr = BOND_DB_HEADER_START;
    bdb.end_hdr = BOND_DB_HEADER_END;

    bond_db_clear_ext();
    bond_db_store_ext();
}

#endif  // BLE_APP_PRESENT

/// @} APP_BOND_DB
