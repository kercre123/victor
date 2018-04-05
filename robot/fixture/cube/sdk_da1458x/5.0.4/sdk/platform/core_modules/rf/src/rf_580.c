/**
 ****************************************************************************************
 *
 * @file rf_580.c
 *
 * @brief DA14580 RF control.
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
 
#include "rwip_config.h"        // RW SW configuration

//#ifdef RADIO_580
#if RADIO_580

#include <string.h>             // for memcpy
#include "co_utils.h"           // common utility definition
#include "co_math.h"            // common math functions
#include "co_endian.h"          // endian definitions
#include "rf.h"                 // RF interface
#include "plf.h"                // Platform functions

#include "rwip.h"               // for RF API structure definition

#if (BLE_EMB_PRESENT)
#include "reg_blecore.h"        // ble core registers
#include "reg_ble_em_cs.h"      // control structure definitions
#endif //BLE_EMB_PRESENT

#include "rf_580.h"
#include "system_settings.h"

#include "pll_vcocal_lut.h"
#include "arch.h"
#include "gpio.h"

#include "arch_system.h"

#define RXPWRUP_VAL 0x54
#define TXPWRUP_VAL 0x4C

#define EM_BLE_FREQ_TABLE_LEN  40
extern uint16_t last_temp_count;

extern uint32_t lp_clk_sel;

/**
 ****************************************************************************************
 * @brief Read IQ_Trim registers from OTP into Retention
 *
 * 
 ****************************************************************************************
 */
extern uint16_t iq_trim_bias_ctrl1_reg;       //stored in Retention PAD, defined in rom_symdef.txt
extern uint16_t iq_trim_rf_mixer_ctrl1_reg;   //stored in Retention PAD, defined in rom_symdef.txt

#define IQTRIM_FROM_OTP 0x7f08     //OTP IQ_Trim offset
void iq_trim_from_otp()
{
    uint32_t iq_trim_from_otp = 0x7f08;
    
# if !(APP_BOOT_FROM_OTP)    
    int cnt = 100000;
#define XPMC_MODE_MREAD   0x1
    uint16_t *iqtrim = (uint16_t *)(0x40000 + iq_trim_from_otp);   //where in OTP header is IQ_Trim
    
    SetBits16(CLK_AMBA_REG, OTP_ENABLE, 1);		// enable OTP clock	
    while ((GetWord16(ANA_STATUS_REG) & LDO_OTP_OK) != LDO_OTP_OK && cnt--)
        /* Just wait */;
        
    // set OTP in read mode 
    SetWord32(OTPC_MODE_REG, XPMC_MODE_MREAD);
# else
    uint16_t *iqtrim = (uint16_t *)(0x20000000 + iq_trim_from_otp);   //where in OTP header is IQ_Trim
# endif //APP_BOOT_FROM_OTP
    
    iq_trim_bias_ctrl1_reg = *iqtrim++;
    iq_trim_rf_mixer_ctrl1_reg = *iqtrim;
    SetBits16(CLK_AMBA_REG, OTP_ENABLE, 0);     //disable OTP clock    

}

/**
 *****************************************************************************************
 * @brief Dummy function. Must be defined in RF driver API function.
 *****************************************************************************************
 */
uint32_t rf_reg_rd (uint16_t address)
{
    return 0;
}

/**
 *****************************************************************************************
 * @brief Dummy function. Must be defined in RF driver API function.
 *****************************************************************************************
 */
void rf_reg_wr (uint16_t address, uint32_t data)
{

}

/**
 *****************************************************************************************
 * @brief Init RF sequence after reset. Not used
 *****************************************************************************************
 */
void rf_reset(void)
{

}

/**
 *****************************************************************************************
 * @brief Enable/disable force AGC mechanism. Not used.
 *
 * @param[in]  True: Enable / False: disable
 *****************************************************************************************
 */
 void rf_force_agc_enable(bool en)
{
 
}

/**
 *****************************************************************************************
 * @brief Get TX power in dBm from the index in the control structure
 *
 * @param[in] txpwr_idx  Index of the TX power in the control structure
 * @param[in] modulation Modulation: 1 or 2 or 3 MBPS
 *
 * @return The TX power in dBm
 *
 *****************************************************************************************
 */
uint8_t rf_txpwr_dbm_get(uint8_t txpwr_idx, uint8_t modulation)
{
    return 0;
}

/**
 *****************************************************************************************
 * @brief Sets RF module in sleep mode
 *
 * @return void
 *****************************************************************************************
 */
