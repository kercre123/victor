
/*******************************************************************************
 This file is part of VL53L1 Protected

 Copyright (c) 2017, STMicroelectronics - All Rights Reserved

 License terms: STMicroelectronics Proprietary in accordance with licensing
 terms at www.st.com/sla0081

 STMicroelectronics confidential
 Reproduction and Communication of this document is strictly prohibited unless
 specifically authorized in writing by STMicroelectronics.

*/






































#ifndef _VL53L1_DMAX_H_
#define _VL53L1_DMAX_H_

#include "vl53l1_types.h"
#include "vl53l1_hist_structs.h"
#include "vl53l1_dmax_private_structs.h"
#include "vl53l1_error_codes.h"

#ifdef __cplusplus
extern "C" {
#endif
























VL53L1_Error VL53L1_f_001(
	uint16_t                              target_reflectance,
	VL53L1_dmax_calibration_data_t	     *pcal,
	VL53L1_hist_gen3_dmax_config_t	     *pcfg,
	VL53L1_histogram_bin_data_t          *pbins,
	VL53L1_hist_gen3_dmax_private_data_t *pdata,
	int16_t                              *pambient_dmax_mm);
















uint32_t VL53L1_f_002(
	uint32_t     events_threshold,
	uint32_t     ref_signal_events,
	uint32_t	 ref_distance_mm,
	uint32_t     signal_thresh_sigma);


#ifdef __cplusplus
}
#endif

#endif


