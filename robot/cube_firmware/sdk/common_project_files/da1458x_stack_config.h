/**
 ****************************************************************************************
 *
 * @file da1458x_stack_config.h
 *
 * @brief RW stack configuration file.
 *
 * Copyright (C) 2014. Dialog Semiconductor Ltd, unpublished work. This computer 
 * program includes Confidential, Proprietary Information and is a Trade Secret of 
 * Dialog Semiconductor Ltd.  All use, disclosure, and/or reproduction is prohibited 
 * unless authorized in writing. All Rights Reserved.
 *
 * <bluetooth.support@diasemi.com> and contributors.
 *
 ****************************************************************************************
 */

#ifndef DA1458X_STACK_CONFIG_H_
    #define DA1458X_STACK_CONFIG_H_

/////////////////////////////////////////////////////////////
/* Do not alter */

    #define CFG_EMB 
    #define CFG_HOST 
    #define CFG_ALLROLES        1  
    #define CFG_GTL 
    #define CFG_BLE  
    #define CFG_EXT_DB  
    #ifndef __DA14581__
        #define CFG_CON             6  
    #else
        #define CFG_CON             8  
    #endif
    #define CFG_SECURITY_ON     1  
    #define CFG_ATTC 
    #define CFG_ATTS  
    #define CFG_BLECORE_11 
    #define POWER_OFF_SLEEP  
    #define CFG_CHNL_ASSESS

    /*FPGA*/
    #define nFPGA_USED

    /*Radio interface*/     
    #ifdef FPGA_USED
        #define RADIO_RIPPLE	    1
        #define RIPPLE_ID           66
        #define RADIO_580           0 
    #else
        #define RADIO_580           1 
    #endif

    /*Misc*/
    #define __NO_EMBEDDED_ASM 

    /*Scatterfile: Memory maps*/
    #define DEEP_SLEEP_SETUP    1
    #define EXT_SLEEP_SETUP     2

    /* Coarse calibration */
    #define CFG_LUT_PATCH

    #define CFG_MGCKMODA_PATCH_ENABLED  

    #undef CFG_HCI_BOTH_EIF
    #undef CFG_HCI_UART
    #undef CFG_HCI_SPI

    /* Enable power optimizations */
    #define CFG_POWER_OPTIMIZATIONS

#endif // DA1458X_STACK_CONFIG_H_
