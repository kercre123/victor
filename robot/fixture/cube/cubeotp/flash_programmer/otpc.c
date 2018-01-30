/**
 ****************************************************************************************
 *
 * @file otpc.c
 *
 * @brief eeprom driver over i2c interface header file.
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
#include "global_io.h"
#include "otpc.h"

/**
 ****************************************************************************************
 * @brief Enable OTP clock
 ****************************************************************************************
 */
void otpc_clock_enable(void)
{
    SetBits16(CLK_AMBA_REG, OTP_ENABLE, 1);     // set divider to 1	
    while ((GetWord16(ANA_STATUS_REG) & LDO_OTP_OK) != LDO_OTP_OK)
        /* Just wait */;
}

/**
 ****************************************************************************************
 * @brief Disable OTP clock
 ****************************************************************************************
 */
void otpc_clock_disable(void)
{
    SetBits16(CLK_AMBA_REG, OTP_ENABLE, 0);     // set divider to 1	
}
 
/**
 ****************************************************************************************
 * @brief Write OTP data
 ****************************************************************************************
 */
int otpc_write_fifo(unsigned int mem_addr, unsigned int cel_addr, unsigned int num_of_words)
{
    int i;
    int tmp;
    int res;

    SetWord32 (OTPC_MODE_REG, OTPC_MODE_STBY);

    SetWord32 (OTPC_CELADR_REG,(cel_addr>>2)&0x1FFF);
    SetWord32 (OTPC_NWORDS_REG, num_of_words - 1);
    SetWord32 (OTPC_MODE_REG, OTPC_MODE_APROG);
    res = 0;

    for (i=0;i < num_of_words; i++) {
        while (((tmp = GetWord32(OTPC_STAT_REG)) & OTPC_STAT_FWORDS) == 0x800);
        if ((tmp & OTPC_STAT_ARDY) == OTPC_STAT_ARDY) {
            res += -10;
            break;
        }
        SetWord32 (OTPC_FFPRT_REG, GetWord32(mem_addr + 4*i)); // Write FIFO data
    }

    // wait end of programming
    while ((GetWord32(OTPC_STAT_REG) & OTPC_STAT_ARDY) != OTPC_STAT_ARDY);

    if ((GetWord32(OTPC_STAT_REG) & OTPC_STAT_PERROR) == OTPC_STAT_PERROR) {
        if ((GetWord32(OTPC_STAT_REG) & OTPC_STAT_PERR_L) == OTPC_STAT_PERR_L)
            res += -1;
        if ((GetWord32(OTPC_STAT_REG) & OTPC_STAT_PERR_U) == OTPC_STAT_PERR_U)
            res += -2;	
    }

    return res;
}

