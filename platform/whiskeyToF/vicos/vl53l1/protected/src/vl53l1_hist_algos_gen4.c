
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






































#include "vl53l1_types.h"
#include "vl53l1_platform_log.h"

#include "vl53l1_core_support.h"
#include "vl53l1_error_codes.h"

#include "vl53l1_hist_core.h"
#include "vl53l1_hist_algos_gen3.h"
#include "vl53l1_hist_algos_gen4.h"
#include "vl53l1_sigma_estimate.h"
#include "vl53l1_dmax.h"




















#define LOG_FUNCTION_START(fmt, ...) \
	_LOG_FUNCTION_START(VL53L1_TRACE_MODULE_HISTOGRAM, fmt, ##__VA_ARGS__)
#define LOG_FUNCTION_END(status, ...) \
	_LOG_FUNCTION_END(VL53L1_TRACE_MODULE_HISTOGRAM, status, ##__VA_ARGS__)
#define LOG_FUNCTION_END_FMT(status, fmt, ...) \
	_LOG_FUNCTION_END_FMT(VL53L1_TRACE_MODULE_HISTOGRAM, status, fmt, ##__VA_ARGS__)

#define trace_print(level, ...) \
	_LOG_TRACE_PRINT(VL53L1_TRACE_MODULE_HISTOGRAM, \
	level, VL53L1_TRACE_FUNCTION_NONE, ##__VA_ARGS__)


void VL53L1_FCTN_00032(
	VL53L1_hist_gen4_algo_filtered_data_t   *palgo)
{





	uint8_t  VL53L1_PRM_00032                 = 0;

	palgo->VL53L1_PRM_00020              = VL53L1_HISTOGRAM_BUFFER_SIZE;
	palgo->VL53L1_PRM_00019                = 0;
	palgo->VL53L1_PRM_00021           = 0;

	for (VL53L1_PRM_00032 = palgo->VL53L1_PRM_00019 ; VL53L1_PRM_00032 < palgo->VL53L1_PRM_00020 ; VL53L1_PRM_00032++) {
		palgo->VL53L1_PRM_00002[VL53L1_PRM_00032]      = 0;
		palgo->VL53L1_PRM_00032[VL53L1_PRM_00032]      = 0;
		palgo->VL53L1_PRM_00001[VL53L1_PRM_00032]      = 0;
		palgo->VL53L1_PRM_00039[VL53L1_PRM_00032] = 0;
		palgo->VL53L1_PRM_00040[VL53L1_PRM_00032] = 0;
		palgo->VL53L1_PRM_00043[VL53L1_PRM_00032]  = 0;
	}
}


VL53L1_Error VL53L1_FCTN_00033(
	VL53L1_dmax_calibration_data_t         *pdmax_cal,
	VL53L1_hist_gen3_dmax_config_t         *pdmax_cfg,
	VL53L1_hist_post_process_config_t      *ppost_cfg,
	VL53L1_histogram_bin_data_t            *pbins_input,
	VL53L1_histogram_bin_data_t            *pxtalk,
	VL53L1_hist_gen3_algo_private_data_t   *palgo3,
	VL53L1_hist_gen4_algo_filtered_data_t  *pfiltered,
	VL53L1_hist_gen3_dmax_private_data_t   *pdmax_algo,
	VL53L1_range_results_t                 *presults)
{





	VL53L1_Error  status  = VL53L1_ERROR_NONE;

	VL53L1_hist_pulse_data_t     *ppulse_data;
	VL53L1_range_data_t          *prange_data;

	uint8_t                       p = 0;

	LOG_FUNCTION_START("");









	VL53L1_FCTN_00016(palgo3);






	memcpy(
		&(palgo3->VL53L1_PRM_00009),
		pbins_input,
		sizeof(VL53L1_histogram_bin_data_t));






	presults->cfg_device_state = pbins_input->cfg_device_state;
	presults->rd_device_state  = pbins_input->rd_device_state;
	presults->zone_id          = pbins_input->zone_id;
	presults->stream_count     = pbins_input->result__stream_count;
	presults->wrap_dmax_mm     = 0;
	presults->max_results      = VL53L1_MAX_RANGE_RESULTS;
	presults->active_results   = 0;

	for (p = 0; p < VL53L1_MAX_AMBIENT_DMAX_VALUES ; p++)
		presults->VL53L1_PRM_00006[p] = 0;




	VL53L1_hist_calc_zero_distance_phase(&(palgo3->VL53L1_PRM_00009));






	if (ppost_cfg->hist_amb_est_method ==
		VL53L1_HIST_AMB_EST_METHOD__THRESHOLDED_BINS)
		VL53L1_hist_estimate_ambient_from_thresholded_bins(
				(int32_t)ppost_cfg->ambient_thresh_sigma0,
				&(palgo3->VL53L1_PRM_00009));
	else
		VL53L1_hist_estimate_ambient_from_ambient_bins(&(palgo3->VL53L1_PRM_00009));





	VL53L1_hist_remove_ambient_bins(&(palgo3->VL53L1_PRM_00009));





	if (ppost_cfg->algo__crosstalk_compensation_enable > 0)
		VL53L1_FCTN_00004(
				pxtalk,
				&(palgo3->VL53L1_PRM_00009),
				&(palgo3->VL53L1_PRM_00038));






	pdmax_cfg->ambient_thresh_sigma =
		ppost_cfg->ambient_thresh_sigma1;

	for (p = 0 ; p < VL53L1_MAX_AMBIENT_DMAX_VALUES ; p++) {
		if (status == VL53L1_ERROR_NONE) {

			status =
				VL53L1_FCTN_00001(
					pdmax_cfg->target_reflectance_for_dmax_calc[p],
					pdmax_cal,
					pdmax_cfg,
					&(palgo3->VL53L1_PRM_00009),
					pdmax_algo,
					&(presults->VL53L1_PRM_00006[p]));
		}
	}









	if (status == VL53L1_ERROR_NONE)
		status =
			VL53L1_FCTN_00018(
				ppost_cfg->ambient_thresh_events_scaler,
				(int32_t)ppost_cfg->ambient_thresh_sigma1,
				(int32_t)ppost_cfg->min_ambient_thresh_events,
				ppost_cfg->algo__crosstalk_compensation_enable,
				&(palgo3->VL53L1_PRM_00009),
				&(palgo3->VL53L1_PRM_00038),
				palgo3);









	if (status == VL53L1_ERROR_NONE)
		status =
			VL53L1_FCTN_00019(palgo3);






	if (status == VL53L1_ERROR_NONE)
		status =
			VL53L1_FCTN_00020(palgo3);






	if (status == VL53L1_ERROR_NONE)
		status =
			VL53L1_FCTN_00021(palgo3);






	for (p = 0 ; p < palgo3->VL53L1_PRM_00051 ; p++) {

		ppulse_data = &(palgo3->VL53L1_PRM_00005[p]);




		if (status == VL53L1_ERROR_NONE)
			status =
				VL53L1_FCTN_00022(
					p,
					&(palgo3->VL53L1_PRM_00009),
					palgo3);




		if (status == VL53L1_ERROR_NONE)
			status =
				VL53L1_FCTN_00023(
					p,
					&(palgo3->VL53L1_PRM_00009),
					palgo3,
					palgo3->VL53L1_PRM_00009.VL53L1_PRM_00028,
					&(palgo3->VL53L1_PRM_00052));




		if (status == VL53L1_ERROR_NONE) {
			status =
				VL53L1_FCTN_00023(
					p,
					&(palgo3->VL53L1_PRM_00009),
					palgo3,
					0,
					&(palgo3->VL53L1_PRM_00053));
		}




		if (status == VL53L1_ERROR_NONE) {
			status =
				VL53L1_FCTN_00023(
					p,
					&(palgo3->VL53L1_PRM_00038),
					palgo3,
					0,
					&(palgo3->VL53L1_PRM_00054));
		}




		if (status == VL53L1_ERROR_NONE)
			status =
				VL53L1_FCTN_00034(
					p,
					&(palgo3->VL53L1_PRM_00052),
					palgo3,
					pfiltered);




		if (status == VL53L1_ERROR_NONE)
			status =
				VL53L1_FCTN_00035(
					p,
					ppost_cfg->noise_threshold,
					pfiltered,
					palgo3);

		if (status == VL53L1_ERROR_NONE)
			status =
				VL53L1_FCTN_00026(
					ppulse_data->VL53L1_PRM_00023,
					ppost_cfg->sigma_estimator__sigma_ref_mm,
					palgo3->VL53L1_PRM_00030,
					ppulse_data->VL53L1_PRM_00055,
					ppost_cfg->algo__crosstalk_compensation_enable,
					&(palgo3->VL53L1_PRM_00052),
					&(palgo3->VL53L1_PRM_00053),
					&(palgo3->VL53L1_PRM_00054),
					&(ppulse_data->VL53L1_PRM_00003));









		if (status == VL53L1_ERROR_NONE)
			status =
				VL53L1_FCTN_00027(
					p,
					1,
					&(palgo3->VL53L1_PRM_00009),
					palgo3);

	}






	if (status == VL53L1_ERROR_NONE)
		status =
			VL53L1_FCTN_00028(
				ppost_cfg->hist_target_order,
				palgo3);






	for (p = 0 ; p < palgo3->VL53L1_PRM_00051 ; p++) {

		ppulse_data = &(palgo3->VL53L1_PRM_00005[p]);




		if (presults->active_results < presults->max_results) {







			if (ppulse_data->VL53L1_PRM_00012 > ppost_cfg->signal_total_events_limit &&
				ppulse_data->VL53L1_PRM_00023 < 0xFF) {

				prange_data = &(presults->VL53L1_PRM_00005[presults->active_results]);

				if (status == VL53L1_ERROR_NONE)
					VL53L1_FCTN_00029(
							presults->active_results,
							ppost_cfg->valid_phase_low,
							ppost_cfg->valid_phase_high,
							ppost_cfg->sigma_thresh,
							&(palgo3->VL53L1_PRM_00009),
							ppulse_data,
							prange_data);

				if (status == VL53L1_ERROR_NONE)
					status =
						VL53L1_FCTN_00011(
							palgo3->VL53L1_PRM_00009.vcsel_width,
							palgo3->VL53L1_PRM_00009.VL53L1_PRM_00022,
							palgo3->VL53L1_PRM_00009.total_periods_elapsed,
							palgo3->VL53L1_PRM_00009.result__dss_actual_effective_spads,
							prange_data);

				if (status == VL53L1_ERROR_NONE)
					 VL53L1_FCTN_00012(
						ppost_cfg->gain_factor,
						ppost_cfg->range_offset_mm,
						prange_data);

				presults->active_results++;
			}
		}

	}




	LOG_FUNCTION_END(status);

	return status;
}



VL53L1_Error VL53L1_FCTN_00034(
	uint8_t                                pulse_no,
	VL53L1_histogram_bin_data_t           *ppulse,
	VL53L1_hist_gen3_algo_private_data_t  *palgo3,
	VL53L1_hist_gen4_algo_filtered_data_t *pfiltered)
{







	VL53L1_Error  status       = VL53L1_ERROR_NONE;

	VL53L1_hist_pulse_data_t *pdata = &(palgo3->VL53L1_PRM_00005[pulse_no]);

	uint8_t  VL53L1_PRM_00032     = 0;
	uint8_t  i     = 0;
	int32_t  suma  = 0;
	int32_t  sumb  = 0;
	int32_t  sumc  = 0;

	LOG_FUNCTION_START("");

	pfiltered->VL53L1_PRM_00020    = palgo3->VL53L1_PRM_00020;
	pfiltered->VL53L1_PRM_00019      = palgo3->VL53L1_PRM_00019;
	pfiltered->VL53L1_PRM_00021 = palgo3->VL53L1_PRM_00021;




	for (VL53L1_PRM_00032 = pdata->VL53L1_PRM_00014 ; VL53L1_PRM_00032 <= pdata->VL53L1_PRM_00015 ; VL53L1_PRM_00032++) {

		i =  VL53L1_PRM_00032  % palgo3->VL53L1_PRM_00030;



		VL53L1_FCTN_00013(
				i,
				pdata->VL53L1_PRM_00055,
				ppulse,
				&suma,
				&sumb,
				&sumc);



		pfiltered->VL53L1_PRM_00002[i] = suma;
		pfiltered->VL53L1_PRM_00032[i] = sumb;
		pfiltered->VL53L1_PRM_00001[i] = sumc;




		pfiltered->VL53L1_PRM_00039[i] =
			(suma + sumb) -
			(sumc + palgo3->VL53L1_PRM_00028);




		pfiltered->VL53L1_PRM_00040[i] =
			(sumb + sumc) -
			(suma + palgo3->VL53L1_PRM_00028);
	}

	return status;
}


VL53L1_Error VL53L1_FCTN_00035(
	uint8_t                                pulse_no,
	uint16_t                               noise_threshold,
	VL53L1_hist_gen4_algo_filtered_data_t *pfiltered,
	VL53L1_hist_gen3_algo_private_data_t  *palgo3)
{






	VL53L1_Error  status       = VL53L1_ERROR_NONE;
	VL53L1_Error  func_status  = VL53L1_ERROR_NONE;

	VL53L1_hist_pulse_data_t *pdata = &(palgo3->VL53L1_PRM_00005[pulse_no]);

	uint8_t  VL53L1_PRM_00032            = 0;
	uint8_t  i            = 0;
	uint8_t  j            = 0;

	SUPPRESS_UNUSED_WARNING(noise_threshold);

	for (VL53L1_PRM_00032 = pdata->VL53L1_PRM_00014 ; VL53L1_PRM_00032 < pdata->VL53L1_PRM_00015 ; VL53L1_PRM_00032++) {

		i =  VL53L1_PRM_00032    % palgo3->VL53L1_PRM_00030;
		j = (VL53L1_PRM_00032+1) % palgo3->VL53L1_PRM_00030;

		if (i < palgo3->VL53L1_PRM_00021 && j < palgo3->VL53L1_PRM_00021) {

			if (pfiltered->VL53L1_PRM_00039[i] == 0 &&
				pfiltered->VL53L1_PRM_00040[i] == 0)


				pfiltered->VL53L1_PRM_00043[i] = 0;

			else if (pfiltered->VL53L1_PRM_00039[i] >= 0 &&
					 pfiltered->VL53L1_PRM_00040[i] >= 0)
				pfiltered->VL53L1_PRM_00043[i] = 1;

			else if (pfiltered->VL53L1_PRM_00039[i] <  0 &&
					 pfiltered->VL53L1_PRM_00040[i] >= 0 &&
					 pfiltered->VL53L1_PRM_00039[j] >= 0 &&
					 pfiltered->VL53L1_PRM_00040[j] <  0)
				pfiltered->VL53L1_PRM_00043[i] = 1;

			else
				pfiltered->VL53L1_PRM_00043[i] = 0;



			if (pfiltered->VL53L1_PRM_00043[i] > 0) {

				pdata->VL53L1_PRM_00023            = VL53L1_PRM_00032;

				func_status =
					VL53L1_FCTN_00036(
						VL53L1_PRM_00032,
						pfiltered->VL53L1_PRM_00002[i],
						pfiltered->VL53L1_PRM_00032[i],
						pfiltered->VL53L1_PRM_00001[i],
						0,

						0,

						0,

						palgo3->VL53L1_PRM_00028,
						palgo3->VL53L1_PRM_00030,
						&(pdata->VL53L1_PRM_00013));

				if (func_status == VL53L1_ERROR_DIVISION_BY_ZERO) {
					pfiltered->VL53L1_PRM_00043[i] = 0;
				}
			}
		}
	}

	return status;
}


VL53L1_Error VL53L1_FCTN_00036(
	uint8_t   bin,
	int32_t   VL53L1_PRM_00002,
	int32_t   VL53L1_PRM_00032,
	int32_t   VL53L1_PRM_00001,
	int32_t   ax,
	int32_t   bx,
	int32_t   cx,
	int32_t   VL53L1_PRM_00028,
	uint8_t   VL53L1_PRM_00030,
	uint32_t *pmean_phase)
{





	VL53L1_Error  status = VL53L1_ERROR_DIVISION_BY_ZERO;

	int64_t  mean_phase  = VL53L1_MAX_ALLOWED_PHASE;
	int64_t  VL53L1_PRM_00041   = 0;
	int64_t  b_minus_amb = 0;








	VL53L1_PRM_00041    =     4096 * ((int64_t)VL53L1_PRM_00001 - (int64_t)cx - (int64_t)VL53L1_PRM_00002 -  (int64_t)ax);
	b_minus_amb  = 2 * 4096 * ((int64_t)VL53L1_PRM_00032 - (int64_t)bx - (int64_t)VL53L1_PRM_00028);

	if (b_minus_amb != 0) {

		mean_phase   =  ((4096 * VL53L1_PRM_00041)  + (b_minus_amb/2)) / b_minus_amb;
		mean_phase  +=  2048;
		mean_phase  += (4096 * (int64_t)bin);



		mean_phase  = (mean_phase + 1)/2;



		if (mean_phase  < 0)
			mean_phase = 0;
		if (mean_phase > VL53L1_MAX_ALLOWED_PHASE)
			mean_phase = VL53L1_MAX_ALLOWED_PHASE;



		mean_phase = mean_phase % ((int64_t)VL53L1_PRM_00030 * 2048);

		status = VL53L1_ERROR_NONE;

	}

	*pmean_phase = (uint32_t)mean_phase;

	return status;
}


