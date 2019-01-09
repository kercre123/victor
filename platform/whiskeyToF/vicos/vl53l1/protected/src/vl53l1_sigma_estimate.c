
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

#include "vl53l1_sigma_estimate.h"


#define LOG_FUNCTION_START(fmt, ...) \
	_LOG_FUNCTION_START(VL53L1_TRACE_MODULE_PROTECTED, fmt, ##__VA_ARGS__)
#define LOG_FUNCTION_END(status, ...) \
	_LOG_FUNCTION_END(VL53L1_TRACE_MODULE_PROTECTED, status, ##__VA_ARGS__)
#define LOG_FUNCTION_END_FMT(status, fmt, ...) \
	_LOG_FUNCTION_END_FMT(VL53L1_TRACE_MODULE_PROTECTED, status, fmt, ##__VA_ARGS__)

#define trace_print(level, ...) \
	_LOG_TRACE_PRINT(VL53L1_TRACE_MODULE_PROTECTED, \
	level, VL53L1_TRACE_FUNCTION_NONE, ##__VA_ARGS__)


uint16_t  VL53L1_FCTN_00042(
		uint8_t	 sigma_estimator__effective_pulse_width_ns,
		uint8_t  sigma_estimator__effective_ambient_width_ns,
		uint8_t	 sigma_estimator__sigma_ref_mm,
		VL53L1_range_data_t *pdata)
{
















	uint16_t    sigma_est  = VL53L1_DEF_00002;

	uint32_t    tmp0 = 0;
	uint32_t    tmp1 = 0;
	uint32_t    tmp2 = 0;

	uint32_t    sigma_est__rtn_array  = 0;
	uint32_t    sigma_est__ref_array  = 0;

	LOG_FUNCTION_START("");

	if (pdata->peak_signal_count_rate_mcps  > 0 &&
		pdata->VL53L1_PRM_00012 > 0) {








		tmp0 =  100 * (uint32_t)sigma_estimator__effective_pulse_width_ns;







		tmp1 = ((uint32_t)sigma_estimator__effective_pulse_width_ns * 100 * (uint32_t)sigma_estimator__effective_ambient_width_ns);
		tmp1 =  (tmp1 + (uint32_t)pdata->peak_signal_count_rate_mcps/2) / (uint32_t)pdata->peak_signal_count_rate_mcps;







		sigma_est__rtn_array =
			VL53L1_FCTN_00043(tmp0, tmp1);






		sigma_est__rtn_array =
			((VL53L1_SPEED_OF_LIGHT_IN_AIR + 1000) / 2000) *
			 sigma_est__rtn_array;







		tmp2 =
			VL53L1_isqrt(12 * (uint32_t)pdata->VL53L1_PRM_00012);

		if (tmp2 > 0) {

			sigma_est__rtn_array =
					(sigma_est__rtn_array + tmp2/2) / tmp2;






			sigma_est__ref_array =
				100 * (uint32_t)sigma_estimator__sigma_ref_mm;

			sigma_est =
				(uint16_t)VL53L1_FCTN_00043(
						(uint32_t)sigma_est__ref_array,
						sigma_est__rtn_array);

		} else {
			sigma_est = VL53L1_DEF_00002;
		}

	}

	pdata->VL53L1_PRM_00003  = sigma_est;

	LOG_FUNCTION_END(0);

	return sigma_est;

}


uint16_t VL53L1_FCTN_00044(
	uint8_t	 sigma_estimator__effective_pulse_width_ns,
	uint8_t  sigma_estimator__effective_ambient_width_ns,
	uint8_t	 sigma_estimator__sigma_ref_mm,
	VL53L1_range_data_t *pdata)
{






















	uint16_t    sigma_est  = VL53L1_DEF_00002;

	uint32_t    eqn7 = 0;
	uint32_t    sigma_est__ref_sq  = 0;
	uint32_t    sigma_est__rtn_sq  = 0;

	uint64_t    tmp0 = 0;
	uint64_t    tmp1 = 0;

	LOG_FUNCTION_START("");

	if (pdata->peak_signal_count_rate_mcps > 0 &&
		pdata->VL53L1_PRM_00012         > 0) {




















		eqn7 =  4573 * 4573;
		eqn7 =  eqn7 / (3 * (uint32_t)pdata->VL53L1_PRM_00012);











		tmp0 = ((uint64_t)sigma_estimator__effective_pulse_width_ns) << 8;














		tmp1 = ((uint64_t)pdata->ambient_count_rate_mcps *
				(uint64_t)sigma_estimator__effective_ambient_width_ns) << 8;
		tmp1 = tmp1 / (uint64_t)pdata->peak_signal_count_rate_mcps;
















		tmp1 = 16 * (uint64_t)eqn7 * (tmp0 * tmp0 + tmp1 * tmp1);
		tmp1 = tmp1 / (15625 * 15625);
		sigma_est__rtn_sq = (uint32_t)tmp1;




		sigma_est__ref_sq = ((uint32_t)sigma_estimator__sigma_ref_mm) << 2;
		sigma_est__ref_sq = sigma_est__ref_sq * sigma_est__ref_sq;




		sigma_est = (uint16_t)VL53L1_isqrt(sigma_est__ref_sq + sigma_est__rtn_sq);

	}

	pdata->VL53L1_PRM_00003  = sigma_est;

	LOG_FUNCTION_END(0);

	return sigma_est;

}




VL53L1_Error VL53L1_FCTN_00045(
	uint8_t	 sigma_estimator__sigma_ref_mm,
	uint32_t VL53L1_PRM_00002,
	uint32_t VL53L1_PRM_00032,
	uint32_t VL53L1_PRM_00001,
	uint32_t a_zp,
	uint32_t c_zp,
	uint32_t bx,
	uint32_t ax_zp,
	uint32_t cx_zp,
	uint32_t VL53L1_PRM_00028,
	uint16_t fast_osc_frequency,
	uint16_t *psigma_est)
{






	VL53L1_Error status = VL53L1_ERROR_DIVISION_BY_ZERO;
	uint32_t sigma_int  = VL53L1_DEF_00002;

	uint32_t pll_period_mm  = 0;

	uint64_t tmp0        = 0;
	uint64_t tmp1        = 0;
	uint64_t b_minus_amb = 0;
	uint64_t VL53L1_PRM_00041   = 0;

	*psigma_est  = VL53L1_DEF_00002;







	if (fast_osc_frequency != 0) {





		pll_period_mm = VL53L1_calc_pll_period_mm(fast_osc_frequency);



		pll_period_mm = (pll_period_mm + 0x02) >> 2;




		if (VL53L1_PRM_00028 > VL53L1_PRM_00032)
			b_minus_amb =  (uint64_t)VL53L1_PRM_00028 - (uint64_t)VL53L1_PRM_00032;
		else
			b_minus_amb =  (uint64_t)VL53L1_PRM_00032 - (uint64_t)VL53L1_PRM_00028;




		if (VL53L1_PRM_00002 > VL53L1_PRM_00001)
			VL53L1_PRM_00041 =  (uint64_t)VL53L1_PRM_00002 - (uint64_t)VL53L1_PRM_00001;
		else
			VL53L1_PRM_00041 =  (uint64_t)VL53L1_PRM_00001 - (uint64_t)VL53L1_PRM_00002;







		if (b_minus_amb != 0) {
















			tmp0 = (uint64_t)pll_period_mm * (uint64_t)pll_period_mm;
			tmp0 = tmp0 * ((uint64_t)c_zp + (uint64_t)cx_zp + (uint64_t)a_zp + (uint64_t)ax_zp);
			tmp0 = (tmp0 + (b_minus_amb >> 1)) / b_minus_amb;









			tmp1 = (uint64_t)pll_period_mm * (uint64_t)pll_period_mm * VL53L1_PRM_00041;
			tmp1 = (tmp1 + (b_minus_amb >> 1)) / b_minus_amb;

			tmp1 =  tmp1 * VL53L1_PRM_00041;
			tmp1 = (tmp1 + (b_minus_amb >> 1)) / b_minus_amb;

			tmp1 =  tmp1 * ((uint64_t)VL53L1_PRM_00032 + (uint64_t)bx + (uint64_t)VL53L1_PRM_00028);
			tmp1 = (tmp1 + (b_minus_amb >> 1)) / b_minus_amb;









			tmp0 = tmp0 + tmp1;
			tmp0 = (tmp0 + (b_minus_amb >> 1)) / b_minus_amb;
			tmp0 = (tmp0 + 0x01) >> 2;






			tmp1 = (uint64_t)sigma_estimator__sigma_ref_mm << 2;
			tmp1 = tmp1 * tmp1;
			tmp0 = tmp0 + tmp1;






			if (tmp0 > 0xFFFFFFFF)
				tmp0 =  0xFFFFFFFF;

			sigma_int = VL53L1_isqrt((uint32_t)tmp0);






			if (sigma_int > VL53L1_DEF_00002)
				*psigma_est = (uint16_t)VL53L1_DEF_00002;
			else
				*psigma_est = (uint16_t)sigma_int;

			status = VL53L1_ERROR_NONE;
		}

	}

	return status;
}




VL53L1_Error VL53L1_FCTN_00014(
	uint8_t	 sigma_estimator__sigma_ref_mm,
	uint32_t VL53L1_PRM_00002,
	uint32_t VL53L1_PRM_00032,
	uint32_t VL53L1_PRM_00001,
	uint32_t a_zp,
	uint32_t c_zp,
	uint32_t bx,
	uint32_t ax_zp,
	uint32_t cx_zp,
	uint32_t VL53L1_PRM_00028,
	uint16_t fast_osc_frequency,
	uint16_t *psigma_est)
{






	VL53L1_Error status = VL53L1_ERROR_DIVISION_BY_ZERO;
	uint32_t sigma_int  = VL53L1_DEF_00002;

	uint32_t pll_period_mm  = 0;

	uint64_t tmp0        = 0;
	uint64_t tmp1        = 0;
	uint64_t b_minus_amb = 0;
	uint64_t VL53L1_PRM_00041   = 0;

	*psigma_est  = VL53L1_DEF_00002;







	if (fast_osc_frequency != 0) {





		pll_period_mm = VL53L1_calc_pll_period_mm(fast_osc_frequency);




		if (VL53L1_PRM_00028 > VL53L1_PRM_00032)
			b_minus_amb =  (uint64_t)VL53L1_PRM_00028 - (uint64_t)VL53L1_PRM_00032;
		else
			b_minus_amb =  (uint64_t)VL53L1_PRM_00032 - (uint64_t)VL53L1_PRM_00028;




		if (VL53L1_PRM_00002 > VL53L1_PRM_00001)
			VL53L1_PRM_00041 =  (uint64_t)VL53L1_PRM_00002 - (uint64_t)VL53L1_PRM_00001;
		else
			VL53L1_PRM_00041 =  (uint64_t)VL53L1_PRM_00001 - (uint64_t)VL53L1_PRM_00002;




		if (b_minus_amb != 0) {

















			tmp0 = (uint64_t)VL53L1_PRM_00032 + (uint64_t)bx + (uint64_t)VL53L1_PRM_00028;
			if (tmp0 > VL53L1_DEF_00003) {
				tmp0 = VL53L1_DEF_00003;
			}



			tmp1 = (uint64_t)VL53L1_PRM_00041 * (uint64_t)VL53L1_PRM_00041;
			tmp1 = tmp1 << 8;



			if (tmp1 > VL53L1_DEF_00004) {
				tmp1 = VL53L1_DEF_00004;
			}



			tmp1 = tmp1 / b_minus_amb;
			tmp1 = tmp1 / b_minus_amb;



			if (tmp1 > (uint64_t)VL53L1_DEF_00005) {
				tmp1 = (uint64_t)VL53L1_DEF_00005;
			}



			tmp0 = tmp1 * tmp0;





			tmp1 = (uint64_t)c_zp + (uint64_t)cx_zp +
				(uint64_t)a_zp + (uint64_t)ax_zp;



			if (tmp1 > (uint64_t)VL53L1_DEF_00003) {
				tmp1 = (uint64_t)VL53L1_DEF_00003;
			}
			tmp1 = tmp1 << 8;




			tmp0 = tmp1 + tmp0;
			if (tmp0 > (uint64_t)VL53L1_DEF_00006) {
				tmp0 = (uint64_t)VL53L1_DEF_00006;
			}













			if (tmp0 > (uint64_t)VL53L1_DEF_00007) {
				tmp0 = tmp0 / b_minus_amb;
				tmp0 = tmp0 * pll_period_mm;
			} else {
				tmp0 = tmp0 * pll_period_mm;
				tmp0 = tmp0 / b_minus_amb;
			}



			if (tmp0 > (uint64_t)VL53L1_DEF_00006) {
				tmp0 = (uint64_t)VL53L1_DEF_00006;
			}



			if (tmp0 > (uint64_t)VL53L1_DEF_00007) {
				tmp0 = tmp0 / b_minus_amb;
				tmp0 = tmp0 / 4;
				tmp0 = tmp0 * pll_period_mm;
			} else {
				tmp0 = tmp0 * pll_period_mm;
				tmp0 = tmp0 / b_minus_amb;
				tmp0 = tmp0 / 4;
			}



			if (tmp0 > (uint64_t)VL53L1_DEF_00006) {
				tmp0 = (uint64_t)VL53L1_DEF_00006;
			}



			tmp0 = tmp0 >> 2;



			if (tmp0 > (uint64_t)VL53L1_DEF_00007) {
				tmp0 = (uint64_t)VL53L1_DEF_00007;
			}



			tmp1 = (uint64_t)sigma_estimator__sigma_ref_mm << 7;
			tmp1 = tmp1 * tmp1;
			tmp0 = tmp0 + tmp1;



			if (tmp0 > (uint64_t)VL53L1_DEF_00007) {
				tmp0 = (uint64_t)VL53L1_DEF_00007;
			}



			sigma_int = VL53L1_isqrt((uint32_t)tmp0);

			*psigma_est = (uint16_t)sigma_int;

			status = VL53L1_ERROR_NONE;
		}

	}

	return status;
}

uint32_t VL53L1_FCTN_00046(
	uint64_t VL53L1_PRM_00002,
	uint32_t size
	)
{





	uint64_t next;
	uint64_t upper;
	uint64_t lower;
	uint32_t stepsize;
	uint32_t count;


	next = VL53L1_PRM_00002;
	upper = 0;
	lower = 0;
	stepsize = size/2;
	count = 0;

	while (1) {
		upper = next >> stepsize;
		lower = next & ((1 << stepsize) - 1);

		if (upper != 0) {
			count += stepsize;
			next = upper;
		} else {
			next = lower;
		}

		stepsize = stepsize / 2;
		if (stepsize == 0) {
			break;
		}
	}

	return count;
}




uint32_t VL53L1_FCTN_00043(
	uint32_t VL53L1_PRM_00002,
	uint32_t VL53L1_PRM_00032)
{












	uint32_t  res = 0;

	if (VL53L1_PRM_00002 > 65535 || VL53L1_PRM_00032 > 65535)
		res = 65535;
	else
		res = VL53L1_isqrt(VL53L1_PRM_00002*VL53L1_PRM_00002 + VL53L1_PRM_00032*VL53L1_PRM_00032);

	return res;
}


