
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


void VL53L1_FCTN_00016(
	VL53L1_hist_gen3_algo_private_data_t   *palgo)
{





	uint8_t  VL53L1_PRM_00032                 = 0;

	palgo->VL53L1_PRM_00020              = VL53L1_HISTOGRAM_BUFFER_SIZE;
	palgo->VL53L1_PRM_00019                = 0;
	palgo->VL53L1_PRM_00021           = 0;
	palgo->VL53L1_PRM_00045         = 0;
	palgo->VL53L1_PRM_00028   = 0;
	palgo->VL53L1_PRM_00031 = 0;

	for (VL53L1_PRM_00032 = palgo->VL53L1_PRM_00019 ; VL53L1_PRM_00032 < palgo->VL53L1_PRM_00020 ; VL53L1_PRM_00032++) {
		palgo->VL53L1_PRM_00043[VL53L1_PRM_00032]      = 0;
		palgo->VL53L1_PRM_00046[VL53L1_PRM_00032] = 0;
		palgo->VL53L1_PRM_00047[VL53L1_PRM_00032]     = 0;
		palgo->VL53L1_PRM_00048[VL53L1_PRM_00032]      = 0;
		palgo->VL53L1_PRM_00007[VL53L1_PRM_00032]     = 0;
	}

	palgo->VL53L1_PRM_00049 = 0;
	palgo->VL53L1_PRM_00050               = VL53L1_DEF_00001;
	palgo->VL53L1_PRM_00051             = 0;




	VL53L1_init_histogram_bin_data_struct(
		0,
		VL53L1_HISTOGRAM_BUFFER_SIZE,
		&(palgo->VL53L1_PRM_00009));
	VL53L1_init_histogram_bin_data_struct(
		0,
		VL53L1_HISTOGRAM_BUFFER_SIZE,
		&(palgo->VL53L1_PRM_00038));
	VL53L1_init_histogram_bin_data_struct(
		0,
		VL53L1_HISTOGRAM_BUFFER_SIZE,
		&(palgo->VL53L1_PRM_00052));
	VL53L1_init_histogram_bin_data_struct(
		0,
		VL53L1_HISTOGRAM_BUFFER_SIZE,
		&(palgo->VL53L1_PRM_00053));
	VL53L1_init_histogram_bin_data_struct(
		0,
		VL53L1_HISTOGRAM_BUFFER_SIZE,
		&(palgo->VL53L1_PRM_00054));
}


