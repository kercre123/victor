/**
 ****************************************************************************************
 *
 * @file gapm.c
 *
 * @brief Generic Access Profile Manager Implementation.
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 ****************************************************************************************
 */


/**
 ****************************************************************************************
 * @addtogroup GAPM Generic Access Profile Manager
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "rwip_config.h"  // Software configuration

#include "gap.h"        // Generic access profile
#include "gapm.h"       // Generic access profile Manager
#include "gapc.h"       // Generic access profile Controller

#include "gapm_task.h"  // Generic access profile Manager

#include "attm_db.h"    // Attribute Database management
#include "attm_util.h"  // Attribute Database management Utils

#include "ke_mem.h"     // Kernel memory management



#include "l2cm.h"
#include "smpm.h"
#include "prf_utils.h"


/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */

/*
 * DEFINES
 ****************************************************************************************
 */

/*
 * MACROS
 ****************************************************************************************
 */

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */
		
extern struct gap_cfg_table_struct *gap_cfg_table; //@RETENTION MEMORY


const struct gap_cfg_table_struct gap_timeout_table  __attribute__((section("timeout_table_area"))) = {
    .GAP_TMR_LIM_ADV_TIMEOUT_VAR = 0x4650,
    .GAP_TMR_GEN_DISC_SCAN_VAR = 0x0300,
    .GAP_TMR_LIM_DISC_SCAN_VAR = 0x0300,
    .GAP_TMR_PRIV_ADDR_INT_VAR = 0x3A98,
    .GAP_TMR_CONN_PAUSE_CT_VAR = 0x0064,
    .GAP_TMR_CONN_PAUSE_PH_VAR = 0x01F4,
    .GAP_TMR_CONN_PARAM_TIMEOUT_VAR = 0x0BB8,
    .GAP_TMR_SCAN_FAST_PERIOD_VAR = 0x0C00,
    .GAP_TMR_ADV_FAST_PERIOD_VAR = 0x0BB8,
    .GAP_LIM_DISC_SCAN_INT_VAR = 0x0012,
    .GAP_SCAN_FAST_INTV_VAR = 0x0030,
    .GAP_SCAN_FAST_WIND_VAR = 0x0030,
    .GAP_SCAN_SLOW_INTV1_VAR = 0x00CD,
    .GAP_SCAN_SLOW_INTV2_VAR = 0x019A,
    .GAP_SCAN_SLOW_WIND1_VAR = 0x0012,
    .GAP_SCAN_SLOW_WIND2_VAR = 0x0024,
    .GAP_ADV_FAST_INTV1_VAR = 0x0030,
    .GAP_ADV_FAST_INTV2_VAR = 0x0064,
    .GAP_ADV_SLOW_INTV_VAR = 0x00B0,
    .GAP_INIT_CONN_MIN_INTV_VAR = 0x0018,
    .GAP_INIT_CONN_MAX_INTV_VAR = 0x0028,
    .GAP_INQ_SCAN_INTV_VAR = 0x0012,
    .GAP_INQ_SCAN_WIND_VAR = 0x0012,
    .GAP_CONN_SUPERV_TIMEOUT_VAR = 0x07D0,
    .GAP_CONN_MIN_CE_VAR = 0x0000,
    .GAP_CONN_MAX_CE_VAR = 0xFFFF,
    .GAP_CONN_LATENCY_VAR = 0x0000,
    .GAP_LE_MASK_VAR = 0x1F,
    .GAP_MAX_LE_MTU_VAR = 512,
#ifdef __DA14581__
    .GAP_DEV_NAME_VAR = "DA14581",
#elif defined (__DA14583__)
    .GAP_DEV_NAME_VAR = "DA14583",
#else
    .GAP_DEV_NAME_VAR = "DA14580-01",
#endif
};
		
		


/// @} GAPM