static void rf_sleep(void)
{
    ble_deepslcntl_set(ble_deepslcntl_get() |
                      BLE_DEEP_SLEEP_ON_BIT |    // RW BLE Core sleep
                      BLE_RADIO_SLEEP_EN_BIT |   // Radio sleep
                      BLE_OSC_SLEEP_EN_BIT);     // Oscillator sleep
   // ble_deepslcntl_set(ble_deepslcntl_get() | BLE_DEEP_SLEEP_ON_BIT );     
   
}

static void RADIOCNTL_Handler(void)
{

}

/*
 * RADIO FUNCTION INTERFACE
 ****************************************************************************************
 */

/**
 *****************************************************************************************
 * @brief Initialize RF module registers
 *
 * @return void
 *****************************************************************************************
 */
void rf_regs(void)
{
    SetWord32( BLE_RADIOPWRUPDN_REG, PREF_BLE_RADIOPWRUPDN_REG);
    SetWord16( RF_LF_CTRL_REG, PREF_RF_LF_CTRL_REG);
    SetWord16( RF_CP_CTRL_REG, PREF_RF_CP_CTRL_REG);
    SetWord16( RF_REF_OSC_REG, PREF_RF_REF_OSC_REG);
    SetWord16( RF_ENABLE_CONFIG1_REG, PREF_RF_ENABLE_CONFIG1_REG);
    SetWord16( RF_ENABLE_CONFIG2_REG, PREF_RF_ENABLE_CONFIG2_REG);
    SetWord16( RF_ENABLE_CONFIG6_REG, PREF_RF_ENABLE_CONFIG6_REG);
    SetWord16( RF_ENABLE_CONFIG9_REG, PREF_RF_ENABLE_CONFIG9_REG);
    SetWord16( RF_ENABLE_CONFIG10_REG, PREF_RF_ENABLE_CONFIG10_REG);
    SetWord16( RF_ENABLE_CONFIG13_REG, PREF_RF_ENABLE_CONFIG13_REG);
    SetWord16( RF_ENABLE_CONFIG14_REG, PREF_RF_ENABLE_CONFIG14_REG);
    SetWord16( RF_ENABLE_CONFIG19_REG, PREF_RF_ENABLE_CONFIG19_REG);
    SetWord16( RF_CNTRL_TIMER_3_REG, PREF_RF_CNTRL_TIMER_3_REG);
    SetWord16( RF_CNTRL_TIMER_4_REG, PREF_RF_CNTRL_TIMER_4_REG);
    SetWord16( RF_CNTRL_TIMER_8_REG, PREF_RF_CNTRL_TIMER_8_REG);
    SetWord16( RF_CNTRL_TIMER_9_REG, PREF_RF_CNTRL_TIMER_9_REG);
    SetWord16( RF_CNTRL_TIMER_10_REG, PREF_RF_CNTRL_TIMER_10_REG);
    SetWord16( RF_CNTRL_TIMER_11_REG, PREF_RF_CNTRL_TIMER_11_REG);
    SetWord16( RF_CNTRL_TIMER_12_REG, PREF_RF_CNTRL_TIMER_12_REG); 
    SetWord16( RF_CNTRL_TIMER_13_REG, PREF_RF_CNTRL_TIMER_13_REG);
    SetWord16( RF_CNTRL_TIMER_14_REG, PREF_RF_CNTRL_TIMER_14_REG);
    //SetWord16( BIAS_CTRL1_REG, PREF_BIAS_CTRL1_REG);
    SetWord16( RF_DEM_CTRL_REG, PREF_RF_DEM_CTRL_REG);
    SetWord16( RF_AGC_CTRL1_REG, PREF_RF_AGC_CTRL1_REG);
    SetWord16( RF_AGC_CTRL2_REG, PREF_RF_AGC_CTRL2_REG);
    SetWord16( RF_AGC_LUT_23_REG, PREF_RF_AGC_LUT_23_REG);
    SetWord16( RF_AFC_CTRL_REG, PREF_RF_AFC_CTRL_REG);
    SetWord16( RF_DC_OFFSET_CTRL2_REG, PREF_RF_DC_OFFSET_CTRL2_REG);
    SetWord16( RF_DC_OFFSET_CTRL3_REG, PREF_RF_DC_OFFSET_CTRL3_REG);
    SetWord16( RF_DC_OFFSET_CTRL4_REG, PREF_RF_DC_OFFSET_CTRL4_REG);
    SetWord16( RF_PA_CTRL_REG, PREF_RF_PA_CTRL_REG);
    SetWord16( RF_SYNTH_CTRL2_REG, PREF_RF_SYNTH_CTRL2_REG);
#if (LUT_PATCH_ENABLED)
    SetBits16( RF_VCOCAL_CTRL_REG, VCO_FREQTRIM_SEL, 0x1);
#else
    SetWord16( RF_VCOCAL_CTRL_REG, PREF_RF_VCOCAL_CTRL_REG);
#endif
#if (MGCKMODA_PATCH_ENABLED)    
  SetWord16( RF_MGAIN_CTRL_REG,     0x9503); // GAUSS_GAIN_SEL=0x1 and KMOD_ALPHA=3 to have some range above/below the final SW based KMOD_ALPHA value 
#else
  SetWord16( RF_MGAIN_CTRL_REG, PREF_RF_MGAIN_CTRL_REG);
#endif
    SetWord16( RF_VCO_CALCAP_BIT14_REG, PREF_RF_VCO_CALCAP_BIT14_REG);
    SetWord16( RF_LF_RES_CTRL_REG, PREF_RF_LF_RES_CTRL_REG);
    SetWord16( RF_MGAIN_CTRL2_REG, PREF_RF_MGAIN_CTRL2_REG);	
    SetWord16( RF_MIXER_CTRL1_REG, iq_trim_rf_mixer_ctrl1_reg);
    SetWord16( BIAS_CTRL1_REG, iq_trim_bias_ctrl1_reg);

    //  Near Field Mode Enable
    if(rf_nfm_is_enabled())
    {
        SetWord16( RF_ENABLE_CONFIG13_REG, 0x0030);
    }

}

