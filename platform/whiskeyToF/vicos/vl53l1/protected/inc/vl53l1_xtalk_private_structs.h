
/*******************************************************************************
 This file is part of VL53L1 Protected

 Copyright (c) 2017, STMicroelectronics - All Rights Reserved

 License terms: STMicroelectronics Proprietary in accordance with licensing
 terms at www.st.com/sla0081

 STMicroelectronics confidential
 Reproduction and Communication of this document is strictly prohibited unless
 specifically authorized in writing by STMicroelectronics.

*/








































#ifndef _VL53L1_XTALK_PRIVATE_STRUCTS_H_
#define _VL53L1_XTALK_PRIVATE_STRUCTS_H_

#include "vl53l1_types.h"
#include "vl53l1_hist_structs.h"


#ifdef __cplusplus
extern "C"
{
#endif

#define VL53L1_D_012  4









typedef struct {





	uint32_t VL53L1_p_062[VL53L1_D_012];



	int16_t  VL53L1_p_060;


	int16_t  VL53L1_p_061;



	VL53L1_histogram_bin_data_t VL53L1_p_057;


	VL53L1_histogram_bin_data_t VL53L1_p_058;




	uint32_t VL53L1_p_059;



	uint32_t VL53L1_p_063[VL53L1_XTALK_HISTO_BINS];



} VL53L1_xtalk_algo_data_t;

#ifdef __cplusplus
}
#endif

#endif

