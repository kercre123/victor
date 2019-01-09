
/*
* This file is part of VL53L1 Protected
*
* Copyright (C) 2016, STMicroelectronics - All Rights Reserved
*
* License terms: STMicroelectronics Proprietary in accordance with licensing
* terms at www.st.com/sla0044
*
* STMicroelectronics confidential
* Reproduction and Communication of this document is strictly prohibited unless
* specifically authorized in writing by STMicroelectronics.
*
*/







































#ifndef _VL53L1_HIST_ALGOS_GEN2_H_
#define _VL53L1_HIST_ALGOS_GEN2_H_

#include "vl53l1_types.h"
#include "vl53l1_ll_def.h"

#include "vl53l1_hist_private_structs.h"

#ifdef __cplusplus
extern "C"
{
#endif





















VL53L1_Error VL53L1_FCTN_00003(
	VL53L1_hist_post_process_config_t      *ppost_cfg,
	VL53L1_histogram_bin_data_t            *pbins,
	VL53L1_histogram_bin_data_t            *pxtalk,
	VL53L1_hist_gen2_algo_filtered_data_t  *pfiltered0,
	VL53L1_hist_gen2_algo_filtered_data_t  *pfiltered1,
	VL53L1_hist_gen2_algo_filtered_data_t  *pfilteredx,
	VL53L1_hist_gen2_algo_detection_data_t *pdetection,
	VL53L1_range_results_t                 *presults);














void  VL53L1_FCTN_00005(
	uint8_t                                 filter_woi,
	VL53L1_histogram_bin_data_t            *pbins,
	VL53L1_hist_gen2_algo_filtered_data_t  *pfiltered);



















void  VL53L1_FCTN_00006(
	int32_t                                 ambient_threshold_sigma,
	VL53L1_hist_gen2_algo_filtered_data_t  *pfiltered,
	VL53L1_hist_gen2_algo_detection_data_t *pdetection);











void  VL53L1_FCTN_00007(
	VL53L1_hist_gen2_algo_filtered_data_t  *pfiltered,
	VL53L1_hist_gen2_algo_detection_data_t *pdetection);














void VL53L1_FCTN_00008(
	uint8_t                                sigma_estimator__sigma_ref_mm,
	VL53L1_hist_gen2_algo_filtered_data_t  *pfiltered,
	VL53L1_hist_gen2_algo_filtered_data_t  *pfilteredx,
	VL53L1_hist_gen2_algo_detection_data_t *pdetection);





















VL53L1_Error VL53L1_FCTN_00015(
	uint8_t   bin,
	int32_t   VL53L1_PRM_00002,
	int32_t   VL53L1_PRM_00032,
	int32_t   VL53L1_PRM_00001,
	int32_t   ax,
	int32_t   bx,
	int32_t   cx,
	int32_t   VL53L1_PRM_00028,
	uint8_t   VL53L1_PRM_00030,
	uint32_t *pmean_phase);











void VL53L1_FCTN_00009(
	VL53L1_hist_gen2_algo_filtered_data_t  *pfiltered,
	VL53L1_hist_gen2_algo_filtered_data_t  *pfilteredx,
	VL53L1_hist_gen2_algo_detection_data_t *pdetection);

















void  VL53L1_FCTN_00010(
	int32_t                                 signal_total_events_limit,
	uint16_t                                sigma_thresh,
	VL53L1_histogram_bin_data_t             *pbins,
	VL53L1_hist_gen2_algo_filtered_data_t   *pfiltered,
	VL53L1_hist_gen2_algo_detection_data_t  *pdetection,
	VL53L1_range_results_t                  *presults);


#ifdef __cplusplus
}
#endif

#endif

