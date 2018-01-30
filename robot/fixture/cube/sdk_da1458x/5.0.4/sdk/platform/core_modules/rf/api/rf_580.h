/**
 ****************************************************************************************
 *
 * @file rf_580.h
 *
 * @brief DA14580 RF control Header File.
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
#ifndef _RF_580_H_
#define _RF_580_H_

#include "global_io.h"
#include "rwip.h"               // for RF API structure definition
#include "reg_blecore.h"        // ble core registers
#include "reg_ble_em_cs.h"      // control structure definitions

#define RC16_CAL_REF_CYCLES 0x400

#define RF_DIAG_IRQ_MODE_TXONLY       0
#define RF_DIAG_IRQ_MODE_RXTX         1

void rf_init(struct rwip_rf_api *api);

void rf_init_func(struct rwip_rf_api *api);

bool rf_calibration(void);

uint16_t get_rc16m_count(void);

void conditionally_run_radio_cals(void);

void enable_rf_diag_irq(uint8_t mode);

void set_gauss_modgain(uint16_t cn);

void iq_trim_from_otp(void);

bool rf_nfm_is_enabled(void);

void rf_nfm_enable(void);

void rf_nfm_disable(void);

#endif // _RF_580_H_