/**
 *****************************************************************************************
 * @brief Radio RSSI convertion routine. Passed to BLE core / RF api structure
 *
 * @param[in] rssi_reg  RF measured RSSI.
 *
 * @return uint8_t  Converted RSSI
 *****************************************************************************************
 */
static uint8_t rf_rssi_convert(uint8_t rssi_reg)
{
    return rssi_reg/2;
}


/**
 *****************************************************************************************
 * @brief Initialize Radio module.
 *
 * @param[in] api  Stucture of API functions of the RF driver that are used by the BLE software.
 *
 * @return void
 *****************************************************************************************
 */
void rf_init_func(struct rwip_rf_api *api)
{

    uint32 tmp32 = 0;
    uint8 idx = 0;
    uint8 temp_freq_tbl[EM_BLE_FREQ_TABLE_LEN];

   // Initialize the RF driver API structure
    api->reg_rd = rf_reg_rd;
    api->reg_wr = rf_reg_wr;
    api->txpwr_dbm_get = rf_txpwr_dbm_get;

    //api->txpwr_max = RPL_POWER_MAX;
    api->sleep = rf_sleep;
    api->reset = rf_reset;
    #ifdef CFG_BLE
    api->isr = RADIOCNTL_Handler;
    api->force_agc_enable = rf_force_agc_enable;
    #endif //CFG_BLE

    api->rssi_convert = rf_rssi_convert;
    
    #ifdef CFG_BT
    api->txpwr_inc = rf_txpwr_inc;
    api->txpwr_dec = rf_txpwr_dec;
    api->txpwr_epc_req = rf_txpwr_epc_req;
    api->txpwr_cs_get = rf_txpwr_cs_get;
    
    api->rssi_high_thr = (uint8_t)RPL_RSSI_20dB_THRHLD;
    api->rssi_low_thr = (uint8_t)RPL_RSSI_60dB_THRHLD;
    api->rssi_interf_thr = (uint8_t)RPL_RSSI_70dB_THRHLD;
    #ifdef CFG_BTCORE_30
    api->wakeup_delay = RPL_WK_UP_DELAY;
    #endif //CFG_BTCORE_30
    api->skew = RPL_RADIO_SKEW;
    #endif //CFG_BT

	// CLK_FREQ_TRIM_REG initialization was moved to main_func() in arch_main.c
	// The initialization of this register is done by the Boot ROM code if a valid
	// value has been written to the corresponding position in the OTP header by
	// the customer.
	// main_func() will write this register with a default value and the customer
	// must remove this code when he has written the OTP header.

    SetBits32(&tmp32, RTRIP_DELAY, 7);    
    SetBits32(&tmp32, TXPWRDN, 0x5);  
    SetBits32(&tmp32, RXPWRUP, RXPWRUP_VAL);  
    SetBits32(&tmp32, TXPWRUP, TXPWRUP_VAL);
    SetWord32(BLE_RADIOPWRUPDN_REG, tmp32);   

    SetBits32(BLE_RADIOCNTL0_REG, DPCORR_EN, 0);  //THIS MAY NOT BE '1', THEN WE MISS 12 BITS IN THE SYNCWORD DURING A RX BURST
    SetBits32(BLE_RADIOCNTL1_REG, XRFSEL, 3);
    SetBits32(BLE_CNTL2_REG, SW_RPL_SPI ,0);
    SetBits32(BLE_CNTL2_REG, BB_ONLY,0);    	

    while(idx < EM_BLE_FREQ_TABLE_LEN)    
    {
		temp_freq_tbl[idx] = idx ;
		idx++;
    }

    em_ble_burst_wr(&temp_freq_tbl[0], EM_BLE_FT_OFFSET, EM_BLE_FREQ_TABLE_LEN);				
	rf_regs();
    
    last_temp_count = get_rc16m_count();
    
#if LUT_PATCH_ENABLED
    pll_vcocal_LUT_InitUpdate(LUT_INIT);    
#endif

	rf_calibration();		
}   

