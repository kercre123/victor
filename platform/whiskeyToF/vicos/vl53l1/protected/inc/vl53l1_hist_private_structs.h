
/*******************************************************************************
 This file is part of VL53L1 Protected

 Copyright (c) 2017, STMicroelectronics - All Rights Reserved

 License terms: STMicroelectronics Proprietary in accordance with licensing
 terms at www.st.com/sla0081

 STMicroelectronics confidential
 Reproduction and Communication of this document is strictly prohibited unless
 specifically authorized in writing by STMicroelectronics.

*/







































#ifndef _VL53L1_HIST_PRIVATE_STRUCTS_H_
#define _VL53L1_HIST_PRIVATE_STRUCTS_H_

#include "vl53l1_types.h"
#include "vl53l1_hist_structs.h"

#define VL53L1_D_001         8

#ifdef __cplusplus
extern "C" {
#endif










typedef struct {

	uint8_t  VL53L1_p_022;


	uint8_t  VL53L1_p_023;


	uint8_t  VL53L1_p_024;


	uint8_t  VL53L1_p_030;


	int32_t  VL53L1_p_020;



	int32_t   VL53L1_p_048[VL53L1_HISTOGRAM_BUFFER_SIZE];

	int32_t   VL53L1_p_069[VL53L1_HISTOGRAM_BUFFER_SIZE];


	uint8_t   VL53L1_p_043[VL53L1_HISTOGRAM_BUFFER_SIZE];


	int32_t   VL53L1_p_008[VL53L1_HISTOGRAM_BUFFER_SIZE];

	uint16_t  VL53L1_p_017[VL53L1_HISTOGRAM_BUFFER_SIZE];

	uint16_t  VL53L1_p_011[VL53L1_HISTOGRAM_BUFFER_SIZE];


} VL53L1_hist_gen1_algo_private_data_t;










typedef struct {

	uint8_t  VL53L1_p_022;


	uint8_t  VL53L1_p_023;


	uint8_t  VL53L1_p_024;


	uint16_t VL53L1_p_019;


	uint8_t  VL53L1_p_009;


	uint8_t  VL53L1_p_030;


	int32_t  VL53L1_p_004;


	int32_t  VL53L1_p_020;



	int32_t   VL53L1_p_003[VL53L1_HISTOGRAM_BUFFER_SIZE];


	int32_t   VL53L1_p_018[VL53L1_HISTOGRAM_BUFFER_SIZE];


	int32_t   VL53L1_p_001[VL53L1_HISTOGRAM_BUFFER_SIZE];



	int32_t   VL53L1_p_008[VL53L1_HISTOGRAM_BUFFER_SIZE];


	int32_t   VL53L1_p_041[VL53L1_HISTOGRAM_BUFFER_SIZE];


	int32_t   VL53L1_p_039[VL53L1_HISTOGRAM_BUFFER_SIZE];


	int32_t   VL53L1_p_040[VL53L1_HISTOGRAM_BUFFER_SIZE];



} VL53L1_hist_gen2_algo_filtered_data_t;










typedef struct {

	uint8_t  VL53L1_p_022;


	uint8_t  VL53L1_p_023;


	uint8_t  VL53L1_p_024;


	int32_t  VL53L1_p_032;



	uint8_t   VL53L1_p_042[VL53L1_HISTOGRAM_BUFFER_SIZE];


	uint8_t   VL53L1_p_044[VL53L1_HISTOGRAM_BUFFER_SIZE];



	uint32_t  VL53L1_p_017[VL53L1_HISTOGRAM_BUFFER_SIZE];


	uint16_t  VL53L1_p_011[VL53L1_HISTOGRAM_BUFFER_SIZE];



	uint8_t   VL53L1_p_043[VL53L1_HISTOGRAM_BUFFER_SIZE];



} VL53L1_hist_gen2_algo_detection_data_t;










typedef struct {

	uint8_t  VL53L1_p_015;


	uint8_t  VL53L1_p_022;


	uint8_t  VL53L1_p_025;


	uint8_t  VL53L1_p_026;


	uint8_t  VL53L1_p_016;



	uint8_t  VL53L1_p_027;


	uint8_t  VL53L1_p_055;



	int32_t  VL53L1_p_020;


	int32_t  VL53L1_p_021;


	int32_t  VL53L1_p_013;



	uint32_t VL53L1_p_028;


	uint32_t VL53L1_p_014;


	uint32_t VL53L1_p_029;



	uint16_t VL53L1_p_005;



} VL53L1_hist_pulse_data_t;










typedef struct {

	uint8_t  VL53L1_p_022;


	uint8_t  VL53L1_p_023;


	uint8_t  VL53L1_p_024;


	uint8_t  VL53L1_p_031;


	uint8_t  VL53L1_p_045;


	int32_t  VL53L1_p_004;


	int32_t  VL53L1_p_032;



	uint8_t  VL53L1_p_043[VL53L1_HISTOGRAM_BUFFER_SIZE];


	uint8_t  VL53L1_p_046[VL53L1_HISTOGRAM_BUFFER_SIZE];


	uint8_t  VL53L1_p_047[VL53L1_HISTOGRAM_BUFFER_SIZE];



	int32_t  VL53L1_p_056[VL53L1_HISTOGRAM_BUFFER_SIZE];


	int32_t  VL53L1_p_048[VL53L1_HISTOGRAM_BUFFER_SIZE];


	int32_t  VL53L1_p_008[VL53L1_HISTOGRAM_BUFFER_SIZE];



	uint8_t  VL53L1_p_049;


	uint8_t  VL53L1_p_050;


	uint8_t  VL53L1_p_051;



	VL53L1_hist_pulse_data_t  VL53L1_p_002[VL53L1_D_001];






	VL53L1_histogram_bin_data_t   VL53L1_p_010;


	VL53L1_histogram_bin_data_t   VL53L1_p_038;


	VL53L1_histogram_bin_data_t   VL53L1_p_052;


	VL53L1_histogram_bin_data_t   VL53L1_p_053;


	VL53L1_histogram_bin_data_t   VL53L1_p_054;






} VL53L1_hist_gen3_algo_private_data_t;










typedef struct {

	uint8_t  VL53L1_p_022;


	uint8_t  VL53L1_p_023;


	uint8_t  VL53L1_p_024;



	int32_t   VL53L1_p_003[VL53L1_HISTOGRAM_BUFFER_SIZE];


	int32_t   VL53L1_p_018[VL53L1_HISTOGRAM_BUFFER_SIZE];


	int32_t   VL53L1_p_001[VL53L1_HISTOGRAM_BUFFER_SIZE];



	int32_t   VL53L1_p_039[VL53L1_HISTOGRAM_BUFFER_SIZE];


	int32_t   VL53L1_p_040[VL53L1_HISTOGRAM_BUFFER_SIZE];



	uint8_t  VL53L1_p_043[VL53L1_HISTOGRAM_BUFFER_SIZE];



} VL53L1_hist_gen4_algo_filtered_data_t;

#ifdef __cplusplus
}
#endif

#endif

