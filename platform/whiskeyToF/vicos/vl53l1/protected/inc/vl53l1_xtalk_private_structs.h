
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








































#ifndef _VL53L1_XTALK_PRIVATE_STRUCTS_H_
#define _VL53L1_XTALK_PRIVATE_STRUCTS_H_

#include "vl53l1_types.h"
#include "vl53l1_hist_structs.h"


#ifdef __cplusplus
extern "C"
{
#endif

#define VL53L1_DEF_00012  4









typedef struct {





	uint32_t VL53L1_PRM_00062[VL53L1_DEF_00012];



	int16_t  VL53L1_PRM_00060;


	int16_t  VL53L1_PRM_00061;



	VL53L1_histogram_bin_data_t VL53L1_PRM_00057;


	VL53L1_histogram_bin_data_t VL53L1_PRM_00058;




	uint32_t VL53L1_PRM_00059;



	uint32_t VL53L1_PRM_00063[VL53L1_XTALK_HISTO_BINS];



} VL53L1_xtalk_algo_data_t;

#ifdef __cplusplus
}
#endif

#endif

