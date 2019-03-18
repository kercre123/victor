
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
#include "vl53l1_hist_algos_gen2.h"
#include "vl53l1_sigma_estimate.h"


#define LOG_FUNCTION_START(fmt, ...) \
	_LOG_FUNCTION_START(VL53L1_TRACE_MODULE_HISTOGRAM, fmt, ##__VA_ARGS__)
#define LOG_FUNCTION_END(status, ...) \
	_LOG_FUNCTION_END(VL53L1_TRACE_MODULE_HISTOGRAM, status, ##__VA_ARGS__)
#define LOG_FUNCTION_END_FMT(status, fmt, ...) \
	_LOG_FUNCTION_END_FMT(VL53L1_TRACE_MODULE_HISTOGRAM, status, fmt, ##__VA_ARGS__)

#define trace_print(level, ...) \
	_LOG_TRACE_PRINT(VL53L1_TRACE_MODULE_HISTOGRAM, \
	level, VL53L1_TRACE_FUNCTION_NONE, ##__VA_ARGS__)


VL53L1_Error VL53L1_FCTN_00003(
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

	VL53L1_histogram_bin_data_t   VL53L1_PRM_00009;
	VL53L1_histogram_bin_data_t  *pbins = &VL53L1_PRM_00009;

	VL53L1_histogram_bin_data_t   VL53L1_PRM_00038;
	VL53L1_histogram_bin_data_t  *pxtalk_realigned = &VL53L1_PRM_00038;

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
		VL53L1_FCTN_00004(
			pxtalk,
			pbins,
			pxtalk_realigned);






	VL53L1_FCTN_00005(
						ppost_cfg->filter_woi0,
						pbins,
						pfiltered0);






	VL53L1_FCTN_00006(
			ppost_cfg->ambient_thresh_sigma1,
			pfiltered0,
			pdetection);






	VL53L1_FCTN_00005(
			ppost_cfg->filter_woi1,
			pbins,
			pfiltered1);







	VL53L1_FCTN_00005(
				ppost_cfg->filter_woi1,
				pxtalk_realigned,
				pfilteredx);






	VL53L1_FCTN_00007(
				pfiltered1,
				pdetection);







	VL53L1_FCTN_00008(
				ppost_cfg->sigma_estimator__sigma_ref_mm,
				pfiltered1,
				pfilteredx,
				pdetection);






	VL53L1_FCTN_00009(
				pfiltered1,
				pfilteredx,
				pdetection);






	VL53L1_FCTN_00010(
				ppost_cfg->signal_total_events_limit,
				ppost_cfg->sigma_thresh,
				pbins,
				pfiltered1,
				pdetection,
				presults);






	pdata = &presults->VL53L1_PRM_00005[0];
	for (i = 0 ; i < presults->active_results ; i++) {

		if (status == VL53L1_ERROR_NONE)
			 status = VL53L1_FCTN_00011(
							pbins->vcsel_width,
							pbins->VL53L1_PRM_00022,
							pbins->total_periods_elapsed,
							pbins->result__dss_actual_effective_spads,
							pdata);

		if (status == VL53L1_ERROR_NONE)
			 VL53L1_FCTN_00012(
					ppost_cfg->gain_factor,
					ppost_cfg->range_offset_mm,
					pdata);

		pdata++;
	}

	LOG_FUNCTION_END(status);

	return status;
}


void VL53L1_FCTN_00005(
	uint8_t                                filter_woi,
	VL53L1_histogram_bin_data_t           *pbins,
	VL53L1_hist_gen2_algo_filtered_data_t *pfiltered)
{






	uint8_t  VL53L1_PRM_00032     = 0;
	int32_t  suma  = 0;
	int32_t  sumb  = 0;
	int32_t  sumc  = 0;




	pfiltered->VL53L1_PRM_00019              = pbins->VL53L1_PRM_00019;
	pfiltered->VL53L1_PRM_00020            = pbins->VL53L1_PRM_00020;
	pfiltered->VL53L1_PRM_00021         = pbins->VL53L1_PRM_00021;
	pfiltered->VL53L1_PRM_00022    = pbins->VL53L1_PRM_00022;
	pfiltered->VL53L1_PRM_00008           = pbins->VL53L1_PRM_00008;
	pfiltered->VL53L1_PRM_00028 = pbins->VL53L1_PRM_00028;



	pfiltered->VL53L1_PRM_00029                    = (2 * filter_woi)+1;
	pfiltered->VL53L1_PRM_00017  = pbins->VL53L1_PRM_00028 * (int32_t)pfiltered->VL53L1_PRM_00029;




	for (VL53L1_PRM_00032 = pbins->VL53L1_PRM_00019 ; VL53L1_PRM_00032 < pbins->VL53L1_PRM_00021 ; VL53L1_PRM_00032++) {



		VL53L1_FCTN_00013(VL53L1_PRM_00032, filter_woi, pbins, &suma, &sumb, &sumc);



		pfiltered->VL53L1_PRM_00002[VL53L1_PRM_00032] = suma;
		pfiltered->VL53L1_PRM_00032[VL53L1_PRM_00032] = sumb;
		pfiltered->VL53L1_PRM_00001[VL53L1_PRM_00032] = sumc;




		pfiltered->VL53L1_PRM_00007[VL53L1_PRM_00032] = suma + sumb + sumc;




		if (pfiltered->VL53L1_PRM_00002[VL53L1_PRM_00032] > pfiltered->VL53L1_PRM_00001[VL53L1_PRM_00032])
			pfiltered->VL53L1_PRM_00041[VL53L1_PRM_00032] = pfiltered->VL53L1_PRM_00002[VL53L1_PRM_00032] - pfiltered->VL53L1_PRM_00001[VL53L1_PRM_00032];
		else
			pfiltered->VL53L1_PRM_00041[VL53L1_PRM_00032] = pfiltered->VL53L1_PRM_00001[VL53L1_PRM_00032] - pfiltered->VL53L1_PRM_00002[VL53L1_PRM_00032];




		pfiltered->VL53L1_PRM_00039[VL53L1_PRM_00032]  = (suma + sumb) -
							(sumc + pbins->VL53L1_PRM_00028);




		pfiltered->VL53L1_PRM_00040[VL53L1_PRM_00032]  = (sumb + sumc) -
							(suma + pbins->VL53L1_PRM_00028);

	}
}


void VL53L1_FCTN_00006(
	int32_t                                 ambient_threshold_sigma,
	VL53L1_hist_gen2_algo_filtered_data_t  *pfiltered,
	VL53L1_hist_gen2_algo_detection_data_t *pdetection)
{







	uint8_t  VL53L1_PRM_00032                 = 0;



	pdetection->VL53L1_PRM_00020     = pfiltered->VL53L1_PRM_00020;
	pdetection->VL53L1_PRM_00019       = pfiltered->VL53L1_PRM_00019;
	pdetection->VL53L1_PRM_00021  = pfiltered->VL53L1_PRM_00021;






	pdetection->VL53L1_PRM_00031  = VL53L1_isqrt(pfiltered->VL53L1_PRM_00017);
	pdetection->VL53L1_PRM_00031  = ((ambient_threshold_sigma * pdetection->VL53L1_PRM_00031) + 0x07) >> 4;
	pdetection->VL53L1_PRM_00031 += pfiltered->VL53L1_PRM_00017;





	for (VL53L1_PRM_00032 = pfiltered->VL53L1_PRM_00019 ; VL53L1_PRM_00032 < pfiltered->VL53L1_PRM_00021 ; VL53L1_PRM_00032++) {
		if (pfiltered->VL53L1_PRM_00007[VL53L1_PRM_00032] > pdetection->VL53L1_PRM_00031)
			pdetection->VL53L1_PRM_00042[VL53L1_PRM_00032] = 1;
		else
			pdetection->VL53L1_PRM_00042[VL53L1_PRM_00032] = 0;

		pdetection->VL53L1_PRM_00043[VL53L1_PRM_00032] = pdetection->VL53L1_PRM_00042[VL53L1_PRM_00032] & pdetection->VL53L1_PRM_00044[VL53L1_PRM_00032];
	}
}


void VL53L1_FCTN_00007(
	VL53L1_hist_gen2_algo_filtered_data_t  *pfiltered,
	VL53L1_hist_gen2_algo_detection_data_t *pdetection)
{





	uint8_t  VL53L1_PRM_00032                 = 0;
	uint8_t  n                 = 0;

	pdetection->VL53L1_PRM_00020     = pfiltered->VL53L1_PRM_00020;
	pdetection->VL53L1_PRM_00019       = pfiltered->VL53L1_PRM_00019;
	pdetection->VL53L1_PRM_00021  = pfiltered->VL53L1_PRM_00021;





	for (VL53L1_PRM_00032 = pfiltered->VL53L1_PRM_00019 ; VL53L1_PRM_00032 < pfiltered->VL53L1_PRM_00021 ; VL53L1_PRM_00032++) {

		n = (VL53L1_PRM_00032+1) % pfiltered->VL53L1_PRM_00021;


		if (pfiltered->VL53L1_PRM_00039[VL53L1_PRM_00032] == 0 &&
			pfiltered->VL53L1_PRM_00040[VL53L1_PRM_00032] == 0)


			pdetection->VL53L1_PRM_00044[VL53L1_PRM_00032] = 0;
		else if (pfiltered->VL53L1_PRM_00039[VL53L1_PRM_00032] >= 0 &&
				 pfiltered->VL53L1_PRM_00040[VL53L1_PRM_00032] >= 0)
				 pdetection->VL53L1_PRM_00044[VL53L1_PRM_00032] = 1;

		else if (pfiltered->VL53L1_PRM_00039[VL53L1_PRM_00032] <  0 &&
				 pfiltered->VL53L1_PRM_00040[VL53L1_PRM_00032] >= 0 &&
				 pfiltered->VL53L1_PRM_00039[n] >= 0 &&
				 pfiltered->VL53L1_PRM_00040[n] <  0)
			pdetection->VL53L1_PRM_00044[VL53L1_PRM_00032] = 1;

		else
			pdetection->VL53L1_PRM_00044[VL53L1_PRM_00032] = 0;

		pdetection->VL53L1_PRM_00043[VL53L1_PRM_00032] = pdetection->VL53L1_PRM_00042[VL53L1_PRM_00032] & pdetection->VL53L1_PRM_00044[VL53L1_PRM_00032];
	}





	for (VL53L1_PRM_00032 = pfiltered->VL53L1_PRM_00019 ; VL53L1_PRM_00032 < pfiltered->VL53L1_PRM_00021 ; VL53L1_PRM_00032++) {

		n = (VL53L1_PRM_00032+1) % pfiltered->VL53L1_PRM_00021;

		if (pdetection->VL53L1_PRM_00044[VL53L1_PRM_00032] > 0 &&
			pdetection->VL53L1_PRM_00044[n] > 0)
			pdetection->VL53L1_PRM_00044[n] = 0;

		pdetection->VL53L1_PRM_00043[VL53L1_PRM_00032] = pdetection->VL53L1_PRM_00042[VL53L1_PRM_00032] & pdetection->VL53L1_PRM_00044[VL53L1_PRM_00032];
	}

}


void VL53L1_FCTN_00008(
	uint8_t                                 sigma_estimator__sigma_ref_mm,
	VL53L1_hist_gen2_algo_filtered_data_t  *pfiltered,
	VL53L1_hist_gen2_algo_filtered_data_t  *pfilteredx,
	VL53L1_hist_gen2_algo_detection_data_t *pdetection)
{





	VL53L1_Error status = VL53L1_ERROR_NONE;

	uint8_t  VL53L1_PRM_00032                  = 0;

	pdetection->VL53L1_PRM_00020     = pfiltered->VL53L1_PRM_00020;
	pdetection->VL53L1_PRM_00019       = pfiltered->VL53L1_PRM_00019;
	pdetection->VL53L1_PRM_00021  = pfiltered->VL53L1_PRM_00021;

	for (VL53L1_PRM_00032 = pfiltered->VL53L1_PRM_00019 ; VL53L1_PRM_00032 < pfiltered->VL53L1_PRM_00021 ; VL53L1_PRM_00032++) {

		if (pdetection->VL53L1_PRM_00043[VL53L1_PRM_00032] > 0) {




			status =
				VL53L1_FCTN_00014(
					sigma_estimator__sigma_ref_mm,
					(uint32_t)pfiltered->VL53L1_PRM_00002[VL53L1_PRM_00032],
					(uint32_t)pfiltered->VL53L1_PRM_00032[VL53L1_PRM_00032],
					(uint32_t)pfiltered->VL53L1_PRM_00001[VL53L1_PRM_00032],
					(uint32_t)pfiltered->VL53L1_PRM_00002[VL53L1_PRM_00032],
					(uint32_t)pfiltered->VL53L1_PRM_00001[VL53L1_PRM_00032],
					(uint32_t)pfilteredx->VL53L1_PRM_00032[VL53L1_PRM_00032],
					(uint32_t)pfilteredx->VL53L1_PRM_00002[VL53L1_PRM_00032],
					(uint32_t)pfilteredx->VL53L1_PRM_00001[VL53L1_PRM_00032],
					(uint32_t)pfiltered->VL53L1_PRM_00028,
					pfiltered->VL53L1_PRM_00022,
					&(pdetection->VL53L1_PRM_00010[VL53L1_PRM_00032]));






			if (status == VL53L1_ERROR_DIVISION_BY_ZERO) {
				pdetection->VL53L1_PRM_00043[VL53L1_PRM_00032] = 0;
				pdetection->VL53L1_PRM_00010[VL53L1_PRM_00032] = 0xFFFF;
			}

		} else {
			pdetection->VL53L1_PRM_00010[VL53L1_PRM_00032] = 0xFFFF;
		}
	}
}


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


void VL53L1_FCTN_00009(
	VL53L1_hist_gen2_algo_filtered_data_t  *pfiltered,
	VL53L1_hist_gen2_algo_filtered_data_t  *pfilteredx,
	VL53L1_hist_gen2_algo_detection_data_t *pdetection)
{





	VL53L1_Error status = VL53L1_ERROR_NONE;

	uint8_t  VL53L1_PRM_00032                  = 0;
	uint8_t  VL53L1_PRM_00030 = 0;

	SUPPRESS_UNUSED_WARNING(pfilteredx);

	pdetection->VL53L1_PRM_00020     = pfiltered->VL53L1_PRM_00020;
	pdetection->VL53L1_PRM_00019       = pfiltered->VL53L1_PRM_00019;
	pdetection->VL53L1_PRM_00021  = pfiltered->VL53L1_PRM_00021;




	VL53L1_PRM_00030 = VL53L1_decode_vcsel_period(pfiltered->VL53L1_PRM_00008);




	for (VL53L1_PRM_00032 = pfiltered->VL53L1_PRM_00019 ; VL53L1_PRM_00032 < pfiltered->VL53L1_PRM_00021 ; VL53L1_PRM_00032++) {

		if (pdetection->VL53L1_PRM_00043[VL53L1_PRM_00032] > 0) {

			status =
				VL53L1_FCTN_00015(
					VL53L1_PRM_00032,
					pfiltered->VL53L1_PRM_00002[VL53L1_PRM_00032],
					pfiltered->VL53L1_PRM_00032[VL53L1_PRM_00032],
					pfiltered->VL53L1_PRM_00001[VL53L1_PRM_00032],
					0,

					0,

					0,

					pfiltered->VL53L1_PRM_00028,
					VL53L1_PRM_00030,
					&(pdetection->VL53L1_PRM_00016[VL53L1_PRM_00032]));



			if (status == VL53L1_ERROR_DIVISION_BY_ZERO) {
				pdetection->VL53L1_PRM_00043[VL53L1_PRM_00032] = 0;
				pdetection->VL53L1_PRM_00016[VL53L1_PRM_00032] = 0;
			}

		} else {
			pdetection->VL53L1_PRM_00016[VL53L1_PRM_00032] = 0;
		}
	}
}


void VL53L1_FCTN_00010(
	int32_t                                 signal_total_events_limit,
	uint16_t                                sigma_thresh,
	VL53L1_histogram_bin_data_t            *pbins,
	VL53L1_hist_gen2_algo_filtered_data_t  *pfiltered,
	VL53L1_hist_gen2_algo_detection_data_t *pdetection,
	VL53L1_range_results_t                 *presults)
{






	uint8_t  VL53L1_PRM_00032                     = 0;
	int32_t  VL53L1_PRM_00018  = 0;
	int32_t  VL53L1_PRM_00012   = 0;

	VL53L1_range_data_t *pdata;

	LOG_FUNCTION_START("");

	presults->cfg_device_state = pbins->cfg_device_state;
	presults->rd_device_state  = pbins->rd_device_state;
	presults->zone_id          = pbins->zone_id;
	presults->stream_count     = pbins->result__stream_count;
	presults->max_results      = VL53L1_MAX_RANGE_RESULTS;
	presults->active_results   = 0;

	for (VL53L1_PRM_00032 = pdetection->VL53L1_PRM_00019 ; VL53L1_PRM_00032 < pdetection->VL53L1_PRM_00021 ; VL53L1_PRM_00032++) {



		if (pdetection->VL53L1_PRM_00043[VL53L1_PRM_00032] > 0) {



			VL53L1_PRM_00018  = pfiltered->VL53L1_PRM_00007[VL53L1_PRM_00032];
			VL53L1_PRM_00012   = VL53L1_PRM_00018 - pfiltered->VL53L1_PRM_00017;



			if (VL53L1_PRM_00012  > signal_total_events_limit &&
				pdetection->VL53L1_PRM_00010[VL53L1_PRM_00032] < sigma_thresh) {



				if (presults->active_results < presults->max_results) {

					pdata = &presults->VL53L1_PRM_00005[presults->active_results];



					pdata->range_id              = presults->active_results;
					pdata->time_stamp            = 0;
					pdata->VL53L1_PRM_00029                   = pfiltered->VL53L1_PRM_00029;
					pdata->zero_distance_phase   = pbins->zero_distance_phase;
					pdata->VL53L1_PRM_00003              = pdetection->VL53L1_PRM_00010[VL53L1_PRM_00032];
					pdata->VL53L1_PRM_00013          = (uint16_t)pdetection->VL53L1_PRM_00016[VL53L1_PRM_00032];
					pdata->VL53L1_PRM_00018  = (uint32_t)VL53L1_PRM_00018;
					pdata->VL53L1_PRM_00012   = VL53L1_PRM_00012;
					pdata->VL53L1_PRM_00017 = (uint32_t)pfiltered->VL53L1_PRM_00017;
					pdata->total_periods_elapsed = pbins->total_periods_elapsed;
					pdata->range_status          = VL53L1_DEVICEERROR_RANGECOMPLETE;



					presults->active_results++;

				}
			}
		}
	}

	LOG_FUNCTION_END(0);

}

