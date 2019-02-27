
/*******************************************************************************
 This file is part of VL53L1 Protected

 Copyright (c) 2017, STMicroelectronics - All Rights Reserved

 License terms: STMicroelectronics Proprietary in accordance with licensing
 terms at www.st.com/sla0081

 STMicroelectronics confidential
 Reproduction and Communication of this document is strictly prohibited unless
 specifically authorized in writing by STMicroelectronics.

*/






































#include "vl53l1_types.h"
#include "vl53l1_platform_log.h"

#include "vl53l1_core_support.h"
#include "vl53l1_error_codes.h"

#include "vl53l1_hist_core.h"
#include "vl53l1_hist_algos_gen2.h"
#include "vl53l1_sigma_estimate.h"


#define LOG_FUNCTION_START(fmt, ...) \
	_LOG_FUNCTION_START(VL53L1_TRACE_MODULE_HISTOGRAM, fmt, ##__VA_ARGS__)
#define LOG_FUNCTION_END(status, ...) \
	_LOG_FUNCTION_END(VL53L1_TRACE_MODULE_HISTOGRAM, status, ##__VA_ARGS__)
#define LOG_FUNCTION_END_FMT(status, fmt, ...) \
	_LOG_FUNCTION_END_FMT(VL53L1_TRACE_MODULE_HISTOGRAM, \
	status, fmt, ##__VA_ARGS__)

#define trace_print(level, ...) \
	_LOG_TRACE_PRINT(VL53L1_TRACE_MODULE_HISTOGRAM, \
	level, VL53L1_TRACE_FUNCTION_NONE, ##__VA_ARGS__)


VL53L1_Error VL53L1_f_003(
	VL53L1_hist_post_process_config_t      *ppost_cfg,
	VL53L1_histogram_bin_data_t            *pbins_input,
	VL53L1_histogram_bin_data_t            *pxtalk,
	VL53L1_hist_gen2_algo_filtered_data_t  *pfiltered0,
	VL53L1_hist_gen2_algo_filtered_data_t  *pfiltered1,
	VL53L1_hist_gen2_algo_filtered_data_t  *pfilteredx,
	VL53L1_hist_gen2_algo_detection_data_t *pdetection,
	VL53L1_range_results_t                 *presults)
{






	VL53L1_Error  status  = VL53L1_ERROR_NONE;
	uint8_t             i = 0;

	VL53L1_histogram_bin_data_t   VL53L1_p_010;
	VL53L1_histogram_bin_data_t  *pbins = &VL53L1_p_010;

	VL53L1_histogram_bin_data_t   VL53L1_p_038;
	VL53L1_histogram_bin_data_t  *pxtalk_realigned = &VL53L1_p_038;

	VL53L1_range_data_t          *pdata;

	LOG_FUNCTION_START("");






	memcpy(
		pbins,
		pbins_input,
		sizeof(VL53L1_histogram_bin_data_t));




	VL53L1_hist_calc_zero_distance_phase(pbins);






	if (ppost_cfg->hist_amb_est_method ==
		VL53L1_HIST_AMB_EST_METHOD__THRESHOLDED_BINS)
		VL53L1_hist_estimate_ambient_from_thresholded_bins(
				(int32_t)ppost_cfg->ambient_thresh_sigma0,
				pbins);
	else
		VL53L1_hist_estimate_ambient_from_ambient_bins(pbins);





	VL53L1_hist_remove_ambient_bins(pbins);






	if (ppost_cfg->algo__crosstalk_compensation_enable > 0)
		VL53L1_f_004(
			pxtalk,
			pbins,
			pxtalk_realigned);






	VL53L1_f_005(
				ppost_cfg->filter_woi0,
				pbins,
				pfiltered0);






	VL53L1_f_006(
			ppost_cfg->ambient_thresh_sigma1,
			pfiltered0,
			pdetection);






	VL53L1_f_005(
			ppost_cfg->filter_woi1,
			pbins,
			pfiltered1);







	VL53L1_f_005(
				ppost_cfg->filter_woi1,
				pxtalk_realigned,
				pfilteredx);






	VL53L1_f_007(
				pfiltered1,
				pdetection);







	VL53L1_f_008(
			ppost_cfg->sigma_estimator__sigma_ref_mm,
			pfiltered1,
			pfilteredx,
			pdetection);






	VL53L1_f_009(
				pfiltered1,
				pfilteredx,
				pdetection);






	VL53L1_f_010(
				ppost_cfg->signal_total_events_limit,
				ppost_cfg->sigma_thresh,
				pbins,
				pfiltered1,
				pdetection,
				presults);






	pdata = &presults->VL53L1_p_002[0];
	for (i = 0; i < presults->active_results; i++) {

		if (status == VL53L1_ERROR_NONE)
			status = VL53L1_f_011(
				pbins->vcsel_width,
				pbins->VL53L1_p_019,
				pbins->total_periods_elapsed,
				pbins->result__dss_actual_effective_spads,
				pdata);

		if (status == VL53L1_ERROR_NONE)
			VL53L1_f_012(
					ppost_cfg->gain_factor,
					ppost_cfg->range_offset_mm,
					pdata);

		pdata++;
	}

	LOG_FUNCTION_END(status);

	return status;
}


void VL53L1_f_005(
	uint8_t                                filter_woi,
	VL53L1_histogram_bin_data_t           *pbins,
	VL53L1_hist_gen2_algo_filtered_data_t *pfiltered)
{






	uint8_t  lb    = 0;
	int32_t  suma  = 0;
	int32_t  sumb  = 0;
	int32_t  sumc  = 0;




	pfiltered->VL53L1_p_022              = pbins->VL53L1_p_022;
	pfiltered->VL53L1_p_023            = pbins->VL53L1_p_023;
	pfiltered->VL53L1_p_024         = pbins->VL53L1_p_024;
	pfiltered->VL53L1_p_019    = pbins->VL53L1_p_019;
	pfiltered->VL53L1_p_009           = pbins->VL53L1_p_009;
	pfiltered->VL53L1_p_004 = pbins->VL53L1_p_004;



	pfiltered->VL53L1_p_030                    = (2 * filter_woi)+1;
	pfiltered->VL53L1_p_020  =
		pbins->VL53L1_p_004 * (int32_t)pfiltered->VL53L1_p_030;




	for (lb = pbins->VL53L1_p_022; lb < pbins->VL53L1_p_024; lb++) {



		VL53L1_f_013(lb, filter_woi,
				pbins, &suma, &sumb, &sumc);



		pfiltered->VL53L1_p_003[lb] = suma;
		pfiltered->VL53L1_p_018[lb] = sumb;
		pfiltered->VL53L1_p_001[lb] = sumc;




		pfiltered->VL53L1_p_008[lb] = suma + sumb + sumc;




		if (pfiltered->VL53L1_p_003[lb] > pfiltered->VL53L1_p_001[lb])
			pfiltered->VL53L1_p_041[lb] =
				pfiltered->VL53L1_p_003[lb] -
				pfiltered->VL53L1_p_001[lb];
		else
			pfiltered->VL53L1_p_041[lb] =
				pfiltered->VL53L1_p_001[lb] -
				pfiltered->VL53L1_p_003[lb];




		pfiltered->VL53L1_p_039[lb]  = (suma + sumb) -
			(sumc + pbins->VL53L1_p_004);




		pfiltered->VL53L1_p_040[lb]  = (sumb + sumc) -
			(suma + pbins->VL53L1_p_004);

	}
}


void VL53L1_f_006(
	int32_t                                 ambient_threshold_sigma,
	VL53L1_hist_gen2_algo_filtered_data_t  *pfiltered,
	VL53L1_hist_gen2_algo_detection_data_t *pdetection)
{







	uint8_t  lb                 = 0;



	pdetection->VL53L1_p_023     = pfiltered->VL53L1_p_023;
	pdetection->VL53L1_p_022       = pfiltered->VL53L1_p_022;
	pdetection->VL53L1_p_024  = pfiltered->VL53L1_p_024;






	pdetection->VL53L1_p_032  =
		VL53L1_isqrt(pfiltered->VL53L1_p_020);
	pdetection->VL53L1_p_032  =
		((ambient_threshold_sigma *
		pdetection->VL53L1_p_032) + 0x07) >> 4;
	pdetection->VL53L1_p_032 +=
			pfiltered->VL53L1_p_020;





	for (lb = pfiltered->VL53L1_p_022;
			lb < pfiltered->VL53L1_p_024; lb++) {
		if (pfiltered->VL53L1_p_008[lb] >
			pdetection->VL53L1_p_032)
			pdetection->VL53L1_p_042[lb] = 1;
		else
			pdetection->VL53L1_p_042[lb] = 0;

		pdetection->VL53L1_p_043[lb] =
			pdetection->VL53L1_p_042[lb] &
			pdetection->VL53L1_p_044[lb];
	}
}


void VL53L1_f_007(
	VL53L1_hist_gen2_algo_filtered_data_t  *pfiltered,
	VL53L1_hist_gen2_algo_detection_data_t *pdetection)
{





	uint8_t  lb                 = 0;
	uint8_t  n                 = 0;

	pdetection->VL53L1_p_023     = pfiltered->VL53L1_p_023;
	pdetection->VL53L1_p_022       = pfiltered->VL53L1_p_022;
	pdetection->VL53L1_p_024  = pfiltered->VL53L1_p_024;





	for (lb = pfiltered->VL53L1_p_022;
		lb < pfiltered->VL53L1_p_024; lb++) {

		n = (lb+1) % pfiltered->VL53L1_p_024;


		if (pfiltered->VL53L1_p_039[lb] == 0 &&
			pfiltered->VL53L1_p_040[lb] == 0)


			pdetection->VL53L1_p_044[lb] = 0;
		else
			if (pfiltered->VL53L1_p_039[lb] >= 0 &&
				pfiltered->VL53L1_p_040[lb] >= 0)
				pdetection->VL53L1_p_044[lb] = 1;

		else
			if (pfiltered->VL53L1_p_039[lb] <  0 &&
				pfiltered->VL53L1_p_040[lb] >= 0 &&
				pfiltered->VL53L1_p_039[n] >= 0 &&
				pfiltered->VL53L1_p_040[n] <  0)
				pdetection->VL53L1_p_044[lb] = 1;

		else
			pdetection->VL53L1_p_044[lb] = 0;

		pdetection->VL53L1_p_043[lb] = pdetection->VL53L1_p_042[lb] &
				pdetection->VL53L1_p_044[lb];
	}





	for (lb = pfiltered->VL53L1_p_022;
			lb < pfiltered->VL53L1_p_024; lb++) {

		n = (lb+1) % pfiltered->VL53L1_p_024;

		if (pdetection->VL53L1_p_044[lb] > 0 &&
			pdetection->VL53L1_p_044[n] > 0)
			pdetection->VL53L1_p_044[n] = 0;

		pdetection->VL53L1_p_043[lb] = pdetection->VL53L1_p_042[lb] &
				pdetection->VL53L1_p_044[lb];
	}

}


void VL53L1_f_008(
	uint8_t                                 sigma_estimator__sigma_ref_mm,
	VL53L1_hist_gen2_algo_filtered_data_t  *pfiltered,
	VL53L1_hist_gen2_algo_filtered_data_t  *pfilteredx,
	VL53L1_hist_gen2_algo_detection_data_t *pdetection)
{





	VL53L1_Error status = VL53L1_ERROR_NONE;

	uint8_t  lb                  = 0;

	pdetection->VL53L1_p_023     = pfiltered->VL53L1_p_023;
	pdetection->VL53L1_p_022       = pfiltered->VL53L1_p_022;
	pdetection->VL53L1_p_024  = pfiltered->VL53L1_p_024;

	for (lb = pfiltered->VL53L1_p_022;
			lb < pfiltered->VL53L1_p_024; lb++) {

		if (pdetection->VL53L1_p_043[lb] > 0) {




			status =
				VL53L1_f_014(
				sigma_estimator__sigma_ref_mm,
				(uint32_t)pfiltered->VL53L1_p_003[lb],
				(uint32_t)pfiltered->VL53L1_p_018[lb],
				(uint32_t)pfiltered->VL53L1_p_001[lb],
				(uint32_t)pfiltered->VL53L1_p_003[lb],
				(uint32_t)pfiltered->VL53L1_p_001[lb],
				(uint32_t)pfilteredx->VL53L1_p_018[lb],
				(uint32_t)pfilteredx->VL53L1_p_003[lb],
				(uint32_t)pfilteredx->VL53L1_p_001[lb],
				(uint32_t)pfiltered->VL53L1_p_004,
				pfiltered->VL53L1_p_019,
				&(pdetection->VL53L1_p_011[lb]));






			if (status == VL53L1_ERROR_DIVISION_BY_ZERO) {
				pdetection->VL53L1_p_043[lb] = 0;
				pdetection->VL53L1_p_011[lb] = 0xFFFF;
			}

		} else {
			pdetection->VL53L1_p_011[lb] = 0xFFFF;
		}
	}
}


VL53L1_Error VL53L1_f_015(
	uint8_t   bin,
	int32_t   VL53L1_p_003,
	int32_t   VL53L1_p_018,
	int32_t   VL53L1_p_001,
	int32_t   ax,
	int32_t   bx,
	int32_t   cx,
	int32_t   VL53L1_p_004,
	uint8_t   VL53L1_p_031,
	uint32_t *pmean_phase)
{





	VL53L1_Error  status = VL53L1_ERROR_DIVISION_BY_ZERO;
	int64_t  mean_phase  = VL53L1_MAX_ALLOWED_PHASE;
	int64_t  VL53L1_p_041   = 0;
	int64_t  b_minus_amb = 0;








	VL53L1_p_041    =     4096 * ((int64_t)VL53L1_p_001 -
			(int64_t)cx - (int64_t)VL53L1_p_003 -  (int64_t)ax);
	b_minus_amb  = 2 * 4096 * ((int64_t)VL53L1_p_018 -
			(int64_t)bx - (int64_t)VL53L1_p_004);

	if (b_minus_amb != 0) {

		mean_phase   =  ((4096 * VL53L1_p_041) +
				(b_minus_amb/2)) / b_minus_amb;
		mean_phase  +=  2048;
		mean_phase  += (4096 * (int64_t)bin);



		mean_phase  = (mean_phase + 1)/2;



		if (mean_phase  < 0)
			mean_phase = 0;
		if (mean_phase > VL53L1_MAX_ALLOWED_PHASE)
			mean_phase = VL53L1_MAX_ALLOWED_PHASE;



		mean_phase = mean_phase % ((int64_t)VL53L1_p_031 * 2048);

		status = VL53L1_ERROR_NONE;

	}

	*pmean_phase = (uint32_t)mean_phase;

	return status;

}


void VL53L1_f_009(
	VL53L1_hist_gen2_algo_filtered_data_t  *pfiltered,
	VL53L1_hist_gen2_algo_filtered_data_t  *pfilteredx,
	VL53L1_hist_gen2_algo_detection_data_t *pdetection)
{





	VL53L1_Error status = VL53L1_ERROR_NONE;

	uint8_t  l                  = 0;
	uint8_t  VL53L1_p_031 = 0;

	SUPPRESS_UNUSED_WARNING(pfilteredx);

	pdetection->VL53L1_p_023     = pfiltered->VL53L1_p_023;
	pdetection->VL53L1_p_022       = pfiltered->VL53L1_p_022;
	pdetection->VL53L1_p_024  = pfiltered->VL53L1_p_024;




	VL53L1_p_031 =
		VL53L1_decode_vcsel_period(pfiltered->VL53L1_p_009);




	for (l = pfiltered->VL53L1_p_022; l < pfiltered->VL53L1_p_024; l++) {

		if (pdetection->VL53L1_p_043[l] > 0) {

			status =
				VL53L1_f_015(
					l,
					pfiltered->VL53L1_p_003[l],
					pfiltered->VL53L1_p_018[l],
					pfiltered->VL53L1_p_001[l],
					0,

					0,

					0,

					pfiltered->VL53L1_p_004,
					VL53L1_p_031,
					&(pdetection->VL53L1_p_017[l]));



			if (status == VL53L1_ERROR_DIVISION_BY_ZERO) {
				pdetection->VL53L1_p_043[l] = 0;
				pdetection->VL53L1_p_017[l] = 0;
			}

		} else {
			pdetection->VL53L1_p_017[l] = 0;
		}
	}
}


void VL53L1_f_010(
	int32_t                                 signal_total_events_limit,
	uint16_t                                sigma_thresh,
	VL53L1_histogram_bin_data_t            *pbins,
	VL53L1_hist_gen2_algo_filtered_data_t  *pfiltered,
	VL53L1_hist_gen2_algo_detection_data_t *pdetection,
	VL53L1_range_results_t                 *presults)
{






	uint8_t  lb                     = 0;
	int32_t  VL53L1_p_021  = 0;
	int32_t  VL53L1_p_013   = 0;
	VL53L1_hist_gen2_algo_filtered_data_t  *pF = pfiltered;
	VL53L1_range_data_t *pdata;
	VL53L1_range_results_t            *pR = presults;

	LOG_FUNCTION_START("");

	presults->cfg_device_state = pbins->cfg_device_state;
	presults->rd_device_state  = pbins->rd_device_state;
	presults->zone_id          = pbins->zone_id;
	presults->stream_count     = pbins->result__stream_count;
	presults->max_results      = VL53L1_MAX_RANGE_RESULTS;
	presults->active_results   = 0;

	for (lb = pdetection->VL53L1_p_022;
			lb < pdetection->VL53L1_p_024; lb++) {



		if (pdetection->VL53L1_p_043[lb] > 0) {



			VL53L1_p_021  = pF->VL53L1_p_008[lb];
			VL53L1_p_013   = VL53L1_p_021 -
					pF->VL53L1_p_020;



			if (VL53L1_p_013  > signal_total_events_limit &&
				pdetection->VL53L1_p_011[lb] < sigma_thresh) {

				pdata =
				&pR->VL53L1_p_002[presults->active_results];



				if (presults->active_results <
						presults->max_results) {



					pdata->range_id              =
					presults->active_results;
					pdata->time_stamp            = 0;
					pdata->VL53L1_p_030                   =
						pF->VL53L1_p_030;
					pdata->zero_distance_phase   =
						pbins->zero_distance_phase;
					pdata->VL53L1_p_005              =
						pdetection->VL53L1_p_011[lb];
					pdata->VL53L1_p_014          =
					(uint16_t)pdetection->VL53L1_p_017[lb];
					pdata->VL53L1_p_021  =
					(uint32_t)VL53L1_p_021;
					pdata->VL53L1_p_013   =
						VL53L1_p_013;
					pdata->VL53L1_p_020 =
					(uint32_t)pF->VL53L1_p_020;
					pdata->total_periods_elapsed =
					pbins->total_periods_elapsed;
					pdata->range_status          =
					VL53L1_DEVICEERROR_RANGECOMPLETE;



					presults->active_results++;

				}
			}
		}
	}

	LOG_FUNCTION_END(0);

}

