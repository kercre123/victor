
/*******************************************************************************
 This file is part of VL53L1 Protected

 Copyright (c) 2017, STMicroelectronics - All Rights Reserved

 License terms: STMicroelectronics Proprietary in accordance with licensing
 terms at www.st.com/sla0081

 STMicroelectronics confidential
 Reproduction and Communication of this document is strictly prohibited unless
 specifically authorized in writing by STMicroelectronics.

*/







































#ifndef _VL53L1_HIST_FUNCS_H_
#define _VL53L1_HIST_FUNCS_H_

#include "vl53l1_types.h"
#include "vl53l1_ll_def.h"

#ifdef __cplusplus
extern "C"
{
#endif


















VL53L1_Error VL53L1_hist_process_data(
	VL53L1_dmax_calibration_data_t    *pdmax_cal,
	VL53L1_hist_gen3_dmax_config_t    *pdmax_cfg,
	VL53L1_hist_post_process_config_t *ppost_cfg,
	VL53L1_histogram_bin_data_t       *pbins,
	VL53L1_xtalk_histogram_data_t     *pxtalk,
	VL53L1_range_results_t            *presults);






















VL53L1_Error VL53L1_hist_ambient_dmax(
	uint16_t                            target_reflectance,
	VL53L1_dmax_calibration_data_t     *pdmax_cal,
	VL53L1_hist_gen3_dmax_config_t     *pdmax_cfg,
	VL53L1_histogram_bin_data_t        *pbins,
	int16_t                            *pambient_dmax_mm);


#ifdef __cplusplus
}
#endif

#endif