/**
 *****************************************************************************************
 * @brief Re-initialize Radio module. Called when system exits sleep mode.
 *
 * @return void
 *****************************************************************************************
 */
void rf_reinit_func(void)
{
    uint32 tmp32 = 0;

    SetBits32(&tmp32, RTRIP_DELAY, 7);    
    SetBits32(&tmp32, TXPWRDN, 0x5);  
    SetBits32(&tmp32, RXPWRUP, RXPWRUP_VAL);  
    SetBits32(&tmp32, TXPWRUP, TXPWRUP_VAL);
    SetWord32(BLE_RADIOPWRUPDN_REG, tmp32);

    SetBits32(BLE_RADIOCNTL1_REG, XRFSEL, 3);
    SetBits32(BLE_RWBTLECNTL_REG, SYNCERR, 0); //this must be always '0'

    SetBits16(CLK_RADIO_REG, RFCU_DIV, 1); //RFCU clock must always be 8MHz!
    SetBits16(CLK_RADIO_REG, RFCU_ENABLE, 1);

    if ( arch_clk_is_XTAL32( ) )
    {
        //If LP clock is XTAL32 in Boost mode set XTAL32K_CUR to 1.
        if (GetBits16(ANA_STATUS_REG, BOOST_SELECTED) == 0x1) 
            SetBits16(CLK_32K_REG,  XTAL32K_CUR, 1);
    }
        
    rf_regs();  
        
#if LUT_PATCH_ENABLED
    const volatile struct LUT_CFG_struct *pLUT_CFG;          // = (const volatile struct LUT_CFG_struct *)(jump_table_struct[lut_cfg_pos]);
    pLUT_CFG = (const volatile struct LUT_CFG_struct *)(jump_table_struct[lut_cfg_pos]);
    if (pLUT_CFG->HW_LUT_MODE)
    {
        SetWord16(RF_VCOCAL_CTRL_REG, vcocal_ctrl_reg_val);
    }
#endif //LUT_PATCH_ENABLED
    enable_rf_diag_irq(RF_DIAG_IRQ_MODE_RXTX);                               // This just enables the TX_EN int. RX_EN int enable status remains as it was
}

/*
 * NEAR FIELD MODE API
 ****************************************************************************************
 */

// Indicates if Near Field mode is active.
static bool near_field_mode __attribute__((section("retention_mem_area0"),zero_init));


/**
 ****************************************************************************************
 * @brief Checks if Near Field mode is enabled.
 *
 * @return bool   true if Near Field mode is enabled, otherwise false.
 ****************************************************************************************
 */
bool rf_nfm_is_enabled(void)
{
    return near_field_mode;
}


/**
 ****************************************************************************************
 * @brief Enables Near Field mode for all connections.
 *
 * @return void
 ****************************************************************************************
 */
void rf_nfm_enable(void)
{
    near_field_mode = true;
    SetWord16(RF_ENABLE_CONFIG13_REG, 0x0030);
}


/**
 ****************************************************************************************
 * @brief Disables Near Field mode for all connections.
 *
 * @return void
 ****************************************************************************************
 */
void rf_nfm_disable(void)
{
    near_field_mode = false;
    SetWord16(RF_ENABLE_CONFIG13_REG, PREF_RF_ENABLE_CONFIG13_REG);
}


#endif
///@} RF_RPL
