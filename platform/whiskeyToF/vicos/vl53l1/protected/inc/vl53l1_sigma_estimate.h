
/*******************************************************************************
 This file is part of VL53L1 Protected

 Copyright (c) 2017, STMicroelectronics - All Rights Reserved

 License terms: STMicroelectronics Proprietary in accordance with licensing
 terms at www.st.com/sla0081

 STMicroelectronics confidential
 Reproduction and Communication of this document is strictly prohibited unless
 specifically authorized in writing by STMicroelectronics.

*/






































#ifndef _VL53L1_SIGMA_ESTIMATE_H_
#define _VL53L1_SIGMA_ESTIMATE_H_

#include "vl53l1_types.h"
#include "vl53l1_ll_def.h"

#ifdef __cplusplus
extern "C" {
#endif

#define  VL53L1_D_002  0xFFFF



#define VL53L1_D_008	0xFFFF
#define VL53L1_D_003	0xFFFFFF
#define VL53L1_D_007	0xFFFFFFFF
#define VL53L1_D_005	0x7FFFFFFFFF
#define VL53L1_D_009	0xFFFFFFFFFF
#define VL53L1_D_010	0xFFFFFFFFFFFF
#define VL53L1_D_004	0xFFFFFFFFFFFFFF
#define VL53L1_D_006	0x7FFFFFFFFFFFFFFF
#define VL53L1_D_011	0xFFFFFFFFFFFFFFFF



















uint16_t VL53L1_f_042(
	uint8_t	 sigma_estimator__effective_pulse_width_ns,
	uint8_t  sigma_estimator__effective_ambient_width_ns,
	uint8_t	 sigma_estimator__sigma_ref_mm,
	VL53L1_range_data_t  *pdata);



















uint16_t VL53L1_f_044(
	uint8_t	 sigma_estimator__effective_pulse_width_ns,
	uint8_t  sigma_estimator__effective_ambient_width_ns,
	uint8_t	 sigma_estimator__sigma_ref_mm,
	VL53L1_range_data_t *pdata);





























VL53L1_Error  VL53L1_f_045(
	uint8_t	      sigma_estimator__sigma_ref_mm,
	uint32_t      VL53L1_p_003,
	uint32_t      VL53L1_p_018,
	uint32_t      VL53L1_p_001,
	uint32_t      a_zp,
	uint32_t      c_zp,
	uint32_t      bx,
	uint32_t      ax_zp,
	uint32_t      cx_zp,
	uint32_t      VL53L1_p_004,
	uint16_t      fast_osc_frequency,
	uint16_t      *psigma_est);





























VL53L1_Error  VL53L1_f_014(
	uint8_t	      sigma_estimator__sigma_ref_mm,
	uint32_t      VL53L1_p_003,
	uint32_t      VL53L1_p_018,
	uint32_t      VL53L1_p_001,
	uint32_t      a_zp,
	uint32_t      c_zp,
	uint32_t      bx,
	uint32_t      ax_zp,
	uint32_t      cx_zp,
	uint32_t      VL53L1_p_004,
	uint16_t      fast_osc_frequency,
	uint16_t      *psigma_est);












uint32_t VL53L1_f_046(
	uint64_t VL53L1_p_003,
	uint32_t size
	);
















uint32_t VL53L1_f_043(
	uint32_t  VL53L1_p_003,
	uint32_t  VL53L1_p_018);


#ifdef __cplusplus
}
#endif

#endif


