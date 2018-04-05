/**
 ****************************************************************************************
 *
 * @file app_security.c
 *
 * @brief Application Security Entry Point
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 *
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @addtogroup APP_SECURITY
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "co_bt.h"

#include "app_api.h"            // Application task Definition
#include "app_security.h"       // Application Security API Definition
#include <stdlib.h>

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

/// Application Security Environment Structure
struct app_sec_env_tag app_sec_env[APP_EASY_MAX_ACTIVE_CONNECTION] __attribute__((section("retention_mem_area0"),zero_init)); // @RETENTION MEMORY

/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */

uint32_t app_sec_gen_tk(void)
{
    // Generate a PIN Code (Between 100000 and 999999)
    return (100000 + (rand()%900000));
}

void app_sec_gen_ltk(uint8_t connection_idx, uint8_t key_size)
{
    // Counter
    uint8_t i;
    app_sec_env[connection_idx].key_size = key_size;

    // Randomly generate the LTK and the Random Number
    for (i = 0; i < RAND_NB_LEN; i++)
    {
        app_sec_env[connection_idx].rand_nb.nb[i] = rand()%256;
    }

    // Randomly generate the end of the LTK
    for (i = 0; i < KEY_LEN; i++)
    {
        app_sec_env[connection_idx].ltk.key[i] = (((key_size) < (16 - i)) ? 0 : rand()%256);
    }

    // Randomly generate the EDIV
    app_sec_env[connection_idx].ediv = rand()%65536;
}

void app_sec_gen_csrk(uint8_t connection_idx)
{
    // Randomly generate the CSRK
    for (uint8_t i = 0; i < KEY_LEN; i++)
    {
        app_sec_env[connection_idx].csrk.key[i] = rand()%256;
    }
}

/// @} APP_SECURITY