VL53L1_Error VL53L1_FCTN_00017(
	VL53L1_dmax_calibration_data_t         *pdmax_cal,
	VL53L1_hist_gen3_dmax_config_t         *pdmax_cfg,
	VL53L1_hist_post_process_config_t      *ppost_cfg,
	VL53L1_histogram_bin_data_t            *pbins_input,
	VL53L1_histogram_bin_data_t            *pxtalk,
	VL53L1_hist_gen3_algo_private_data_t   *palgo,
	VL53L1_hist_gen3_dmax_private_data_t   *pdmax_algo,
	VL53L1_range_results_t                 *presults)
{





	VL53L1_Error  status  = VL53L1_ERROR_NONE;

	VL53L1_hist_pulse_data_t     *ppulse_data;
	VL53L1_range_data_t          *prange_data;

	uint8_t                       p = 0;

	LOG_FUNCTION_START("");









	VL53L1_FCTN_00016(palgo);






	memcpy(
		&(palgo->VL53L1_PRM_00009),
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




	VL53L1_hist_calc_zero_distance_phase(&(palgo->VL53L1_PRM_00009));






	if (ppost_cfg->hist_amb_est_method ==
		VL53L1_HIST_AMB_EST_METHOD__THRESHOLDED_BINS) {
		VL53L1_hist_estimate_ambient_from_thresholded_bins(
				(int32_t)ppost_cfg->ambient_thresh_sigma0,
				&(palgo->VL53L1_PRM_00009));
	} else {
		VL53L1_hist_estimate_ambient_from_ambient_bins(&(palgo->VL53L1_PRM_00009));
	}





	VL53L1_hist_remove_ambient_bins(&(palgo->VL53L1_PRM_00009));





	if (ppost_cfg->algo__crosstalk_compensation_enable > 0) {
		VL53L1_FCTN_00004(
				pxtalk,
				&(palgo->VL53L1_PRM_00009),
				&(palgo->VL53L1_PRM_00038));
	}






	pdmax_cfg->ambient_thresh_sigma =
		ppost_cfg->ambient_thresh_sigma1;

	for (p = 0 ; p < VL53L1_MAX_AMBIENT_DMAX_VALUES ; p++) {
		if (status == VL53L1_ERROR_NONE) {

			status =
				VL53L1_FCTN_00001(
					pdmax_cfg->target_reflectance_for_dmax_calc[p],
					pdmax_cal,
					pdmax_cfg,
					&(palgo->VL53L1_PRM_00009),
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
				&(palgo->VL53L1_PRM_00009),
				&(palgo->VL53L1_PRM_00038),
				palgo);









	if (status == VL53L1_ERROR_NONE) {
		status =
			VL53L1_FCTN_00019(palgo);
	}






	if (status == VL53L1_ERROR_NONE) {
		status =
			VL53L1_FCTN_00020(palgo);
	}






	if (status == VL53L1_ERROR_NONE) {
		status =
			VL53L1_FCTN_00021(palgo);
	}






	for (p = 0 ; p < palgo->VL53L1_PRM_00051 ; p++) {

		ppulse_data = &(palgo->VL53L1_PRM_00005[p]);




		if (status == VL53L1_ERROR_NONE) {
			status =
				VL53L1_FCTN_00022(
					p,
					&(palgo->VL53L1_PRM_00009),
					palgo);
		}




		if (status == VL53L1_ERROR_NONE) {
			status =
				VL53L1_FCTN_00023(
					p,
					&(palgo->VL53L1_PRM_00009),
					palgo,
					palgo->VL53L1_PRM_00009.VL53L1_PRM_00028,
					&(palgo->VL53L1_PRM_00052));
		}




		if (status == VL53L1_ERROR_NONE) {
			status =
				VL53L1_FCTN_00023(
					p,
					&(palgo->VL53L1_PRM_00009),
					palgo,
					0,
					&(palgo->VL53L1_PRM_00053));
		}




		if (status == VL53L1_ERROR_NONE) {
			status =
				VL53L1_FCTN_00023(
					p,
					&(palgo->VL53L1_PRM_00038),
					palgo,
					0,
					&(palgo->VL53L1_PRM_00054));
		}




		if (status == VL53L1_ERROR_NONE) {
			status =
				VL53L1_FCTN_00024(
					p,
					&(palgo->VL53L1_PRM_00052),
					palgo);
		}




		if (status == VL53L1_ERROR_NONE) {
			status =
				VL53L1_FCTN_00025(
					p,
					ppost_cfg->noise_threshold,
					palgo);
		}

		if (status == VL53L1_ERROR_NONE) {
			status =
				VL53L1_FCTN_00026(
					ppulse_data->VL53L1_PRM_00023,
					ppost_cfg->sigma_estimator__sigma_ref_mm,
					palgo->VL53L1_PRM_00030,
					ppulse_data->VL53L1_PRM_00055,
					ppost_cfg->algo__crosstalk_compensation_enable,
					&(palgo->VL53L1_PRM_00052),
					&(palgo->VL53L1_PRM_00053),
					&(palgo->VL53L1_PRM_00054),
					&(ppulse_data->VL53L1_PRM_00003));
		}









		if (status == VL53L1_ERROR_NONE) {
			status =
				VL53L1_FCTN_00027(
					p,
					1,
					&(palgo->VL53L1_PRM_00009),
					palgo);
		}
	}






	if (status == VL53L1_ERROR_NONE)
		status =
			VL53L1_FCTN_00028(
				ppost_cfg->hist_target_order,
				palgo);






	for (p = 0 ; p < palgo->VL53L1_PRM_00051 ; p++) {

		ppulse_data = &(palgo->VL53L1_PRM_00005[p]);




		if (presults->active_results < presults->max_results) {







			if (ppulse_data->VL53L1_PRM_00012 >
				ppost_cfg->signal_total_events_limit &&
				ppulse_data->VL53L1_PRM_00023 < 0xFF) {

				prange_data = &(presults->VL53L1_PRM_00005[presults->active_results]);

				if (status == VL53L1_ERROR_NONE) {
					VL53L1_FCTN_00029(
							presults->active_results,
							ppost_cfg->valid_phase_low,
							ppost_cfg->valid_phase_high,
							ppost_cfg->sigma_thresh,
							&(palgo->VL53L1_PRM_00009),
							ppulse_data,
							prange_data);
				}

				if (status == VL53L1_ERROR_NONE) {
					status =
						VL53L1_FCTN_00011(
							palgo->VL53L1_PRM_00009.vcsel_width,
							palgo->VL53L1_PRM_00009.VL53L1_PRM_00022,
							palgo->VL53L1_PRM_00009.total_periods_elapsed,
							palgo->VL53L1_PRM_00009.result__dss_actual_effective_spads,
							prange_data);
				}

				if (status == VL53L1_ERROR_NONE) {
					 VL53L1_FCTN_00012(
						ppost_cfg->gain_factor,
						ppost_cfg->range_offset_mm,
						prange_data);
				}

				presults->active_results++;
			}
		}
	}




	LOG_FUNCTION_END(status);

	return status;
}




VL53L1_Error VL53L1_FCTN_00018(
	uint16_t                               ambient_threshold_events_scaler,
	int32_t                                ambient_threshold_sigma,
	int32_t                                min_ambient_threshold_events,
	uint8_t                                algo__crosstalk_compensation_enable,
	VL53L1_histogram_bin_data_t           *pbins,
	VL53L1_histogram_bin_data_t           *pxtalk,
	VL53L1_hist_gen3_algo_private_data_t  *palgo)
{






	VL53L1_Error  status  = VL53L1_ERROR_NONE;
	uint8_t  VL53L1_PRM_00032            = 0;
	uint8_t  VL53L1_PRM_00001            = 0;
	int64_t  tmp          = 0;
	int32_t  amb_events   = 0;
	int32_t  VL53L1_PRM_00007       = 0;
	int32_t  samples      = 0;

	LOG_FUNCTION_START("");



	palgo->VL53L1_PRM_00020            = pbins->VL53L1_PRM_00020;
	palgo->VL53L1_PRM_00019              = pbins->VL53L1_PRM_00019;
	palgo->VL53L1_PRM_00021         = pbins->VL53L1_PRM_00021;
	palgo->VL53L1_PRM_00028 = pbins->VL53L1_PRM_00028;






	palgo->VL53L1_PRM_00030 =
			VL53L1_decode_vcsel_period(pbins->VL53L1_PRM_00008);







	tmp  = (int64_t)pbins->VL53L1_PRM_00028;
	tmp *= (int64_t)ambient_threshold_events_scaler;
	tmp += 2048;
	tmp = do_division_s(tmp, 4096);
	amb_events = (int32_t)tmp;








	for (VL53L1_PRM_00032 = 0 ; VL53L1_PRM_00032 < pbins->VL53L1_PRM_00021 ; VL53L1_PRM_00032++) {

		VL53L1_PRM_00001 = VL53L1_PRM_00032 >> 2;
		samples = (int32_t)pbins->bin_rep[VL53L1_PRM_00001];

		if (samples > 0) {

			if (VL53L1_PRM_00032 < pxtalk->VL53L1_PRM_00021 &&
				algo__crosstalk_compensation_enable > 0)
				VL53L1_PRM_00007 = samples * (amb_events + pxtalk->bin_data[VL53L1_PRM_00032]);
			else
				VL53L1_PRM_00007 = samples *  amb_events;

			VL53L1_PRM_00007  = VL53L1_isqrt(VL53L1_PRM_00007);

			VL53L1_PRM_00007 += (samples/2);
			VL53L1_PRM_00007 /= samples;
			VL53L1_PRM_00007 *= ambient_threshold_sigma;
			VL53L1_PRM_00007 += 8;
			VL53L1_PRM_00007 /= 16;
			VL53L1_PRM_00007 += amb_events;

			if (VL53L1_PRM_00007 < min_ambient_threshold_events) {
				VL53L1_PRM_00007 = min_ambient_threshold_events;
			}

			palgo->VL53L1_PRM_00056[VL53L1_PRM_00032]             = VL53L1_PRM_00007;
			palgo->VL53L1_PRM_00031 = VL53L1_PRM_00007;
		}








	}






	palgo->VL53L1_PRM_00045 = 0;

	for (VL53L1_PRM_00032 = pbins->VL53L1_PRM_00019 ; VL53L1_PRM_00032 < pbins->VL53L1_PRM_00021 ; VL53L1_PRM_00032++) {

		if (pbins->bin_data[VL53L1_PRM_00032] > palgo->VL53L1_PRM_00056[VL53L1_PRM_00032]) {
			palgo->VL53L1_PRM_00043[VL53L1_PRM_00032]      = 1;
			palgo->VL53L1_PRM_00046[VL53L1_PRM_00032] = 1;
			palgo->VL53L1_PRM_00045++;
		} else {
			palgo->VL53L1_PRM_00043[VL53L1_PRM_00032]      = 0;
			palgo->VL53L1_PRM_00046[VL53L1_PRM_00032] = 0;
		}
	}

	LOG_FUNCTION_END(status);

	return status;

}





VL53L1_Error VL53L1_FCTN_00019(
	VL53L1_hist_gen3_algo_private_data_t  *palgo)
{







	VL53L1_Error  status  = VL53L1_ERROR_NONE;

	uint8_t  i            = 0;
	uint8_t  j            = 0;
	uint8_t  found        = 0;

	LOG_FUNCTION_START("");

	palgo->VL53L1_PRM_00049 = 0;

	for (i = 0 ; i < palgo->VL53L1_PRM_00030 ; i++) {

		j = (i + 1) % palgo->VL53L1_PRM_00030;






		if (i < palgo->VL53L1_PRM_00021 && j < palgo->VL53L1_PRM_00021) {
			if (palgo->VL53L1_PRM_00046[i] == 0 && palgo->VL53L1_PRM_00046[j] == 1 &&
				found == 0) {
				palgo->VL53L1_PRM_00049 = i;
				found = 1;
			}
		}
	}

	LOG_FUNCTION_END(status);

	return status;
}


VL53L1_Error VL53L1_FCTN_00020(
	VL53L1_hist_gen3_algo_private_data_t  *palgo)
{








	VL53L1_Error  status  = VL53L1_ERROR_NONE;
	uint8_t  i            = 0;
	uint8_t  j            = 0;
	uint8_t  VL53L1_PRM_00032            = 0;

	LOG_FUNCTION_START("");

	for (VL53L1_PRM_00032 = palgo->VL53L1_PRM_00049 ;
		 VL53L1_PRM_00032 < (palgo->VL53L1_PRM_00049 + palgo->VL53L1_PRM_00030) ;
		 VL53L1_PRM_00032++) {






		i =  VL53L1_PRM_00032      % palgo->VL53L1_PRM_00030;
		j = (VL53L1_PRM_00032 + 1) % palgo->VL53L1_PRM_00030;






		if (i < palgo->VL53L1_PRM_00021 && j < palgo->VL53L1_PRM_00021) {

			if (palgo->VL53L1_PRM_00046[i] == 0 && palgo->VL53L1_PRM_00046[j] == 1)
				palgo->VL53L1_PRM_00051++;

			if (palgo->VL53L1_PRM_00046[i] > 0)
				palgo->VL53L1_PRM_00047[i] = palgo->VL53L1_PRM_00051;
			else
				palgo->VL53L1_PRM_00047[i] = 0;
		}

	}



	if (palgo->VL53L1_PRM_00051 > palgo->VL53L1_PRM_00050) {
		palgo->VL53L1_PRM_00051 = palgo->VL53L1_PRM_00050;
	}

	LOG_FUNCTION_END(status);

	return status;

}


VL53L1_Error VL53L1_FCTN_00021(
	VL53L1_hist_gen3_algo_private_data_t  *palgo)
{






	VL53L1_Error  status  = VL53L1_ERROR_NONE;

	uint8_t  i            = 0;
	uint8_t  j            = 0;
	uint8_t  VL53L1_PRM_00032            = 0;
	uint8_t  pulse_no     = 0;

	uint8_t  max_filter_half_width = 0;

	VL53L1_hist_pulse_data_t *pdata;

	LOG_FUNCTION_START("");




	max_filter_half_width = palgo->VL53L1_PRM_00030 - 1;
	max_filter_half_width = max_filter_half_width >> 1;

	for (VL53L1_PRM_00032 = palgo->VL53L1_PRM_00049 ;
		 VL53L1_PRM_00032 < (palgo->VL53L1_PRM_00049 + palgo->VL53L1_PRM_00030) ;
		 VL53L1_PRM_00032++) {






		i =  VL53L1_PRM_00032      % palgo->VL53L1_PRM_00030;
		j = (VL53L1_PRM_00032 + 1) % palgo->VL53L1_PRM_00030;






		if (i < palgo->VL53L1_PRM_00021 && j < palgo->VL53L1_PRM_00021) {




			if (palgo->VL53L1_PRM_00047[i] == 0 && palgo->VL53L1_PRM_00047[j] > 0) {

				pulse_no = palgo->VL53L1_PRM_00047[j] - 1;
				pdata   = &(palgo->VL53L1_PRM_00005[pulse_no]);

				if (pulse_no < palgo->VL53L1_PRM_00050) {
					pdata->VL53L1_PRM_00014 = VL53L1_PRM_00032;
					pdata->VL53L1_PRM_00019    = VL53L1_PRM_00032 + 1;
					pdata->VL53L1_PRM_00023   = 0xFF;

					pdata->VL53L1_PRM_00024     = 0;
					pdata->VL53L1_PRM_00015   = 0;
				}
			}




			if (palgo->VL53L1_PRM_00047[i] > 0 && palgo->VL53L1_PRM_00047[j] == 0) {

				pulse_no = palgo->VL53L1_PRM_00047[i] - 1;
				pdata   = &(palgo->VL53L1_PRM_00005[pulse_no]);

				if (pulse_no < palgo->VL53L1_PRM_00050) {

					pdata->VL53L1_PRM_00024    = VL53L1_PRM_00032;
					pdata->VL53L1_PRM_00015  = VL53L1_PRM_00032 + 1;

					pdata->VL53L1_PRM_00025 =
						(pdata->VL53L1_PRM_00024 + 1) - pdata->VL53L1_PRM_00019;
					pdata->VL53L1_PRM_00055 =
						(pdata->VL53L1_PRM_00015 + 1) - pdata->VL53L1_PRM_00014;

					if (pdata->VL53L1_PRM_00055 > max_filter_half_width)
						pdata->VL53L1_PRM_00055 = max_filter_half_width;
				}

			}
		}
	}

	LOG_FUNCTION_END(status);

	return status;

}


VL53L1_Error VL53L1_FCTN_00028(
	VL53L1_HistTargetOrder                target_order,
	VL53L1_hist_gen3_algo_private_data_t  *palgo)
{






	VL53L1_Error  status  = VL53L1_ERROR_NONE;

	VL53L1_hist_pulse_data_t  tmp;
	VL53L1_hist_pulse_data_t *ptmp = &tmp;
	VL53L1_hist_pulse_data_t *p0;
	VL53L1_hist_pulse_data_t *p1;

	uint8_t i       = 0;
	uint8_t swapped = 1;

	LOG_FUNCTION_START("");

	if (palgo->VL53L1_PRM_00051 > 1) {

		while (swapped > 0) {

			swapped = 0;

			for (i = 1 ; i < palgo->VL53L1_PRM_00051; i++) {

				p0 = &(palgo->VL53L1_PRM_00005[i-1]);
				p1 = &(palgo->VL53L1_PRM_00005[i]);




				if (target_order == VL53L1_HIST_TARGET_ORDER__STRONGEST_FIRST) {

					if (p0->VL53L1_PRM_00012 < p1->VL53L1_PRM_00012) {




						memcpy(ptmp, p1,   sizeof(VL53L1_hist_pulse_data_t));
						memcpy(p1,   p0,   sizeof(VL53L1_hist_pulse_data_t));
						memcpy(p0,   ptmp, sizeof(VL53L1_hist_pulse_data_t));

						swapped = 1;
					}

				} else {

					if (p0->VL53L1_PRM_00013 > p1->VL53L1_PRM_00013) {




						memcpy(ptmp, p1,   sizeof(VL53L1_hist_pulse_data_t));
						memcpy(p1,   p0,   sizeof(VL53L1_hist_pulse_data_t));
						memcpy(p0,   ptmp, sizeof(VL53L1_hist_pulse_data_t));

						swapped = 1;
					}

				}
			}
		}
	}

	LOG_FUNCTION_END(status);

	return status;

}


VL53L1_Error VL53L1_FCTN_00022(
	uint8_t                                pulse_no,
	VL53L1_histogram_bin_data_t           *pbins,
	VL53L1_hist_gen3_algo_private_data_t  *palgo)
{








	VL53L1_Error  status  = VL53L1_ERROR_NONE;

	uint8_t  i            = 0;
	uint8_t  VL53L1_PRM_00032            = 0;

	VL53L1_hist_pulse_data_t *pdata = &(palgo->VL53L1_PRM_00005[pulse_no]);

	LOG_FUNCTION_START("");















	pdata->VL53L1_PRM_00018  = 0;
	pdata->VL53L1_PRM_00017 = 0;

	for (VL53L1_PRM_00032 = pdata->VL53L1_PRM_00014 ; VL53L1_PRM_00032 <= pdata->VL53L1_PRM_00015 ; VL53L1_PRM_00032++) {
		i =  VL53L1_PRM_00032 % palgo->VL53L1_PRM_00030;
		pdata->VL53L1_PRM_00018  += pbins->bin_data[i];
		pdata->VL53L1_PRM_00017 += palgo->VL53L1_PRM_00028;
	}






	pdata->VL53L1_PRM_00012 =
		pdata->VL53L1_PRM_00018 - pdata->VL53L1_PRM_00017;

	LOG_FUNCTION_END(status);

	return status;
}


VL53L1_Error VL53L1_FCTN_00027(
	uint8_t                                pulse_no,
	uint8_t                                clip_events,
	VL53L1_histogram_bin_data_t           *pbins,
	VL53L1_hist_gen3_algo_private_data_t  *palgo)
{







	VL53L1_Error  status  = VL53L1_ERROR_NONE;

	uint8_t   i            = 0;
	int16_t   VL53L1_PRM_00014 = 0;
	int16_t   VL53L1_PRM_00015   = 0;
	int16_t   window_width = 0;
	uint32_t  tmp_phase    = 0;

	VL53L1_hist_pulse_data_t *pdata = &(palgo->VL53L1_PRM_00005[pulse_no]);

	LOG_FUNCTION_START("");








	i = pdata->VL53L1_PRM_00023 % palgo->VL53L1_PRM_00030;

	VL53L1_PRM_00014  = (int16_t)i;
	VL53L1_PRM_00014 += (int16_t)pdata->VL53L1_PRM_00014;
	VL53L1_PRM_00014 -= (int16_t)pdata->VL53L1_PRM_00023;

	VL53L1_PRM_00015    = (int16_t)i;
	VL53L1_PRM_00015   += (int16_t)pdata->VL53L1_PRM_00015;
	VL53L1_PRM_00015   -= (int16_t)pdata->VL53L1_PRM_00023;




	window_width = VL53L1_PRM_00015 - VL53L1_PRM_00014;
	if (window_width > 3)
		window_width = 3;

	if (status == VL53L1_ERROR_NONE) {

		status =
			VL53L1_FCTN_00030(
				VL53L1_PRM_00014,
				VL53L1_PRM_00014 + window_width,
				palgo->VL53L1_PRM_00030,
				clip_events,
				pbins,
				&(pdata->VL53L1_PRM_00026));
	}

	if (status == VL53L1_ERROR_NONE) {
		status =
			VL53L1_FCTN_00030(
				VL53L1_PRM_00015 - window_width,
				VL53L1_PRM_00015,
				palgo->VL53L1_PRM_00030,
				clip_events,
				pbins,
				&(pdata->VL53L1_PRM_00027));
	}





	if (pdata->VL53L1_PRM_00026 > pdata->VL53L1_PRM_00027) {
		tmp_phase        = pdata->VL53L1_PRM_00026;
		pdata->VL53L1_PRM_00026 = pdata->VL53L1_PRM_00027;
		pdata->VL53L1_PRM_00027 = tmp_phase;
	}






	if (pdata->VL53L1_PRM_00013 < pdata->VL53L1_PRM_00026)
		pdata->VL53L1_PRM_00026 = pdata->VL53L1_PRM_00013;






	if (pdata->VL53L1_PRM_00013 > pdata->VL53L1_PRM_00027)
		pdata->VL53L1_PRM_00027 = pdata->VL53L1_PRM_00013;

	LOG_FUNCTION_END(status);

	return status;
}


VL53L1_Error VL53L1_FCTN_00030(
	int16_t                            VL53L1_PRM_00019,
	int16_t                            VL53L1_PRM_00024,
	uint8_t                            VL53L1_PRM_00030,
	uint8_t                            clip_events,
	VL53L1_histogram_bin_data_t       *pbins,
	uint32_t                          *pphase)
{






	VL53L1_Error  status  = VL53L1_ERROR_NONE;

	int16_t  i            = 0;
	int16_t  VL53L1_PRM_00032            = 0;

	int64_t VL53L1_PRM_00007        = 0;
	int64_t event_sum     = 0;
	int64_t weighted_sum  = 0;

	LOG_FUNCTION_START("");

	*pphase = VL53L1_MAX_ALLOWED_PHASE;

	for (VL53L1_PRM_00032 = VL53L1_PRM_00019 ; VL53L1_PRM_00032 <= VL53L1_PRM_00024 ; VL53L1_PRM_00032++) {



		if (VL53L1_PRM_00032 < 0)
			i =  VL53L1_PRM_00032 + (int16_t)VL53L1_PRM_00030;
		else
			i =  VL53L1_PRM_00032 % (int16_t)VL53L1_PRM_00030;

		VL53L1_PRM_00007 =
			(int64_t)pbins->bin_data[i] -
			(int64_t)pbins->VL53L1_PRM_00028;






		if (clip_events > 0 && VL53L1_PRM_00007 < 0)
			VL53L1_PRM_00007 = 0;

		event_sum += VL53L1_PRM_00007;

		weighted_sum +=
			(VL53L1_PRM_00007 * (1024 + (2048*(int64_t)VL53L1_PRM_00032)));

		trace_print(
			VL53L1_TRACE_LEVEL_INFO,
			"\tb = %5d : i = %5d : VL53L1_PRM_00007 = %8d, event_sum = %8d, weighted_sum = %8d\n",
			VL53L1_PRM_00032, i, VL53L1_PRM_00007, event_sum, weighted_sum);
	}

	if (event_sum  > 0) {

		weighted_sum += (event_sum/2);
		weighted_sum /= event_sum;

		if (weighted_sum < 0)
			weighted_sum = 0;

		*pphase = (uint32_t)weighted_sum;
	}

	LOG_FUNCTION_END(status);

	return status;
}


VL53L1_Error VL53L1_FCTN_00023(
	uint8_t                                pulse_no,
	VL53L1_histogram_bin_data_t           *pbins,
	VL53L1_hist_gen3_algo_private_data_t  *palgo,
	int32_t                                pad_value,
	VL53L1_histogram_bin_data_t           *ppulse)
{







	VL53L1_Error  status  = VL53L1_ERROR_NONE;

	uint8_t  i            = 0;
	uint8_t  VL53L1_PRM_00032            = 0;

	VL53L1_hist_pulse_data_t *pdata = &(palgo->VL53L1_PRM_00005[pulse_no]);

	LOG_FUNCTION_START("");




	memcpy(ppulse, pbins, sizeof(VL53L1_histogram_bin_data_t));






	for (VL53L1_PRM_00032 = palgo->VL53L1_PRM_00049 ;
		 VL53L1_PRM_00032 < (palgo->VL53L1_PRM_00049 + palgo->VL53L1_PRM_00030) ;
		 VL53L1_PRM_00032++) {

		if (VL53L1_PRM_00032 < pdata->VL53L1_PRM_00014 || VL53L1_PRM_00032 > pdata->VL53L1_PRM_00015) {
			i =  VL53L1_PRM_00032 % palgo->VL53L1_PRM_00030;
			if (i < ppulse->VL53L1_PRM_00021) {
				ppulse->bin_data[i] = pad_value;
			}
		}
	}

	LOG_FUNCTION_END(status);

	return status;
}


VL53L1_Error VL53L1_FCTN_00024(
	uint8_t                                pulse_no,
	VL53L1_histogram_bin_data_t           *ppulse,
	VL53L1_hist_gen3_algo_private_data_t  *palgo)
{







	VL53L1_Error  status       = VL53L1_ERROR_NONE;

	VL53L1_hist_pulse_data_t *pdata = &(palgo->VL53L1_PRM_00005[pulse_no]);

	uint8_t  VL53L1_PRM_00032            = 0;
	uint8_t  i            = 0;
	uint8_t  j            = 0;
	uint8_t  w            = 0;

	LOG_FUNCTION_START("");




	for (VL53L1_PRM_00032 = pdata->VL53L1_PRM_00014 ; VL53L1_PRM_00032 <= pdata->VL53L1_PRM_00015 ; VL53L1_PRM_00032++) {

		i =  VL53L1_PRM_00032  % palgo->VL53L1_PRM_00030;



		palgo->VL53L1_PRM_00048[i]  = 0;
		palgo->VL53L1_PRM_00007[i] = 0;

		for (w = 0 ; w < (pdata->VL53L1_PRM_00055 << 1) ; w++) {





			j = VL53L1_PRM_00032 + w + palgo->VL53L1_PRM_00030;
			j = j - pdata->VL53L1_PRM_00055;
			j = j % palgo->VL53L1_PRM_00030;






			if (i < ppulse->VL53L1_PRM_00021 && j < ppulse->VL53L1_PRM_00021) {
				if (w < pdata->VL53L1_PRM_00055) {
					palgo->VL53L1_PRM_00048[i] += ppulse->bin_data[j];
				} else {
					palgo->VL53L1_PRM_00048[i] -= ppulse->bin_data[j];
				}
			}
		}
	}

	return status;
}


VL53L1_Error VL53L1_FCTN_00025(
	uint8_t                                pulse_no,
	uint16_t                               noise_threshold,
	VL53L1_hist_gen3_algo_private_data_t  *palgo)
{







	VL53L1_Error  status       = VL53L1_ERROR_NONE;

	VL53L1_hist_pulse_data_t *pdata = &(palgo->VL53L1_PRM_00005[pulse_no]);

	uint8_t  VL53L1_PRM_00032            = 0;
	uint8_t  i            = 0;
	uint8_t  j            = 0;
	int32_t  bin_x_delta  = 0;

	for (VL53L1_PRM_00032 = pdata->VL53L1_PRM_00014 ; VL53L1_PRM_00032 < pdata->VL53L1_PRM_00015 ; VL53L1_PRM_00032++) {

		i =  VL53L1_PRM_00032    % palgo->VL53L1_PRM_00030;
		j = (VL53L1_PRM_00032+1) % palgo->VL53L1_PRM_00030;

		if (i < palgo->VL53L1_PRM_00021 && j < palgo->VL53L1_PRM_00021) {

			if (palgo->VL53L1_PRM_00048[i] <= 0 && palgo->VL53L1_PRM_00048[j] > 0) {





				bin_x_delta = palgo->VL53L1_PRM_00048[j] - palgo->VL53L1_PRM_00048[i];



				if (bin_x_delta > (int32_t)noise_threshold) {

					pdata->VL53L1_PRM_00023            = VL53L1_PRM_00032;

					VL53L1_FCTN_00031(
						VL53L1_PRM_00032,
						palgo->VL53L1_PRM_00048[i],
						palgo->VL53L1_PRM_00048[j],
						palgo->VL53L1_PRM_00030,
						&(pdata->VL53L1_PRM_00013));
				}
			}
		}
	}

	LOG_FUNCTION_END(status);

	return status;
}


VL53L1_Error VL53L1_FCTN_00031(
	uint8_t   bin,
	int32_t   filta0,
	int32_t   filta1,
	uint8_t   VL53L1_PRM_00030,
	uint32_t *pmean_phase)
{










	VL53L1_Error  status = VL53L1_ERROR_NONE;
	int32_t  mean_phase  = VL53L1_MAX_ALLOWED_PHASE;
	int32_t  bin_x_phase  = abs(filta0);
	int32_t  bin_x_delta  = filta1 - filta0;

	LOG_FUNCTION_START("");



	if (bin_x_delta == 0) {
		mean_phase = 1024;
	} else {
		mean_phase  = ((bin_x_phase  * 2048) + (bin_x_delta >> 1)) / bin_x_delta;
	}

	mean_phase += (2048 * (int64_t)bin);



	if (mean_phase  < 0) {
		mean_phase = 0;
	}

	if (mean_phase > VL53L1_MAX_ALLOWED_PHASE) {
		mean_phase = VL53L1_MAX_ALLOWED_PHASE;
	}



	mean_phase = mean_phase % ((int32_t)VL53L1_PRM_00030 * 2048);

	*pmean_phase = (uint32_t)mean_phase;

	LOG_FUNCTION_END(status);

	return status;
}


VL53L1_Error VL53L1_FCTN_00026(
	uint8_t                       bin,
	uint8_t                       sigma_estimator__sigma_ref_mm,
	uint8_t                       VL53L1_PRM_00030,
	uint8_t                       VL53L1_PRM_00055,
	uint8_t                       crosstalk_compensation_enable,
	VL53L1_histogram_bin_data_t  *phist_data_ap,
	VL53L1_histogram_bin_data_t  *phist_data_zp,
	VL53L1_histogram_bin_data_t  *pxtalk_hist,
	uint16_t                     *psigma_est)
{





	VL53L1_Error status      = VL53L1_ERROR_NONE;
	VL53L1_Error func_status = VL53L1_ERROR_NONE;

	uint8_t  i    = 0;
	int32_t  VL53L1_PRM_00002    = 0;
	int32_t  VL53L1_PRM_00032    = 0;
	int32_t  VL53L1_PRM_00001    = 0;
	int32_t  a_zp = 0;
	int32_t  c_zp = 0;
	int32_t  ax   = 0;
	int32_t  bx   = 0;
	int32_t  cx   = 0;




	i = bin % VL53L1_PRM_00030;




	VL53L1_FCTN_00013(
			i,
			VL53L1_PRM_00055,
			phist_data_zp,
			&a_zp,
			&VL53L1_PRM_00032,
			&c_zp);




	VL53L1_FCTN_00013(
			i,
			VL53L1_PRM_00055,
			phist_data_ap,
			&VL53L1_PRM_00002,
			&VL53L1_PRM_00032,
			&VL53L1_PRM_00001);

	if (crosstalk_compensation_enable > 0) {
		VL53L1_FCTN_00013(
				i,
				VL53L1_PRM_00055,
				pxtalk_hist,
				&ax,
				&bx,
				&cx);
	}








	func_status =
		VL53L1_FCTN_00014(
			sigma_estimator__sigma_ref_mm,
			(uint32_t)VL53L1_PRM_00002,

			(uint32_t)VL53L1_PRM_00032,

			(uint32_t)VL53L1_PRM_00001,

			(uint32_t)a_zp,

			(uint32_t)c_zp,

			(uint32_t)bx,
			(uint32_t)ax,
			(uint32_t)cx,
			(uint32_t)phist_data_ap->VL53L1_PRM_00028,
			phist_data_ap->VL53L1_PRM_00022,
			psigma_est);






	if (func_status == VL53L1_ERROR_DIVISION_BY_ZERO) {
		*psigma_est = 0xFFFF;
	}

	return status;
}


void VL53L1_FCTN_00029(
	uint8_t                      range_id,
	uint8_t                      valid_phase_low,
	uint8_t                      valid_phase_high,
	uint16_t                     sigma_thres,
	VL53L1_histogram_bin_data_t *pbins,
	VL53L1_hist_pulse_data_t    *ppulse,
	VL53L1_range_data_t         *pdata)
{

	uint16_t  lower_phase_limit = 0;
	uint16_t  upper_phase_limit = 0;




	pdata->range_id              = range_id;
	pdata->time_stamp            = 0;

	pdata->VL53L1_PRM_00014          = ppulse->VL53L1_PRM_00014;
	pdata->VL53L1_PRM_00019             = ppulse->VL53L1_PRM_00019;
	pdata->VL53L1_PRM_00023            = ppulse->VL53L1_PRM_00023;
	pdata->VL53L1_PRM_00024              = ppulse->VL53L1_PRM_00024;
	pdata->VL53L1_PRM_00015            = ppulse->VL53L1_PRM_00015;
	pdata->VL53L1_PRM_00025             = ppulse->VL53L1_PRM_00025;




	pdata->VL53L1_PRM_00029  =
		(ppulse->VL53L1_PRM_00015 + 1) - ppulse->VL53L1_PRM_00014;




	pdata->zero_distance_phase   = pbins->zero_distance_phase;
	pdata->VL53L1_PRM_00003              = ppulse->VL53L1_PRM_00003;
	pdata->VL53L1_PRM_00026             = (uint16_t)ppulse->VL53L1_PRM_00026;
	pdata->VL53L1_PRM_00013          = (uint16_t)ppulse->VL53L1_PRM_00013;
	pdata->VL53L1_PRM_00027             = (uint16_t)ppulse->VL53L1_PRM_00027;
	pdata->VL53L1_PRM_00018  = (uint32_t)ppulse->VL53L1_PRM_00018;
	pdata->VL53L1_PRM_00012   = ppulse->VL53L1_PRM_00012;
	pdata->VL53L1_PRM_00017 = (uint32_t)ppulse->VL53L1_PRM_00017;
	pdata->total_periods_elapsed = pbins->total_periods_elapsed;




	pdata->range_status = VL53L1_DEVICEERROR_RANGECOMPLETE_NO_WRAP_CHECK;



	if (sigma_thres > 0 &&
		(uint32_t)ppulse->VL53L1_PRM_00003 > ((uint32_t)sigma_thres<<5)) {
		pdata->range_status = VL53L1_DEVICEERROR_SIGMATHRESHOLDCHECK;
	}




	lower_phase_limit  = (uint8_t)valid_phase_low << 8;
	if (lower_phase_limit < pdata->zero_distance_phase)
		lower_phase_limit =
			pdata->zero_distance_phase -
			lower_phase_limit;
	else
		lower_phase_limit  = 0;

	upper_phase_limit  = (uint8_t)valid_phase_high << 8;
	upper_phase_limit += pbins->zero_distance_phase;

	if (pdata->VL53L1_PRM_00013 < lower_phase_limit ||
		pdata->VL53L1_PRM_00013 > upper_phase_limit) {
		pdata->range_status = VL53L1_DEVICEERROR_RANGEPHASECHECK;
	}
}


