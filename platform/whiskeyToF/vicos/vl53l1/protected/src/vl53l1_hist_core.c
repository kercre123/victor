
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






































#include "vl53l1_platform_log.h"

#include "vl53l1_core_support.h"
#include "vl53l1_hist_structs.h"

#include "vl53l1_xtalk.h"
#include "vl53l1_sigma_estimate.h"

#include "vl53l1_hist_core.h"






#define LOG_FUNCTION_START(fmt, ...) \
	_LOG_FUNCTION_START(VL53L1_TRACE_MODULE_HISTOGRAM, fmt, ##__VA_ARGS__)
#define LOG_FUNCTION_END(status, ...) \
	_LOG_FUNCTION_END(VL53L1_TRACE_MODULE_HISTOGRAM, status, ##__VA_ARGS__)
#define LOG_FUNCTION_END_FMT(status, fmt, ...) \
	_LOG_FUNCTION_END_FMT(VL53L1_TRACE_MODULE_HISTOGRAM, status, fmt, ##__VA_ARGS__)

#define trace_print(level, ...) \
	_LOG_TRACE_PRINT(VL53L1_TRACE_MODULE_HISTOGRAM, \
	level, VL53L1_TRACE_FUNCTION_NONE, ##__VA_ARGS__)


void VL53L1_FCTN_00013(
	uint8_t                         VL53L1_PRM_00032,
	uint8_t                         filter_woi,
	VL53L1_histogram_bin_data_t    *pbins,
	int32_t                        *pa,
	int32_t                        *pb,
	int32_t                        *pc)
{





	uint8_t w = 0;
	uint8_t j = 0;

	*pa = 0;
	*pb = pbins->bin_data[VL53L1_PRM_00032];
	*pc = 0;

	for (w = 0 ; w < ((filter_woi << 1)+1) ; w++) {





		j = ((VL53L1_PRM_00032 + w + pbins->VL53L1_PRM_00021) - filter_woi) % pbins->VL53L1_PRM_00021;





		if (w < filter_woi)
			*pa += pbins->bin_data[j];
		else if (w > filter_woi)
			*pc += pbins->bin_data[j];
	}
}


VL53L1_Error VL53L1_FCTN_00011(
	uint16_t           vcsel_width,
	uint16_t           fast_osc_frequency,
	uint32_t           total_periods_elapsed,
	uint16_t           VL53L1_PRM_00004,
	VL53L1_range_data_t  *pdata)
{
	VL53L1_Error     status = VL53L1_ERROR_NONE;

	uint32_t    pll_period_us       = 0;
	uint32_t    periods_elapsed     = 0;
	uint32_t    count_rate_total    = 0;

	LOG_FUNCTION_START("");




	pdata->width                  = vcsel_width;
	pdata->fast_osc_frequency     = fast_osc_frequency;
	pdata->total_periods_elapsed  = total_periods_elapsed;
	pdata->VL53L1_PRM_00004 = VL53L1_PRM_00004;




	if (pdata->fast_osc_frequency == 0)
		status = VL53L1_ERROR_DIVISION_BY_ZERO;

	if (pdata->total_periods_elapsed == 0)
		status = VL53L1_ERROR_DIVISION_BY_ZERO;

	if (status == VL53L1_ERROR_NONE) {




		pll_period_us =
			VL53L1_calc_pll_period_us(pdata->fast_osc_frequency);




		periods_elapsed      = pdata->total_periods_elapsed + 1;






		pdata->peak_duration_us    = VL53L1_duration_maths(
			pll_period_us,
			(uint32_t)pdata->width,
			VL53L1_RANGING_WINDOW_VCSEL_PERIODS,
			periods_elapsed);

		pdata->woi_duration_us     = VL53L1_duration_maths(
			pll_period_us,
			((uint32_t)pdata->VL53L1_PRM_00029) << 4,
			VL53L1_RANGING_WINDOW_VCSEL_PERIODS,
			periods_elapsed);




		pdata->peak_signal_count_rate_mcps = VL53L1_rate_maths(
			(int32_t)pdata->VL53L1_PRM_00012,
			pdata->peak_duration_us);

		pdata->avg_signal_count_rate_mcps = VL53L1_rate_maths(
			(int32_t)pdata->VL53L1_PRM_00012,
			pdata->woi_duration_us);

		pdata->ambient_count_rate_mcps    = VL53L1_rate_maths(
			(int32_t)pdata->VL53L1_PRM_00017,
			pdata->woi_duration_us);




		count_rate_total =
			(uint32_t)pdata->peak_signal_count_rate_mcps +
			(uint32_t)pdata->ambient_count_rate_mcps;

		pdata->total_rate_per_spad_mcps   =
			VL53L1_rate_per_spad_maths(
					 0x06,
					 count_rate_total,
					 pdata->VL53L1_PRM_00004,
					 0xFFFF);




		pdata->VL53L1_PRM_00011   =
			VL53L1_events_per_spad_maths(
				pdata->VL53L1_PRM_00012,
				pdata->VL53L1_PRM_00004,
				pdata->peak_duration_us);





		trace_print(
			VL53L1_TRACE_LEVEL_DEBUG,
			"    %d:%-46s : %10d\n",
			pdata->range_id, "peak_duration_us",
			pdata->peak_duration_us);
		trace_print(
			VL53L1_TRACE_LEVEL_DEBUG,
			"    %d:%-46s : %10d\n",
			pdata->range_id, "woi_duration_us",
			pdata->woi_duration_us);
		trace_print(
			VL53L1_TRACE_LEVEL_DEBUG,
			"    %d:%-46s : %10u\n",
			pdata->range_id, "peak_signal_count_rate_mcps",
			pdata->peak_signal_count_rate_mcps);
		trace_print(
			VL53L1_TRACE_LEVEL_DEBUG,
			"    %d:%-46s : %10u\n",
			pdata->range_id, "ambient_count_rate_mcps",
			pdata->ambient_count_rate_mcps);
		trace_print(
			VL53L1_TRACE_LEVEL_DEBUG,
			"    %d:%-46s : %10u\n",
			pdata->range_id, "total_rate_per_spad_mcps",
			pdata->total_rate_per_spad_mcps);
		trace_print(
			VL53L1_TRACE_LEVEL_DEBUG,
			"    %d:%-46s : %10u\n",
			pdata->range_id, "VL53L1_PRM_00011",
			pdata->VL53L1_PRM_00011);

	}

	LOG_FUNCTION_END(status);

	return status;
}


void VL53L1_FCTN_00012(
	uint16_t             gain_factor,
	int16_t              range_offset_mm,
	VL53L1_range_data_t *pdata)
{




	LOG_FUNCTION_START("");




	pdata->min_range_mm =
		(int16_t)VL53L1_range_maths(
				pdata->fast_osc_frequency,
				pdata->VL53L1_PRM_00026,
				pdata->zero_distance_phase,
				0,

				(int32_t)gain_factor,
				(int32_t)range_offset_mm);

	pdata->median_range_mm =
		(int16_t)VL53L1_range_maths(
				pdata->fast_osc_frequency,
				pdata->VL53L1_PRM_00013,
				pdata->zero_distance_phase,
				0,

				(int32_t)gain_factor,
				(int32_t)range_offset_mm);

	pdata->max_range_mm =
		(int16_t)VL53L1_range_maths(
				pdata->fast_osc_frequency,
				pdata->VL53L1_PRM_00027,
				pdata->zero_distance_phase,
				0,

				(int32_t)gain_factor,
				(int32_t)range_offset_mm);

























	LOG_FUNCTION_END(0);
}


void  VL53L1_FCTN_00037(
	VL53L1_histogram_bin_data_t   *pdata,
	int32_t                        ambient_estimate_counts_per_bin)
{






	uint8_t i = 0;

	for (i = 0 ; i <  pdata->VL53L1_PRM_00021 ; i++)
		pdata->bin_data[i] = pdata->bin_data[i] -
			ambient_estimate_counts_per_bin;
}


void  VL53L1_FCTN_00004(
	VL53L1_histogram_bin_data_t   *pxtalk,
	VL53L1_histogram_bin_data_t   *pbins,
	VL53L1_histogram_bin_data_t   *pxtalk_realigned)
{






	uint8_t i          = 0;
	uint8_t min_bins   = 0;
	int8_t  bin_offset = 0;
	int8_t  bin_access = 0;

	LOG_FUNCTION_START("");















	memcpy(
		pxtalk_realigned,
		pbins,
		sizeof(VL53L1_histogram_bin_data_t));

	for (i = 0 ; i < pxtalk_realigned->VL53L1_PRM_00020 ; i++)
		pxtalk_realigned->bin_data[i] = 0;









	bin_offset =  VL53L1_FCTN_00038(
						pbins,
						pxtalk);






	if (pxtalk->VL53L1_PRM_00021 < pbins->VL53L1_PRM_00021)
		min_bins = pxtalk->VL53L1_PRM_00021;
	else
		min_bins = pbins->VL53L1_PRM_00021;


	for (i = 0 ; i <  min_bins ; i++) {




		if (bin_offset >= 0)
			bin_access = ((int8_t)i + (int8_t)bin_offset)
				% (int8_t)pbins->VL53L1_PRM_00021;
		else
			bin_access = ((int8_t)pbins->VL53L1_PRM_00021 +
				((int8_t)i + (int8_t)bin_offset))
					% (int8_t)pbins->VL53L1_PRM_00021;

		trace_print(
			VL53L1_TRACE_LEVEL_DEBUG,
			"Subtract:     %8d : %8d : %8d : %8d : %8d : %8d\n",
			i, bin_access, bin_offset, pbins->VL53L1_PRM_00021,
			pbins->bin_data[(uint8_t)bin_access], pxtalk->bin_data[i]);




		if (pbins->bin_data[(uint8_t)bin_access] > pxtalk->bin_data[i]) {

			pbins->bin_data[(uint8_t)bin_access] =
				pbins->bin_data[(uint8_t)bin_access]
				- pxtalk->bin_data[i];

		} else {
			pbins->bin_data[(uint8_t)bin_access] = 0;
		}






		pxtalk_realigned->bin_data[(uint8_t)bin_access] =
			pxtalk->bin_data[i];




	}












	LOG_FUNCTION_END(0);
}


int8_t  VL53L1_FCTN_00038(
	VL53L1_histogram_bin_data_t   *pdata1,
	VL53L1_histogram_bin_data_t   *pdata2)
{










	int32_t  bin_offset_int   = 0;
	int8_t   bin_offset       = 0;
	uint32_t period           = 0;
	uint32_t remapped_phase   = 0;

	LOG_FUNCTION_START("");




	period = 2048 *
		(uint32_t)VL53L1_decode_vcsel_period(pdata1->VL53L1_PRM_00008);

	remapped_phase = (uint32_t)pdata2->zero_distance_phase % period;





	bin_offset_int = ((int32_t)pdata1->zero_distance_phase / 2048)
				- ((int32_t)remapped_phase / 2048);

	bin_offset = (int8_t)bin_offset_int;

	LOG_FUNCTION_END(0);

	return bin_offset;
}


VL53L1_Error  VL53L1_FCTN_00039(
	VL53L1_histogram_bin_data_t   *pidata,
	VL53L1_histogram_bin_data_t   *podata)
{












	VL53L1_Error status = VL53L1_ERROR_NONE;

	uint8_t  bin_initial_index[VL53L1_MAX_BIN_SEQUENCE_CODE+1];
	uint8_t  bin_repeat_count[VL53L1_MAX_BIN_SEQUENCE_CODE+1];

	uint8_t  bin_cfg        = 0;
	uint8_t  bin_seq_length = 0;
	int32_t  repeat_count   = 0;

	uint8_t  VL53L1_PRM_00032       = 0;
	uint8_t  VL53L1_PRM_00001       = 0;
	uint8_t  i       = 0;

	LOG_FUNCTION_START("");




	memcpy(podata, pidata, sizeof(VL53L1_histogram_bin_data_t));



	podata->VL53L1_PRM_00021 = 0;

	for (VL53L1_PRM_00001 = 0 ; VL53L1_PRM_00001 < VL53L1_MAX_BIN_SEQUENCE_LENGTH ; VL53L1_PRM_00001++)
		podata->bin_seq[VL53L1_PRM_00001] = VL53L1_MAX_BIN_SEQUENCE_CODE+1;

	for (VL53L1_PRM_00032 = 0 ; VL53L1_PRM_00032 < podata->VL53L1_PRM_00020 ; VL53L1_PRM_00032++)
		podata->bin_data[VL53L1_PRM_00032] = 0;




	for (VL53L1_PRM_00001 = 0 ; VL53L1_PRM_00001 <= VL53L1_MAX_BIN_SEQUENCE_CODE ; VL53L1_PRM_00001++) {
		bin_initial_index[VL53L1_PRM_00001] = 0x00;
		bin_repeat_count[VL53L1_PRM_00001]  = 0x00;
	}





	bin_seq_length = 0x00;

	for (VL53L1_PRM_00001 = 0 ; VL53L1_PRM_00001 < VL53L1_MAX_BIN_SEQUENCE_LENGTH ; VL53L1_PRM_00001++) {

		bin_cfg = pidata->bin_seq[VL53L1_PRM_00001];







		if (bin_repeat_count[bin_cfg] == 0) {
			bin_initial_index[bin_cfg]      = bin_seq_length * 4;
			podata->bin_seq[bin_seq_length] = bin_cfg;
			bin_seq_length++;
		}

		bin_repeat_count[bin_cfg]++;




		VL53L1_PRM_00032 = bin_initial_index[bin_cfg];

		for (i = 0 ; i < 4 ; i++)
			podata->bin_data[VL53L1_PRM_00032+i] += pidata->bin_data[VL53L1_PRM_00001*4+i];

	}




	for (VL53L1_PRM_00001 = 0 ; VL53L1_PRM_00001 < VL53L1_MAX_BIN_SEQUENCE_LENGTH ; VL53L1_PRM_00001++) {

		bin_cfg = podata->bin_seq[VL53L1_PRM_00001];

		if (bin_cfg <= VL53L1_MAX_BIN_SEQUENCE_CODE)
			podata->bin_rep[VL53L1_PRM_00001] = bin_repeat_count[bin_cfg];
		else
			podata->bin_rep[VL53L1_PRM_00001] = 0;
	}

	podata->VL53L1_PRM_00021 = bin_seq_length * 4;




















	for (VL53L1_PRM_00001 = 0 ; VL53L1_PRM_00001 <= VL53L1_MAX_BIN_SEQUENCE_CODE ; VL53L1_PRM_00001++) {

		repeat_count = (int32_t)bin_repeat_count[VL53L1_PRM_00001];

		if (repeat_count > 0) {

			VL53L1_PRM_00032 = bin_initial_index[VL53L1_PRM_00001];

			for (i = 0 ; i < 4 ; i++) {
				podata->bin_data[VL53L1_PRM_00032+i] += (repeat_count/2);
				podata->bin_data[VL53L1_PRM_00032+i] /=  repeat_count;
			}
		}
	}




	podata->number_of_ambient_bins = 0;
	if ((bin_repeat_count[7] > 0) ||
		(bin_repeat_count[15] > 0))
		podata->number_of_ambient_bins = 4;

	LOG_FUNCTION_END(status);

	return status;
}

